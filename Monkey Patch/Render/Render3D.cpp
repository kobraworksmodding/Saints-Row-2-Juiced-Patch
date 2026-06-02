// Render3D.cpp (uzis, Tervel)
// --------------------
// Created: 15/12/2024

#include "../FileLogger.h"
#include "../Patcher/patch.h"
#include <thread>
#include <vector>
#include "../iat_functions.h"
#include "../GameConfig.h"
#include "../SafeWrite.h"
#include "../Shaders.h"
#include "../BlingMenu_public.h"
#include "Render3D.h"
#include <mutex>

#include <safetyhook.hpp>

#include "d3d9.h"
#include "../General/General.h"

#include "Render2D.h"
#include "../Game/CrashFixes.h"
#include "../LUA/InGameConfig.h"
#include "Shadows.h"
#include "../Math/Math.h"
#include "../UtilsGlobal.h"
#include <fstream>
#include <map>
#include <Hooking.Patterns.h>
#include "..\Hooker.h"
#include <set>
#include <DirectXTex/DDSTextureLoader9.h>
#include "Textures.h"
#include "..\Game\Game.h"
import OptionsManager; 

namespace Render3D
{
	int SHADER_LOD = 0;
	int OVERRIDE_SHADER_LOD = 1;
	float SHADER_DISTANCE_SQUARED_MULT = 1.6f;
	const char FPSCam[] = "camera_fpss.xtbl";
	bool useFPSCam = 0;
	bool VFXP_fixFog = 0;
	float AOStrength = 1.5;
	bool ARfov = 0;
	double FOVMultiplier = 1;
	double UltrawideFixRatio = 1;
	const double fourbythreeAR = 1.333333373069763;
	//TODO: put this dds in some file somewhere instead of a header
	IDirect3DCubeTexture9* shd_cubedefault_tex;
	bool DitherFilter;

	void AspectRatioFix(bool update_aspect_ratio) {
		float currentAR = *(float*)0x022FD8EC;
		const float a169 = 1.777777791f;
		const double defaultFOV = 1.33333337306976;
		//double currentFOV = *(double*)0x0E5C808;
		double FOVmulti;
		if (currentAR > 1.55f)
			FOVmulti = defaultFOV;
		else FOVmulti = 1.0;
		double correctFOV;

		UltrawideFixRatio = ((double)currentAR / (double)a169);

		if (currentAR > 1.55f)
		correctFOV = (FOVmulti * UltrawideFixRatio);
		else
		correctFOV = (FOVmulti * ((double)currentAR / (double)currentAR));
		correctFOV *= Render3D::FOVMultiplier;
		if ((currentAR > a169 && Render3D::ARfov && update_aspect_ratio)) { // otherwise causes issues for odd ARs like 16:10/5:4 and the common 4:3.
			//patchDouble((BYTE*)0x00E5C808, correctFOV);
			patchNop((BYTE*)0x00797181, 6); // Crosshair location that is read from FOV, we'll replace with our own logic below.
			//patchFloat((BYTE*)0x00EC2614, correctFOV * Render3D::FOVMultiplier);
			Logger::TypedLog(CHN_DEBUG, "Aspect Ratio FOV fixed...\n");
		}

			double multipliedFOV = correctFOV * Render3D::FOVMultiplier;
			//patchDouble((BYTE*)0x00E5C808, multipliedFOV);
			patchNop((BYTE*)0x00797181, 6);
			patchFloat((BYTE*)0x00EC2614, (float)correctFOV);
		
		return;

	}

	SAFETYHOOK_NOINLINE float GetSmartCutsceneFOVMultiplier()
	{
		float screenW = (float)(*(unsigned int*)0x022f63f8_g);
		float screenH = (float)(*(unsigned int*)0x022f63fc_g);

		if (screenW <= 0.0f || screenH <= 0.0f)
			return 1.0f;

		constexpr float targetAspect = 24.0f / 10.0f; // 2.4

		float aspect = screenW / screenH;

		// do not touch FOV, borders handle it.
		if (aspect <= targetAspect)
			return 1.0f;

		// widen full render so the visible 24:10 center is not zoomed in.
		return aspect / targetAspect;
	}

	double __cdecl ConvertVerticalFOVToHorizontal_fixwidescreen(float verticalFOV, bool cutscene)
	{
		float radians = verticalFOV * 0.01745299994945526f;
		float tangent = tanf(radians * 0.5f);

		float adjusted;

		if (cutscene && Render2D::bSmartCutsceneBorder)
		{
			auto mult = GetSmartCutsceneFOVMultiplier();
			adjusted = tangent * mult;
		}
		else
		{
			adjusted = tangent * static_cast<float>(UltrawideFixRatio);

			if (!cutscene)
			{
				adjusted *= static_cast<float>(1.333333373069763f * Render3D::FOVMultiplier);
			}
		}
		// Convert back to degrees
		return atan(adjusted) * 2.0f * 57.29582977294922f;
	}
	double GetFOV() {
		bool* is_cutscene_active = (bool*)0x02527D14;
		bool* unk = (bool*)((*(int*)0x2527D10) + 0xAFF);
		float* cf_real_fov_deg = (float*)0x025F5BA4;
		bool* r_is_widescreen = (bool*)0x025272DD;

		bool isCutsceneMode = *is_cutscene_active && !*unk;
		// HUD Ultrawide HUD fix needs to be active so I can get this bool.
		// Early return for cutscene when UltrawideFix is not active, this is original game behaviour
		if (isCutsceneMode && !Render2D::UltrawideFix)
			return *cf_real_fov_deg;

		if (*r_is_widescreen || Render3D::FOVMultiplier != 1.f)
			return ConvertVerticalFOVToHorizontal_fixwidescreen(*cf_real_fov_deg, isCutsceneMode);

		return *cf_real_fov_deg;
	}
	void __declspec(naked) LoadShadersHook() {
		static int Continue = 0x00D1B7D3;
		static int* ShaderPointer;
		static const char* ShaderName;
		__asm {
			mov edi, eax
			mov ShaderPointer, eax
			mov ShaderName, esi
		}

		__asm pushad

		if (_stricmp(ShaderName, "distortion_tint_desat") == 0) {
			SafeWriteBuf((UInt32)ShaderPointer, X360GammaShader, sizeof(X360GammaShader));
		}
		
		if (_stricmp(ShaderName, "shadow_combiner_xxxx") == 0) {
				SafeWriteBuf((UInt32)ShaderPointer, ShadowShader, sizeof(ShadowShader));
		}
		__asm popad

		__asm {
			add esp, 8
			jmp Continue
		}
	}

	void LoadShadersHookMid(SafetyHookContext& ctx) {

		uintptr_t ShaderPointer = ctx.eax;
		const char* ShaderName = (const char*)(ctx.esi);

		bool valid = ShaderPointer && ShaderName;

		if (valid && _stricmp(ShaderName, "distortion_tint_desat") == 0) {
			SafeWriteBuf((UInt32)ShaderPointer, X360GammaShader, sizeof(X360GammaShader));
		}

		if (valid && _stricmp(ShaderName, "shadow_combiner_xxxx") == 0) {
			SafeWriteBuf((UInt32)ShaderPointer, ShadowShader, sizeof(ShadowShader));
		}

	}

	CMultiPatch CMPatches_PatchLowSleepHack = {

		//[](CMultiPatch& mp) {
		//	mp.AddPatchNop(0x0052108C,3);
		//},

		//[](CMultiPatch& mp) {
		//	mp.AddSafeWriteBuf(0x00521FC0, "\x6A\x00", 2);
		//},

		//[](CMultiPatch& mp) {
		//	mp.AddSafeWriteBuf(0x00521FE5, "\x6A\x00", 2);
		//},

		[](CMultiPatch& mp) {
			mp.AddSafeWriteBuf(0x005285A2, "\x6A\x00", 2);
		},
	};
	void PatchLowSleepHack()
	{
	if (GameConfig::GetValue("Debug", "FixPerformance", 1, "Fixes performance by waiting properly between main game thread and render thread.\nrecommended to be used alongside sync_shadows_threads=1 (Clippy95)") != 0)
			return;
		// Woohoo, this is a dirty patch, but we'll include it for people who want it and CAN actually run it.
		// This will destroy older and weaker pcs, but we'll make sure to let the people who are on that, know that.

		// This is the lower spec version of the patch, the things that will cause the LEAST cpu usage.

		Logger::TypedLog(CHN_DLL, "Removing a Very Safe Amount of Sleep Calls...\n");
		//patchNop((BYTE*)0x0052108C, 3); // patch win main sleep call
		patchBytesM((BYTE*)0x00521FC0, (BYTE*)"\x6A\x00", 2); // wait call in a threaded function, i think
		patchBytesM((BYTE*)0x00521FE5, (BYTE*)"\x6A\x00", 2); // same with this one
		patchBytesM((BYTE*)0x005285A2, (BYTE*)"\x6A\x00", 2); // this ones a doozy, this is some weird threaded exchange function, for each something, sleep. 
		//CMPatches_PatchLowSleepHack.Apply();
	}
	CPatch CPatches_MediumSleepHack = CPatch::SafeWriteBuf(0x0052847C, "\x6A\x00", 2);
	void PatchMediumSleepHack()
	{
		Logger::TypedLog(CHN_DLL, "Removing a Safe Amount of Sleep Calls...\n");
		/*patchBytesM((BYTE*)0x00521FC0, (BYTE*)"\x6A\x00", 2); // wait call in a threaded function, i think
		patchBytesM((BYTE*)0x00521FE5, (BYTE*)"\x6A\x00", 2); // same with this one
		patchBytesM((BYTE*)0x005285A2, (BYTE*)"\x6A\x00", 2); // this ones a doozy, this is some weird threaded exchange function, for each something, sleep. 
		patchBytesM((BYTE*)0x0052847C, (BYTE*)"\x6A\x00", 2); //make the shadow pool less sleepy*/
			
		//CMPatches_PatchLowSleepHack.Apply();

		CPatches_MediumSleepHack.Apply();
	}
	typedef void (WINAPI* SleepFn)(DWORD dwMilliseconds);
	SleepFn OriginalSleep = nullptr;

