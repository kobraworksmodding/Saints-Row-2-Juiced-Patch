/*	
	Removes the 30 fps frame limiter for the GOG version.
	The GOG version replaces a call to check the IDirect3DDevice9::TestCooperativeLevel of the renderer, after IDirect3DDevice9::Present is called 
	to a jump to the frame limiter routine.
	This patch restores the original call. It will have no effect on the Steam version as the code it's replacing is identical.
*/


#include "GOG No FPS limiter.h"
#include "../FileLogger.h"
#include "../SafeWrite.h"

void PatchGOGNoFPSLimit()
{
	Logger::TypedLog(CHN_DLL, "Patching GOG No FPS Limit.\n");
	WriteRelCall(0x00d20d5a,0x00d20230);
	return;
}