/* Production consumer with fake Win32 loader + fake exports. Never loads a
 * DLL, initializes SDL/DirectInput, registers exit handlers or emits torque. */
#define NOMINMAX
#include <windows.h>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>
#include <string>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <chrono>
#include <filesystem>

static HMODULE WINAPI fake_load(LPCWSTR);
static HMODULE WINAPI fake_self(LPCWSTR);
static DWORD WINAPI fake_path(HMODULE, LPWSTR, DWORD);
static FARPROC WINAPI fake_export(HMODULE, LPCSTR);
static BOOL WINAPI fake_unload(HMODULE);
#define LoadLibraryW fake_load
#define GetModuleHandleW fake_self
#define GetModuleFileNameW fake_path
#define GetProcAddress fake_export
#define FreeLibrary fake_unload
#include "../src/fzero_ffb.cpp"
#undef LoadLibraryW
#undef GetModuleHandleW
#undef GetModuleFileNameW
#undef GetProcAddress
#undef FreeLibrary
#include "../src/fzero_inject.h"
#include "../src/fzero_shim.h"
#include "../src/fzero_output_latch.h"
#include <ctime>

#define CHECK(x) do { if (!(x)) { std::fprintf(stderr,"%d: %s\n",__LINE__,#x); std::abort(); } } while (0)
static std::mutex events_mutex;
static std::condition_variable events_cv;
static std::vector<std::string> events;
static int loads, unloads;
static bool missing_export, block_frame, frame_entered, release_frame;
static bool shutdown_requested, free_entered, release_shutdown, worker_active;
static bool shutdown_completed;
static bool refuse_zero, refuse_init, strict_selected;
static void note(const char *);
static bool shared_exports, shared_bad_sample;
static uint64_t shared_sequence;
static int shared_info(int,WheelReadDeviceInfoV1 *out) {
  note("GetReadDeviceInfoV1");*out={};out->struct_size=144;out->abi=1;out->generation=7;
  std::memset(out->product,2,16);std::memset(out->instance,1,16);out->flags=1;out->axis_mask=out->range_mask=1;out->maximum[0]=65535;return 1;
}
static int shared_read(int,WheelReadSnapshotV1 *out) {
  note("ReadDeviceSnapshotV1");out->generation=7;out->sequence=++shared_sequence;out->valid=shared_bad_sample?0:1;
  out->read_start_us=1000+int64_t(shared_sequence)*10;out->read_end_us=out->read_start_us+1;
  out->poll_call=out->state_call=0;return 1;
}
static int shared_path(int,uint64_t,uint16_t *out,int) {note("GetReadDevicePathV1");out[0]='x';out[1]=0;return 1;}
static int shared_open(int) {note("OpenReadDevice");return 0;}
static int shared_version(){return 600;}
static int initial_x, initial_y;
static void note(const char *name) {
  std::unique_lock<std::mutex> lock(events_mutex);
  events.emplace_back(name);
  if (!std::strcmp(name,"SetHoldTimeoutMs")) worker_active = true;
  if (!std::strcmp(name,"SetDeviceForcesXY") && block_frame) {
    frame_entered = true; events_cv.notify_all();
    CHECK(events_cv.wait_for(lock,std::chrono::seconds(5),[]{return release_frame;}));
  }
  if (!std::strcmp(name,"FreeDirectInput")) {
    free_entered = true; events_cv.notify_all();
    CHECK(events_cv.wait_for(lock,std::chrono::seconds(5),[]{return release_shutdown;}));
    worker_active = false; shutdown_completed = true;
    events.emplace_back("ShutdownReturned");
  }
}
template<class T> static T result(const char *name) { note(name); return (T)1; }
template<> void result<void>(const char *name) { note(name); }
#define FAKE_EXPORT(ret,name,args) static ret __cdecl stub_##name args { return result<ret>(#name); }
WHEELFFB_API_LIST(FAKE_EXPORT)
#undef FAKE_EXPORT
static int __cdecl device_name(int, char *name, int cap) {
  note("GetDeviceName"); std::snprintf(name,(size_t)cap,"FAKE WHEEL"); return 1;
}
static int __cdecl device_guid(int, void *guid) {
  note("GetDeviceGuid"); std::memset(guid,1,16); return 1;
}
static void __cdecl strict_selection(int enabled) {
  note("SetStrictDeviceSelection"); strict_selected = enabled == 1;
}
static int __cdecl initialize(int) {
  note("InitDirectInput"); CHECK(strict_selected); return refuse_init ? 0 : 1;
}
static int __cdecl forces(int x, int y) {
  initial_x=x; initial_y=y; note("SetDeviceForcesXY"); return refuse_zero ? 0 : 1;
}
static HMODULE WINAPI fake_load(LPCWSTR) {
  std::lock_guard<std::mutex> lock(events_mutex); ++loads; return (HMODULE)1;
}
static HMODULE WINAPI fake_self(LPCWSTR) { return (HMODULE)2; }
static DWORD WINAPI fake_path(HMODULE, LPWSTR path, DWORD cap) {
  const wchar_t *value=L"C:\\fake\\consumer.exe";
  CHECK(cap>wcslen(value)); wcscpy_s(path,cap,value); return (DWORD)wcslen(value);
}
static FARPROC WINAPI fake_export(HMODULE, LPCSTR name) {
  if(shared_exports) {
    if(!std::strcmp(name,"GetWheelFfbVersion"))return (FARPROC)shared_version;
    if(!std::strcmp(name,"GetReadDeviceInfoV1"))return (FARPROC)shared_info;
    if(!std::strcmp(name,"ReadDeviceSnapshotV1"))return (FARPROC)shared_read;
    if(!std::strcmp(name,"GetReadDevicePathV1"))return (FARPROC)shared_path;
    if(!std::strcmp(name,"OpenReadDevice"))return (FARPROC)shared_open;
    if(!std::strcmp(name,"GetAnyDeviceGuid"))return (FARPROC)device_guid;
  }
  if (missing_export && !std::strcmp(name,"GetLastHResult")) return nullptr;
  if (!std::strcmp(name,"GetDeviceName")) return (FARPROC)device_name;
  if (!std::strcmp(name,"GetDeviceGuid")) return (FARPROC)device_guid;
  if (!std::strcmp(name,"SetStrictDeviceSelection")) return (FARPROC)strict_selection;
  if (!std::strcmp(name,"InitDirectInput")) return (FARPROC)initialize;
  if (!std::strcmp(name,"SetDeviceForcesXY")) return (FARPROC)forces;
#define RESOLVE(ret,export_name,args) if (!std::strcmp(name,#export_name)) return (FARPROC)stub_##export_name;
  WHEELFFB_API_LIST(RESOLVE)
#undef RESOLVE
  return nullptr; /* Optional constant-burst extensions deliberately absent. */
}
static BOOL WINAPI fake_unload(HMODULE) {
  std::lock_guard<std::mutex> lock(events_mutex);
  CHECK(!worker_active); ++unloads; events.emplace_back("FreeLibrary"); return TRUE;
}
static size_t event_count() {
  std::lock_guard<std::mutex> lock(events_mutex); return events.size();
}
static void write_text(const char *path, const char *text) {
  FILE *f=std::fopen(path,"wb"); CHECK(f); std::fputs(text,f); std::fclose(f);
}
/* The no-force latch is process-wide and permanent, so each order runs as its own process (two ctest entries).
 * --injection-latch: inject.on at start latches "no force" even though the session then refuses to arm (no
 * [Controls]); config.ini says Enabled=1 by FfbInit (0 -> 1, as the launcher can write it) and still nothing loads.
 * --force-first: force already started, so a later inject.on is refused and the latch stays with force. */
