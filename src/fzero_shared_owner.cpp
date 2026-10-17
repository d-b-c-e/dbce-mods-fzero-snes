#include "fzero_shared_owner.h"
#include "vendor/controls/dbce_controls.hpp"
#include <algorithm>
#include <cstring>
#include <sstream>
#include <set>

namespace {
using Bytes = std::array<unsigned char,16>;
// Canonical parser has already validated GUID syntax. Convert its normalized
// text to the native Windows GUID byte layout, including the three LE fields.
Bytes bytes(const std::string &text) {
  Bytes out{}; unsigned char raw[16]{}; int index=0, high=-1;
  for(char c:text) {
    int n=c>='0'&&c<='9'?c-'0':c>='a'&&c<='f'?c-'a'+10:-1;
    if(n<0)continue;
    if(high<0)high=n;else {if(index<16)raw[index++]=static_cast<unsigned char>(high*16+n);high=-1;}
  }
  if(index!=16 || high>=0)return out;
  const int order[16]={3,2,1,0,5,4,7,6,8,9,10,11,12,13,14,15};
  for(int i=0;i<16;++i)out[i]=raw[order[i]];
  return out;
}
bool same(const void *a,const void *b){return std::memcmp(a,b,16)==0;}
}
bool FzeroSharedSteeringMatches(const char *profile,size_t length,const unsigned char force[16]) {
  if(!profile || !length || length>131072 || !force)return false;
  std::istringstream input(std::string(profile,length));std::string line,section;
  std::vector<std::string> lines;
  while(std::getline(input,line)) {
    auto s=dbce::controls::trim(line);
    if(!s.empty() && s[0]=='['){section=s;for(char &c:section)if(c>='A'&&c<='Z')c+= 'a'-'A';continue;}
    if(section=="[controls]")lines.push_back(s);
  }
  auto parsed=dbce::controls::parseSection(lines);
  auto steer=dbce::controls::find(parsed,"steer");
  return parsed.error.empty() && parsed.invalid.empty() && steer && same(bytes(steer->dev).data(),force);
}
bool FzeroSharedOwner::prepare(const char *profile,size_t length,const unsigned char force[16],
                              FzeroSharedOwnerApi api,std::string &why) {
  close();
  auto fail=[&](const char *s){why=s;close();return false;};
  if(!profile || !length || length>131072 || !force || !api.enumerate || !api.guid ||
      !api.open || !api.info || !api.read || !api.path || !api.close)
    return fail("owned snapshot exports/profile missing");
  std::istringstream input(std::string(profile,length));
  std::vector<std::string> controls,extras;std::string line,section;
  while(std::getline(input,line)) {
    auto s=dbce::controls::trim(line);
    if(!s.empty() && s[0]=='['){section=s;for(char &c:section)if(c>='A'&&c<='Z')c+= 'a'-'A';continue;}
    if(section=="[controls]")controls.push_back(s);
    else if(section=="[extras]")extras.push_back(s);
  }
  auto parsed=dbce::controls::parseSection(controls);
  if(!parsed.error.empty() || !parsed.invalid.empty())return fail("owned source requires a valid schema1 Controls profile");
  const auto *steer=dbce::controls::find(parsed,"steer");
  if(!steer || !same(bytes(steer->dev).data(),force))return fail("steering instance must equal the selected force wheel");
  std::vector<dbce::controls::Binding> bindings;
  for(const auto &entry:parsed.bound)bindings.push_back(entry.binding);
  std::set<std::string> extra_keys;
  for(const auto &s:extras) {
    if(s.empty() || s[0]=='#' || s[0]==';')continue;
    auto eq=s.find('=');if(eq==std::string::npos)continue;
    auto key=dbce::controls::trim(s.substr(0,eq)),value=dbce::controls::trim(s.substr(eq+1));
    if(key=="Schema" || key=="Game" || key=="Declaration")continue;
    if(!extra_keys.insert(key).second)return fail("duplicate extra binding");
    if(value.empty())continue;
    dbce::controls::Binding binding;std::string reason;
    if(!dbce::controls::parse(value,binding,reason))return fail("invalid extra source binding");
    bindings.push_back(binding); // Core still owns declaration/action admission.
  }
  std::vector<Bytes> wanted;
  for(const auto &binding:bindings) {
    auto id=bytes(binding.dev);
    if(std::find(wanted.begin(),wanted.end(),id)==wanted.end())wanted.push_back(id);
  }
  if(wanted.empty() || wanted.size()>capacity)return fail("owned source slot capacity exceeded");
  api_=api;
  int total=api.enumerate();if(total<1 || total>256)return fail("owned source enumeration unavailable or oversized");
  std::vector<int> indices;
  for(const auto &id:wanted) {
    int match=-1;
    for(int i=0;i<total;++i) {Bytes actual{};if(api.guid(i,actual.data()) && actual==id){if(match>=0)return fail("ambiguous source instance");match=i;}}
    if(match<0)return fail("configured source instance missing");
    indices.push_back(match);
  }
  // Identity/capacity preflight completes before the first source slot opens.
  for(size_t i=0;i<wanted.size();++i) {
    opened_=true;int slot=api.open(indices[i]);if(slot<0)return fail("owned source open refused");
    slots_.push_back(slot);
    WheelReadDeviceInfoV1 info{};info.struct_size=sizeof(info);info.abi=1;
    if(api.info(slot,&info)!=WHEEL_READ_OK || info.struct_size!=sizeof(info) || info.abi!=1 ||
        !info.generation || !same(info.instance,wanted[i].data()) ||
        (same(wanted[i].data(),force) && !(info.flags&1)))return fail("actual opened identity/borrowed wheel qualification failed");
    for(const auto &old:devices_)if(old.generation==info.generation)return fail("duplicate source generation");
    for(const auto &binding:bindings)if(bytes(binding.dev)==wanted[i]) {
      if(!binding.prod.empty() && !same(info.product,bytes(binding.prod).data()))return fail("actual source product mismatch");
      using Kind=dbce::controls::Kind;
      if(binding.kind==Kind::Axis && (!(info.axis_mask&(1u<<binding.index)) || !(info.range_mask&(1u<<binding.index)) ||
          info.minimum[binding.index]!=binding.min || info.maximum[binding.index]!=binding.max))return fail("actual source axis/range mismatch");
      if(binding.kind==Kind::Button && !(info.button_mask[binding.index/8]&(1u<<(binding.index%8))))return fail("actual source button absent");
      if(binding.kind==Kind::Hat && !(info.pov_mask&(1u<<binding.index)))return fail("actual source hat absent");
    }
    std::array<uint16_t,260> path{};
    if(api.path(slot,info.generation,path.data(),260)!=WHEEL_READ_OK || !path[0] ||
        std::find(path.begin(),path.end(),0)==path.end())return fail("qualified owned interface path unavailable");
    std::array<wchar_t,260> wide{};
    for(size_t k=0;k<path.size();++k)wide[k]=path[k];
    paths_.push_back(wide);devices_.push_back(info);
  }
  sequences_.assign(wanted.size(),0);ends_.assign(wanted.size(),0);failed_=false;return true;
}
bool FzeroSharedOwner::sample(WheelReadSnapshotV1 out[capacity],std::string &why) {
  if(out)for(int i=0;i<capacity;++i){out[i]={};out[i].struct_size=sizeof(out[i]);out[i].abi=1;for(auto &p:out[i].pov)p=0xffffffffu;}
  if(!out || failed_ || !opened_){why="owned source inactive; explicit restart required";return false;}
  for(size_t i=0;i<slots_.size();++i) {
    auto &s=out[i];
    if(api_.read(slots_[i],&s)!=WHEEL_READ_OK || s.struct_size!=sizeof(s) || s.abi!=1 || !s.valid ||
        s.generation!=devices_[i].generation || s.sequence<=sequences_[i] || s.read_start_us<=0 ||
        s.read_start_us<ends_[i] || s.read_end_us<s.read_start_us || s.poll_call!=WHEEL_READ_CALL_OK || s.state_call!=WHEEL_READ_CALL_OK || s.poll_hresult<0 || s.state_hresult<0 || s.reserved || s.reserved2) {
      failed_=true;why="owned read failed/stale/reopened; restart to rearm";
      for(int k=0;k<capacity;++k){out[k]={};out[k].struct_size=sizeof(out[k]);out[k].abi=1;for(auto &p:out[k].pov)p=0xffffffffu;}
      return false;
    }
    sequences_[i]=s.sequence;ends_[i]=s.read_end_us;
  }
  return true;
}
void FzeroSharedOwner::close() {
  failed_=true;if(opened_ && api_.close)api_.close();opened_=false;
  api_={};slots_.clear();devices_.clear();paths_.clear();sequences_.clear();ends_.clear();
}
