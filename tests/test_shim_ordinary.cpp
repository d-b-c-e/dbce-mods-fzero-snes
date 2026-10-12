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
#include <process.h>
static unsigned checks;
#define CHECK(x) do {++checks;if(!(x)){std::fprintf(stderr,"%d: %s\n",__LINE__,#x);return 1;}}while(0)
static std::string read(const std::filesystem::path &p) {
 std::ifstream f(p,std::ios::binary);return {(std::istreambuf_iterator<char>(f)),{}};
}
int main(int argc,char **argv) {
 CHECK(argc==2);const std::string mode=argv[1];
 for(const char *key:{"FZERO_SHIM_DIAGNOSTIC_RUN","FZERO_SHIM_SESSION_NONCE","FZERO_SHIM_SESSION_FILE",
     "FZERO_SHIM_BATCH_FILE","FZERO_FAKE_BAD_ABI","FZERO_FAKE_BAD_RECORD","FZERO_FAKE_BAD_STOP",
     "FZERO_FAKE_BAD_PROFILE","FZERO_FAKE_MISSING","FZERO_FAKE_UNBOUND","FZERO_FAKE_ORDINARY_LATCH"}) _putenv_s(key,"");
 auto temp=std::filesystem::temp_directory_path();
 auto root=temp/("fzo-"+std::to_string(_getpid())+"-"+mode);
 CHECK(!std::filesystem::exists(root) && std::filesystem::create_directory(root));
 auto cfg=root/"config.ini";
 const std::string profile="[Controls]\r\nSchema=1\r\nProfile=fixture\r\nsteer = axis 0 dev={11111111-2222-3333-4444-555555555555} "
  "prod={0006346e-0000-0000-0000-504944564944} range=0..65535 rest=32768 travel=-1\r\n";
 const bool force=mode=="force" || mode=="replay" || mode=="native";
 const std::string original="[Input]\r\nBackend="+std::string(mode=="native"?"Native":mode=="unknown"?"Typo":"Shared")+
  "\r\n"+profile+"[ForceFeedback]\r\nEnabled="+(force?"1":"0")+"\r\nStrength=12\r\n[Telemetry]\r\nEnabled=1\r\n";
 {std::ofstream f(cfg,std::ios::binary);f<<original;}
 if(mode=="session") _putenv_s("FZERO_SHIM_SESSION_NONCE","noordinaryinjection");
 bool prepared=FzeroShimPrepare(cfg.string().c_str(),0,mode=="replay",0,1)!=0;
 if(mode=="native" || mode=="replay") {
  CHECK(prepared && !FzeroShimUsingInput() && !FzeroNoForceLatched());
  CHECK(FzeroShimStart(nullptr,"fixture") && FzeroShimStop());
 } else if(mode=="force" || mode=="unknown" || mode=="session") {
  CHECK(!prepared && !std::filesystem::exists(root/"diagnostics"));
  CHECK(!FzeroTelemetryDeliveryBlocked());
 } else {
  CHECK(prepared && FzeroShimUsingInput() && FzeroShimOrdinary() && !FzeroShimRequested());
  CHECK(!FzeroShimLatched() && FzeroNoForceLatched() && !FzeroTelemetryDeliveryBlocked());
  CHECK(!FzeroLatchForceOutput()); // the real native-force admission latch refuses a second owner
  CHECK(!*FzeroShimRecordingPath() && !*FzeroShimSaveRoot());
  CHECK(!std::filesystem::exists(root/"diagnostics")); // Prepare is read-only
  if(mode=="forcechanged") { // edit existing key, not a duplicate-section test
   auto changed=original;changed.replace(changed.find("Enabled=0"),9,"Enabled=1");
   std::ofstream f(cfg,std::ios::binary);f<<changed;
  }
  if(mode=="profile") _putenv_s("FZERO_FAKE_BAD_PROFILE","1");
  const bool started=FzeroShimStart(nullptr,"fixture")!=0;
  if(mode=="forcechanged") {
   CHECK(!started && !std::filesystem::exists(root/"diagnostics"));
   CHECK(!FzeroShimStop());
   {std::ofstream f(cfg,std::ios::binary);f<<original;}
  } else {
   auto entries=std::filesystem::directory_iterator(root/"diagnostics");
   auto run=entries->path();++entries;CHECK(entries==std::filesystem::directory_iterator());
   CHECK(read(run/"profile.spy")==profile && read(run/"flags.spy")=="0");
   if(mode=="profile") CHECK(!started && !FzeroShimStop());
   else {
    CHECK(started);
    if(mode=="unexpected") _putenv_s("FZERO_FAKE_ORDINARY_LATCH","1");
    if(mode=="missing") {_putenv_s("FZERO_FAKE_MISSING","1");_putenv_s("FZERO_FAKE_UNBOUND","1");}
    uint32_t word=0;
    if(mode=="unexpected") CHECK(!FzeroShimInput(&word) && !FzeroShimStop());
    else {
     CHECK(FzeroShimInput(&word));
     if(mode=="missing") {
      CHECK((word&0xc0)==0); // no steering fallback
      CHECK((FzeroShimSupplementalMask()&0x100)==0); // missing Confirm never falls back
      CHECK(FzeroShimSupplementalMask()&1u); // explicitly Unbound Back retains legacy button
     }
     CHECK(FzeroShimConsumed(0,word,nullptr,0));
     _putenv_s("FZERO_FAKE_BAD_RECORD","1"); // ordinary must never call shared capture
     CHECK(FzeroShimInput(&word));
     FzeroShimNeutral(); // host overlay cancels this pending guest frame
     CHECK(FzeroShimOverlayInput(&word) && word==0); // pedal/steering never navigate host UI
     CHECK(FzeroShimInput(&word));
     uint32_t consumed=FzeroShimGuestWord(word | (FzeroShimSupplementalMask()&0x400) | 0x40000000u);
     CHECK(FzeroShimConsumed(1,consumed,nullptr,0));
     // Same final merged word enters the actual .fzpt writer used after RtlRunFrame.
     FzeroPlaythrough tape{},replay{};uint8_t digest[32]={7},ram[8]={1};
     auto path=(root/"ordinary.fzpt").string();
     CHECK(FzeroPlaythroughRecordOpen(&tape,path.c_str(),digest));
     CHECK(FzeroPlaythroughRecordFrame(&tape,consumed,ram,sizeof(ram)) && FzeroPlaythroughClose(&tape));
     CHECK(FzeroPlaythroughPlaybackOpen(&replay,path.c_str(),digest));
     CHECK(FzeroPlaythroughNextInput(&replay,&word) && word==consumed);
     CHECK(FzeroPlaythroughVerifyFrame(&replay,ram,sizeof(ram)) && FzeroPlaythroughClose(&replay));
     CHECK(FzeroShimStop());
    }
    CHECK(!std::filesystem::exists(run/"consumed.spy") && !std::filesystem::exists(run/"capture"));
   }
   CHECK(read(run/"stop.spy")=="stopped");
  }
 }
 CHECK(read(cfg)==original);
 CHECK(std::filesystem::canonical(root).parent_path()==std::filesystem::canonical(temp));
 std::filesystem::remove_all(root);
 std::printf("ordinary host %s: %u checks\n",mode.c_str(),checks);return 0;
}