	void WINAPI SleepDetour(DWORD dwMilliseconds) {
		if (dwMilliseconds == 0) {
			std::this_thread::yield(); // not sure if this helps at all? can be yeeted if its useless
			return;
		}
		else {
			OriginalSleep(static_cast<DWORD>(dwMilliseconds / 1.5f));
		}
	}
	bool IsSleepHooked = false;
	void HookSleep() {
		if (IsSleepHooked)
			return;
		HMODULE main_handle = GetModuleHandleA(NULL);

		void* old_proc;

		if (PatchIat(main_handle, (char*)"Kernel32.dll", (char*)"Sleep", (void*)SleepDetour, &old_proc) == S_OK) {
			OriginalSleep = (SleepFn)old_proc;
			IsSleepHooked = true;
		}
	}

	void UnHookSleep() {
		if (!IsSleepHooked)
			return;
		HMODULE main_handle = GetModuleHandleA(NULL);

		if (PatchIat(main_handle, (char*)"Kernel32.dll", (char*)"Sleep", OriginalSleep, NULL) == S_OK) {
			IsSleepHooked = false;
		}
	}

	void FPSCamHack() {
		BYTE PlayerStatus = *(BYTE*)0x00E9A5BC; // Status Byte for the Players Actions.
		FLOAT* WalkCamZoom = (FLOAT*)0x025F6334;
		BYTE ActorFade = *(BYTE*)0x00E8825F;

		if (*(FLOAT*)WalkCamZoom > -0.5f) {
			*(FLOAT*)0x025F6334 = -0.6f; // Force camera zoom to chest/in front of player.
		}
		if (ActorFade == 0x01) {
			*(BYTE*)0x00E8825F = 0x00; // Force ActorFade to off.
		}
		if (PlayerStatus == 0x01 || PlayerStatus == 0x10 || PlayerStatus == 0x02 || PlayerStatus == 0x17) {
			*(BYTE*)0x00E9A5BC = 0x00; // Force the cam(?) state to 0x00 -- (Walking Outside) if got Running Outside, Running Inside or Walking Inside.
		}
	}
	CPatch CUncapFPS = CPatch::PatchNop(0x00D20E3E, 7);
	void UncapFPS()
	{
		Logger::TypedLog(CHN_DLL, "Uncapping FPS...\n");
		CUncapFPS.Apply();
	}

	void AltTabFPS()
	{
		Logger::TypedLog(CHN_DLL, "Making ALT-TAB smoother...\n");
		patchNop((BYTE*)0x005226F3, 8); // Bye bye sleep call.
	}

	void FasterLoading()
	{
		Logger::TypedLog(CHN_MOD, "Making loading screens slightly faster.\n");
		patchBytesM((BYTE*)0x0068C714, (BYTE*)"\x6A\x0F", 2);
		patchBytesM((BYTE*)0x00A72FD2, (BYTE*)"\x90\x90", 2); // this is a sleep call for first load/legal disclaimers, its set to 30 by default, halfing increases fps to 60 and makes loading faster.
	}
	volatile float Brightness = 1.32f;
	volatile float Saturation = 0.8f;
	volatile float Contrast = 1.58f;
	inline void VFXBrightnesstoggle() {
		if ((ShaderOptions.X360Gamma != 0)) {
			  Brightness = 1.32f;
			  Saturation = 0.8f;
			  Contrast = 1.58f;
		}
		else {
			Brightness = 1.26f;
			Saturation = 0.8f;
			Contrast = 1.62f;
		}
		//printf("Brit %f Satur %f Cont %f \n", Brightness, Saturation, Contrast);
	}
	CMultiPatch CMPatches_VFXPlus = {

		[](CMultiPatch& mp) {
			mp.AddPatchNop(0x00773797,5); // prevent the game from disabling/enabling the tint.
		},

		[](CMultiPatch& mp) {
			mp.AddSafeWrite32(0x0051A952 + 2, (uint32_t)&Brightness);
		},

		[](CMultiPatch& mp) {
			mp.AddSafeWrite32(0x0051A997 + 2, (uint32_t)&Saturation);
		},

		[](CMultiPatch& mp) {
			mp.AddSafeWrite32(0x0051A980 + 2, (uint32_t)&Contrast);
		},

		[](CMultiPatch& mp) {
			mp.AddSafeWrite8(0x00E9787F,0x1);
		},

		[](CMultiPatch& mp) {
			mp.AddPatchNop(0x00773792,5);
		},

		[](CMultiPatch& mp) {
			mp.AddSafeWrite8(0x00517051,0x8B);
		},
		[](CMultiPatch& mp) {
		mp.AddSafeWriteBuf(0x00524BA4, (BYTE*)"\xD9\x05\xBA\x2C\x7B\x02", 6);
		mp.AddSafeWriteBuf(0x00D1A333, (BYTE*)"\xD9\x05\xBA\x2C\x7B\x02", 6);
		mp.AddSafeWriteBuf(0x00524BB0, (BYTE*)"\xD9\x05\xBE\x2C\x7B\x02", 6);
		mp.AddSafeWriteBuf(0x00D1A3A3, (BYTE*)"\xD9\x05\xBE\x2C\x7B\x02", 6);
		},

		[](CMultiPatch& mp) {
		// lol
		mp.AddSafeWrite8((uintptr_t)&VFXP_fixFog,1);
	if (GameConfig::GetValue("Graphics", "UHQScreenEffects", 1, "2 = Maps the resolution to your screens resolution\n1 = Same as above but half res (Example: 1920x1080 > 1280x720 resolution scale.)\n0 = Vanilla Resolution\nIncreases resolution of Depth-of-field, skydive blur, reflections and bloom (Tervel)") == 0) {
			mp.AddSafeWriteBuf(0x005170EF, (BYTE*)"\x75", 1); // prevent bloom from appearing without breaking glow
		}
		}
	};

