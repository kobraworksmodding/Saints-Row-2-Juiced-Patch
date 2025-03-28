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

namespace Render3D
{
	const char FPSCam[] = "camera_fpss.xtbl";
	bool useFPSCam = 0;
	bool VFXP_fixFog = 0;
	float AOStrength = 1.5;
	bool ARfov = 0;
	bool ARCutscene = 0;
	double FOVMultiplier = 1;
	const double fourbythreeAR = 1.333333373069763;

	void AspectRatioFix(bool update_aspect_ratio) {
		float currentAR = *(float*)0x022FD8EC;
		const float a169 = 1.777777791;
		const double defaultFOV = 1.33333337306976;
		//double currentFOV = *(double*)0x0E5C808;
		double correctFOV = (defaultFOV * ((double)currentAR / (double)a169));
		correctFOV *= Render3D::FOVMultiplier;
		if (currentAR > a169 && Render3D::ARfov && update_aspect_ratio) { // otherwise causes issues for odd ARs like 16:10/5:4 and the common 4:3.
			patchDouble((BYTE*)0x00E5C808, correctFOV);
			patchNop((BYTE*)0x00797181, 6); // Crosshair location that is read from FOV, we'll replace with our own logic below.
			patchFloat((BYTE*)0x00EC2614, correctFOV);
			Logger::TypedLog(CHN_DEBUG, "Aspect Ratio FOV fixed...\n");

			if (Render3D::ARCutscene) {
				static double correctCFOV = 57.2957795131 * ((double)currentAR / (double)a169);
				if (correctCFOV > 125) {
					correctCFOV = 125; // arbiratry number close to 32:9 CFOV, 
					//this will stop most scenes from going upside down in 48:9, we need a beter address for cutscenes similiar to world FOV.
				}
				patchDWord((BYTE*)0x00494DE8 + 2, (uint32_t)&correctCFOV);
				Logger::TypedLog(CHN_DEBUG, "Aspect Ratio Cutscenes (might break above 21:9) hack...\n");
				Render3D::ARCutscene = 0;

			}
		}

			double multipliedFOV = correctFOV * Render3D::FOVMultiplier;
			patchDouble((BYTE*)0x00E5C808, multipliedFOV);
			patchNop((BYTE*)0x00797181, 6);
			patchFloat((BYTE*)0x00EC2614, (float)multipliedFOV);
		
		return;

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
		if (GameConfig::GetValue("Graphics", "X360Gamma", 1)) {
			if (_stricmp(ShaderName, "distortion_tint_desat") == 0) {
				SafeWriteBuf((UInt32)ShaderPointer, X360GammaShader, sizeof(X360GammaShader));
			}
		}

		if (GameConfig::GetValue("Graphics", "ShadowMapFiltering", 0)) {
			if (_stricmp(ShaderName, "shadow_combiner_xxxx") == 0) {
				SafeWriteBuf((UInt32)ShaderPointer, ShadowMapShader, sizeof(ShadowMapShader));
			}
		}
		__asm popad

		__asm {
			add esp, 8
			jmp Continue
		}
	}

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
	CMultiPatch CMPatches_PatchLowSleepHack = {

		[](CMultiPatch& mp) {
			mp.AddPatchNop(0x0052108C,3);
		},

		[](CMultiPatch& mp) {
			mp.AddSafeWriteBuf(0x00521FC0, "\x6A\x00", 2);
		},

		[](CMultiPatch& mp) {
			mp.AddSafeWriteBuf(0x00521FE5, "\x6A\x00", 2);
		},

		[](CMultiPatch& mp) {
			mp.AddSafeWriteBuf(0x005285A2, "\x6A\x00", 2);
		},
	};
	void PatchLowSleepHack()
	{
		// Woohoo, this is a dirty patch, but we'll include it for people who want it and CAN actually run it.
		// This will destroy older and weaker pcs, but we'll make sure to let the people who are on that, know that.

		// This is the lower spec version of the patch, the things that will cause the LEAST cpu usage.

		Logger::TypedLog(CHN_DLL, "Removing a Very Safe Amount of Sleep Calls...\n");
		//patchNop((BYTE*)0x0052108C, 3); // patch win main sleep call
		/*patchBytesM((BYTE*)0x00521FC0, (BYTE*)"\x6A\x00", 2); // wait call in a threaded function, i think
		patchBytesM((BYTE*)0x00521FE5, (BYTE*)"\x6A\x00", 2); // same with this one
		patchBytesM((BYTE*)0x005285A2, (BYTE*)"\x6A\x00", 2); // this ones a doozy, this is some weird threaded exchange function, for each something, sleep. */
		CMPatches_PatchLowSleepHack.Apply();
	}
	CPatch CPatches_MediumSleepHack = CPatch::SafeWriteBuf(0x0052847C, "\x6A\x00", 2);
	void PatchMediumSleepHack()
	{
		Logger::TypedLog(CHN_DLL, "Removing a Safe Amount of Sleep Calls...\n");
		/*patchBytesM((BYTE*)0x00521FC0, (BYTE*)"\x6A\x00", 2); // wait call in a threaded function, i think
		patchBytesM((BYTE*)0x00521FE5, (BYTE*)"\x6A\x00", 2); // same with this one
		patchBytesM((BYTE*)0x005285A2, (BYTE*)"\x6A\x00", 2); // this ones a doozy, this is some weird threaded exchange function, for each something, sleep. 
		patchBytesM((BYTE*)0x0052847C, (BYTE*)"\x6A\x00", 2); //make the shadow pool less sleepy*/
			
		CMPatches_PatchLowSleepHack.Apply();

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
			OriginalSleep(dwMilliseconds / 1.5);
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

		void* old_proc;

		if (PatchIat(main_handle, (char*)"Kernel32.dll", (char*)"Sleep", OriginalSleep, NULL) == S_OK) {
			IsSleepHooked = false;
		}
	}

