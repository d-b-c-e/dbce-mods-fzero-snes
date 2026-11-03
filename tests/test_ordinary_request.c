/* The exact production file reader under concurrent atomic replacement. */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "fzero_ordinary_request.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
static volatile LONG checks,done,failed;static unsigned replacements;
#define CHECK(x) do{InterlockedIncrement(&checks);if(!(x)){fprintf(stderr,"%d: %s winerror=%lu\n",__LINE__,#x,GetLastError());exit(1);}}while(0)
static const char *target="ordinary-request-test.gate",*next="ordinary-request-test.new";
static void write(const char *path,unsigned sequence){
 FILE *f=fopen(path,"wb");CHECK(f);
 fprintf(f,"nonce=1234567890abcdef1234567890abcdef\nuntilFileTime=%u\n",sequence);CHECK(!fclose(f));
}
static DWORD WINAPI writer(LPVOID ignored){
 (void)ignored;
 for(unsigned i=1;i<=1500;i++){
  write(next,i);ULONGLONG until=GetTickCount64()+500;
  while(!MoveFileExA(next,target,MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)){
   if(GetTickCount64()>=until){InterlockedExchange(&failed,1);InterlockedExchange(&done,1);return 1;}Sleep(1);
  }
  ++replacements;Sleep(1);
 }
 InterlockedExchange(&done,1);return 0;
}
/* Frozen 4fd712d per-key stdio reader, for reproducing the old race. */
static int legacy_value(const char *file,const char *key,char *out,size_t size){
 FILE *f=fopen(file,"rb");if(!f)return 0;char line[1400];int found=0;size_t n=strlen(key);
 while(fgets(line,sizeof(line),f)){size_t z=strcspn(line,"\r\n");line[z]=0;
  if(!strncmp(line,key,n)&&line[n]=='='){if(found++||strlen(line+n+1)>=size){fclose(f);return 0;}snprintf(out,size,"%s",line+n+1);}}
 fclose(f);return found==1;
}
int main(void){
 CHECK(GetFileAttributesA(target)==INVALID_FILE_ATTRIBUTES&&GetLastError()==ERROR_FILE_NOT_FOUND);
 CHECK(GetFileAttributesA(next)==INVALID_FILE_ATTRIBUTES&&GetLastError()==ERROR_FILE_NOT_FOUND);
 write(target,0);write(next,1);
 FILE *legacy=fopen(target,"rb");CHECK(legacy);
 CHECK(!MoveFileExA(next,target,MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH));
 DWORD legacy_error=GetLastError();CHECK(legacy_error==ERROR_ACCESS_DENIED||legacy_error==ERROR_SHARING_VIOLATION);
 CHECK(!fclose(legacy));CHECK(MoveFileExA(next,target,MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH));
 printf("Legacy stdio reader deterministically denied replacement: winerror=%lu\n",legacy_error);
 char snapshot[256],nonce[33],value[32];unsigned long error=0;unsigned reads=0;
 HANDLE thread=CreateThread(NULL,0,writer,NULL,0,NULL);CHECK(thread);
 unsigned legacy_reads=0,legacy_failures=0;int legacy_errno=0;
 while(!InterlockedCompareExchange(&done,0,0)){
  if(!legacy_value(target,"nonce",nonce,sizeof(nonce))||!legacy_value(target,"untilFileTime",value,sizeof(value))){++legacy_failures;legacy_errno=errno;}
  ++legacy_reads;Sleep(1);
 }
 CHECK(WaitForSingleObject(thread,1000)==WAIT_OBJECT_0);CHECK(CloseHandle(thread));CHECK(!failed&&replacements==1500);
 printf("Legacy reader under replacement: %u reads, %u failures, last errno=%d\n",legacy_reads,legacy_failures,legacy_errno);
 done=failed=0;replacements=0;thread=CreateThread(NULL,0,writer,NULL,0,NULL);CHECK(thread);
 while(!InterlockedCompareExchange(&done,0,0)){
  CHECK(FzeroOrdinaryReadFile(target,snapshot,sizeof(snapshot),&error));CHECK(error==0);
  CHECK(FzeroOrdinaryReadValue(snapshot,"nonce",nonce,sizeof(nonce)));CHECK(!strcmp(nonce,"1234567890abcdef1234567890abcdef"));
  CHECK(FzeroOrdinaryReadValue(snapshot,"untilFileTime",value,sizeof(value)));CHECK(strspn(value,"0123456789")==strlen(value));
  ++reads;Sleep(1);
 }
 CHECK(WaitForSingleObject(thread,1000)==WAIT_OBJECT_0);CHECK(CloseHandle(thread));CHECK(!failed&&replacements==1500&&reads>500);
 CHECK(!FzeroOrdinaryReadValue("nonce=one\nnonce=two\n","nonce",nonce,sizeof(nonce)));
 CHECK(!FzeroOrdinaryReadValue("nonce=too-long\n","nonce",nonce,2));
 CHECK(!FzeroOrdinaryReadValue("nonces=one\n","nonce",nonce,sizeof(nonce)));
 CHECK(!FzeroOrdinaryReadFile(target,snapshot,4,&error));
 CHECK(DeleteFileA(target));CHECK(!FzeroOrdinaryReadFile(target,snapshot,sizeof(snapshot),&error));CHECK(error==ERROR_FILE_NOT_FOUND);
 printf("PASS ordinary shared-delete snapshot: %ld checks, %u reads/%u atomic replacements; no game/SDL/input\n",checks,reads,replacements);return 0;
}