	void VFXPlus()
	{
		Logger::TypedLog(CHN_DEBUG, "Patching VanillaFXPlus...\n");
		CMPatches_VFXPlus.Apply();
		return;
		Logger::TypedLog(CHN_DEBUG, "Patching VanillaFXPlus...\n");
		patchNop((BYTE*)0x00773797, 5); // prevent the game from disabling/enabling the tint.
		patchBytesM((BYTE*)0x0051A952, (BYTE*)"\xD9\x05\x7F\x2C\x7B\x02", 6); // new brightness address
		patchBytesM((BYTE*)0x0051A997, (BYTE*)"\xD9\x05\x83\x2C\x7B\x02", 6); // new sat address patch
		patchBytesM((BYTE*)0x0051A980, (BYTE*)"\xD9\x05\x87\x2C\x7B\x02", 6); // new contr address patch
		patchByte((BYTE*)0x00E9787F, 0x01); // force HDR on
		patchNop((BYTE*)0x00773792, 5); // prevent the game from turning HDR on/off
		patchBytesM((BYTE*)0x00517051, (BYTE*)"\x8B", 1); // flip the logic for the HDR strength (or radius?) float check
		//patchNop((BYTE*)0x00533C25, 5); // disable sky refl (prevent the absurd blue tint on reflections)

	if (GameConfig::GetValue("Graphics", "X360Gamma", 1, "Accurately replicates the XBOX 360 gamma visuals. (Tervel)")) {
			patchFloat((BYTE*)0x027B2C7F, 1.32f); //Bright
			patchFloat((BYTE*)0x027B2C83, 0.8f); //Sat
			patchFloat((BYTE*)0x027B2C87, 1.58f); //Contr
		}
		else {
			patchFloat((BYTE*)0x027B2C7F, 1.26f); //Bright
			patchFloat((BYTE*)0x027B2C83, 0.8f); //Sat
			patchFloat((BYTE*)0x027B2C87, 1.62f); //Contr
		}

		patchBytesM((BYTE*)0x00524BA4, (BYTE*)"\xD9\x05\xBA\x2C\x7B\x02", 6);
		patchBytesM((BYTE*)0x00D1A333, (BYTE*)"\xD9\x05\xBA\x2C\x7B\x02", 6);
		patchBytesM((BYTE*)0x00524BB0, (BYTE*)"\xD9\x05\xBE\x2C\x7B\x02", 6);
		patchBytesM((BYTE*)0x00D1A3A3, (BYTE*)"\xD9\x05\xBE\x2C\x7B\x02", 6);
		VFXP_fixFog = 1;

	if (GameConfig::GetValue("Graphics", "UHQScreenEffects", 1, "2 = Maps the resolution to your screens resolution\n1 = Same as above but half res (Example: 1920x1080 > 1280x720 resolution scale.)\n0 = Vanilla Resolution\nIncreases resolution of Depth-of-field, skydive blur, reflections and bloom (Tervel)") == 0) {
			patchBytesM((BYTE*)0x005170EF, (BYTE*)"\x75", 1); // prevent bloom from appearing without breaking glow
		}
	}
	CMultiPatch CMPatches_DisableSkyRefl = {
	[](CMultiPatch& mp) {
			mp.AddPatchNop(0x00532A4F,6); // nop for whatever the fuck
			mp.AddSafeWriteBuf(0x00532992, "\xDD\x05\xAA\x2C\x7B\x02", 6); // new opacity address for sky reflections
			mp.AddSafeWrite<double>(0x027B2CAA, 128.0);
		},
	};
	void DisableSkyRefl() {
		CMPatches_DisableSkyRefl.Apply();
		//patchNop((BYTE*)0x00532A4F, 6); // nop for whatever the fuck
		//patchBytesM((BYTE*)0x00532992, (BYTE*)"\xDD\x05\xAA\x2C\x7B\x02", 6); // new opacity address for sky reflections
		//patchDouble((BYTE*)0x027B2CAA, 128.0);
	}

	CMultiPatch CMPatches_DisableFog = {

		[](CMultiPatch& mp) {
			mp.AddSafeWrite8(0x0025273BE, 1);
		},

		[](CMultiPatch& mp) {
			mp.AddSafeWrite<float>(0x00E989A0, 0.0f);
		},

		[](CMultiPatch& mp) {
			mp.AddSafeWrite<float>(0x00E989A4, 0.0f);
		},

	};

	void DisableFog()
	{
		/*patchBytesM((BYTE*)0x0025273BE, (BYTE*)"\x01", 1); // leftover debug bool for being able to overwrite fog values
		patchFloat((BYTE*)0x00E989A0, 0.0f);
		patchFloat((BYTE*)0x00E989A4, 0.0f);*/
		CMPatches_DisableFog.Apply();
	}

	void RemoveVignette()
	{
		Logger::TypedLog(CHN_MOD, "Disabling Vignette...\n");
		patchNop((BYTE*)0x00E0C62C, 9); // nop aVignette
	}
	CPatch CBetterAO = CPatch::SafeWriteBuf(0x00518AFE, "\xEB\x0A", 2);
	void BetterAO()
	{
		Logger::TypedLog(CHN_MOD, "Making AO Better...\n");
		//patchNop((BYTE*)0x0052AA90, 6);
		//patchNop((BYTE*)0x005183C8, 6);
		//*(float*)0x348FFDC = (float)AOStrength;
													//patchBytesM((BYTE*)0x00518AFE, (BYTE*)"\xEB\x0A", 2);
		CBetterAO.Apply();
		//patchFloat((BYTE*)0x00518375 + 2, AOStrength);

		//patchFloat((BYTE*)0x00518B00 + 2, AOSmoothness);
		//patchFloat((BYTE*)0x00518AEE + 2, AOSmoothness);

		//patchFloat((BYTE*)0x00E9898C, (float)AOQuality);
	}

	CPatch CRemoveBlackBars = CPatch::PatchNop(0x0075A265, 5);

	void RemBlackBars()
	{
		Logger::TypedLog(CHN_DLL, "Removing Black Bars.\n");
		//patchNop((BYTE*)(0x0075A265), 5);
		CRemoveBlackBars.Apply();
	}

	double FilteringStrength;

	void __declspec(naked) StrengthWorkaround() {
		static int Continue = 0x00515CA0;
		__asm {
			cmp ebx, 3
			jnz Skip
			cmp ds : byte ptr[0x2527D14], 1
			jnz Skip
			fld FilteringStrength
			jmp Continue

			Skip :
			fld ds : dword ptr[0x00E849AC]
				jmp Continue
		}
	}

	void SetAORes(int X, int Y) {

		std::vector<int*> XAddresses = {
			(int*)0x00DC8F6C, (int*)0x00E863A4, (int*)0x00E86398
		};

		std::vector<int*> YAddresses = {
			(int*)0x00DC8EE4, (int*)0x00E863A8, (int*)0x00E8639C
		};

		for (int* Addr : XAddresses) {
			SafeWrite32(UInt32(Addr), (UInt32)X);
		}

		for (int* Addr : YAddresses) {
			SafeWrite32(UInt32(Addr), (UInt32)Y);
		}
	}

	void SetVehReflRes(int X) {

		std::vector<int*> Addresses = {
			(int*)0x00DC8E78, (int*)0x00DC8F00, (int*)0x00E86264, (int*)0x00E86260
		};

		for (int* Addr : Addresses) {
			SafeWrite32(UInt32(Addr), (UInt32)X);
		}
	}

	float BloomResX;
	float BloomResY;

	void SetWaterReflRes(int X, int Y) {

		std::vector<int*> XAddresses = {
			(int*)0x00DC8F60, (int*)0x00E86380
		};

		std::vector<int*> YAddresses = {
			(int*)0x00DC8ED8, (int*)0x00E86384
		};

		for (int* Addr : XAddresses) {
			SafeWrite32(UInt32(Addr), (UInt32)X);
		}

		for (int* Addr : YAddresses) {
			SafeWrite32(UInt32(Addr), (UInt32)Y);
		}
	}

	void SetBloomRes(int X, int Y, float XFloat, float YFloat) {

		BloomResX = XFloat;
		BloomResY = YFloat;

		std::vector<int*> XAddresses = {
			(int*)0x00516947, (int*)0x00516A27, (int*)0x00516C6B, (int*)0x00E86368,
			(int*)0x00E86374, (int*)0x00DC8F5C, (int*)0x00DC8F58
		};

		std::vector<int*> YAddresses = {
			(int*)0x00516956, (int*)0x00516C76, (int*)0x00E8636C, (int*)0x00E86378,
			(int*)0x00DC8ED0, (int*)0x00DC8ED4
		};

		for (int* Addr : XAddresses) {
			SafeWrite32(UInt32(Addr), (UInt32)X);
		}

		for (int* Addr : YAddresses) {
			SafeWrite32(UInt32(Addr), (UInt32)Y);
		}
	}

	void SetDOFRes(int X, int Y) {

		FilteringStrength = Y / 1080.0;

		std::vector<int*> Addresses = {
			(int*)0x00DC8E80, (int*)0x00DC8E84, (int*)0x00DC8F0C, (int*)0x00DC8F08,
			(int*)0x00E86278, (int*)0x00E8627C, (int*)0x00E86284, (int*)0x00E86288
		};

		for (int* Addr : Addresses) {
			SafeWrite32(UInt32(Addr), (UInt32)X);
		}
	}

	typedef int SetGraphicsT();
	SetGraphicsT* SetGraphics = (SetGraphicsT*)(0x7735C0);

	void ResizeEffects() {
		int CurrentX = *(int*)0x22FD84C;
		int CurrentY = *(int*)0x22FD850;

		float aspectRatio = (float)CurrentX / (float)CurrentY;

		// DOF
		{
			int dofX = CurrentX;
			int dofY = CurrentY;
			if (dofY > 640) {
				dofY = 640;
				dofX = (int)(dofY * aspectRatio);
			}
			SetDOFRes(dofX, dofY);
		}

		// Bloom
		{
			int bloomX = CurrentX;
			int bloomY = CurrentY;
			if (bloomY > 1080) {
				bloomY = 1080;
				bloomX = (int)(bloomY * aspectRatio);
			}
			SetBloomRes(bloomX, bloomY, (float)bloomX, (float)bloomY);
		}

		// Water reflections
		{
			int waterX = CurrentX;
			int waterY = CurrentY;
			if (waterY > 800) {
				waterY = 800;
				waterX = (int)(waterY * aspectRatio);
			}
			SetWaterReflRes(waterX, waterY);
		}

		// Vehicle reflections
		{
			int vehX = CurrentX;
			int vehY = CurrentY;
			if (vehY > 640) {
				vehY = 640;
				vehX = (int)(vehY * aspectRatio);
			}
			SetVehReflRes(vehX);
		}
		//SetAORes(CurrentX, CurrentY);
		SetGraphics();
	}

