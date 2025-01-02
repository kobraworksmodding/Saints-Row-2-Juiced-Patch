// Render3D.cpp (uzis, Tervel)
// --------------------
// Created: 15/12/2024

#include "../FileLogger.h"
#include "../Patcher/patch.h"
#include <thread>
#include "../iat_functions.h"
#include "../GameConfig.h"

namespace Render3D
{
	const char FPSCam[] = "camera_fpss.xtbl";
	bool useFPSCam = 0;
	bool VFXP_fixFog = 0;
	float AOStrength = 14.55;

	void PatchHQTreeShadows() 
	{
		//10x resolution for TreeShadows
		patchBytesM((BYTE*)0x0053833F, (BYTE*)"\x68\x00\x3C\x00\x00", 5);
		patchBytesM((BYTE*)0x00538344, (BYTE*)"\x68\x00\x3C\x00\x00", 5);
		patchBytesM((BYTE*)0x0051CE75, (BYTE*)"\x68\x00\x3C\x00\x00", 5);
		patchBytesM((BYTE*)0x0051CE7A, (BYTE*)"\x68\x00\x3C\x00\x00", 5);
		patchBytesM((BYTE*)0x00D1F8B2, (BYTE*)"\x68\x00\x3C\x00\x00", 5);
		patchBytesM((BYTE*)0x00D1F8B7, (BYTE*)"\x68\x00\x3C\x00\x00", 5);
		patchBytesM((BYTE*)0x0051FE40, (BYTE*)"\x68\x00\x3C\x00\x00", 5);
		patchBytesM((BYTE*)0x0051FE45, (BYTE*)"\x68\x00\x3C\x00\x00", 5);
	}

	void PatchLowSleepHack()
	{
		// Woohoo, this is a dirty patch, but we'll include it for people who want it and CAN actually run it.
		// This will destroy older and weaker pcs, but we'll make sure to let the people who are on that, know that.

		// This is the lower spec version of the patch, the things that will cause the LEAST cpu usage.

		Logger::TypedLog(CHN_DLL, "Removing a Very Safe Amount of Sleep Calls...\n");
		//patchNop((BYTE*)0x0052108C, 3); // patch win main sleep call
		patchBytesM((BYTE*)0x00521FC0, (BYTE*)"\x6A\x00", 2); // wait call in a threaded function, i think
		patchBytesM((BYTE*)0x00521FE5, (BYTE*)"\x6A\x00", 2); // same with this one
		patchBytesM((BYTE*)0x005285A2, (BYTE*)"\x6A\x00", 2); // this ones a doozy, this is some weird threaded exchange function, for each something, sleep. 
	}

	void PatchMediumSleepHack()
	{
		Logger::TypedLog(CHN_DLL, "Removing a Safe Amount of Sleep Calls...\n");
		patchBytesM((BYTE*)0x00521FC0, (BYTE*)"\x6A\x00", 2); // wait call in a threaded function, i think
		patchBytesM((BYTE*)0x00521FE5, (BYTE*)"\x6A\x00", 2); // same with this one
		patchBytesM((BYTE*)0x005285A2, (BYTE*)"\x6A\x00", 2); // this ones a doozy, this is some weird threaded exchange function, for each something, sleep. 
		patchBytesM((BYTE*)0x0052847C, (BYTE*)"\x6A\x00", 2); //make the shadow pool less sleepy
	}
	typedef void (WINAPI* SleepFn)(DWORD dwMilliseconds);
	SleepFn OriginalSleep = nullptr;

	void WINAPI SleepDetour(DWORD dwMilliseconds) {
		if (dwMilliseconds == 0) {
			std::this_thread::yield(); // not sure if this helps at all? can be yeeted if its useless
			return;
		}
		else {
			OriginalSleep(dwMilliseconds / 1.5);
		}
	}

	void HookSleep() {
		HMODULE main_handle = GetModuleHandleA(NULL);

		void* old_proc;

		if (PatchIat(main_handle, "Kernel32.dll", "Sleep", (void*)SleepDetour, &old_proc) == S_OK) {
			OriginalSleep = (SleepFn)old_proc;
		}
	}

