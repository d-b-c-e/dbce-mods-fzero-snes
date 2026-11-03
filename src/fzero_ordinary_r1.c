/* Private opt-in ordinary consumer experiment. No virtual/physical device. */
#include "fzero_ordinary_r1.h"
#include "fzero_controls.h"
#include "fzero_gamepad.h"
#include "fzero_output_latch.h"
#include "fzero_session_control.h"
#include "fzero_telemetry.h"
#include "fzero_hotkeys.h"
#include "sha256.h"
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int active,revoked,ui_open,ui_saved,show_fps,ui_tick,host_sent;
static unsigned frame_now,sequence,port,word_now,controllers_now;
static uint64_t expires,birth;
static char nonce[33],root[80],gate[1200],revision[72],status[128],temporary[128];
static int16_t raw[8];static uint32_t buttons;static uint8_t state[3];
static unsigned x,y,yaw;static FILE *trace;
static uint64_t last_published;
static const char *guid="00000000d1a600000000000000000001";
static uint64_t file_now(void){FILETIME f;GetSystemTimeAsFileTime(&f);return ((uint64_t)f.dwHighDateTime<<32)|f.dwLowDateTime;}
static int decimal(const char *s,uint64_t *out){
  if(!s||!*s||strlen(s)>19||strspn(s,"0123456789")!=strlen(s))return 0;
  *out=strtoull(s,NULL,10);return 1;
}
static int read_value(const char *file,const char *key,char *out,size_t size){
  FILE *f=fopen(file,"rb");if(!f)return 0;char line[1400];int found=0;size_t n=strlen(key);
  while(fgets(line,sizeof(line),f)){size_t z=strcspn(line,"\r\n");line[z]=0;
    if(!strncmp(line,key,n)&&line[n]=='='){if(found++ || strlen(line+n+1)>=size){fclose(f);return 0;}snprintf(out,size,"%s",line+n+1);}}
  fclose(f);return found==1;
}
static void hex(const uint8_t *bytes,size_t size,char out[65]){uint8_t sha[32];sha256_compute(bytes,size,sha);for(int i=0;i<32;i++)sprintf(out+2*i,"%02x",sha[i]);}
static void publish(const char *phase){
  if(!last_published)last_published=file_now();
  uint8_t packet[324];unsigned source=0;size_t sent=FzeroTelemetryLastSent(packet,&source);char hash[65]="";
  if(sent)hex(packet,sent,hash);double values[3]={0};int canonical[3]={0};FzeroGamepadDiagnosticObserved(values,canonical);
  FILE *f=fopen(temporary,"wb");if(!f){revoked=1;return;}
  fprintf(f,"{\"schema\":\"fzero.ordinary-r1@1\",\"nonce\":\"%s\",\"pid\":%lu,\"birthFileTime\":%llu,\"seq\":%u,\"frame\":%u,\"phase\":\"%s\",\"syntheticRaw\":true,\"forceBlocked\":%s,\"controllers\":%u,\"port\":%u,\"sourcePort\":%u,\"word\":%u,\"state\":[%u,%u,%u],\"x\":%u,\"y\":%u,\"yaw\":%u,\"raw\":[%d,%d,%d],\"normalized\":[%.9f,%.9f,%.9f],\"canonical\":[%d,%d,%d],\"revision\":\"%s\",\"panelOpen\":%d,\"panelSaved\":%d,\"showFps\":%d,\"packetSha256\":\"%s\"}",
    nonce,(unsigned long)GetCurrentProcessId(),(unsigned long long)birth,++sequence,frame_now,phase,FzeroNoForceLatched()?"true":"false",controllers_now,port,source,word_now,state[0],state[1],state[2],x,y,yaw,raw[0],raw[1],raw[2],values[0],values[1],values[2],canonical[0],canonical[1],canonical[2],revision,ui_open,ui_saved,show_fps,hash);
  if(fclose(f)){revoked=1;return;}
  if(MoveFileExA(temporary,status,MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))last_published=file_now();
  else if(last_published && file_now()-last_published>10000000ULL)revoked=1;
}
int FzeroOrdinaryOpen(const char *config){
  const char *request=getenv("FZERO_ORDINARY_R1_REQUEST");if(!request || !*request)return 0;
  if(!FzeroLatchNoForce())return -1;
  char schema[64],text[64],backend[32];int enabled=1;
  if(!read_value(request,"schema",schema,sizeof(schema))||strcmp(schema,"fzero.ordinary-r1@1")||
     !read_value(request,"nonce",nonce,sizeof(nonce))||strlen(nonce)!=32||strspn(nonce,"0123456789abcdef")!=32||
     !read_value(request,"root",root,sizeof(root))||!read_value(request,"gate",gate,sizeof(gate))||
     !read_value(request,"expiresFileTime",text,sizeof(text)))return -1;
  uint64_t now=file_now();if(!decimal(text,&expires)||expires<=now || expires-now>3000000000ULL)return -1;
  uint64_t requested_port=0;
  if(!read_value(request,"port",text,sizeof(text))||!decimal(text,&requested_port)||requested_port>65535)return -1;port=(unsigned)requested_port;
  if(!FzeroTelemetryRestrictLoopback(port))return -1;
  if(!FzeroIniReadInt(config,"ForceFeedback","Enabled",&enabled)||enabled)return -1;
  if(!FzeroIniReadString(config,"Input","Backend",backend,sizeof(backend))||strcmp(backend,"Native"))return -1;
  if(!FzeroSessionCreatePrivateRoot(root,"keybinds.ini"))return -1;
  snprintf(status,sizeof(status),"%s/status.json",root);snprintf(temporary,sizeof(temporary),"%s/status.tmp",root);
  char path[128];snprintf(path,sizeof(path),"%s/frames.jsonl",root);trace=fopen(path,"wbx");if(!trace)return -1;
  FILETIME created,exit,kernel,user;if(!GetProcessTimes(GetCurrentProcess(),&created,&exit,&kernel,&user))return -1;
  birth=((uint64_t)created.dwHighDateTime<<32)|created.dwLowDateTime;active=1;raw[1]=raw[2]=-32768;
  fprintf(stderr,"[fzero-ordinary-r1] nonce=%s no-force controllers-excluded synthetic-raw loopback=%u\n",nonce,port);
  return FzeroOrdinaryAlive()?1:-1;
}
int FzeroOrdinaryActive(void){return active;}
const char *FzeroOrdinaryRoot(void){return root;}
int FzeroOrdinaryAlive(void){
  if(!active)return 1;char n[33],text[64];uint64_t now=file_now();
  if(revoked||now>=expires||!read_value(gate,"nonce",n,sizeof(n))||strcmp(n,nonce)||!read_value(gate,"untilFileTime",text,sizeof(text))){revoked=1;return 0;}
  uint64_t until;if(!decimal(text,&until)||until<=now||until-now>30000000ULL){revoked=1;return 0;}return 1;
}
int FzeroOrdinaryConfigure(const char *config){
  FzeroControlsPlan plan;if(!active||!FzeroControlsPlanFile(config,&plan)||!plan.ok)return 0;
  if(!FzeroControlsWrite(config,guid,&plan)||FzeroControlsPending(config,&plan))return 0;
  if(!FzeroIniReadString(config,"ControlsApplied","Revision",revision,sizeof(revision))||strcmp(revision,plan.revision))return 0;
  return FzeroGamepadDiagnosticStart(config,guid);
}
void FzeroOrdinaryInput(unsigned frame){
  frame_now=frame;memset(raw,0,sizeof(raw));raw[1]=raw[2]=-32768;buttons=0;
  if(!FzeroOrdinaryAlive()){FzeroGamepadDiagnosticSample(raw,0);return;}
  if(frame==240)buttons=1u<<2; /* real mapped Start */
  if(frame==400||frame==600||frame==800)buttons=1u; /* mapped Confirm */
  if(frame>=1100&&frame<2000)raw[1]=32767;
  if(frame>=1500&&frame<=1530)raw[0]=-28672;
  if(frame>=1800&&frame<=1830)raw[0]=28672;
  if(frame>=2100&&frame<2160)raw[2]=32767;
  if(frame>=2200&&frame<3000)raw[1]=32767;
  FzeroGamepadDiagnosticSample(raw,buttons);
}
void FzeroOrdinaryObserve(unsigned frame,uint32_t word,const uint8_t *ram,size_t size,unsigned controllers){
  if(!active||!ram||size<=0xbe1)return;frame_now=frame;word_now=word;controllers_now=controllers;
  memcpy(state,ram+0x54,3);x=(ram[0xb70]|ram[0xb71]<<8)&8191;y=(ram[0xb90]|ram[0xb91]<<8)&4095;yaw=ram[0xbe1];
  uint8_t packet[324];unsigned source=0;char hash[65]="";size_t n=FzeroTelemetryLastSent(packet,&source);if(n)hex(packet,n,hash);
  double values[3]={0};int canonical[3]={0};FzeroGamepadDiagnosticObserved(values,canonical);
  fprintf(trace,"{\"nonce\":\"%s\",\"frame\":%u,\"word\":%u,\"state\":[%u,%u,%u],\"x\":%u,\"y\":%u,\"yaw\":%u,\"raw\":[%d,%d,%d],\"normalized\":[%.9f,%.9f,%.9f],\"canonical\":[%d,%d,%d],\"sourcePort\":%u,\"packetSha256\":\"%s\"}\n",nonce,frame,word,state[0],state[1],state[2],x,y,yaw,raw[0],raw[1],raw[2],values[0],values[1],values[2],canonical[0],canonical[1],canonical[2],source,hash);
  if(frame%30==0)fflush(trace);if(controllers)revoked=1;
  if(frame%6==0)publish("running");
}
int FzeroOrdinaryHostKey(unsigned frame){if(active&&!host_sent&&frame>=2400){host_sent=1;return 1;}return 0;}
int FzeroOrdinaryPanelKey(unsigned row){
  if(!active)return 0;if(!FzeroOrdinaryAlive())return 3;
  ++ui_tick;publish("panel");
  if(ui_tick<12 && row<5)return 1;
  if(row==5 && !show_fps && ui_tick<15)return 2;
  return ui_tick>=150?3:0;
}
void FzeroOrdinaryPanelState(int open,int fps,int saved){if(!active)return;ui_open=open;show_fps=fps;if(saved)ui_saved=1;publish(open?"panel":"running");}
void FzeroOrdinaryFinish(void){if(!active)return;publish(revoked?"revoked":"done");if(trace){fclose(trace);trace=NULL;}}