	void UHQEffects() {
		WriteRelJump(0x00515C9A, (UInt32)&StrengthWorkaround);
		SafeWrite32(0x005169C8 + 2, (UInt32)&BloomResX);
		SafeWrite32(0x005169BB + 2, (UInt32)&BloomResY);
		patchCall((void*)0x007740D9, (void*)ResizeEffects);
		patchCall((void*)0x007743CE, (void*)ResizeEffects);

		Logger::TypedLog(CHN_MOD, "Patching UHQScreenEffects at full quality...\n");

	}

	class MemoryAccessCache {
	private:
		std::unordered_map<uintptr_t, bool> cache;
		std::mutex cacheMutex;

		static const size_t PAGE_SIZE = 4096;
		static const size_t CACHE_MAX_SIZE = 1024;

	public:
		static uintptr_t GetPageBase(void* address) {
			return reinterpret_cast<uintptr_t>(address) & ~(PAGE_SIZE - 1);
		}

		bool IsMemoryReadable(void* address) {
			if (!address) return false;

			uintptr_t pageBase = GetPageBase(address);

			{
				std::lock_guard<std::mutex> lock(cacheMutex);
				auto it = cache.find(pageBase);
				if (it != cache.end()) {
					return it->second;
				}
			}

			MEMORY_BASIC_INFORMATION mbi;
			bool isReadable = false;

			if (VirtualQuery(address, &mbi, sizeof(mbi))) {
				isReadable = (mbi.State == MEM_COMMIT) &&
					((mbi.Protect & PAGE_READONLY) ||
						(mbi.Protect & PAGE_READWRITE) ||
						(mbi.Protect & PAGE_EXECUTE_READ) ||
						(mbi.Protect & PAGE_EXECUTE_READWRITE));
			}

			{
				std::lock_guard<std::mutex> lock(cacheMutex);


				if (cache.size() >= CACHE_MAX_SIZE) {
					cache.clear();
				}

				cache[pageBase] = isReadable;
			}

			return isReadable;
		}


		static MemoryAccessCache& GetInstance() {
			static MemoryAccessCache instance;
			return instance;
		}
	};
	bool IsMemoryReadable(void* address) {
		return MemoryAccessCache::GetInstance().IsMemoryReadable(address);
	}
	bool crash;

	constexpr uintptr_t add_to_entry_func_addr = 0xC080C0;

// This whole thing might have a performance hit.
// SafeAddToEntry might be a bit redundant? since it seems to have not worked at all. but I'll keep it.
	SafetyHookMid add_to_entry_test;

	void add_to_entry_crashaddr_hook(safetyhook::Context32& ctx) {
		/*
		if (crash) {
			printf("lol crash attempt");
			ctx.eax = 0xDEADBEEF;
			crash = false;
		}*/
		uint16_t* width = (uint16_t*)(ctx.eax + 0x4);
		DWORD* wtf = (DWORD*)(ctx.eax + 0x24);
		// gtfo out of function
		if (!IsMemoryReadable(width) || !IsMemoryReadable(wtf)) {
			AssertHandler::AssertOnce("the other add_to_entry hook", "Crashed prevented due to an invalid be->current_peg_entry in add_to_entry, it's recommended to make a save of your game at this point as the game still has a high chance to crash! \n");
			Logger::TypedLog("the other add_to_entry hook", "!!!Invalid result->width: {}\n", fmt::ptr(width));
			ctx.eip = 0x00C08101;
		}
		

	}
	void possible_unload_entry_func(SafetyHookContext& ctx) {

		if (!IsMemoryReadable((void*)ctx.ecx)) {
			AssertHandler::AssertOnce("possible_unload_entry_func", "Crashed prevented due to an invalid be->current_peg_entry in add_to_entry, it's recommended to make a save of your game at this point as the game still has a high chance to crash! \n");
			ctx.eip = 0x00BD8665;
		}
	}
#if !JLITE
	CMultiPatch CMPatches_ClassicGTAIdleCam = {

		[](CMultiPatch& mp) {
			mp.AddPatchNop(0x00994541, 5);
		},

		[](CMultiPatch& mp) {
			mp.AddPatchNop(0x0099454C, 5);
		},
	};
#endif
	shaderOptions ShaderOptions;
	

	struct RenderCMDBuffer
	{
		char* Buffer;
		int   BufferSize;
		char* ExecuteWritePos;
		char* WritePos;
		char* WritePosStart;
		char* ReadPos;
	};

	RenderCMDBuffer& RenderBuffer = *(RenderCMDBuffer*)0x033D62EC;

	void SetPSConstF(UINT reg, const float* data, UINT count = 1)
	{
		if (reg < 0) return;

		uint32_t* cmd = (uint32_t*)RenderBuffer.WritePos;

		cmd[0] = 7;
		cmd[1] = reg;
		cmd[2] = count;

		memcpy(&cmd[3], data, 16 * count);

		RenderBuffer.WritePos += 12 + (16 * count);
	}

	void ChangeShaderOptions() {
		float arr4[4];
		arr4[0] = (ShaderOptions.X360Gamma) != 0 ? 0.0f : 1.0f;
		arr4[1] = (ShaderOptions.ShadowFilter) != 0 ? 0.0f : 1.0f;
		arr4[2] = 0.f;
		arr4[3] = 0.f;
		float Res[4] = { (float)*General::GameResX, (float)*General::GameResY, 0.f, 0.f};
		// distortion_juicedsettings for Gamma.
		SetPSConstF(187, &arr4[0], 1);
		SetPSConstF(188, &Res[0], 1);
	}
	void SETLOD(SafetyHookContext& ctx) {
		if (OVERRIDE_SHADER_LOD == 1) {
			ctx.eax = SHADER_LOD;
			ctx.eip = 0x00D19D24;
		}
		else if (OVERRIDE_SHADER_LOD == 2) {
			float* distance_squared = (float*)(ctx.esp + 0xC);
			*distance_squared /= SHADER_DISTANCE_SQUARED_MULT;
		}
	}

	// Current hooks the parsing function for shaders_pc to only DELETE lines.. it can be expanded to do addition by using a new buffer, but that isn't really a use-case right now. - Clippy
	void shaders_pc_hook() {
		static auto shaders_pc_parse_hook = safetyhook::create_mid(0x00D1B67F, [](SafetyHookContext& ctx) {
			char* buffer = (char*)ctx.ebx;
			size_t& buffer_size1 = ctx.eax;
			size_t& buffer_size2 = ctx.edi;

			auto remove_line = [&](const char* line_to_remove) {
				std::string buffer_str(buffer, buffer_size1);
				std::string target_line = line_to_remove;

				size_t pos = buffer_str.find(target_line);
				if (pos != std::string::npos) {
					size_t line_start = pos;
					while (line_start > 0 && buffer_str[line_start - 1] != '\n') {
						line_start--;
					}

					size_t line_end = pos + target_line.length();
					while (line_end < buffer_str.length() &&
						(buffer_str[line_end] == '\r' || buffer_str[line_end] == '\n')) {
						line_end++;
					}

					size_t line_length = line_end - line_start;

					buffer_str.erase(line_start, line_length);

					size_t new_size = buffer_str.length();
					memcpy(buffer, buffer_str.c_str(), new_size);

					buffer_size1 = new_size;
					buffer_size2 = new_size;

					return true;
				}
				return false;
				};

	if (GameConfig::GetValue("Graphics", "RemovePixelationShader", 0, "Removes Pixelation/censor filter from the game. (Clippy95)")) {
					remove_line("data\\shaders\\standard\\sr2-pixelate_c.fxo_pc");
					Logger::TypedLog(CHN_MOD, "Patching shaders_pc to remove {}\n", "data\\shaders\\standard\\sr2-pixelate_c.fxo_pc");
				}
			});
	}

	volatile float VehicleDespawnDistance = 140.f;
	CPatch CIncreaseVehicleDespawnDistance = CPatch::SafeWrite32(0x0093BDF9, (uint32_t)&VehicleDespawnDistance);
	static inline void setBL(SafetyHookContext& ctx, int val)
	{
		ctx.ebx = (ctx.ebx & 0xFFFFFF00) | (val & 0xFF);
	}
	SafetyHookMid screen_3d_to_2d_midhook;


	struct batch_entry
	{
		void* m_ri;
		unsigned int m_flags;
		unsigned int m_vis_bits0;
		unsigned int m_vis_bits1;
		float render_distance;
	};


	template <size_t N>
	class render_batch {
	public:
		batch_entry m_batch[N];
		unsigned int m_size;
	};
	static_assert(sizeof(batch_entry) == 0x14);
	static_assert(offsetof(batch_entry, m_flags) == 0x4);
	static_assert(offsetof(render_batch<1536>, m_size) == 0x7800);
	static_assert(offsetof(render_batch<1024>, m_size) == 0x5000);
	static_assert(offsetof(render_batch<512>, m_size) == 0x2800);

