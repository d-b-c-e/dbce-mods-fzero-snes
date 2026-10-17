#include "fzero_shim.h"
#include "fzero_telemetry.h"
#include "fzero_ffb.h"
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
       mode=="abi" || mode=="pending" || mode=="record" || mode=="stop" || mode=="fences" ||
       mode=="latched" || mode=="expired" || mode=="provenance" || mode=="session" || mode=="hostcontrols");
 _putenv_s("FZERO_SHIM_DIAGNOSTIC_RUN","");
 _putenv_s("FZERO_FAKE_BAD_ABI",""); _putenv_s("FZERO_FAKE_BAD_RECORD",""); _putenv_s("FZERO_FAKE_BAD_STOP","");
 _putenv_s("FZERO_SHIM_SESSION_FILE",""); _putenv_s("FZERO_SHIM_BATCH_FILE",""); _putenv_s("FZERO_SHIM_SESSION_NONCE","");
 _putenv_s("FZERO_FAKE_EXPIRED",""); _putenv_s("FZERO_FAKE_NO_PROVENANCE","");
 CHECK(FzeroShimPrepare("does-not-exist.ini",0,0,0,0,0));
 CHECK(!FzeroShimRequested() && !FzeroNoForceLatched());
 CHECK(FzeroShimStart(nullptr,nullptr) && FzeroShimStop());
 if(mode=="off") { std::printf("shim host off: %u checks\n",checks); return 0; }
 auto fixture_root=std::filesystem::temp_directory_path();
 auto parent=fixture_root/("fzt-"+std::to_string(_getpid())+"-"+mode);
 CHECK(!std::filesystem::exists(parent));
 CHECK(std::filesystem::create_directory(parent));
 auto cfg=parent/"config.ini", run=parent/"run";
 const std::string profile="[Controls]\r\nSchema=1\r\nProfile=fixture\r\nRevision=1\r\n"
   "steer = axis 0 dev={11111111-2222-3333-4444-555555555555} prod={0006346e-0000-0000-0000-504944564944} "
   "range=0..65535 rest=32768 travel=-1\r\n";
 const std::string original="[Other]\r\nKept=yes\r\n"+profile+"[ForceFeedback]\r\nEnabled=1\r\nStrength=12\r\n[Telemetry]\r\nEnabled=1\r\n"+
   (mode=="hostcontrols"?"[Input]\r\nSharedHostControls=1\r\n":"");
 {std::ofstream f(cfg,std::ios::binary); f<<original;}
 _putenv_s("FZERO_SHIM_DIAGNOSTIC_RUN",run.string().c_str());
 bool latched=mode=="latched" || mode=="expired" || mode=="provenance";
 if(latched || mode=="session") {
   auto session=parent/"inject.on", batch=parent/"inject.txt";
   std::ofstream(session)<<"nonce=fixture123\nexpires=1999999999\n";
   std::ofstream(batch)<<"nonce=fixture123\nbatch=1\nend\n";
   _putenv_s("FZERO_SHIM_SESSION_NONCE","fixture123");
   _putenv_s("FZERO_SHIM_SESSION_FILE",session.string().c_str());
   if(mode!="session") _putenv_s("FZERO_SHIM_BATCH_FILE",batch.string().c_str());
 }
 if(mode=="args" || mode=="replay" || mode=="session" || mode=="hostcontrols") {
   CHECK(!FzeroShimPrepare(cfg.string().c_str(),mode!="args",mode=="replay",0,0,0));
   CHECK(FzeroShimRequested() && FzeroNoForceLatched()); CHECK(!std::filesystem::exists(run));
 } else {
   CHECK(FzeroShimPrepare(cfg.string().c_str(),1,0,0,0,0));
   CHECK(FzeroShimRequested() && FzeroNoForceLatched());
   CHECK(FzeroTelemetryDeliveryBlocked());
   FzeroTelemetryInit(cfg.string().c_str()); CHECK(FzeroTelemetryDeliveryBlocked());
   CHECK((FzeroShimLatched()!=0)==latched);
   CHECK(std::filesystem::path(FzeroShimSaveRoot())==run/"saves");
   CHECK(std::filesystem::path(FzeroShimRecordingPath())==run/"original.fzpt");
   CHECK(read(run/"controls.ini")==profile);
   CHECK(read(run/"config.ini")==original);
   if(mode=="abi") _putenv_s("FZERO_FAKE_BAD_ABI","1");
   FzeroShimHostFences(mode=="fences" ? "owner/saves" : FzeroShimSaveRoot(),1);
   int started=FzeroShimStart(nullptr,"fixture");
   if(mode=="abi" || mode=="fences") CHECK(!started && !FzeroShimStop());
   else {
     CHECK(started); CHECK(read(run/"profile.spy")==profile);
     uint8_t ram[0x20000]{}; ram[0x54]=2; ram[0x55]=3; ram[0xca]=4;
     ram[0x5a]=1; ram[0x58]=1; ram[0x14c92]=0x87;
     FzeroFfbState force{}; FzeroFfbOutput expected_force{};
     FzeroTelemetryState telemetry{}; uint8_t packet[324];
     if(mode=="expired") _putenv_s("FZERO_FAKE_EXPIRED","1");
     if(mode=="provenance") _putenv_s("FZERO_FAKE_NO_PROVENANCE","1");
     uint32_t word=0;
     if(mode=="expired" || mode=="provenance") {
       CHECK(!FzeroShimInput(&word,ram,sizeof(ram))); CHECK(!FzeroShimStop());
     } else {
     CHECK(FzeroShimInput(&word,ram,sizeof(ram))); CHECK(!FzeroShimCompleted());
     if(mode=="pending") {
       CHECK(!FzeroShimInput(&word,ram,sizeof(ram))); CHECK(!FzeroShimStop());
     } else if(mode=="record") {
       _putenv_s("FZERO_FAKE_BAD_RECORD","1");
       CHECK(!FzeroShimConsumed(0,word,ram,sizeof(ram))); CHECK(!FzeroShimStop());
     } else {
       uint32_t consumed=word|(1u<<30)|(1u<<11);
       CHECK(FzeroShimConsumed(0,consumed,ram,sizeof(ram)));
       FzeroFfbComputeSteering(&force,ram,sizeof(ram),consumed,12,12,&expected_force);
       CHECK(FzeroTelemetryBuild(&telemetry,ram,sizeof(ram),consumed,packet)==324);
       { std::istringstream values(read(run/"signals.spy")); double v[27];
         for(auto &n:v) CHECK(bool(values>>n));
         CHECK(v[0]==2 && v[1]==3 && v[2]==0 && v[3]==expected_force.constant_force);
         CHECK(v[4]==expected_force.spring_coefficient && v[5]==expected_force.damper_coefficient);
         CHECK(v[6]==expected_force.road_magnitude && v[7]==expected_force.road_frequency_millihz);
         CHECK(v[8]==expected_force.collision_pulse && v[9]==expected_force.racing);
         CHECK(v[10]==telemetry.speed_mps && v[11]==telemetry.total_distance_m);
         CHECK(v[13]==120 && v[14]==0);
         CHECK(v[15]==1 && v[16]==1 && v[23]==0x87 && v[24]==0);
       }
       std::ostringstream expected; expected<<"1 0 "<<consumed<<"\n";
       CHECK(read(run/"consumed.spy")==expected.str());
       CHECK(!read(run/"signals.spy").empty());
       CHECK(FzeroShimInput(&word,ram,sizeof(ram))); CHECK((FzeroShimCompleted()!=0)==latched);
       if(latched) CHECK(word==0);
       CHECK(FzeroShimConsumed(1,word,ram,sizeof(ram)));
       if(mode=="stop") _putenv_s("FZERO_FAKE_BAD_STOP","1");
       CHECK((FzeroShimStop()!=0)==(mode!="stop"));
     }
     }
     CHECK(read(run/"stop.spy")=="stopped");
   }
 }
 CHECK(read(cfg)==original); CHECK(FzeroNoForceLatched());
 // These paths were made fresh above, exclusively by this offline fixture.
 CHECK(std::filesystem::canonical(parent).parent_path()==std::filesystem::canonical(fixture_root));
 std::filesystem::remove_all(parent);
 std::printf("shim production host %s: %u checks\n",mode.c_str(),checks);
 return 0;
}
