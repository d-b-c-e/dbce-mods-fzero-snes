#include "fzero_shim_map.h"
#include "fzero_playthrough.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks;
#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr, "%d: %s\n", __LINE__, #x); exit(1); } } while (0)
static dbce_shim_frame frame(void) {
  dbce_shim_frame f = {0};
  f.struct_size = sizeof(f); f.abi = DBCE_SHIM_ABI; f.seq = 1;
  for (int i=0; i<DBCE_SHIM_ACTIONS; ++i) f.action[i].status = 12;
  return f;
}
int main(void) {
  CHECK(sizeof(dbce_shim_frame) == 296);
  CHECK(sizeof(dbce_shim_open_args) == 128);
  CHECK(FzeroShimGuestWord(0x400000f1u)==0x40000001u);
  CHECK(FzeroShimGuestWord(0x400f00f1u)==0x40000001u);
  CHECK(FzeroShimGuestWord(0x40040041u)==0x40040041u);
  for (int script=0; script<2; ++script)
    for (int replay=0; replay<2; ++replay)
      for (int shim=0; shim<2; ++shim)
        CHECK(FzeroControlsStartupMayWrite(script,replay,shim) == !(script||replay||shim));
  FzeroShimMap map; FzeroShimMapInit(&map,100,100,.5f);
  dbce_shim_frame f = frame(); uint32_t word;
  CHECK(FzeroShimMapFrame(&map,&f,&word) && word == 0);
  CHECK(!FzeroShimMapFrame(&map,&f,&word) && word == 0); // no second PDM advancement
  f.seq = 0; CHECK(!FzeroShimMapFrame(&map,&f,&word));
  f.seq = 2; f.abi = 1; CHECK(!FzeroShimMapFrame(&map,&f,&word));
  f.abi = 2; f.struct_size--; CHECK(!FzeroShimMapFrame(&map,&f,&word)); f.struct_size++;
  f.action[0].value = -1; CHECK(FzeroShimMapFrame(&map,&f,&word) && word == 0x40);
  f.seq++; f.action[0].value = 1; CHECK(FzeroShimMapFrame(&map,&f,&word) && word == 0x80);
  f.action[0].value = 0;
  static const unsigned bits[] = {8,0,3,2,4,5,6,7};
  for (int i=0; i<8; ++i) {
    f.seq++; f.action[9+i].value = 1;
    CHECK(FzeroShimMapFrame(&map,&f,&word) && word == (1u<<bits[i]));
    f.action[9+i].value = 0;
  }
  f.seq++; f.action[1].value = .5f; f.action[2].value = .5f;
  CHECK(FzeroShimMapFrame(&map,&f,&word) && word == 0);
  f.seq++; f.action[1].value = .51f; f.action[2].value = .51f;
  CHECK(FzeroShimMapFrame(&map,&f,&word) && word == 3);
  // Unsupported generic actions never invent SNES shoulder/boost mappings.
  f = frame(); f.seq = 100;
  for (int i=3; i<9; ++i) f.action[i].value = 1;
  for (int i=17; i<31; ++i) f.action[i].value = 1;
  CHECK(FzeroShimMapFrame(&map,&f,&word) && word == 0);
  f = frame(); f.seq = 101;
  for (int status=0; status<=13; ++status) {
    if (status==12) continue;
    for (int i=0; i<31; ++i) { f.action[i].value=1; f.action[i].status=(uint8_t)status; }
    f.seq++; CHECK(FzeroShimMapFrame(&map,&f,&word) && word == 0);
    CHECK(map.steering.phase == 0);
  }
  for (int i=0; i<31; ++i) { f.action[i].value=NAN; f.action[i].status=12; }
  f.seq++; CHECK(FzeroShimMapFrame(&map,&f,&word) && word == 0);
  for (int i=0; i<31; ++i) f.action[i].value=INFINITY;
  f.seq++; CHECK(FzeroShimMapFrame(&map,&f,&word) && word == 0);

  /* The ordinary compatibility path preserves digital extras and only
   * explicitly unbound generic buttons. Every unavailable state stays owned
   * by Core, with no legacy binding substituted after a device fault. */
  f=frame();
  const uint32_t extras=(1u<<1)|(1u<<9)|(1u<<10)|(1u<<11)|(1u<<24)|(1u<<25);
  CHECK(FzeroShimLegacyButtonMask(&f)==extras);
  for(int status=0;status<=13;++status) {
    for(int i=9;i<=16;++i) f.action[i].status=(uint8_t)status;
    CHECK(FzeroShimLegacyButtonMask(&f)==(status==0 ? extras|0x1fdu : extras));
  }
  CHECK(FzeroShimLegacyButtonMask(NULL)==0);
  f.abi=1;CHECK(FzeroShimLegacyButtonMask(&f)==0);
  f=frame();FzeroShimMapInit(&map,100,50,.5f);
  f.action[0].value=.25f;f.action[1].value=f.action[2].value=1;
  CHECK(FzeroShimMapFrame(&map,&f,&word));
  f.seq++;f.action[13].value=1;
  CHECK(FzeroShimMapOverlayFrame(&map,&f,&word) && word==0x10);
  CHECK(map.steering.phase==0 && map.steering.direction==0);
  CHECK(!FzeroShimMapOverlayFrame(&map,&f,&word));
  f.seq++;f.action[13].status=3;
  CHECK(FzeroShimMapOverlayFrame(&map,&f,&word) && word==0);

  // Production PDM parity: one frame per simulation, regardless of how many
  // presentations the host performs between them. The tape stores consumed
  // input including keyboard bits, not merely the wheel preview.
  const char *path="fzero-shim-consumed-test.fzpt";
  FzeroPlaythrough tape={0}, playback={0}; uint8_t digest[32]={42}, ram[8]={0};
  uint32_t expected[120];
  CHECK(FzeroPlaythroughRecordOpen(&tape,path,digest));
  FzeroShimMapInit(&map,100,50,.5f);
  FzeroAnalogSteering legacy={0}; f=frame();
  for (int n=0; n<120; ++n) {
    f.seq=(uint64_t)n+1; f.action[0].value = n<60 ? .25f : -.25f;
    f.action[1].value = 1;
    CHECK(FzeroShimMapFrame(&map,&f,&word));
    int axis=(int)roundf(f.action[0].value*(n<60 ? 32767.f : 32768.f));
    uint32_t steering = FzeroAnalogSteeringReadTuned(&legacy,axis,0,100,50);
    CHECK(word==(steering|1u));
    expected[n] = FzeroShimGuestWord(word | (1u<<30) | ((n%9)==0 ? 0x30u | 1u<<11 : 0));
    double values[2]; FzeroShimConsumedValues((uint64_t)n,expected[n],values);
    CHECK(values[0]==n && values[1]==expected[n]);
    ram[0]=(uint8_t)n;
    CHECK(FzeroPlaythroughRecordFrame(&tape,(uint32_t)values[1],ram,sizeof(ram)));
  }
  CHECK(FzeroPlaythroughClose(&tape));
  CHECK(FzeroPlaythroughPlaybackOpen(&playback,path,digest));
  for (int n=0; n<120; ++n) {
    CHECK(FzeroPlaythroughNextInput(&playback,&word) && word==expected[n]);
    ram[0]=(uint8_t)n; CHECK(FzeroPlaythroughVerifyFrame(&playback,ram,sizeof(ram)));
  }
  CHECK(FzeroPlaythroughClose(&playback)); CHECK(remove(path)==0);
  f.seq=121; f.action[0].value=.1f;
  CHECK(FzeroShimMapFrame(&map,&f,&word));
  FzeroShimMapNeutral(&map); CHECK(map.steering.phase==0 && map.steering.direction==0);
  /* Read-only evidence uses exact widths and refuses truncated bank 7F. */
  static uint8_t menu_ram[0x20000]; double menu[FZERO_SHIM_MENU_VALUES];
  menu_ram[0x5a]=1; menu_ram[0x58]=1; menu_ram[0x60]=2;
  menu_ram[0x52]=3; menu_ram[0x53]=4; menu_ram[0x90]=5;
  menu_ram[0x14c84]=0x34; menu_ram[0x14c85]=0x12;
  menu_ram[0x14c8a]=0xcd; menu_ram[0x14c8b]=0xab;
  menu_ram[0x14c92]=0x87; menu_ram[0x14c98]=1;
  menu_ram[0x14c27]=24; menu_ram[0x5b]=1;
  CHECK(FzeroShimMenuValues(menu_ram,sizeof(menu_ram),menu));
  CHECK(menu[0]==1 && menu[1]==1 && menu[2]==2 && menu[3]==3 && menu[4]==4 && menu[5]==5);
  CHECK(menu[6]==0x1234 && menu[7]==0xabcd && menu[8]==0x87 && menu[9]==1 && menu[10]==24 && menu[11]==1);
  CHECK(menu_ram[0x14c84]==0x34 && menu_ram[0x14c85]==0x12 && menu_ram[0x5a]==1);
  CHECK(!FzeroShimMenuValues(menu_ram,0x14c99,menu));
  for(int i=0;i<FZERO_SHIM_MENU_VALUES;++i) CHECK(menu[i]==0);
  CHECK(!FzeroShimMenuValues(NULL,sizeof(menu_ram),menu));
  CHECK(!FzeroShimMenuValues(menu_ram,sizeof(menu_ram),NULL));
  printf("F-Zero shared shim mapper: %u checks passed\n",checks);
  return 0;
}