	struct addr_xref {
		uintptr_t patch_location;
		intptr_t offset;
	};

	constexpr size_t kMainRenderBatchNewCapacity = 5000;
	constexpr size_t kAlphaRenderBatchNewCapacity = 2000;
	constexpr size_t kSuppPassRenderBatchNewCapacity = 2000;

	constexpr size_t kRenderBatchFlagsOffset = offsetof(batch_entry, m_flags);
	constexpr size_t kMainRenderBatchNewSizeOffset = offsetof(render_batch<kMainRenderBatchNewCapacity>, m_size);
	constexpr size_t kAlphaRenderBatchNewSizeOffset = offsetof(render_batch<kAlphaRenderBatchNewCapacity>, m_size);
	constexpr size_t kSuppPassBatchNewSizeOffset = offsetof(render_batch<kSuppPassRenderBatchNewCapacity>, m_size);

#define BATCH_XREF(addr, offset) { addr, offset }
	static constexpr addr_xref g_main_render_batch_xrefs[] = {
		BATCH_XREF(0x005242F9, 0),
		BATCH_XREF(0x0052433D, 0),
		BATCH_XREF(0x00524350, 0),
		BATCH_XREF(0x00524386, 0),
		BATCH_XREF(0x0052550E, 0),
		BATCH_XREF(0x005255C9, 0),
		BATCH_XREF(0x00526C13, 0),
		BATCH_XREF(0x005284BA, 0),
		BATCH_XREF(0x0052870E, 0),
		BATCH_XREF(0x0052AE71, 0),
		BATCH_XREF(0x0052AE8B, 0),
		BATCH_XREF(0x0052AE96, 0),
		BATCH_XREF(0x0052FD9B, 0),
		BATCH_XREF(0x00536C3C, 0),
		BATCH_XREF(0x00538435, 0),
		BATCH_XREF(0x0052595C, kRenderBatchFlagsOffset),
		BATCH_XREF(0x005260DB, kRenderBatchFlagsOffset),
		BATCH_XREF(0x0052648A, kRenderBatchFlagsOffset),
		BATCH_XREF(0x005267B9, kRenderBatchFlagsOffset),
		BATCH_XREF(0x00526A34, kRenderBatchFlagsOffset),
		BATCH_XREF(0x00526D44, kRenderBatchFlagsOffset),
		BATCH_XREF(0x004B7CC1, kMainRenderBatchNewSizeOffset),
		BATCH_XREF(0x004B7EC4, kMainRenderBatchNewSizeOffset),
		BATCH_XREF(0x004B8271, kMainRenderBatchNewSizeOffset),
		BATCH_XREF(0x005242E9, kMainRenderBatchNewSizeOffset),
		BATCH_XREF(0x0052431B, kMainRenderBatchNewSizeOffset),
		BATCH_XREF(0x00525943, kMainRenderBatchNewSizeOffset),
		BATCH_XREF(0x00525B8D, kMainRenderBatchNewSizeOffset),
		BATCH_XREF(0x00526A22, kMainRenderBatchNewSizeOffset),
		BATCH_XREF(0x00526C03, kMainRenderBatchNewSizeOffset),
		BATCH_XREF(0x00526C33, kMainRenderBatchNewSizeOffset),
		BATCH_XREF(0x00526D23, kMainRenderBatchNewSizeOffset),
		BATCH_XREF(0x00526F3F, kMainRenderBatchNewSizeOffset),
		BATCH_XREF(0x00528619, kMainRenderBatchNewSizeOffset),
		BATCH_XREF(0x00528703, kMainRenderBatchNewSizeOffset),
		BATCH_XREF(0x0052878F, kMainRenderBatchNewSizeOffset),
		BATCH_XREF(0x00528834, kMainRenderBatchNewSizeOffset),
		BATCH_XREF(0x0052899D, kMainRenderBatchNewSizeOffset),
		BATCH_XREF(0x00528B57, kMainRenderBatchNewSizeOffset),
		BATCH_XREF(0x00528BF5, kMainRenderBatchNewSizeOffset),
		BATCH_XREF(0x00528CE6, kMainRenderBatchNewSizeOffset),
		BATCH_XREF(0x00528DFE, kMainRenderBatchNewSizeOffset),
		BATCH_XREF(0x00528F13, kMainRenderBatchNewSizeOffset),
		BATCH_XREF(0x0052920D, kMainRenderBatchNewSizeOffset),
		BATCH_XREF(0x0052929F, kMainRenderBatchNewSizeOffset),
		BATCH_XREF(0x0052AE81, kMainRenderBatchNewSizeOffset),
		BATCH_XREF(0x0052E7D2, kMainRenderBatchNewSizeOffset),
		BATCH_XREF(0x0052FD92, kMainRenderBatchNewSizeOffset),
		BATCH_XREF(0x0052FDA1, kMainRenderBatchNewSizeOffset),
		BATCH_XREF(0x0052FDDC, kMainRenderBatchNewSizeOffset),
		BATCH_XREF(0x0052FDF2, kMainRenderBatchNewSizeOffset),
		BATCH_XREF(0x0052FDC0, -static_cast<intptr_t>(sizeof(batch_entry))),
		BATCH_XREF(0x00536C09, kMainRenderBatchNewSizeOffset),
		BATCH_XREF(0x00536C87, kMainRenderBatchNewSizeOffset),
		BATCH_XREF(0x0053842E, kMainRenderBatchNewSizeOffset),
		BATCH_XREF(0x0053846C, kMainRenderBatchNewSizeOffset),
	};

	static constexpr addr_xref g_alpha_render_batch_xrefs[] = {
		BATCH_XREF(0x005243A0, 0),
		BATCH_XREF(0x005243E3, 0),
		BATCH_XREF(0x005243F6, 0),
		BATCH_XREF(0x0052442C, 0),
		BATCH_XREF(0x005272EC, 0),
		BATCH_XREF(0x0052AEB1, 0),
		BATCH_XREF(0x0052AECB, 0),
		BATCH_XREF(0x0052AED6, 0),
		BATCH_XREF(0x0052FE0B, 0),
		BATCH_XREF(0x00524390, kAlphaRenderBatchNewSizeOffset),
		BATCH_XREF(0x005243C1, kAlphaRenderBatchNewSizeOffset),
		BATCH_XREF(0x005272DA, kAlphaRenderBatchNewSizeOffset),
		BATCH_XREF(0x00527A05, kAlphaRenderBatchNewSizeOffset),
		BATCH_XREF(0x0052AEA1, kAlphaRenderBatchNewSizeOffset),
		BATCH_XREF(0x0052AEC1, kAlphaRenderBatchNewSizeOffset),
		BATCH_XREF(0x0052E7D8, kAlphaRenderBatchNewSizeOffset),
		BATCH_XREF(0x0052FE02, kAlphaRenderBatchNewSizeOffset),
		BATCH_XREF(0x0052FE11, kAlphaRenderBatchNewSizeOffset),
		BATCH_XREF(0x0052FE4C, kAlphaRenderBatchNewSizeOffset),
		BATCH_XREF(0x0052FE62, kAlphaRenderBatchNewSizeOffset),
		BATCH_XREF(0x0052FE30, -static_cast<intptr_t>(sizeof(batch_entry))),
	};

	static constexpr addr_xref g_supp_pass_batch_xrefs[] = {
		BATCH_XREF(0x0052AEF6, 0),
		BATCH_XREF(0x0052FE7B, 0),
		BATCH_XREF(0x005248D0, kRenderBatchFlagsOffset),
		BATCH_XREF(0x00529BD8, kRenderBatchFlagsOffset),
		BATCH_XREF(0x005248B6, kSuppPassBatchNewSizeOffset),
		BATCH_XREF(0x00524AA4, kSuppPassBatchNewSizeOffset),
		BATCH_XREF(0x00529BBA, kSuppPassBatchNewSizeOffset),
		BATCH_XREF(0x00529E67, kSuppPassBatchNewSizeOffset),
		BATCH_XREF(0x0052AEE1, kSuppPassBatchNewSizeOffset),
		BATCH_XREF(0x0052E7DE, kSuppPassBatchNewSizeOffset),
		BATCH_XREF(0x0052FE72, kSuppPassBatchNewSizeOffset),
		BATCH_XREF(0x0052FE81, kSuppPassBatchNewSizeOffset),
		BATCH_XREF(0x0052FEBC, kSuppPassBatchNewSizeOffset),
		BATCH_XREF(0x0052FED2, kSuppPassBatchNewSizeOffset),
		BATCH_XREF(0x0052FEA0, -static_cast<intptr_t>(sizeof(batch_entry))),
	};
#undef BATCH_XREF

