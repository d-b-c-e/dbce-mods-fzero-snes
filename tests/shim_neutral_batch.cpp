// Link the reviewed shared shim's production core.cpp + parse.cpp, with its
// fake provider. argv[1] is emitted by the wrapper's New-NeutralBatch function.
// No DirectInput, device, game, display, output or native DLL is involved.
#include "shim.hpp"
#include "shim_fake_provider.hpp"
#include <fstream>
#include <iostream>
#include <iterator>
#include <cmath>
#include <stdexcept>
using namespace dbce::shim;
int main(int argc,char** argv){try{
    if(argc!=2)throw std::runtime_error("wrapper neutral file required");
    std::ifstream input(argv[1],std::ios::binary);
    const std::string bytes((std::istreambuf_iterator<char>(input)),{});
    const std::string nonce="FzeroNeutralFixture", dev="{11111111-1111-1111-1111-111111111111}", prod="{22222222-2222-2222-2222-222222222222}";
    int checks=0;auto check=[&](bool v,const char* why){++checks;if(!v)throw std::runtime_error(why);};
    fake::Provider provider;fake::Device device;device.listed={dev,prod,"fake"};device.caps=fake::wheel_caps();
    device.state.buttons[35]=0x80; // even a physical hold must stay suppressed.
    provider.add(device);CoreConfig cfg;cfg.latched=true;Core core(provider,cfg);
    core.set_profile(read_profile("[Controls]\nSchema=1\nstart=button 35 dev="+dev+" prod="+prod+"\n"+
        "throttle=axis 2 dev="+dev+" prod="+prod+" range=0..65535 rest=0 travel=+1\n"+
        "steer=axis 0 dev="+dev+" prod="+prod+" range=0..65535 rest=33265 travel=-1\n"));
    core.request_admission(Session{nonce,2000},1000,true,true);core.service_devices();
    auto frame=core.tick();check(frame.run==Run::Waiting&&frame.down==0,"admitted physical hold remains neutral");
    auto admitted=core.offer_batch(bytes);
    if(admitted.verdict!=BatchVerdict::Accepted)throw std::runtime_error("neutral batch refused: "+admitted.why);
    check(true,"actual wrapper neutral bytes accepted by production parser");
    frame=core.tick();auto index=std::size_t(action_index("start"));
    check(frame.run==Run::Running&&frame.action[index].value==0&&frame.down==0&&frame.up==0,"neutral activation has no value or button edge");
    provider.advance(51000);frame=core.tick();
    check(frame.run==Run::Running&&frame.action[index].value==0&&frame.down==0&&frame.up==0,"expired neutral command still has no edge");
    check(core.offer_batch("nonce="+nonce+"\nbatch=2\ninject action start 1 ms=50\n").verdict==BatchVerdict::Accepted,"next inspected command accepted");
    frame=core.tick();check(frame.action[index].value==1&&(frame.down&(uint64_t(1)<<index)),"positive control really presses Start");
    provider.advance(51000);frame=core.tick();check(frame.action[index].value==0&&(frame.up&(uint64_t(1)<<index)),"positive control releases");
    // The route's driving commands use the real parser and owned raw objects,
    // not a game-specific action bypass. Mirrors the owner's axis calibration.
    const auto throttle=std::size_t(action_index("throttle")),steer=std::size_t(action_index("steer"));
    check(core.offer_batch("nonce="+nonce+"\nbatch=3\ninject action throttle 1 ms=2000\n").verdict==BatchVerdict::Accepted,"route throttle accepted");
    frame=core.tick();
    check(frame.action[throttle].status==Status::Live&&frame.action[throttle].injected&&frame.action[throttle].value==1,"throttle delivered through normal normalization");
    check(frame.samples[0].delivered.axes[2]==65535&&frame.samples[0].physical.axes[2]==0,"raw delivery differs from untouched physical sample");
    provider.advance(2001000);frame=core.tick();
    check(frame.action[throttle].status==Status::TestNeutral&&frame.action[throttle].value==0,"throttle naturally releases");
    check(core.offer_batch("nonce="+nonce+"\nbatch=4\ninject action throttle 1 ms=1000\ninject action steer 0.25 ms=1000\n").verdict==BatchVerdict::Accepted,"route combined axes accepted");
    frame=core.tick();
    check(frame.action[throttle].value==1&&frame.action[steer].status==Status::Live&&frame.action[steer].injected&&std::abs(frame.action[steer].value-.25f)<.0001f,"both normalized axes delivered in same Core frame");
    check(frame.samples[0].delivered.axes[0]==40959&&frame.samples[0].physical.axes[0]==0,"centred steering uses the shared range midpoint rule");
    provider.advance(1001000);frame=core.tick();
    check(frame.action[throttle].value==0&&frame.action[steer].value==0,"both axes release without another command");
    check(core.offer_batch("nonce="+nonce+"\nbatch=5\nend\n").verdict==BatchVerdict::Accepted,"end accepted");
    check(core.tick().run==Run::Completed,"completed normally");
    std::cout<<"PASS "<<checks<<" production Core neutral/navigation/driving checks\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