	void FPSCamHack() {
		BYTE PlayerStatus = *(BYTE*)0x00E9A5BC; // Status Byte for the Players Actions.
		FLOAT* WalkCamZoom = (FLOAT*)0x025F6334;
		BYTE ActorFade = *(BYTE*)0x00E8825F;

		if (*(FLOAT*)WalkCamZoom > -0.5) {
			*(FLOAT*)0x025F6334 = -0.4; // Force camera zoom to chest/in front of player.
		}
		if (ActorFade == 0x01) {
			*(BYTE*)0x00E8825F = 0x00; // Force ActorFade to off.
		}
		if (PlayerStatus == 0x01 || PlayerStatus == 0x10 || PlayerStatus == 0x02 || PlayerStatus == 0x17) {
			*(BYTE*)0x00E9A5BC = 0x00; // Force the cam(?) state to 0x00 -- (Walking Outside) if got Running Outside, Running Inside or Walking Inside.
		}
	}

	void UncapFPS()
	{
		Logger::TypedLog(CHN_DLL, "Uncapping FPS...\n");
		patchNop((BYTE*)0x00D20E3E, 7);
	}

	void AltTabFPS()
	{
		Logger::TypedLog(CHN_DLL, "Making ALT-TAB smoother...\n");
		patchNop((BYTE*)0x005226F3, 8); // Bye bye sleep call.
	}

	void FasterLoadingScreens()
	{
		Logger::TypedLog(CHN_MOD, "Makiug loading screens slightly faster.\n");
		patchBytesM((BYTE*)0x0068C714, (BYTE*)"\x6A\x0F", 2); // this is a sleep call for first load/legal disclaimers, its set to 30 by default, halfing increases fps to 60 and makes loading faster.
	}

	void VFXPlus()
	{
		Logger::TypedLog(CHN_DEBUG, "Patching VanillaFXPlus...\n");
		patchNop((BYTE*)0x00773797, 5); // prevent the game from disabling/enabling the tint.
		patchBytesM((BYTE*)0x0051A952, (BYTE*)"\xD9\x05\x7F\x2C\x7B\x02", 6); // new brightness address
		patchBytesM((BYTE*)0x0051A997, (BYTE*)"\xD9\x05\x83\x2C\x7B\x02", 6); // new sat address patch
		patchBytesM((BYTE*)0x0051A980, (BYTE*)"\xD9\x05\x87\x2C\x7B\x02", 6); // new contr address patch
		patchByte((BYTE*)0x00E9787F, 0x01); // force HDR on
		patchNop((BYTE*)0x00773792, 5); // prevent the game from turning HDR on/off
		patchBytesM((BYTE*)0x005170EF, (BYTE*)"\x75", 1); // prevent bloom from appearing without breaking glow
		patchBytesM((BYTE*)0x00517051, (BYTE*)"\x8B", 1); // flip the logic for the HDR strength (or radius?) float check
		//patchNop((BYTE*)0x00533C25, 5); // disable sky refl (prevent the absurd blue tint on reflections)

		patchNop((BYTE*)0x00532A4F, 6); // nop for whatever the fuck
		patchBytesM((BYTE*)0x00532992, (BYTE*)"\xDD\x05\xAA\x2C\x7B\x02", 6); // new opacity address for sky reflections
		patchDouble((BYTE*)0x027B2CAA, 128.0);

		patchFloat((BYTE*)0x027B2C7F, 1.26f); //Bright
		patchFloat((BYTE*)0x027B2C83, 0.8f); //Sat
		patchFloat((BYTE*)0x027B2C87, 1.62f); //Contr

		patchBytesM((BYTE*)0x00524BA4, (BYTE*)"\xD9\x05\xBA\x2C\x7B\x02", 6);
		patchBytesM((BYTE*)0x00D1A333, (BYTE*)"\xD9\x05\xBA\x2C\x7B\x02", 6);
		patchBytesM((BYTE*)0x00524BB0, (BYTE*)"\xD9\x05\xBE\x2C\x7B\x02", 6);
		patchBytesM((BYTE*)0x00D1A3A3, (BYTE*)"\xD9\x05\xBE\x2C\x7B\x02", 6);
		VFXP_fixFog = 1;
	}

	void DisableFog()
	{
		patchBytesM((BYTE*)0x0025273BE, (BYTE*)"\x01", 1); // leftover debug bool for being able to overwrite fog values
		patchFloat((BYTE*)0x00E989A0, 0.0f);
		patchFloat((BYTE*)0x00E989A4, 0.0f);
	}

	void ConsoleLikeBrightness()
	{
		Logger::TypedLog(CHN_DEBUG, "Patching Console-like Brightness...\n");
		patchBytesM((BYTE*)0x0051A952, (BYTE*)"\xD9\x05\x7F\x2C\x7B\x02", 6); // new brightness address
		patchFloat((BYTE*)0x027B2C7F, 1.05f); //Bright
		patchBytesM((BYTE*)0x0051A980, (BYTE*)"\xD9\x05\x87\x2C\x7B\x02", 6); // new contr address patch
		patchFloat((BYTE*)0x027B2C87, 1.40f); //Contr
		patchBytesM((BYTE*)0x0051A997, (BYTE*)"\xD9\x05\x83\x2C\x7B\x02", 6); // new sat address patch
		patchFloat((BYTE*)0x027B2C83, 0.65f); //Sat
	}