	constexpr auto new_size_n = kMainRenderBatchNewCapacity;
	render_batch<new_size_n> main_render_bigger{};
	bool push_back_force_old_size = false;
	size_t debug_push_back_size(unsigned int old_size, unsigned int new_size) {

		if (!push_back_force_old_size)
			return new_size;
		
		return old_size;

	}



#define Alpha_Size_New kAlphaRenderBatchNewCapacity
	render_batch<Alpha_Size_New> Alpha_Render_batch;


#define Supp_pass_Size_New kSuppPassRenderBatchNewCapacity
	render_batch<Supp_pass_Size_New> Supp_pass_increased;

	template<size_t OriginalSize, size_t NewSize, size_t XrefCount>
	void PatchRenderBatch(const char* batch_name, uintptr_t original_address, render_batch<NewSize>& new_structure, const addr_xref(&xrefs)[XrefCount]) {
		printf("=== Patching %s render_batch Structure ===\n", batch_name);
		printf("Original address: 0x%08X (size: %zu entries)\n", static_cast<unsigned int>(original_address), OriginalSize);
		printf("New structure address: 0x%08X (size: %zu entries)\n",
			static_cast<unsigned int>(reinterpret_cast<uintptr_t>(&new_structure)), NewSize);
		printf("New structure size: %zu bytes (was %zu)\n\n", sizeof(new_structure), sizeof(render_batch<OriginalSize>));

		for (const auto& xref : xrefs) {
			uintptr_t new_value = static_cast<uintptr_t>(reinterpret_cast<intptr_t>(&new_structure) + xref.offset);
			SafeWrite32(static_cast<UInt32>(xref.patch_location), static_cast<UInt32>(new_value));
		}

		new_structure.m_size = 0;

		printf("Patched %zu references\n", XrefCount);
		printf("New structure capacity: %zu entries (was %zu)\n", NewSize, OriginalSize);
		printf("Memory size increase: %zu bytes\n", sizeof(new_structure) - sizeof(render_batch<OriginalSize>));
		printf("New %s structure initialized and ready for use!\n", batch_name);
		printf("========================================\n\n");
	}

#define Instance_pool_buffer_old_size 4587520
#define Instance_pool_buffer_new_size Instance_pool_buffer_old_size * 6
	unsigned __int8 Instance_pool_buffer[Instance_pool_buffer_new_size];


	bool& rendered_second_this_frame = *(bool*)DynAddress(0x0252737E);

	HANDLE render_handshake_event;
	HANDLE render_frame_event;

	void signal_handshake_done() {
		SetEvent(render_handshake_event);
	}

	void signal_frame_done() {
		SetEvent(render_frame_event);
	}
	unsigned char oldbytes[0x48];
	unsigned char newbytes[0x48];
	bool use_old = false;
	void apply_old() {
		if (use_old) {
			SafeWriteBuf((uint32_t)0x00521FAB, oldbytes, 0x48);
		} else
			SafeWriteBuf((uint32_t)0x00521FAB, newbytes, 0x48);


	}

	BOOL __cdecl sub_522D00_signal_frame_done() {
		signal_frame_done();

		static auto sub_522D00_addr = DynAddress(0x522D00);
		return ((bool(__cdecl*)(void))sub_522D00_addr)();

	}

	
	unsigned int* __fastcall debug_metrics_start_signal_handshake_done(char* metric_name,void* unused ,int* metric_handle) {
		signal_handshake_done();

		static auto debug_metrics_start_addr = DynAddress(0xC0BE10);
		return ((unsigned int* (__thiscall*)(char*, int*))debug_metrics_start_addr)(metric_name,metric_handle);
	}

	void sync_handhake_render() {
	if (GameConfig::GetValue("Debug", "FixPerformance", 1, "Fixes performance by waiting properly between main game thread and render thread.\nrecommended to be used alongside sync_shadows_threads=1 (Clippy95)") == 0) {
			return;
		}
		SafeWrite8(0x005285A3, 0);
		render_handshake_event = CreateEvent(NULL, FALSE, FALSE, NULL);
		render_frame_event = CreateEvent(NULL, FALSE, FALSE, NULL);
		memcpy(oldbytes, (void*)0x00521FAB, 0x48);
		oldbytes[0x16] = 0;
		oldbytes[0x3B] = 0;

		patchNop((void*)0x00521FAB, 0x48);

		patchCall((void*)0x522865, debug_metrics_start_signal_handshake_done);

		patchCall((void*)0x522A05, sub_522D00_signal_frame_done);

		static auto sync_hack = safetyhook::create_mid(0x00521FAB, [](SafetyHookContext& ctx) {

			WaitForSingleObject(render_handshake_event, INFINITE);

			if (!rendered_second_this_frame) {
				WaitForSingleObject(render_frame_event, INFINITE);
			}
			});
		memcpy(newbytes, (void*)0x00521FAB, 0x48);

	}

	static uint32_t alpha_lastframe = 0;
	bool AlphaMaskAvailable() { // 2 in 1, fix for the NVIDIA oversight (no unnecessary additional check), as well as updating the shader constant for our modified shaders
		int MSAA = *(int*)DynAddress(0x252A2DC);
		bool AlphaMaskVal = *(bool*)DynAddress(0x0252A2EC);

		bool Result = MSAA > 0 && AlphaMaskVal;
		float AlphaMask[4] = { Result ? 1.0f : 0.0f, DitherFilter ? 1.0f : 0.0f, 0.0f, 0.0f };
		if (alpha_lastframe != Game::Timer::GetFrameCount()) {
			ChangeShaderOptions();
			SetPSConstF(189, &AlphaMask[0], 1);
		}
		static auto alpha_lastframe = Game::Timer::GetFrameCount();
		return Result;
	}

	float RenderDistance_old;

	int __cdecl cutscene_func_city_render_fade_dist_scale(int a1)
	{
		int result; // eax

		result = a1;
		float& value = *(float*)(a1 + 8);
		if(value != 1.f)
		*(float*)0xE996B4 = value;
		else {
			*(float*)0xE996B4 = RenderDistance_old;
		}
		return result;
	}

	uint32_t AdapterCount = 0;
	std::vector<std::pair<uint32_t, uint32_t>> AdapterModes;

	bool init_directx9(void* unk) {
		bool result = ((bool(__cdecl*)(void*))0xD1F3F0)(unk);
		if (result == false)
			return false;

		IDirect3D9* pIDirect3D9 = *reinterpret_cast<IDirect3D9**>(0x0252A2CC);

		UINT selectedAdapter = *(bool*)0x02527348;



		UINT adapterCount = pIDirect3D9->GetAdapterCount();
		for (UINT i = 0; i < adapterCount; ++i) {
			D3DADAPTER_IDENTIFIER9 identifier;
			pIDirect3D9->GetAdapterIdentifier(i, 0, &identifier);
			if (strstr(identifier.Description, "PerfHUD")) {
				selectedAdapter = i;
				break;
			}
		}

		AdapterModes.clear();
		std::set<std::pair<uint32_t, uint32_t>> uniqueResolutions;


		uint32_t modeCount = pIDirect3D9->GetAdapterModeCount(selectedAdapter, D3DFMT_X8R8G8B8);

		for (uint32_t mode = 0; mode < modeCount; ++mode) {
			D3DDISPLAYMODE displayMode;
			HRESULT hr = pIDirect3D9->EnumAdapterModes(selectedAdapter, D3DFMT_X8R8G8B8, mode, &displayMode);

			if (SUCCEEDED(hr)) {
				std::pair<uint32_t, uint32_t> resolution = { displayMode.Width, displayMode.Height };
				uniqueResolutions.insert(resolution);
			}
		}

		AdapterModes.assign(uniqueResolutions.begin(), uniqueResolutions.end());

		std::sort(AdapterModes.begin(), AdapterModes.end(),
			[](const std::pair<uint32_t, uint32_t>& a, const std::pair<uint32_t, uint32_t>& b) {
				if (a.first != b.first) return a.first < b.first;
				return a.second < b.second;
			});

		Logger::TypedLog(CHN_MOD, "Found {} unique resolutions for adapter {}:\n",
			(int)AdapterModes.size(), selectedAdapter);
		for (const auto& res : AdapterModes) {
			Logger::TypedLog(CHN_MOD, "  {}x{}\n", res.first, res.second);
		}

		patchByte((BYTE*)0x775F56, (uint8_t)AdapterModes.size());

		static auto res_loop = safetyhook::create_mid(0x775F40, [](SafetyHookContext& ctx) {
			auto& counter = ctx.eax;
			auto& target_width = ctx.edx;
			auto& target_height = ctx.ecx;
			if (counter >= AdapterModes.size()) {
				ctx.eip = 0x775F5B;
				return;
			}
			UINT current_width = AdapterModes[counter].first;
			UINT current_height = AdapterModes[counter].second;
			if (current_width != target_width) {
				ctx.eip = 0x775F52;
				return;
			}
			if (current_height == target_height) {
				ctx.eip = 0x775F5B;
			}
			else {
				ctx.eip = 0x775F52;
			}
			});

		static auto res_something = safetyhook::create_mid(0x77519E, [](SafetyHookContext& ctx) {
			ctx.ecx = AdapterModes[ctx.eax].first;
			ctx.edx = AdapterModes[ctx.eax].second;
			ctx.eip = 0x7751AC;
			});

		return !AdapterModes.empty();
	}
	std::vector<std::pair<uint32_t, uint32_t>> getAvailableResolutions() {
		return AdapterModes;
	}

