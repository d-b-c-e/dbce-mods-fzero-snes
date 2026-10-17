#include "fzero_shared_owner.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#undef assert
#define assert(x) do { if(!(x)){std::fprintf(stderr,"%d: %s\n",__LINE__,#x);std::abort();} } while(0)

namespace {
int opens,closes,reads,total=1;
bool wrong_product,borrowed=true,stale,bad_read,wrong_epoch;
uint64_t sequence;
int enumerate(){return total;}
int guid(int i,void *out){std::memset(out,i+1,16);return 1;}
int open(int i){++opens;return i;}
int info(int slot,WheelReadDeviceInfoV1 *out){
  assert(out->struct_size==144 && out->abi==1);
  *out={};out->struct_size=144;out->abi=1;out->generation=uint64_t(slot+10);
  std::memset(out->instance,slot+1,16);std::memset(out->product,wrong_product?9:2,16);
  out->axis_mask=out->range_mask=1;out->maximum[0]=65535;out->flags=borrowed?1:0;
  out->button_mask[0]=4;return 1;
}
int path(int,uint64_t,uint16_t *out,int cap){assert(cap==260);out[0]='x';out[1]=0;return 1;}
int read(int slot,WheelReadSnapshotV1 *out){
  ++reads;out->generation=uint64_t(slot+10+(wrong_epoch?1:0));out->sequence=stale?sequence:++sequence;
  out->read_start_us=1000+int64_t(sequence)*10;out->read_end_us=out->read_start_us+1;
  out->valid=bad_read?0:1;out->poll_call=out->state_call=0;return 1;
}
void close(){++closes;}
FzeroSharedOwnerApi api{enumerate,guid,open,info,read,path,close};
const char *profile="[Controls]\nSchema=1\nsteer=axis 0 dev={01010101-0101-0101-0101-010101010101} "
  "prod={02020202-0202-0202-0202-020202020202} range=0..65535 rest=32768 travel=+1\n";
}
int main(){
  unsigned char force[16];std::memset(force,1,16);std::string why;
  FzeroSharedOwner owner;WheelReadSnapshotV1 samples[8]{};
  assert(FzeroSharedSteeringMatches(profile,std::strlen(profile),force));
  unsigned char different[16]{};
  assert(!owner.prepare(profile,std::strlen(profile),different,api,why) && opens==0);
  auto missing=api;missing.read=nullptr;
  assert(!owner.prepare(profile,std::strlen(profile),force,missing,why) && opens==0);
  total=0;assert(!owner.prepare(profile,std::strlen(profile),force,api,why) && opens==0);total=1;
  wrong_product=true;assert(!owner.prepare(profile,std::strlen(profile),force,api,why) && closes==1);
  wrong_product=false;borrowed=false;
  assert(!owner.prepare(profile,std::strlen(profile),force,api,why) && closes==2);borrowed=true;
  assert(owner.prepare(profile,std::strlen(profile),force,api,why));
  assert(owner.count()==1 && owner.devices()[0].flags==1 && owner.path(0)[0]=='x');
  assert(owner.sample(samples,why) && reads==1 && samples[0].sequence==1);
  stale=true;assert(!owner.sample(samples,why) && reads==2 && !samples[0].valid);
  stale=false;assert(!owner.sample(samples,why) && reads==2); // no silent retry/rearm
  owner.close();assert(closes==3);owner.close();assert(closes==3);
  assert(owner.prepare(profile,std::strlen(profile),force,api,why));
  wrong_epoch=true;assert(!owner.sample(samples,why));wrong_epoch=false;owner.close();
  // Extras on a separate exact instance join the same owner and sample batch.
  total=2;sequence=0;
  std::string extras=std::string(profile)+"[Extras]\nSchema=1\nGame=fzero-snes\nfzero-snes.x=button 2 dev={02020202-0202-0202-0202-020202020202} prod={02020202-0202-0202-0202-020202020202}\n";
  assert(owner.prepare(extras.data(),extras.size(),force,api,why) && owner.count()==2);
  int before=reads;assert(owner.sample(samples,why) && reads==before+2);
  bad_read=true;assert(!owner.sample(samples,why) && !samples[0].valid && !samples[1].valid);
  before=reads;bad_read=false;assert(!owner.sample(samples,why) && reads==before);owner.close();
  // Capacity preflight is before any source open.
  std::string too_many=profile;
  too_many+="[Extras]\n";
  for(int i=2;i<=9;++i){char line[256];std::snprintf(line,sizeof(line),"extra%d=button 2 dev={%08x-0000-0000-0000-000000000000} prod={02020202-0202-0202-0202-020202020202}\n",i,i);too_many+=line;}
  before=opens;assert(!owner.prepare(too_many.data(),too_many.size(),force,api,why) && opens==before);
  std::puts("PASS owned source exact identity/product/borrowed handle, one sample/slot, capacity, extras and permanent stale/failure stop");
}
