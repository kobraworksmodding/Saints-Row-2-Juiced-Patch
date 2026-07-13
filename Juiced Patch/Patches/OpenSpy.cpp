/*
	Integrates OpenSpy into Saints Row 2.
	See http://beta.openspy.net/en/howto/generic/generic page to see how it is implemented.
*/



#include "OpenSpy.h"
#include "../FileLogger.h"
#include "../SafeWrite.h"

void PatchOpenSpy()
{
	char AuthService[]="http://%s.auth.pubsvs.openspy.net/AuthService/AuthService.asmx";
	char newkey[]="afb5818995b3708d0656a5bdd20760aee76537907625f6d23f40bf17029e56808d36966c0804e1d797e310fedd8c06e6c4121d963863d765811fc9baeb2315c9a6eaeb125fad694d9ea4d4a928f223d9f4514533f18a5432dd0435c5c6ac8e276cf29489cb5ac880f16b0d7832ee927d4e27d622d6a450cd1560d7fa882c6c13";
	char openspy[]="openspy.net";

	Logger::TypedLog(CHN_DLL, "Patching openspy.\n");
	SafeWriteBuf(0x00e33568+13,&openspy,sizeof(openspy)-1);
	SafeWriteBuf(0x00e33e60+0,&openspy,sizeof(openspy)-1);
	SafeWriteBuf(0x00e34088+15,&openspy,sizeof(openspy)-1);
	SafeWriteBuf(0x00e346f8+22,&openspy,sizeof(openspy)-1);
	SafeWriteBuf(0x00e34a54+10,&openspy,sizeof(openspy)-1);
	SafeWriteBuf(0x00e355b0+8,&openspy,sizeof(openspy)-1);
	SafeWriteBuf(0x00e37e44+8,&openspy,sizeof(openspy)-1);
	SafeWriteBuf(0x00e37e58+8,&openspy,sizeof(openspy)-1);
	SafeWriteBuf(0x00e37e6c+8,&openspy,sizeof(openspy)-1);
	SafeWriteBuf(0x00e38018+12,&openspy,sizeof(openspy)-1);
	SafeWriteBuf(0x00e38080+12,&openspy,sizeof(openspy)-1);
	SafeWriteBuf(0x00e88cd0+5,&openspy,sizeof(openspy)-1);
	SafeWriteBuf(0x00e88e18+5,&openspy,sizeof(openspy)-1);
	SafeWriteBuf(0x00e35600,&AuthService,sizeof(AuthService));
	SafeWriteBuf(0x00dd0c78,&newkey,sizeof(newkey));
	return;
}