// Implements the actual dynamically loaded ABI without any device or worker.
// Only the isolated offline test directory stages this file as dbce_shim.dll.
#define DBCE_SHIM_BUILD
#include <cstddef>
#include "../vendor/dbce-shim/dbce_shim.h"
#include "../vendor/dbce-shim/dbce_shim_extras.h"
#include "../vendor/dbce-shim/dbce_shim_bridge.h"
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
struct Fake { uint64_t seq=0; bool latched=false, capture=false, extras=false, bridge=false, published=false; std::filesystem::path folder; };
static bool flag(const char *name) { auto s=std::getenv(name); return s && *s; }
int32_t dbce_shim_abi_version(void) { return flag("FZERO_FAKE_BAD_ABI") ? 1 : 2; }
int32_t dbce_shim_frame_size(void) { return sizeof(dbce_shim_frame); }
int32_t dbce_shim_open_args_size(void) { return sizeof(dbce_shim_open_args); }
const char *dbce_shim_action_name(int32_t i) { return i>=0&&i<31 ? names[i] : nullptr; }
int32_t dbce_shim_open2(const dbce_shim_open_args *a,void **out,char *why,int32_t n) {
 if (n>0) why[0]=0;
 if (!a || a->struct_size!=128 || (a->flags!=0 && a->flags!=DBCE_SHIM_CAPTURE && a->flags!=3) ||
     !a->folder || !a->nonce || !*a->nonce || !a->profile) return -1;
 bool capture=(a->flags&DBCE_SHIM_CAPTURE)!=0;
 if (capture ? (!a->capture_folder || a->hz<=0 || a->seconds<=0 || a->seconds>300 ||
     !a->game_channels || !std::strstr(a->game_channels,"ffb.spring") || !std::strstr(a->game_channels,"telemetry.speedMps")) :
     (a->capture_folder || a->game_id || a->game_channels || a->context || a->adapter_version || a->hz || a->seconds)) return -1;
 bool latched=(a->flags&DBCE_SHIM_LATCHED)!=0;
 if (latched ? (!a->force_absent || !a->fences_verified || !a->session_file || !a->batch_file ||
     std::strcmp(a->nonce,"fixture123") || !std::strstr(a->context,"diagnostic-injected")) :
     (a->force_absent || a->fences_verified || a->session_file || a->batch_file)) return -1;
 auto f=new Fake; f->folder=a->folder; f->latched=latched; f->capture=capture; *out=f;
 std::ofstream p(f->folder/"profile.spy",std::ios::binary); p.write(a->profile,a->profile_len);
 std::ofstream(f->folder/"flags.spy")<<a->flags;
 return 0;
}
int32_t dbce_shim_profile_info(void *,char *text,int32_t len,int32_t *needed) {
 const char *info=flag("FZERO_FAKE_BAD_PROFILE") ? "ok=0\nerror=invalid profile\n" :
  "ok=1\nerror=\nprofile=fixture\nrevision=fixture-revision\ntransmission=Sequential\n"
  "sha256=0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef\n";
 *needed=(int32_t)std::strlen(info)+1;
 if(len<*needed) return -1;
 std::memcpy(text,info,*needed);
 if(flag("FZERO_FAKE_BAD_PROFILE_INFO")) text[*needed-1]='x';
 return 0;
}
static int32_t fill_tick(void *handle,dbce_shim_frame *out) {
 auto f=static_cast<Fake *>(handle);
 if (!f || !out || out->struct_size!=296) return -1;
 *out={}; out->struct_size=296; out->abi=2; out->seq=++f->seq;
 for(auto &a:out->action) a.status=12;
 out->action[0].value=.25f; out->action[1].value=1;
 if(flag("FZERO_FAKE_MISSING")) {out->action[0].status=3;out->action[9].status=3;}
 if(flag("FZERO_FAKE_UNBOUND")) out->action[10].status=0;
 if(flag("FZERO_FAKE_ORDINARY_LATCH")) out->latched=1;
 if(f->latched) {
   out->latched=out->suppress_native=1; out->run=f->seq==1?3:4;
   for(auto &a:out->action) {a.injected=1; if(out->run==4){a.status=13;a.value=0;}}
   if(flag("FZERO_FAKE_EXPIRED")) out->run=5;
   if(flag("FZERO_FAKE_NO_PROVENANCE")) out->action[0].injected=0;
 }
 return 0;
}
int32_t dbce_shim_tick(void *handle,dbce_shim_frame *out) {
 auto f=static_cast<Fake *>(handle);
 if (f && f->extras) return DBCE_SHIM_REFUSED; // common and extras cannot poll separately
 return fill_tick(handle,out);
}
int32_t dbce_shim_extras_abi_version(void) { return flag("FZERO_FAKE_BAD_EXTRAS_ABI") ? 0 : DBCE_SHIM_EXTRAS_ABI; }
int32_t dbce_shim_extras_frame_size(void) { return sizeof(dbce_shim_extras_frame); }
int32_t dbce_shim_open_extras(const wchar_t *folder,const char *nonce,const char *profile,int32_t profile_len,
 const char *unsupported,void *hwnd,const char *declaration,int32_t declaration_len,void **out,char *why,int32_t len) {
 dbce_shim_open_args a{};a.struct_size=sizeof(a);a.folder=folder;a.nonce=nonce;a.profile=profile;
 a.profile_len=profile_len;a.unsupported=unsupported;a.hwnd=hwnd;
 if (!declaration || declaration_len<=0) return DBCE_SHIM_BAD_ARGUMENT;
 int r=dbce_shim_open2(&a,out,why,len);if(r) return r;
 auto f=static_cast<Fake *>(*out);f->extras=true;
 std::ofstream d(f->folder/"declaration.spy",std::ios::binary);d.write(declaration,declaration_len);
 return DBCE_SHIM_OK;
}
int32_t dbce_shim_tick_extras(void *handle,uint32_t contexts,dbce_shim_frame *out,dbce_shim_extras_frame *x) {
 auto f=static_cast<Fake *>(handle);
 if (!f || !f->extras || contexts || !x || x->struct_size!=sizeof(*x)) return DBCE_SHIM_BAD_ARGUMENT;
 int r=fill_tick(handle,out);if(r) return r;
 *x={};x->struct_size=sizeof(*x);x->abi=DBCE_SHIM_EXTRAS_ABI;x->seq=out->seq;x->count=4;
 for(int i=0;i<4;++i){x->slot[i].status=flag("FZERO_FAKE_EXTRAS_MISSING")?3:12;x->slot[i].value=1;}
 if(flag("FZERO_FAKE_EXTRAS_SEQ")) ++x->seq;
 if(flag("FZERO_FAKE_EXTRAS_COUNT")) --x->count;
 std::ofstream(f->folder/"extras-tick.spy",std::ios::app|std::ios::binary)<<out->seq<<"\n";
 return DBCE_SHIM_OK;
}
int32_t dbce_shim_extras_info(void *,char *text,int32_t len,int32_t *needed) {
 const char *info="{\"acceptedSchema\":1,\"game\":\"fzero-snes\"}";
 *needed=(int32_t)std::strlen(info)+1;
 if(len<*needed) return DBCE_SHIM_BAD_ARGUMENT;
 std::memcpy(text,info,*needed);
 if(flag("FZERO_FAKE_BAD_EXTRAS_INFO")) *needed=len+1;
 return DBCE_SHIM_OK;
}
int32_t dbce_shim_record(void *handle,uint64_t seq,const double *v,int32_t count) {
 auto f=static_cast<Fake *>(handle);
 if (flag("FZERO_FAKE_BAD_RECORD") || !f || !f->capture || seq!=f->seq || count!=29) return -5;
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

int32_t dbce_shim_bridge_abi_version(){return 1;}
int32_t dbce_shim_bridge_device_size(){return sizeof(dbce_shim_bridge_device_v1);}
int32_t dbce_shim_bridge_sample_size(){return sizeof(dbce_shim_bridge_sample_v1);}
int32_t dbce_shim_bridge_open(const dbce_shim_open_args *a,void **out,char *why,int32_t len) {
 if(!a || a->flags)return DBCE_SHIM_REFUSED;
 int r=dbce_shim_open2(a,out,why,len);if(!r)static_cast<Fake*>(*out)->bridge=true;return r;
}
int32_t dbce_shim_bridge_devices_with_paths(void *handle,const dbce_shim_bridge_device_v1 *d,int32_t count,const wchar_t*const*paths) {
 auto f=static_cast<Fake*>(handle);if(!f || !f->bridge || count!=1 || !d || d[0].generation!=7 || !paths || !paths[0] || paths[0][0]!='x')return DBCE_SHIM_REFUSED;
 f->published=true;return 0;
}
int32_t dbce_shim_bridge_devices(void *,const dbce_shim_bridge_device_v1 *,int32_t){return DBCE_SHIM_REFUSED;}
static bool bridge_sample(void *handle,const dbce_shim_bridge_sample_v1 *s,int32_t count) {
 auto f=static_cast<Fake*>(handle);if(!f || !f->bridge || !f->published || count!=1 || !s || !s[0].valid || s[0].sequence!=f->seq+1)return false;
 std::ofstream(f->folder/"bridge-sample.spy",std::ios::app)<<s[0].sequence<<"\n";return true;
}
int32_t dbce_shim_bridge_tick(void *handle,const dbce_shim_bridge_sample_v1 *s,int32_t count,dbce_shim_frame *out) {
 auto f=static_cast<Fake*>(handle);if(!bridge_sample(handle,s,count) || f->extras)return DBCE_SHIM_REFUSED;return fill_tick(handle,out);
}
extern "C" DBCE_SHIM_API int32_t dbce_shim_bridge_open_extras(const dbce_shim_open_args *a,const char *declaration,int32_t declaration_len,void **out,char *why,int32_t len) {
 if(!a || a->flags)return DBCE_SHIM_REFUSED;
 int r=dbce_shim_open_extras(a->folder,a->nonce,a->profile,a->profile_len,a->unsupported,a->hwnd,declaration,declaration_len,out,why,len);
 if(!r)static_cast<Fake*>(*out)->bridge=true;return r;
}
extern "C" DBCE_SHIM_API int32_t dbce_shim_bridge_tick_extras(void *handle,const dbce_shim_bridge_sample_v1 *s,int32_t count,uint32_t contexts,dbce_shim_frame *out,dbce_shim_extras_frame *extras) {
 if(!bridge_sample(handle,s,count))return DBCE_SHIM_REFUSED;
 return dbce_shim_tick_extras(handle,contexts,out,extras);
}
