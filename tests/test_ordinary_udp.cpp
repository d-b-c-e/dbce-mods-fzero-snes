// Real transport, exclusive ephemeral loopback receiver. Never uses port 8000.
#include "fzero_telemetry.h"
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
static unsigned checks;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"%d: %s\n",__LINE__,#x);std::exit(1);}}while(0)
static void config(unsigned port,const char *host="127.0.0.1"){
 FILE *f=std::fopen("ordinary-udp-test.ini","wb");CHECK(f);
 std::fprintf(f,"[Telemetry]\nEnabled=1\nHost=%s\nPort=%u\n",host,port);CHECK(!std::fclose(f));
}
static void packet(SOCKET sink,unsigned source,bool racing){
 uint8_t bytes[324],observed[324];sockaddr_in from{};int size=sizeof(from);unsigned producer=0;
 CHECK(recvfrom(sink,(char*)bytes,sizeof(bytes),0,(sockaddr*)&from,&size)==324);
 CHECK(from.sin_addr.s_addr==htonl(INADDR_LOOPBACK));CHECK(ntohs(from.sin_port)==source);
 CHECK(FzeroTelemetryLastSent(observed,&producer)==324);CHECK(producer==source);CHECK(!std::memcmp(bytes,observed,324));
 CHECK((bytes[0]!=0)==racing);CHECK(bytes[315]==(racing?255:0));CHECK(bytes[316]==0);
 CHECK(bytes[320]==(racing?129:0)); // -127 signed steering
}
int main(){
 WSADATA data;CHECK(!WSAStartup(MAKEWORD(2,2),&data));
 SOCKET sink=socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);CHECK(sink!=INVALID_SOCKET);
 BOOL exclusive=TRUE;CHECK(!setsockopt(sink,SOL_SOCKET,SO_EXCLUSIVEADDRUSE,(char*)&exclusive,sizeof(exclusive)));
 sockaddr_in address{};address.sin_family=AF_INET;address.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
 CHECK(!bind(sink,(sockaddr*)&address,sizeof(address)));int size=sizeof(address);CHECK(!getsockname(sink,(sockaddr*)&address,&size));
 unsigned port=ntohs(address.sin_port);CHECK(port>=1024&&port!=8000);
 DWORD timeout=2000;CHECK(!setsockopt(sink,SOL_SOCKET,SO_RCVTIMEO,(char*)&timeout,sizeof(timeout)));
 CHECK(!FzeroTelemetryRestrictLoopback(8000));CHECK(!FzeroTelemetryRestrictLoopback(0));CHECK(FzeroTelemetryRestrictLoopback(port));
 CHECK(!FzeroTelemetryRestrictLoopback(port==65535?65534:port+1));
 uint8_t ram[0x20000]{},last[324];unsigned source=0;ram[0x54]=2;ram[0x55]=3;
 config(port,"127.0.0.2");FzeroTelemetryInit("ordinary-udp-test.ini");FzeroTelemetryFrame(ram,sizeof(ram),0x41);
 CHECK(FzeroTelemetryLastSent(last,&source)==0);CHECK(source==0);
 config(port);FzeroTelemetryInit("ordinary-udp-test.ini");FzeroTelemetryFrame(ram,sizeof(ram),0x41);
 CHECK(FzeroTelemetryLastSent(last,&source)==324);CHECK(source>=1024);packet(sink,source,true);
 FzeroTelemetryShutdown();packet(sink,source,false);
 CHECK(!closesocket(sink));CHECK(!WSACleanup());CHECK(!std::remove("ordinary-udp-test.ini"));
 std::printf("PASS restricted ordinary UDP: %u checks; owned 127.0.0.1:%u only\n",checks,port);return 0;
}