	void FPSCamHack() {
		BYTE PlayerStatus = *(BYTE*)0x00E9A5BC; // Status Byte for the Players Actions.
		FLOAT* WalkCamZoom = (FLOAT*)0x025F6334;
		BYTE ActorFade = *(BYTE*)0x00E8825F;

		if (*(FLOAT*)WalkCamZoom > -0.5) {
			*(FLOAT*)0x025F6334 = -0.6; // Force camera zoom to chest/in front of player.
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

	void FasterLoadingScreens()
	{
		Logger::TypedLog(CHN_MOD, "Making loading screens slightly faster.\n");
		patchBytesM((BYTE*)0x0068C714, (BYTE*)"\x6A\x0F", 2); // this is a sleep call for first load/legal disclaimers, its set to 30 by default, halfing increases fps to 60 and makes loading faster.
	}

	CMultiPatch CMPatches_VFXPlus = {

		[](CMultiPatch& mp) {
			mp.AddPatchNop(0x00773797,5); // prevent the game from disabling/enabling the tint.
		},

		[](CMultiPatch& mp) {
			mp.AddSafeWriteBuf(0x0051A952, "\xD9\x05\x7F\x2C\x7B\x02", 6);
		},

		[](CMultiPatch& mp) {
			mp.AddSafeWriteBuf(0x0051A997, "\xD9\x05\x83\x2C\x7B\x02", 6);
		},

		[](CMultiPatch& mp) {
			mp.AddSafeWriteBuf(0x0051A980, "\xD9\x05\x87\x2C\x7B\x02", 6);
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
		if (GameConfig::GetValue("Graphics", "X360Gamma", 1)) {
			mp.AddSafeWrite<float>(0x027B2C7F,1.32f);
			mp.AddSafeWrite<float>(0x027B2C83, 0.8f);
			mp.AddSafeWrite<float>(0x027B2C87, 1.58f);
			}
		else {
			mp.AddSafeWrite<float>(0x027B2C7F,1.26f);
			mp.AddSafeWrite<float>(0x027B2C83, 0.8f);
			mp.AddSafeWrite<float>(0x027B2C87, 1.62f);
}
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
		if (GameConfig::GetValue("Graphics", "UHQScreenEffects", 2) == 0) {
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

		if (GameConfig::GetValue("Graphics", "X360Gamma", 1)) {
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

		if (GameConfig::GetValue("Graphics", "UHQScreenEffects", 2) == 0) {
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

	bool halfFxQuality = false;

	void ResizeEffects() {
		int CurrentX = *(int*)0x22FD84C;
		int CurrentY = *(int*)0x22FD850;
		if (halfFxQuality == true) {
			CurrentX = CurrentX / 2;
			CurrentY = CurrentY / 2;
		}
		SetDOFRes(CurrentX, CurrentY);
		SetBloomRes(CurrentX, CurrentY, (float)CurrentX, (float)CurrentY);
		SetWaterReflRes(CurrentX, CurrentY);
		SetVehReflRes(CurrentX);
		//SetAORes(CurrentX, CurrentY);
		SetGraphics();
	}

	void UHQEffects() {
		WriteRelJump(0x00515C9A, (UInt32)&StrengthWorkaround);
		SafeWrite32(0x005169C8 + 2, (UInt32)&BloomResX);
		SafeWrite32(0x005169BB + 2, (UInt32)&BloomResY);
		patchCall((void*)0x007740D9, (void*)ResizeEffects);
		patchCall((void*)0x007743CE, (void*)ResizeEffects);
		if (GameConfig::GetValue("Graphics", "UHQScreenEffects", 2) == 1) {
			Logger::TypedLog(CHN_MOD, "Patching UHQScreenEffects at half quality...\n");
			halfFxQuality = true;
		}
		else {
			Logger::TypedLog(CHN_MOD, "Patching UHQScreenEffects at full quality...\n");
		}

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
	__declspec(naked) int __cdecl add_to_entry_func(void* be, void* pe) {
		__asm {

			mov edi, dword ptr[esp + 4]  // be
			mov esi, dword ptr[esp + 8]  // pe

			call dword ptr[add_to_entry_func_addr]

			ret
		}
	}
	struct peg_entry
	{
		unsigned __int8* data;
		unsigned __int16 width;
		unsigned __int16 height;
		unsigned __int16 bm_fmt;
		unsigned __int16 pal_fmt;
		unsigned int mip_filter_value;
		unsigned __int16 num_frames;
		unsigned __int16 flags;
		char* filename;
		unsigned __int16 pal_size;
		unsigned __int8 fps;
		unsigned __int8 mip_levels;
		unsigned int frame_size;
		peg_entry* next;
		peg_entry* prev;
		unsigned int cache[2];
	};

	struct bitmap_entry
	{
		BYTE gap0[4];
		peg_entry* current_peg_entry;
		WORD word8;
		WORD wordA;
	};

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
		// gtfo out of function
		if (!IsMemoryReadable(width)) {
			Logger::TypedLog("the other add_to_entry hook", "!!!Invalid result->width: %p\n", width);
			ctx.eip = 0x00C08101;
		}
		

	}
 int __stdcall SafeAddToEntry(bitmap_entry* be, peg_entry* pe) {
	 /*if (crash) {
		 Logger::TypedLog(CHN_DEBUG, "CRASH TEST: Forcing invalid memory access\n");
		 be = reinterpret_cast<void*>(0xDEADBEEF);
		 crash = false;
	 }*/
		if (!IsMemoryReadable(be)) {
			for(int i = 0; i<11; i++)
			Logger::TypedLog("add_to_entry hook", "!!!Invalid be pointer: %p\n", be);
			return 0;
		}
		

		peg_entry* result = be->current_peg_entry;
		if (!IsMemoryReadable(&result)) {
			for (int i = 0; i < 11; i++)
			Logger::TypedLog("add_to_entry hook","!!!Invalid memory at be->current_peg_entry: %p\n", reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(be) + 4));
			return 0;
		}
		if (result) {
			//printf("width 0x%X \n", &result->width);
			if (pe && !IsMemoryReadable(&result->width)) {
				for (int i = 0; i < 11; i++)
					Logger::TypedLog("add_to_entry hook", "!!!Invalid pe pointer: %p\n", pe);
				return 0;
			}
		}
		if (pe && !IsMemoryReadable(pe)) {
			for (int i = 0; i < 11; i++)
			Logger::TypedLog("add_to_entry hook", "!!!Invalid pe pointer: %p\n", pe);
			return 0;
		}
			return add_to_entry_func(be, pe);

	}


	__declspec(naked) void PatchAddToEntryPoint() {
		__asm {
			push ebx
			push ecx
			push edx

			push esi    // pe
			push edi    // be
			call SafeAddToEntry

			pop edx
			pop ecx
			pop ebx

			ret
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
	// This whole thing might have a performance hit.
	CMultiPatch CMPatches_ClippysIdiotTextureCrashExceptionHandle = {

		[](CMultiPatch& mp) {
			mp.AddWriteRelCall(0x00C08493,(uintptr_t)&PatchAddToEntryPoint);
		},

		[](CMultiPatch& mp) {
			mp.AddWriteRelCall(0x00C0900D,(uintptr_t)&PatchAddToEntryPoint);
		},
	};

	void Init()
	{
		add_to_entry_test = safetyhook::create_mid(0x00C080EC, &add_to_entry_crashaddr_hook);
#if !JLITE
		if (GameConfig::GetValue("Graphics", "RemoveVignette", 0))
		{
			Render3D::RemoveVignette();
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

		if (GameConfig::GetValue("Graphics", "VanillaFXPlus", 0))
		{
			Render3D::VFXPlus();
		}


		if (GameConfig::GetValue("Graphics", "DisableFog", 0)) // Option for the 2 psychopaths that think no fog looks better.
		{
			Render3D::DisableFog();
		}

		if (GameConfig::GetValue("Graphics", "FirstPersonCamera", 0) == 1)
		{
			Logger::TypedLog(CHN_MOD, "Turning SR2 into an FPS...\n");
			patchDWord((BYTE*)0x00495AC3 + 1, (uint32_t)&FPSCam);
			patchNop((BYTE*)0x0099453D, 2);
			CMPatches_ClassicGTAIdleCam.Apply();
		}
		if (GameConfig::GetValue("Graphics", "FirstPersonCamera", 0) == 2)
		{
			Logger::TypedLog(CHN_MOD, "Turning SR2 into an FPS with Viewmodel...\n");
			patchDWord((BYTE*)0x00495AC3 + 1, (uint32_t)&FPSCam);
			patchNop((BYTE*)0x0099453D, 2);
			CMPatches_ClassicGTAIdleCam.Apply();
			useFPSCam = 1;
		}
		if (GameConfig::GetValue("Graphics", "ClassicGTAIdle", 0) &&
			!GameConfig::GetValue("Graphics", "FirstPersonCamera", 0) == 1
			|| !GameConfig::GetValue("Graphics", "FirstPersonCamera", 0) == 2)
		{
			Logger::TypedLog(CHN_MOD, "Patching in Classic GTA Idle...\n");
			//patchByte((BYTE*)0x00960C30, 0xC3);
			//patchNop((BYTE*)0x0099453D, 2);
			CMPatches_ClassicGTAIdleCam.Apply();
		}
#endif
		WriteRelJump(0x00D1B7CE, (UInt32)&LoadShadersHook);

		if (GameConfig::GetValue("Gameplay", "FixUltrawideFOV", 1))
		{
			ARfov = 1;
		}

		if (GameConfig::GetValue("Gameplay", "FixUltrawideCutsceneFOV", 1))
		{
			ARCutscene = 1;
		}

		if (GameConfig::GetDoubleValue("Gameplay", "FOVMultiplier", 1.0)) // 1.0 isn't go anywhere.
		{
			FOVMultiplier = GameConfig::GetDoubleValue("Gameplay", "FOVMultiplier", FOVMultiplier);
			if (FOVMultiplier > 1.0) {
				ARfov = 1;
				Logger::TypedLog(CHN_DEBUG, "Applying FOV Multiplier.\n");
			}
			SafeWrite32(0x00AA5648 + 0x2, (UInt32)&fourbythreeAR); // patch vehicle turning radius, this read from the FOV and the radius gets smaller if FOV is lower than 4/3
			Logger::TypedLog(CHN_DEBUG, "FOV Multiplier: %f,\n", FOVMultiplier);
		}

		if (GameConfig::GetValue("Graphics", "UHQScreenEffects", 2) > 0 && GameConfig::GetValue("Graphics", "UHQScreenEffects", 2) < 3)
		{
			UHQEffects();
		}

		if (GameConfig::GetValue("Graphics", "RemoveBlackBars", 0)) // Another Tervel moment
		{
			RemBlackBars();
		}

		if (GameConfig::GetValue("Graphics", "DisableSkyRefl", 0))
		{
			Render3D::DisableSkyRefl();
		}

		// Beefs up Tree Shadows considerably
		if (GameConfig::GetValue("Graphics", "UHQTreeShadows", 0))
		{
			Logger::TypedLog(CHN_SHADER, "Juicing up Tree Shadow Resolutions...\n");
			Render3D::PatchHQTreeShadows();
		}

		if (GameConfig::GetValue("Graphics", "BetterAmbientOcclusion", 1))
		{
	 		BetterAO();
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
	}
}