	void RemoveVignette()
	{
		Logger::TypedLog(CHN_MOD, "Disabling Vignette...\n");
		patchNop((BYTE*)0x00E0C62C, 9); // nop aVignette
	}

	void BetterAO()
	{
		Logger::TypedLog(CHN_MOD, "Making AO Better...\n");
		patchNop((BYTE*)0x0052AA90, 6);
		patchNop((BYTE*)0x0052AA9D, 6);
		*(float*)0x00E98D74 = (float)AOStrength;

		//patchFloat((BYTE*)0x00518B00 + 2, AOSmoothness);
		//patchFloat((BYTE*)0x00518AEE + 2, AOSmoothness);

		//patchFloat((BYTE*)0x00E9898C, (float)AOQuality);
	}

	void Init()
	{

		if (GameConfig::GetValue("Graphics", "RemoveVignette", 0))
		{
			Render3D::RemoveVignette();
		}

		if (GameConfig::GetValue("Graphics", "BetterAmbientOcclusion", 0))
		{
			BetterAO();
		}
		if (GameConfig::GetValue("Graphics", "DisableScreenBlur", 0))
		{
			Logger::TypedLog(CHN_MOD, "Disabling Screen Blur...\n");
			patchByte((BYTE*)0x02527297, 0x0);
		}
		else
		{
			Logger::TypedLog(CHN_MOD, "Enabling Screen Blur...\n");
			patchByte((BYTE*)0x02527297, 0x1);

		}

		if (GameConfig::GetValue("Graphics", "ConsoleBrightness", 0))
		{
			Render3D::ConsoleLikeBrightness();
		}

		if (GameConfig::GetValue("Graphics", "VanillaFXPlus", 0))
		{
			Render3D::VFXPlus();
		}

		if (GameConfig::GetValue("Graphics", "DisableFog", 0)) // Option for the 2 psychopaths that think no fog looks better.
		{
			Render3D::DisableFog();
		}

		if (GameConfig::GetValue("Debug", "AltTabFPS", 1)) // Removes a sleep call in main render loop, this one seems to slow the game to below 25 fps when the game is alt-tabbed.
		{
			Render3D::AltTabFPS();
		}

		if (GameConfig::GetValue("Debug", "UncapFPS", 0)) // Removes a sleep call in main render loop, this one seems to slow the game to below 25 fps when the game is alt-tabbed.
		{  // Uncapping frames can lead to broken doors among other issues not yet noted.
			Render3D::UncapFPS();
		}

		// Removes all necessary sleep calls in the game, doubles fps and mitigates stutter, tanks CPU usage.
		if (GameConfig::GetValue("Debug", "SleepHack", 0) == 1) // LOW patch
		{
			Render3D::PatchLowSleepHack();
		}

		if (GameConfig::GetValue("Debug", "SleepHack", 0) == 2) // MEDIUM patch
		{
			Render3D::PatchMediumSleepHack();
		}
		if (GameConfig::GetValue("Debug", "SleepHack", 0) == 3) // HIGH patch, because why not i guess.
		{
			Logger::TypedLog(CHN_DLL, "Hooking sleep...\n");
			Render3D::HookSleep();
		}

		if (GameConfig::GetValue("Debug", "FasterLoadingScreens", 1))
		{
			Render3D::FasterLoadingScreens();
		}

		// Beefs up Tree Shadows considerably
		if (GameConfig::GetValue("Graphics", "UHQTreeShadows", 0))
		{
			Logger::TypedLog(CHN_NET, "Juicing up Tree Shadow Resolutions...\n");
			Render3D::PatchHQTreeShadows();
		}
		if (GameConfig::GetValue("Graphics", "FirstPersonCamera", 0) == 1)
		{
			Logger::TypedLog(CHN_MOD, "Turning SR2 into an FPS...\n");
			patchDWord((BYTE*)0x00495AC3 + 1, (uint32_t)&FPSCam);
		}
		if (GameConfig::GetValue("Graphics", "FirstPersonCamera", 0) == 2)
		{
			Logger::TypedLog(CHN_MOD, "Turning SR2 into an FPS with Viewmodel...\n");
			patchDWord((BYTE*)0x00495AC3 + 1, (uint32_t)&FPSCam);
			useFPSCam = 1;
		}
	}
}
