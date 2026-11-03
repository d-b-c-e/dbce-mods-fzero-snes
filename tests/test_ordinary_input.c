/* Same native mapper, no SDL_Init, no virtual pad and no physical device. */
#include "fzero_gamepad.h"
#include "fzero_controls.h"
#include "fzero_output_latch.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned checks;
#define CHECK(x) do{++checks;if(!(x)){fprintf(stderr,"%d: %s\n",__LINE__,#x);exit(1);}}while(0)
int main(void){
 const char *file="ordinary-input-test.ini",*guid="00000000d1a600000000000000000001";
 FILE *f=fopen(file,"wbx");CHECK(f!=NULL);
 fputs("[Controls]\nSchema=2\nProfile=synthetic R1\nRevision=ordinary-fixture\n"
 "steer=axis 0 dev={11111111-1111-1111-1111-111111111111} prod={0006346e-0000-0000-0000-504944564944} rest=32768 range=0..65535 travel=+1 cal=4096..32768..61440\n"
 "throttle=axis 1 dev={11111111-1111-1111-1111-111111111111} prod={0006346e-0000-0000-0000-504944564944} rest=0 range=0..65535 travel=+1 cal=0..65535\n"
 "brake=axis 2 dev={11111111-1111-1111-1111-111111111111} prod={0006346e-0000-0000-0000-504944564944} rest=0 range=0..65535 travel=+1 cal=0..65535\n"
 "confirm=button 0 dev={11111111-1111-1111-1111-111111111111} prod={0006346e-0000-0000-0000-504944564944}\n",f);
 fclose(f);FzeroControlsPlan plan;CHECK(FzeroControlsPlanFile(file,&plan));CHECK(plan.ok);
 CHECK(FzeroControlsWrite(file,guid,&plan));CHECK(!FzeroControlsPending(file,&plan));
 char section[64];snprintf(section,sizeof(section),"Controller.%s",guid);
 CHECK(FzeroControlsIniSet(file,section,"AnalogSteering","1"));
 CHECK(FzeroControlsIniSet(file,section,"Deadzone","0"));
 CHECK(FzeroControlsIniSet(file,section,"SteeringResponsePercent","100"));
 CHECK(FzeroControlsIniSet(file,section,"SteeringRangePercent","100"));
 CHECK(!FzeroGamepadDiagnosticStart(file,guid));CHECK(FzeroLatchNoForce());
 CHECK(FzeroGamepadDiagnosticStart(file,guid));
 int16_t raw[8]={0,-32768,-32768};FzeroGamepadDiagnosticSample(raw,0);
 CHECK(FzeroGamepadReadFrame(NULL,1)==0);
 raw[0]=-28672;FzeroGamepadDiagnosticSample(raw,0);CHECK(FzeroGamepadReadFrame(NULL,1)==0x40);
 raw[0]=28672;FzeroGamepadDiagnosticSample(raw,0);CHECK(FzeroGamepadReadFrame(NULL,1)==0x80);
 CHECK(FzeroGamepadReadFrame(NULL,0)==0); /* analog steering cannot pulse a menu */
 raw[0]=0;raw[1]=32767;FzeroGamepadDiagnosticSample(raw,0);CHECK(FzeroGamepadReadFrame(NULL,1)==1);
 raw[1]=-32768;raw[2]=32767;FzeroGamepadDiagnosticSample(raw,0);CHECK(FzeroGamepadReadFrame(NULL,1)==2);
 raw[2]=-32768;FzeroGamepadDiagnosticSample(raw,1);CHECK(FzeroGamepadReadFrame(NULL,1)==0x100);
 FzeroGamepadDiagnosticSample(raw,0);CHECK(FzeroGamepadReadFrame(NULL,1)==0);
 double v[3];int canonical[3];FzeroGamepadDiagnosticObserved(v,canonical);
 CHECK(canonical[0]==1&&canonical[1]==1&&canonical[2]==1);CHECK(v[0]==0&&v[1]==0&&v[2]==0);
 CHECK(SDL_WasInit(SDL_INIT_JOYSTICK|SDL_INIT_GAMECONTROLLER|SDL_INIT_HAPTIC)==0);
 FzeroGamepadShutdown(NULL);CHECK(remove(file)==0);remove("ordinary-input-test.ini.before-profile-controls");
 printf("PASS ordinary synthetic raw mapper: %u checks; no SDL initialization/device\n",checks);return 0;
}