	void render_batch_increase() {
		GameConfig::Initialize();
		Logger::Initialize();

		auto value = GameConfig::GetValue("Graphics", "ExtendRenderBatches", 0);

		if (!value) {
			return;
		}
			
		patchDWord((void*)(0xC0BD0B + 1), Instance_pool_buffer_new_size);
		patchDWord((void*)(0xC0BD13 + 1), (uintptr_t)&Instance_pool_buffer);

		PatchRenderBatch<1536, new_size_n>("Main", 0x0277D190, main_render_bigger, g_main_render_batch_xrefs);
		PatchRenderBatch<1024, Alpha_Size_New>("Alpha", 0x02784998, Alpha_Render_batch, g_alpha_render_batch_xrefs);
		PatchRenderBatch<512, Supp_pass_Size_New>("Supp", 0x027899A0, Supp_pass_increased, g_supp_pass_batch_xrefs);
		SafeWrite32(0x52FDAC, new_size_n);
		SafeWrite32(0x52FDE0, new_size_n);
		SafeWrite32(0x52FE1C, Alpha_Size_New);
		SafeWrite32(0x52FE50, Alpha_Size_New);
		SafeWrite32(0x52FE8C, Supp_pass_Size_New);
		SafeWrite32(0x52FEC0, Supp_pass_Size_New);

		patchDWord((void*)0x00DD28F8, (uintptr_t)&cutscene_func_city_render_fade_dist_scale);

		patchNop((void*)0x006C6297, 6);
		static auto update_render = safetyhook::create_mid(0x006C6297, [](SafetyHookContext& ctx) {
			*(float*)0xE996B4 = RenderDistance_old;
			});
		
	}

	float ExtendedRenderDistance = 1.f;
	int UseExtendedRenderBatch = 0;
	void patch_render_batch() {
		static int last_mode = -1;
		if (UseExtendedRenderBatch != last_mode || UseExtendedRenderBatch == 1) {
			if (UseExtendedRenderBatch == 0) {
				*(float*)0xE996B4 = 1.0f;
			}
			else if (UseExtendedRenderBatch == 1) {
				*(float*)0xE996B4 = ExtendedRenderDistance;
			}

			RenderDistance_old = *(float*)0xE996B4;
			last_mode = UseExtendedRenderBatch;
		}
	}
	SafetyHookInline ReleaseTextures;
	void ReleaseTexturesHook() {
		if (shd_cubedefault_tex) {
			shd_cubedefault_tex->Release();
			shd_cubedefault_tex = nullptr;
		}
		ReleaseTextures.unsafe_ccall();
	}

	SafetyHookInline CreateRTs;
	void CreateRTsHook() {
		IDirect3DDevice9* pDevice = *reinterpret_cast<IDirect3DDevice9**>(reinterpret_cast<void*>(DynAddress(0x0252A2D0)));
		if (!shd_cubedefault_tex) DirectX::CreateDDSTextureFromMemory(pDevice, shd_cubedefault, sizeof(shd_cubedefault), &shd_cubedefault_tex);
		CreateRTs.call();
	}

