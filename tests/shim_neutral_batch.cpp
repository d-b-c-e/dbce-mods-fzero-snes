// Link the reviewed shared shim's production core.cpp + parse.cpp, with its
// fake provider. argv[1] is emitted by the wrapper's New-NeutralBatch function.
// No DirectInput, device, game, display, output or native DLL is involved.
#include "shim.hpp"
#include "shim_fake_provider.hpp"
#include <fstream>
#include <iostream>
#include <iterator>
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
    core.set_profile(read_profile("[Controls]\nSchema=1\nstart=button 35 dev="+dev+" prod="+prod+"\n"));
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
    check(core.offer_batch("nonce="+nonce+"\nbatch=3\nend\n").verdict==BatchVerdict::Accepted,"end accepted");
    check(core.tick().run==Run::Completed,"completed normally");
    std::cout<<"PASS "<<checks<<" production Core neutral startup/edge checks\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
