#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "fzero_ordinary_request.h"
#include <string.h>
int FzeroOrdinaryReadFile(const char *path,char *out,size_t size,unsigned long *error){
 if(error)*error=0;if(!path||!out||size<2||size>65536)return 0;
 HANDLE file=CreateFileA(path,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
   NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL|FILE_FLAG_OPEN_REPARSE_POINT,NULL);
 if(file==INVALID_HANDLE_VALUE){if(error)*error=GetLastError();return 0;}
 BY_HANDLE_FILE_INFORMATION info;LARGE_INTEGER length;DWORD read=0;int ok=0;
 if(!GetFileInformationByHandle(file,&info)||!GetFileSizeEx(file,&length))goto done;
 if((info.dwFileAttributes&(FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_REPARSE_POINT))||
    length.QuadPart<1||length.QuadPart>=(LONGLONG)size){SetLastError(ERROR_INVALID_DATA);goto done;}
 if(!ReadFile(file,out,(DWORD)length.QuadPart,&read,NULL))goto done;
 if(read!=(DWORD)length.QuadPart){SetLastError(ERROR_HANDLE_EOF);goto done;}
 if(memchr(out,0,read)){SetLastError(ERROR_INVALID_DATA);goto done;}out[read]=0;ok=1;
done:
 if(!ok&&error)*error=GetLastError();CloseHandle(file);return ok;
}
int FzeroOrdinaryReadValue(const char *text,const char *key,char *out,size_t size){
 if(!text||!key||!out||!size)return 0;size_t n=strlen(key);int found=0;
 for(const char *line=text;*line;){
  size_t length=strcspn(line,"\r\n");
  if(length>n&&line[n]=='='&&!strncmp(line,key,n)){
   size_t value=length-n-1;if(found++||value>=size)return 0;
   memcpy(out,line+n+1,value);out[value]=0;
  }
  line+=length;while(*line=='\r'||*line=='\n')++line;
 }
 return found==1;
}
