// Implements the actual dynamically loaded ABI without any device or worker.
// Only the isolated offline test directory stages this file as dbce_shim.dll.
#define DBCE_SHIM_BUILD
#include <cstddef>
#include "../vendor/dbce-shim/dbce_shim.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>

static const char *names[] = {
 "steer","throttle","brake","clutch","handbrake","shiftUp","shiftDown","neutral","reverse",
 "confirm","back","start","select","navUp","navDown","navLeft","navRight","camera","lookBack",
 "reset","horn","modMenu","muteFfb","gear1","gear2","gear3","gear4","gear5","gear6","gear7","gear8"
};
struct Fake { uint64_t seq=0; bool latched=false; std::filesystem::path folder; };
static bool flag(const char *name) { auto s=std::getenv(name); return s && *s; }
int32_t dbce_shim_abi_version(void) { return flag("FZERO_FAKE_BAD_ABI") ? 1 : 2; }
int32_t dbce_shim_frame_size(void) { return sizeof(dbce_shim_frame); }
int32_t dbce_shim_open_args_size(void) { return sizeof(dbce_shim_open_args); }
const char *dbce_shim_action_name(int32_t i) { return i>=0&&i<31 ? names[i] : nullptr; }
int32_t dbce_shim_open2(const dbce_shim_open_args *a,void **out,char *why,int32_t n) {
 if (n>0) why[0]=0;
 if (!a || a->struct_size!=128 || (a->flags!=DBCE_SHIM_CAPTURE && a->flags!=3) ||
     !a->folder || !a->nonce || !*a->nonce || !a->capture_folder || !a->profile ||
     a->hz<=0 || a->seconds<=0 || a->seconds>300 ||
     !a->game_channels || !std::strstr(a->game_channels,"ffb.spring") || !std::strstr(a->game_channels,"telemetry.speedMps")) return -1;
 bool latched=(a->flags&DBCE_SHIM_LATCHED)!=0;
 if (latched ? (!a->force_absent || !a->fences_verified || !a->session_file || !a->batch_file ||
     std::strcmp(a->nonce,"fixture123") || !std::strstr(a->context,"diagnostic-injected")) :
     (a->force_absent || a->fences_verified || a->session_file || a->batch_file)) return -1;
 auto f=new Fake; f->folder=a->folder; f->latched=latched; *out=f;
 std::ofstream p(f->folder/"profile.spy",std::ios::binary); p.write(a->profile,a->profile_len);
 return 0;
}
int32_t dbce_shim_tick(void *handle,dbce_shim_frame *out) {
 auto f=static_cast<Fake *>(handle);
 if (!f || !out || out->struct_size!=296) return -1;
 *out={}; out->struct_size=296; out->abi=2; out->seq=++f->seq;
 for(auto &a:out->action) a.status=12;
 out->action[0].value=.25f; out->action[1].value=1;
 if(f->latched) {
   out->latched=out->suppress_native=1; out->run=f->seq==1?3:4;
   for(auto &a:out->action) {a.injected=1; if(out->run==4){a.status=13;a.value=0;}}
   if(flag("FZERO_FAKE_EXPIRED")) out->run=5;
   if(flag("FZERO_FAKE_NO_PROVENANCE")) out->action[0].injected=0;
 }
 return 0;
}
int32_t dbce_shim_record(void *handle,uint64_t seq,const double *v,int32_t count) {
 auto f=static_cast<Fake *>(handle);
 if (flag("FZERO_FAKE_BAD_RECORD") || !f || seq!=f->seq || count!=17) return -5;
 std::ofstream row(f->folder/"consumed.spy",std::ios::app|std::ios::binary);
 row<<seq<<" "<<(uint64_t)v[0]<<" "<<(uint32_t)v[1]<<"\n";
 std::ofstream signals(f->folder/"signals.spy",std::ios::app|std::ios::binary);
 for(int i=2;i<count;++i) signals<<v[i]<<" "; signals<<"\n";
 return 0;
}
int32_t dbce_shim_stop(void *handle,char *text,int32_t n,int32_t *needed) {
 auto f=static_cast<Fake *>(handle);
 if (n>0) text[0]=0; if (needed) *needed=1;
 std::ofstream(f->folder/"stop.spy")<<"stopped";
 delete f;
 return flag("FZERO_FAKE_BAD_STOP") ? 1 : 0;
}