static int latch_test(bool force_first) {
  char dir[MAX_PATH];
  CHECK(GetFullPathNameA("fake-latch-appdata",MAX_PATH,dir,nullptr));
  const std::string base=dir, dbce=base+"\\dbce", fzero=dbce+"\\fzero", on=fzero+"\\inject.on";
  CreateDirectoryA(base.c_str(),nullptr); CreateDirectoryA(dbce.c_str(),nullptr); CreateDirectoryA(fzero.c_str(),nullptr);
  CHECK(_putenv_s("LOCALAPPDATA",base.c_str())==0);
  char session[96];
  std::snprintf(session,sizeof(session),"nonce=latchtest01\nexpires=%lld\n",(long long)std::time(nullptr)+600);
  const char *cfg="fake-ffb-latch.ini";
  if (force_first) {
    write_text(cfg,"[ForceFeedback]\nEnabled=1\nDevice=FAKE WHEEL\nImpactType=Sine\n");
    FzeroFfbInit(cfg,nullptr);
    CHECK(s_active && loads==1);
    write_text(on.c_str(),session);
    write_text(cfg,"[ForceFeedback]\nEnabled=0\n");
    FzeroInjectInit(cfg);
    CHECK(!FzeroInjectArmed() && !FzeroNoForceLatched());
    release_shutdown=true;
    FzeroFfbShutdown();
    CHECK(!s_active && unloads==1);
  } else {
    write_text(on.c_str(),session);
    write_text(cfg,"[ForceFeedback]\nEnabled=0\nDevice=FAKE WHEEL\n");
    FzeroInjectInit(cfg);
    CHECK(!FzeroInjectArmed() && FzeroNoForceLatched());
    write_text(cfg,"[ForceFeedback]\nEnabled=1\nDevice=FAKE WHEEL\nImpactType=Sine\n");
    FzeroFfbInit(cfg,nullptr);
    CHECK(!s_active && loads==0 && event_count()==0);
    FzeroFfbFrame(nullptr,0,0); FzeroFfbSilence();
    CHECK(loads==0 && event_count()==0);
  }
  std::remove(cfg); std::remove(on.c_str());
  RemoveDirectoryA(fzero.c_str()); RemoveDirectoryA(dbce.c_str()); RemoveDirectoryA(base.c_str());
  std::puts(force_first ? "Force first: a later test injection request is refused (fake backend only)"
                        : "Test injection request latches no force before a 0 -> 1 config change (fake backend only)");
  return 0;
}
static int shared_test(bool exports) {
  shared_exports=exports;release_shutdown=true;
  const char *cfg="fake-shared-owner.ini";
  write_text(cfg,"[ForceFeedback]\nEnabled=1\nDevice=FAKE WHEEL\nImpactType=Sine\n");
  const char *profile="[Controls]\nSchema=1\nsteer=axis 0 dev={01010101-0101-0101-0101-010101010101} prod={02020202-0202-0202-0202-020202020202} range=0..65535 rest=32768 travel=+1\n";
  CHECK(FzeroFfbSharedStart(cfg,nullptr,profile,std::strlen(profile))==int(exports));
  if(!exports) {
    for(const auto &e:events)CHECK(e!="InitDirectInput" && e!="OpenReadDevice");
    std::remove(cfg);std::puts("PASS missing owned snapshot exports refuses before device initialization");return 0;
  }
  CHECK(s_active && s_shared_owner.count()==1);
  unsigned char ram[0x20000]{};ram[0x54]=2;ram[0x55]=3;
  size_t before=events.size();FzeroFfbFrame(ram,sizeof(ram),0);
  CHECK(events.size()==before+1 && events.back()=="ZeroForces");
  FzeroFfbSharedCoreState(1);WheelReadSnapshotV1 samples[8]{};
  CHECK(FzeroFfbSharedRead(samples,8)==1 && shared_sequence==1);
  FzeroFfbSharedAccept(1);CHECK(s_shared_ready);
  before=events.size();FzeroFfbFrame(ram,sizeof(ram),0);CHECK(events.size()>before+1);
  FzeroFfbSilence();CHECK(!s_shared_ready);
  shared_bad_sample=true;CHECK(!FzeroFfbSharedRead(samples,8));
  shared_bad_sample=false;CHECK(!FzeroFfbSharedRead(samples,8) && shared_sequence==2);
  FzeroFfbShutdown();CHECK(!s_active && s_ffb.module && unloads==0); // Core still owns source lifetime
  for(const auto &e:events)CHECK(e!="CloseReadDevices" && e!="FreeDirectInput");
  FzeroFfbSharedCoreState(0);before=events.size();FzeroFfbShutdown();
  CHECK(events[before]=="CloseReadDevices" && events[before+1]=="PanicStop" && events.back()=="FreeLibrary");
  CHECK(unloads==1 && !worker_active);
  before=events.size();FzeroFfbFrame(ram,sizeof(ram),0);CHECK(!FzeroFfbSharedRead(samples,8) && events.size()==before);
  std::remove(cfg);std::puts("PASS shared force/read owner gate, one sample, permanent fault and stop-before-source-close/unload");return 0;
}
#ifdef FZERO_SHARED_CORE_TEST
static int shared_core_test(bool extras) {
  shared_exports=true;release_shutdown=true;
  auto root=std::filesystem::temp_directory_path()/("fzero-shared-core-"+std::to_string(GetCurrentProcessId()));
  CHECK(!std::filesystem::exists(root) && std::filesystem::create_directory(root));
  auto config=root/"config.ini";
  std::string text="[Input]\nBackend=Shared\nSharedForceBridge=1\nSharedExtras="+std::string(extras?"1":"0")+
    "\n[ForceFeedback]\nEnabled=1\nDevice=FAKE WHEEL\nImpactType=Sine\n[Controls]\nSchema=1\nsteer=axis 0 dev={01010101-0101-0101-0101-010101010101} prod={02020202-0202-0202-0202-020202020202} range=0..65535 rest=32768 travel=+1\n";
  if(extras)text+="[Extras]\nSchema=1\nGame=fzero-snes\nDeclaration=c4ffc4c85f1b7f72437e5871bdec57f9d5b997a62a9fdd3fd2208fefa0d98fb1\nfzero-snes.x=\n";
  write_text(config.string().c_str(),text.c_str());
  CHECK(FzeroShimPrepare(config.string().c_str(),1,0,0,0,0));
  CHECK(!FzeroNoForceLatched());CHECK(FzeroShimStart(nullptr,"fixture"));
  unsigned char ram[0x20000]{};ram[0x54]=2;ram[0x55]=3;uint32_t word=0;
  CHECK(FzeroShimInput(&word,ram,sizeof(ram)) && shared_sequence==1 && s_shared_ready);
  CHECK(FzeroShimConsumed(1,word,ram,sizeof(ram)));
  size_t before=events.size();FzeroFfbFrame(ram,sizeof(ram),word);CHECK(events.size()>before+1);
  FzeroShimNeutral();CHECK(!s_shared_ready);
  CHECK(FzeroShimOverlayInput(&word) && shared_sequence==2);
  shared_bad_sample=true;CHECK(!FzeroShimInput(&word,ram,sizeof(ram)) && word==0 && !s_shared_ready);
  CHECK(!FzeroShimStop()); // input failure retained even though cleanup succeeds
  CHECK(!s_ffb.module && unloads==1 && !worker_active);
  std::filesystem::remove_all(root);
  std::puts("PASS full SharedForceBridge host +native fake owner, one sample/Core tick, extras, fault neutral and normal shutdown");return 0;
}