	void Init()
	{

		/*ExtendedRenderDistance = std::clamp((float)GameConfig::GetDoubleValue("Graphics", "ExtendedRenderDistance", 3.f), 1.f, FLT_MAX);
		
		RenderDistance_old = *(float*)0xE996B4;
		if(GameConfig::GetValue("Graphics", "ExtendRenderBatches", 0) != 0)
		OptionsManager::registerOption("Graphics", "ToggleExtendedRenderDistance", (int*)&UseExtendedRenderBatch, 0);*/

		patchJmp((void*)DynAddress(0x00D755F0), &AlphaMaskAvailable);
		// ~ Shadows::Init(); // Don't know if this is needed, this gets called again at the end of init. (Uzis)

		if (GameConfig::GetValue("Debug", "Hook_lua_load_dynamic_script_buffer", 1, "Patches in Juiced Patch custom updates to settings adding MSAA 8x Support and fixing up label names, required for Ultrawide support.")) { // cuz rn this just patches in the resolutions, if init is expanded, please move this check inside
			patchCall((void*)0xD1526E, init_directx9);
		}	

		if(GameConfig::GetValue("Graphics","RemovePixelationShader",0, "Removes Pixelation/censor filter from the game. (Clippy95)"))
		shaders_pc_hook();
		OptionsManager::registerOption("Graphics", "ShaderOverride", &OVERRIDE_SHADER_LOD,1);
		static auto GiveLOD = safetyhook::create_mid(0x00D19D1B,&SETLOD);

		if (GameConfig::GetValue("Graphics", "X360Gamma", 1, "Accurately replicates the XBOX 360 gamma visuals. (Tervel)")) {
			ShaderOptions.X360Gamma = 1;
		}
		if (GameConfig::GetValue("Graphics", "ShadowFiltering", 0, "Applies a smooth filtering shader to shadows (Tervel)")) {
			ShaderOptions.ShadowFilter = 1;
		}
		add_to_entry_test = safetyhook::create_mid(0x00C080EC, &add_to_entry_crashaddr_hook,safetyhook::MidHook::StartDisabled);
		if (GameConfig::GetValue("Debug", "ClippyTextureCrashExceptionHandle", 1)) {
			(void)add_to_entry_test.enable();
		}

		if (GameConfig::GetValue("Gameplay", "IncreaseVehicleFadeDistance", 1, "Increases the distance where vehicles fade if you are not looking at them")) {
			CIncreaseVehicleDespawnDistance.Apply();
		}

#if !JLITE
		if (GameConfig::GetValue("Graphics", "RemoveVignette", 0, "Removes the Vignette screen effect (Creds to Clippy95 and Tervel)"))
		{
			Render3D::RemoveVignette();
		}

		if (GameConfig::GetValue("Graphics", "DisableScreenBlur", 1, "Disables an FXAA like effect that runs on the game"))
		{
			Logger::TypedLog(CHN_MOD, "Disabling Screen Blur...\n");
			patchByte((BYTE*)0x02527297, 0x0);
		}
		else
		{
			Logger::TypedLog(CHN_MOD, "Enabling Screen Blur...\n");
			patchByte((BYTE*)0x02527297, 0x1);

		}

		if (GameConfig::GetValue("Graphics", "VanillaFXPlus", 0, "Overhauls the lighting, keeping the SR2 feel while removing the orangey/yellow screen filter and sharpening up the look of everything.\n(MAY INTERFERE WITH GRAPHICS/TIME OF DAY MODS FOR SR2)"))
		{
			Render3D::VFXPlus();
		}


		if (GameConfig::GetValue("Graphics", "DisableFog", 0, "Disables Distance Fog (Creds to Tervel)")) // Option for the 2 psychopaths that think no fog looks better.
		{
			Render3D::DisableFog();
		}

		if (GameConfig::GetValue("Graphics", "FirstPersonCamera", 0, "Uses a custom first person mode camera on-foot. 2 has viewmodel, 1 has no viewmodel.") == 1)
		{
			Logger::TypedLog(CHN_MOD, "Turning SR2 into an FPS...\n");
			patchDWord((BYTE*)0x00495AC3 + 1, (uint32_t)&FPSCam);
			patchNop((BYTE*)0x0099453D, 2);
			CMPatches_ClassicGTAIdleCam.Apply();
		}
		if (GameConfig::GetValue("Graphics", "FirstPersonCamera", 0, "Uses a custom first person mode camera on-foot. 2 has viewmodel, 1 has no viewmodel.") == 2)
		{
			Logger::TypedLog(CHN_MOD, "Turning SR2 into an FPS with Viewmodel...\n");
			patchDWord((BYTE*)0x00495AC3 + 1, (uint32_t)&FPSCam);
			patchNop((BYTE*)0x0099453D, 2);
			CMPatches_ClassicGTAIdleCam.Apply();
			useFPSCam = 1;
		}
		const uint32_t firstPersonCameraMode = GameConfig::GetValue("Graphics", "FirstPersonCamera", 0, "Uses a custom first person mode camera on-foot. 2 has viewmodel, 1 has no viewmodel.");
		if (GameConfig::GetValue("Graphics", "ClassicGTAIdle", 0, "Makes idle animations snap to where the camera is pointing (like GTA3/VC)") &&
			firstPersonCameraMode != 1 &&
			firstPersonCameraMode != 2)
		{
			Logger::TypedLog(CHN_MOD, "Patching in Classic GTA Idle...\n");
			//patchByte((BYTE*)0x00960C30, 0xC3);
			//patchNop((BYTE*)0x0099453D, 2);
			CMPatches_ClassicGTAIdleCam.Apply();
		}
#endif
		//WriteRelJump(0x00D1B7CE, (UInt32)&LoadShadersHook);
		auto static LoadShadersMidHook = safetyhook::create_mid(0x00D1B7D3, LoadShadersHookMid);

		WriteRelJump(0x00494080, (UInt32)&GetFOV);
		// CLIPPY TODO MAKE THIS A TOGGLEABLE OPTION!!!
		screen_3d_to_2d_midhook = safetyhook::create_mid(0xD22BE8, [](SafetyHookContext& ctx) {
			if (UltrawideFixRatio == 1.0)
				return;
			float x_bound = 1.0f / static_cast<float>(UltrawideFixRatio);
			float& x = *(float*)(ctx.esp + 0x10);
			bool ultrawide = x < -x_bound || x > x_bound;
			if (ultrawide) {
				setBL(ctx, false);
			}
			x *= static_cast<float>(Render3D::UltrawideFixRatio);
			});

		if (GameConfig::GetValue("Gameplay", "FixUltrawideFOV", 1))
		{
			ARfov = 1;
		}

		if (GameConfig::GetDoubleValue("Gameplay", "FOVMultiplier", 1.0, "Field of View, affects cutscenes and gameplay.")) // 1.0 isn't go anywhere.
		{
			FOVMultiplier = GameConfig::GetDoubleValue("Gameplay", "FOVMultiplier", FOVMultiplier, "Field of View, affects cutscenes and gameplay.");
			FOVMultiplier = std::clamp(Render3D::FOVMultiplier, 1.0, 10.0);
			if (FOVMultiplier > 1.0) {
				ARfov = 1;
				Logger::TypedLog(CHN_DEBUG, "Applying FOV Multiplier.\n");
			}
			SafeWrite32(0x00AA5648 + 0x2, (UInt32)&fourbythreeAR); // patch vehicle turning radius, this read from the FOV and the radius gets smaller if FOV is lower than 4/3
			Logger::TypedLog(CHN_DEBUG, "FOV Multiplier: {:f},\n", FOVMultiplier);
		}

		OptionsManager::registerOption(
			"Gameplay",
			"FOVMultiplier",
			Option::Type::Double,
			1.0,
			[]() -> double {
				return NormalizeRange(Render3D::FOVMultiplier, 1.0, 2.0);
			},
			[](double value) {
				double mapped = DenormalizeRange(value, 1.0, 2.0);
				Render3D::FOVMultiplier = std::clamp(mapped, 1.0, 10.0);
				GameConfig::SetDoubleValue("Gameplay", "FOVMultiplier", Render3D::FOVMultiplier);
				AspectRatioFix(false);
			}
		);

		if (GameConfig::GetValue("Graphics", "UHQScreenEffects", 1, "2 = Maps the resolution to your screens resolution\n1 = Same as above but half res (Example: 1920x1080 > 1280x720 resolution scale.)\n0 = Vanilla Resolution\nIncreases resolution of Depth-of-field, skydive blur, reflections and bloom (Tervel)"))
		{
			UHQEffects();
		}

		if (GameConfig::GetValue("Graphics", "RemoveBlackBars", 0, "Removes Cutscene Black Bars (By Tervel)")) // Another Tervel moment
		{
			RemBlackBars();
		}

		if (GameConfig::GetValue("Graphics", "DisableSkyRefl", 0, "Disables the blue sky reflections in building windows (Tervel)"))
		{
			Render3D::DisableSkyRefl();
		}

		if (GameConfig::GetValue("Graphics", "BetterAmbientOcclusion", 1, "Smoothens the Ambient occlusion, crushing the noisy output the base games AO has."))
		{
	 		BetterAO();
	    }

		if (GameConfig::GetValue("Debug", "AltTabFPS", 1, "Uncaps the fps from sub-30 when alt-tabbed.")) // Removes a sleep call in main render loop, this one seems to slow the game to below 25 fps when the game is alt-tabbed.
		{
			Render3D::AltTabFPS();
		}

		if (GameConfig::GetValue("Debug", "UncapFPS", 0, "UncapFPS quite literally Uncaps the Framerate past the 100 fps limit on the base game. !!!! THIS WILL ALSO INCREASE CPU USAGE !!!!")) // Removes a sleep call in main render loop, this one seems to slow the game to below 25 fps when the game is alt-tabbed.
		{  // Uncapping frames can lead to broken doors among other issues not yet noted.
			Render3D::UncapFPS();
		}

		sync_handhake_render();

		//// THIS IS KEPT IN JUICED AS IS, DEPRECATED!!!! SEE AND SHOULD USE sync_handhake_render() -- Clippy95
		//if (GameConfig::GetValue("Debug", "SleepHack", 2) == 1) // LOW patch
		//{
		//	Render3D::PatchLowSleepHack();
		//}

		//if (GameConfig::GetValue("Debug", "SleepHack", 2) == 2)
		//{
		//	Render3D::PatchMediumSleepHack();
		//}
		if (GameConfig::GetValue("Debug", "SleepHook", 0, "This will hook sleep and divide sleep-time by half, NOT RECOMMENDED, no support will be provided for playing with this option toggled on!, EXPECT ISSUES!") == 1)
		{
			Logger::TypedLog(CHN_DLL, "Hooking sleep...\n");
			Render3D::HookSleep();
		}

		if (GameConfig::GetValue("Debug", "FasterLoading", 1, "Raises fps cap in Legal/Loading screens to 60 from 30 to make those screens slightly faster.\nAlso removes a sleep call during world/asset streaming, hopefully reducing stuttering and bringing parity with the X360 version."))
		{
			Render3D::FasterLoading();
		}

		if (GameConfig::GetValue("Graphics", "FixGlares", 1, "Fixes the glares not showing up when you drive up to a chunk as opposed to teleporting or loading into one (Tervel)")) {
			patchByte((void*)0x004AFBA2, 0xEB);
		}

		DitherFilter = GameConfig::GetValue("Graphics", "DitherFiltering", 1, "Adds a PostFX filter to smooth out the fallback dithering with MSAA off/no alpha mask available, but blurs the screen slightly (Tervel)");

		Shadows::Init();

		ReleaseTextures = safetyhook::create_inline(0xD1F960, &ReleaseTexturesHook);
		CreateRTs = safetyhook::create_inline(0xD1E9A0, &CreateRTsHook);

		// - Tervel
		// the game uses a shared shader (sr2_carpaint1) which expects a sampler cube but the default cubemap
		// is a regular sampler 2d tex stored in the always_loaded_effects peg, and it causes certain objects (mission marker 3d models for example)
		// to not have a reflection because the fetching fails due to the mismatch, which for some reason works on X360
		// but on Xenia it fails too despite the shader working fine with real cubemaps, it also works on some old GPUs
		// based on old footage
		static auto FixDefaultCubemap = safetyhook::create_mid(0x00D20B5C, [](SafetyHookContext& ctx) {

			auto IsDefaultCubemap = [](IDirect3DBaseTexture9* Tex) {
				if (!Tex) return false;

				D3DSURFACE_DESC Desc;
				((IDirect3DTexture9*)Tex)->GetLevelDesc(0, &Desc);

				return Desc.Width == 256 && Desc.Height == 1536;
				};

			if (*(UINT*)ctx.eax == 4) { // s4
				IDirect3DBaseTexture9* CurrentTex = (IDirect3DBaseTexture9*)ctx.ecx;
				if (shd_cubedefault_tex && IsDefaultCubemap(CurrentTex)) ctx.ecx = reinterpret_cast<uintptr_t>(shd_cubedefault_tex);
			}
			});

		static auto SunFlareFix = safetyhook::create_mid(0x004D6CBD, [](SafetyHookContext& ctx) {
				float ViewW, ViewH, BaseViewW;

				if (General::IsWidescreen) {
					BaseViewW = 1280.0f;
					ViewH = 720.0f;
					ViewW = 1280.0f;

					float AR = (float)*General::GameResX / (float)*General::GameResY;

					if (AR > Render2D::widescreenvalue) ViewW = ViewH * AR;
				}
				else {
					BaseViewW = ViewW = 640.0f;
					ViewH = 480.0f;
				}

				float& PrjX = *(float*)(ctx.esp + 0x30);
				float& PrjY = *(float*)(ctx.esp + 0x34);

				if (General::IsWidescreen && ViewW > BaseViewW) PrjX *= ViewW / BaseViewW;

				float HViewW = ViewW * 0.5f;
				float HViewH = ViewH * 0.5f;

				float DX = HViewW - PrjX;
				float DY = HViewH - PrjY;

				*(float*)(ctx.esp + 0x24) = DX;
				*(float*)(ctx.esp + 0x28) = DY;
				*(float*)(ctx.esp + 0x14) = (DX * 2.0f) / ViewW;
				*(float*)(ctx.esp + 0x18) = (DY * 2.0f) / ViewH;
			});

	}
}
