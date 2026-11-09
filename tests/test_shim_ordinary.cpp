// The actual host against an ABI fake: no device, force driver or UDP socket.
#include "fzero_shim.h"
#include "fzero_shim_map.h"
#include "fzero_telemetry.h"
extern "C" {
#include "fzero_output_latch.h"
#include "fzero_playthrough.h"
}
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <cstring>
#include <process.h>
#include <io.h>
static unsigned checks;
#define CHECK(x) do {++checks;if(!(x)){std::fprintf(stderr,"%d: %s\n",__LINE__,#x);return 1;}}while(0)
static std::string read(const std::filesystem::path &p) {
 std::ifstream f(p,std::ios::binary);return {(std::istreambuf_iterator<char>(f)),{}};
}
int main(int argc,char **argv) {
 CHECK(argc==2);const std::string mode=argv[1];
 for(const char *key:{"FZERO_SHIM_DIAGNOSTIC_RUN","FZERO_SHIM_SESSION_NONCE","FZERO_SHIM_SESSION_FILE",
     "FZERO_SHIM_BATCH_FILE","FZERO_FAKE_BAD_ABI","FZERO_FAKE_BAD_RECORD","FZERO_FAKE_BAD_STOP",
     "FZERO_FAKE_BAD_PROFILE","FZERO_FAKE_BAD_PROFILE_INFO","FZERO_FAKE_MISSING","FZERO_FAKE_UNBOUND","FZERO_FAKE_ORDINARY_LATCH",
     "FZERO_FAKE_BAD_EXTRAS_ABI","FZERO_FAKE_BAD_EXTRAS_INFO","FZERO_FAKE_EXTRAS_MISSING","FZERO_FAKE_EXTRAS_SEQ","FZERO_FAKE_EXTRAS_COUNT"}) _putenv_s(key,"");
 auto temp=std::filesystem::temp_directory_path();
 auto root=temp/("fzo-"+std::to_string(_getpid())+"-"+mode);
 CHECK(!std::filesystem::exists(root) && std::filesystem::create_directory(root));
 auto cfg=root/"config.ini";
 const std::string profile="[Controls]\r\nSchema=1\r\nProfile=fixture\r\nsteer = axis 0 dev={11111111-2222-3333-4444-555555555555} "
  "prod={0006346e-0000-0000-0000-504944564944} range=0..65535 rest=32768 travel=-1\r\n";
 const bool force=mode=="force" || mode=="replay";
 const bool hosts=mode.rfind("hosts",0)==0;
 const bool extras=mode.rfind("extras",0)==0 || hosts;
 const std::string extras_profile="[Extras]\r\nSchema=1\r\nGame=fzero-snes\r\nDeclaration=c4ffc4c85f1b7f72437e5871bdec57f9d5b997a62a9fdd3fd2208fefa0d98fb1\r\nfzero-snes.x=button 9 dev={11111111-2222-3333-4444-555555555555}\r\n";
 std::string extra_section=extras && mode!="extrasnosection" ? extras_profile : "";
 if(hosts){auto at=extra_section.find("c4ffc4c85f1b7f72437e5871bdec57f9d5b997a62a9fdd3fd2208fefa0d98fb1");extra_section.replace(at,64,"761789a517b35b88395d6b3ba06e2d2ba60628168abda6637e69d581a7a51cf7");}
 const bool extras_first=mode=="extrasfirst" || mode=="extrasbom";
 const bool force_success=force || mode.rfind("launcher",0)==0 || mode.rfind("hosts",0)==0 || mode.rfind("extras",0)==0 || mode=="configlimit" || mode=="configoversize" || mode=="extrasconfiglimit" || mode=="extrasconfigoversize";
 std::string original=(mode=="extrasbom"?std::string("\xef\xbb\xbf"):"")+
  (extras_first?extra_section:"")+
  "[ForceFeedback]\r\nEnabled="+(force_success?"1":"0")+"\r\nStrength=12\r\n[Telemetry]\r\nEnabled=1\r\n"+
  (mode=="launcherempty"?"":profile)+(extras_first?"":extra_section)+
  (mode=="extrasduplicate"?extras_profile:"");
 const bool config_limit=mode=="configlimit" || mode=="extrasconfiglimit";
 const bool config_oversize=mode=="configoversize" || mode=="extrasconfigoversize";
 if(config_limit || config_oversize) {
  original+="[Padding]\r\n;"; // outside the exact sections passed to Core
  const size_t bytes=1024*1024+(config_oversize?1:0);
  CHECK(original.size()<bytes);
  original.append(bytes-original.size(),'p'); // no terminal newline, including the exact-limit case
 }
 {std::ofstream f(cfg,std::ios::binary);f<<original;}
 if(mode=="session") _putenv_s("FZERO_SHIM_SESSION_NONCE","noordinaryinjection");
 const bool launcher_mode=mode.rfind("launcher",0)==0;
 const bool prerecord=mode=="replay" || (hosts ? mode=="hostsrecord" : false);
 bool prepared=FzeroShimPrepare(cfg.string().c_str(),!launcher_mode,mode=="replay",0,
     prerecord,
     launcher_mode && mode!="launcher")!=0;
 if(mode=="replay") {
  CHECK(prepared && !FzeroShimUsingInput() && !FzeroNoForceLatched());
  CHECK(FzeroShimStart(nullptr,"fixture") && FzeroShimStop());
 } else if(mode=="launcherempty") {
  CHECK(prepared && FzeroShimOrdinary() && FzeroNoForceLatched());
  CHECK(!std::filesystem::exists(root/"diagnostics") && FzeroShimStop()); // Configure/quit has no owner to stop
 } else if(mode=="hostsrecord") {
  CHECK(prepared && !FzeroShimUsingInput() && !FzeroNoForceLatched());
  CHECK(FzeroShimStart(nullptr,"fixture") && FzeroShimStop());
 } else if(mode=="unknown" || mode=="session" || mode=="launcher" || mode=="extrasnosection" || mode=="extrasduplicate" || mode=="hostsnoflag" || config_oversize) {
  CHECK(!prepared && !std::filesystem::exists(root/"diagnostics"));
  CHECK(!FzeroTelemetryDeliveryBlocked());
  if(config_oversize) {
   CHECK(FzeroShimUsingInput() && FzeroNoForceLatched());
   CHECK(!FzeroShimStart(nullptr,"fixture") && !FzeroShimStop());
   CHECK(!std::filesystem::exists(root/"diagnostics")); // refusal before opening the provider
  }
 } else {
  CHECK(prepared && FzeroShimUsingInput() && FzeroShimOrdinary() && !FzeroShimRequested());
  CHECK(!FzeroShimLatched() && FzeroNoForceLatched() && !FzeroTelemetryDeliveryBlocked());
  CHECK(!FzeroLatchForceOutput()); // the real native-force admission latch refuses a second owner
  CHECK(!*FzeroShimRecordingPath() && !*FzeroShimSaveRoot());
  CHECK(!std::filesystem::exists(root/"diagnostics")); // Prepare is read-only
  if(mode=="forcechanged") {
   auto changed=original;
   changed.replace(changed.find("Enabled=0"),9,"Enabled=1");
   std::ofstream f(cfg,std::ios::binary);f<<changed;
  }
  std::string applied_profile=profile;
  if(mode=="launchersaved") {
   applied_profile.replace(applied_profile.find("Profile=fixture"),std::strlen("Profile=fixture"),"Profile=launcher-saved");
   auto changed=original;changed.replace(changed.find(profile),profile.size(),applied_profile);
   std::ofstream f(cfg,std::ios::binary);f<<changed;
  }
  if(mode=="profile") _putenv_s("FZERO_FAKE_BAD_PROFILE","1");
  if(mode=="profileinfo") _putenv_s("FZERO_FAKE_BAD_PROFILE_INFO","1");
  if(mode=="extrasabi") _putenv_s("FZERO_FAKE_BAD_EXTRAS_ABI","1");
  if(mode=="extrasinfo") _putenv_s("FZERO_FAKE_BAD_EXTRAS_INFO","1");
  int previous_stderr=_dup(_fileno(stderr));CHECK(previous_stderr>=0);
  FILE *log=nullptr;CHECK(!freopen_s(&log,(root/"startup.log").string().c_str(),"wb",stderr));
  const bool started=FzeroShimStart(nullptr,"fixture")!=0;
  CHECK(FzeroShimHasExtras()==(started && extras));
  CHECK(FzeroShimHasHostControls()==(started && hosts));
  std::fflush(stderr);CHECK(!_dup2(previous_stderr,_fileno(stderr)));_close(previous_stderr);
  auto startup=read(root/"startup.log");
  const std::string accepted="[fzero-shim] ordinary profile accepted:\n";
  if(started) {
   auto at=startup.find(accepted);CHECK(at!=std::string::npos && startup.find(accepted,at+1)==std::string::npos);
   CHECK(startup.find("revision=fixture-revision\n")!=std::string::npos);
   CHECK(startup.find("sha256=0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef\n")!=std::string::npos);
  } else CHECK(startup.find(accepted)==std::string::npos);
  if(mode=="forcechanged" || mode=="extrasabi") {
   CHECK(!started && !std::filesystem::exists(root/"diagnostics"));
   CHECK(!FzeroShimStop());
   {std::ofstream f(cfg,std::ios::binary);f<<original;}
  } else {
   auto entries=std::filesystem::directory_iterator(root/"diagnostics");
   auto run=entries->path();++entries;CHECK(entries==std::filesystem::directory_iterator());
   CHECK(read(run/"profile.spy")==applied_profile+extra_section && read(run/"flags.spy")=="0");
   if(extras) CHECK(read(run/"declaration.spy")== (hosts?kFzeroShimHostDeclaration:kFzeroShimExtrasDeclaration));
   else CHECK(!std::filesystem::exists(run/"declaration.spy"));
   if(mode=="profile" || mode=="profileinfo" || mode=="extrasinfo") CHECK(!started && !FzeroShimStop());
   else {
    CHECK(started);
    if(mode=="unexpected") _putenv_s("FZERO_FAKE_ORDINARY_LATCH","1");
    if(mode=="missing") {_putenv_s("FZERO_FAKE_MISSING","1");_putenv_s("FZERO_FAKE_UNBOUND","1");}
    if(mode=="extrasmissing") _putenv_s("FZERO_FAKE_EXTRAS_MISSING","1");
    if(mode=="extrasseq") _putenv_s("FZERO_FAKE_EXTRAS_SEQ","1");
    if(mode=="extrascount") _putenv_s("FZERO_FAKE_EXTRAS_COUNT","1");
    uint32_t word=0; uint8_t scene[0x56]{};
    scene[0x54]=2; scene[0x55]=3;
    if(mode=="unexpected" || mode=="extrasseq" || mode=="extrascount") CHECK(!FzeroShimInput(&word,scene,sizeof(scene)) && !FzeroShimStop());
    else {
     CHECK(FzeroShimInput(&word,scene,sizeof(scene)));
     if(extras) CHECK((word&0xe02u)==(mode=="extrasmissing"?0:0xe02u));
     if(hosts)CHECK((word&FZERO_HOST_BUTTON_MASK)==FZERO_HOST_BUTTON_MASK);
     if(mode=="missing") {
      CHECK((word&0xc0)==0); // no steering fallback
      CHECK((FzeroShimSupplementalMask()&0x100)==0); // missing Confirm never falls back
     }
     CHECK(FzeroShimConsumed(std::numeric_limits<uint64_t>::max(),word,scene,sizeof(scene)));
     CHECK(!FzeroShimStop());
    }
   }
  }
 }
 std::filesystem::remove_all(root);
 std::fprintf(stderr,"%s ok (%u checks)\n",mode.c_str(),checks);
 return 0;
}