#endif
int main(int argc, char **argv) {
  #ifdef FZERO_SHARED_CORE_TEST
  if(argc>1 && !std::strcmp(argv[1],"--shared-core"))return shared_core_test(false);
  if(argc>1 && !std::strcmp(argv[1],"--shared-core-extras"))return shared_core_test(true);
  #endif
  if(argc>1 && !std::strcmp(argv[1],"--shared-owner"))return shared_test(true);
  if(argc>1 && !std::strcmp(argv[1],"--shared-missing"))return shared_test(false);
  if (argc>1 && !std::strcmp(argv[1],"--injection-latch")) return latch_test(false);
  if (argc>1 && !std::strcmp(argv[1],"--force-first")) return latch_test(true);
  char names[2][256]{};
  missing_export=true;
  CHECK(FzeroFfbListDevices(names,2)==0 && loads==1 && unloads==1);
  missing_export=false;
  FILE *cfg=std::fopen("fake-ffb-lifecycle.ini","wb"); CHECK(cfg);
  std::fputs("[ForceFeedback]\nEnabled=1\nDevice=FAKE WHEEL\nImpactType=Sine\n",cfg);
  std::fclose(cfg);
  FzeroFfbInit("fake-ffb-lifecycle.ini",nullptr);
  CHECK(worker_active);
  CHECK(initial_x==0 && initial_y==0);
  for (const auto &e: events) CHECK(e!="StartEffect");
  unsigned char ram[0x20000]{};
  ram[0x54]=2; ram[0x55]=3;
  block_frame=true;
  std::thread frame([&]{FzeroFfbFrame(ram,sizeof(ram),0);});
  {
    std::unique_lock<std::mutex> lock(events_mutex);
    CHECK(events_cv.wait_for(lock,std::chrono::seconds(5),[]{return frame_entered;}));
  }
  std::thread shutdown([]{
    { std::lock_guard<std::mutex> lock(events_mutex); shutdown_requested=true; events_cv.notify_all(); }
    FzeroFfbShutdown();
  });
  {
    std::unique_lock<std::mutex> lock(events_mutex);
    CHECK(events_cv.wait_for(lock,std::chrono::seconds(5),[]{return shutdown_requested;}));
    CHECK(!free_entered && worker_active); /* In-flight frame owns call gate. */
    release_frame=true; events_cv.notify_all();
    CHECK(events_cv.wait_for(lock,std::chrono::seconds(5),[]{return free_entered;}));
    CHECK(unloads==1 && worker_active && !shutdown_completed);
  }
  frame.join();
  size_t closed_count=event_count();
  std::thread late_frame([&]{FzeroFfbFrame(ram,sizeof(ram),0);});
  std::thread late_silence([]{FzeroFfbSilence();});
  std::thread late_discovery([&]{CHECK(FzeroFfbListDevices(names,2)==0);});
  {
    std::lock_guard<std::mutex> lock(events_mutex);
    CHECK(events.size()==closed_count && unloads==1);
    release_shutdown=true; events_cv.notify_all();
  }
  shutdown.join(); late_frame.join(); late_silence.join(); late_discovery.join();
  CHECK(shutdown_completed && !worker_active && unloads==2 && loads==2);
  CHECK(event_count()==closed_count+2); /* ShutdownReturned then FreeLibrary only. */
  CHECK(events[events.size()-2]=="ShutdownReturned" && events.back()=="FreeLibrary");
  size_t final_count=event_count();
  FzeroFfbShutdown(); FzeroFfbSilence(); FzeroFfbFrame(ram,sizeof(ram),0);
  CHECK(FzeroFfbListDevices(names,2)==0 && event_count()==final_count && loads==2);
  // A refused neutral start must not create/start any other effect or watchdog.
  block_frame=false; refuse_zero=true; strict_selected=false;
  size_t start=event_count();
  FzeroFfbInit("fake-ffb-lifecycle.ini",nullptr);
  CHECK(!s_active && !worker_active && loads==3 && unloads==3);
  CHECK(initial_x==0 && initial_y==0 && strict_selected);
  for (size_t i=start;i<events.size();++i) {
    CHECK(events[i]!="StartEffect" && events[i]!="SetHoldTimeoutMs");
    CHECK(events[i]!="CreateConditionEffect" && events[i]!="CreatePeriodicEffect");
  }
  final_count=event_count();
  FzeroFfbFrame(ram,sizeof(ram),0); FzeroFfbSilence();
  CHECK(event_count()==final_count);
  // Strict device open refusal also completes cleanup without an output call.
  refuse_zero=false; refuse_init=true; strict_selected=false; start=event_count();
  FzeroFfbInit("fake-ffb-lifecycle.ini",nullptr);
  CHECK(!s_active && !worker_active && loads==4 && unloads==4 && strict_selected);
  for (size_t i=start;i<events.size();++i)
    CHECK(events[i]!="SetDeviceForcesXY" && events[i]!="StartEffect" && events[i]!="SetHoldTimeoutMs");
  // A deliberate later initialization can start neutrally after the failure.
  refuse_init=false;
  FzeroFfbInit("fake-ffb-lifecycle.ini",nullptr);
  CHECK(s_active && worker_active && initial_x==0 && initial_y==0);
  FzeroFfbShutdown();
  CHECK(!s_active && !worker_active && loads==5 && unloads==5);
  std::remove("fake-ffb-lifecycle.ini");
  std::puts("Consumer drain, close, completed-shutdown/unload and late-call gates passed (fake backend only)");
}
