#include "fzero_shim.h"
extern "C" {
#include "fzero_output_latch.h"
}
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <process.h>
static unsigned checks;
#define CHECK(x) do { ++checks; if(!(x)){std::fprintf(stderr,"%d: %s\n",__LINE__,#x);return 1;} } while(0)
static std::string read(const std::filesystem::path &p) {
 std::ifstream f(p,std::ios::binary); return std::string((std::istreambuf_iterator<char>(f)),{});
}
int main(int argc,char **argv) {
 CHECK(argc==2); std::string mode=argv[1];
 CHECK(mode=="off" || mode=="args" || mode=="replay" || mode=="normal" ||
       mode=="abi" || mode=="pending" || mode=="record" || mode=="stop");
 _putenv_s("FZERO_SHIM_DIAGNOSTIC_RUN","");
 _putenv_s("FZERO_FAKE_BAD_ABI",""); _putenv_s("FZERO_FAKE_BAD_RECORD",""); _putenv_s("FZERO_FAKE_BAD_STOP","");
 CHECK(FzeroShimPrepare("does-not-exist.ini",0,0,0,0));
 CHECK(!FzeroShimRequested() && !FzeroNoForceLatched());
 CHECK(FzeroShimStart(nullptr,nullptr) && FzeroShimStop());
 if(mode=="off") { std::printf("shim host off: %u checks\n",checks); return 0; }
 auto parent=std::filesystem::current_path()/("fzero-shim-host-"+mode+"-"+std::to_string(_getpid()));
 CHECK(!std::filesystem::exists(parent));
 CHECK(std::filesystem::create_directory(parent));
 auto cfg=parent/"config.ini", run=parent/"run";
 const std::string profile="[Controls]\r\nSchema=1\r\nProfile=fixture\r\nRevision=1\r\n"
   "steer = axis 0 dev={11111111-2222-3333-4444-555555555555} prod={0006346e-0000-0000-0000-504944564944} "
   "range=0..65535 rest=32768 travel=-1\r\n";
 const std::string original="[Other]\r\nKept=yes\r\n"+profile+"[ForceFeedback]\r\nEnabled=1\r\nStrength=12\r\n";
 {std::ofstream f(cfg,std::ios::binary); f<<original;}
 _putenv_s("FZERO_SHIM_DIAGNOSTIC_RUN",run.string().c_str());
 if(mode=="args" || mode=="replay") {
   CHECK(!FzeroShimPrepare(cfg.string().c_str(),mode!="args",mode=="replay",0,0));
   CHECK(FzeroShimRequested() && FzeroNoForceLatched()); CHECK(!std::filesystem::exists(run));
 } else {
   CHECK(FzeroShimPrepare(cfg.string().c_str(),1,0,0,0));
   CHECK(FzeroShimRequested() && FzeroNoForceLatched());
   CHECK(std::filesystem::path(FzeroShimRecordingPath())==run/"original.fzpt");
   CHECK(read(run/"controls.ini")==profile);
   if(mode=="abi") _putenv_s("FZERO_FAKE_BAD_ABI","1");
   int started=FzeroShimStart(nullptr,"fixture");
   if(mode=="abi") CHECK(!started && !FzeroShimStop());
   else {
     CHECK(started); CHECK(read(run/"profile.spy")==profile);
     uint32_t word=0; CHECK(FzeroShimInput(&word));
     if(mode=="pending") {
       CHECK(!FzeroShimInput(&word)); CHECK(!FzeroShimStop());
     } else if(mode=="record") {
       _putenv_s("FZERO_FAKE_BAD_RECORD","1");
       CHECK(!FzeroShimConsumed(0,word)); CHECK(!FzeroShimStop());
     } else {
       uint32_t consumed=word|(1u<<30)|(1u<<11);
       CHECK(FzeroShimConsumed(0,consumed));
       std::ostringstream expected; expected<<"1 0 "<<consumed<<"\n";
       CHECK(read(run/"consumed.spy")==expected.str());
       CHECK(FzeroShimInput(&word)); CHECK(FzeroShimConsumed(1,word));
       if(mode=="stop") _putenv_s("FZERO_FAKE_BAD_STOP","1");
       CHECK((FzeroShimStop()!=0)==(mode!="stop"));
     }
     CHECK(read(run/"stop.spy")=="stopped");
   }
 }
 CHECK(read(cfg)==original); CHECK(FzeroNoForceLatched());
 // These paths were made fresh above, exclusively by this offline fixture.
 CHECK(std::filesystem::canonical(parent).parent_path()==std::filesystem::canonical(std::filesystem::current_path()));
 std::filesystem::remove_all(parent);
 std::printf("shim production host %s: %u checks\n",mode.c_str(),checks);
 return 0;
}
