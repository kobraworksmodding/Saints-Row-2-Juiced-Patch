// XACT.cpp (uzis, Scanti)
// --------------------
// Created: 22/02/2025

#include "../FileLogger.h"
#include "../Patcher/patch.h"
#include "../GameConfig.h"
#include "../SafeWrite.h"
#include "../RPCHandler.h"
#include "../Game/Game.h"
#include "../General/General.h"
#include <safetyhook.hpp>
#include "Hooking.Patterns.h"
#include "../Game/CrashFixes.h"
#include "..\UtilsGlobal.h"
#include "XACT.h"
namespace XACT
{

	void Cutscene3DAudio(SafetyHookContext& ctx) {
		if (*General::InCutscene) {
			char* Flag = (char*)(ctx.esi + 0x196);
			*Flag &= ~0x0001; // disabling this flag makes directional audio work at the cost of the volume sliders not doing anything (hence the cutscene check)
		}
	}

	void FixAudioHack()
	{
#if !JLITE
		if (GameConfig::GetValue("Debug", "FixAudio", 0)) // solid workaround for making cutscenes play with directional/3D audio (breaks the volume sliders)
														  // keep the name unchanged as we could use this in the future for fixing music or other audio issues?
		{
			static SafetyHookMid XACT3DAudioHack = safetyhook::create_mid(0x0047EA5E, &Cutscene3DAudio);
		}
#endif
	}
	float empty[9]{};
	SAFETYHOOK_NOINLINE float* __fastcall XACT_CCue_GetMatrixCoefficients(DWORD* CCue)
	{
		if (CCue != NULL)
			return (float*)*((DWORD*)CCue + 18);
		else {
			AssertHandler::AssertOnce("XACT_CCue_GetMatrixCoefficients hook", "CCue is null\n",true);
			return empty;
		}
	}

	float* __fastcall XACT_CGlobalSettings_GetCategoryVolumes(DWORD* CGlobalSettings)
	{
		static float* lastValidPointer = nullptr;

		float* currentPointer = (float*)*((DWORD*)CGlobalSettings + 0x21);

		if (currentPointer != nullptr) {
			lastValidPointer = currentPointer;
			return currentPointer;
		}
		else {
			AssertHandler::AssertOnce("XACT_CGlobalSettings_GetCategoryVolumes hook", "CCue the return is NULL, so we return last valid one.\n",true);
			return lastValidPointer;
		}
	}

	void __cdecl setup_audio_hook() {
		((void(__cdecl*)())0x465020)();
		HMODULE hXAct32 = GetModuleHandle(L"xactengine3_2.dll");

		if (hXAct32 != NULL) {
			Logger::TypedLog(CHN_XACT,"xactengine3_2.dll is loaded at: 0x%p\n", hXAct32);
		}
		else {
			Logger::TypedLog(CHN_XACT,"xactengine3_2.dll is not loaded\n");
			return;
		}
		auto pattern = hook::pattern(hXAct32, "8B FF 55 8B EC 51 89 4D ? 8B 45 ? 8B 40 ? 8B E5 5D C3 CC CC CC CC CC CC CC CC CC CC CC CC CC 8B FF 55 8B EC 51 89 4D ? 8B 45 ? 66 8B 40 ? 8B E5 5D C3 CC CC CC CC CC CC CC CC CC CC CC CC 8B FF 55 8B EC 51");
		if (pattern.empty()) {
			Logger::TypedLog(CHN_XACT, "XACT pattern is empty, possible non-syswow version?\n");
			return;
		}

		patchJmp(pattern.get_first<void*>(), XACT_CCue_GetMatrixCoefficients);
		pattern = hook::pattern(hXAct32, "8B FF 55 8B EC 51 89 4D ? 8B 45 ? 8B 80 ? ? ? ? 8B E5 5D C3 CC CC CC CC CC CC CC CC CC CC 8B FF 55 8B EC 51 89 4D ? 8B 45 ? 8B 4D ? 89 88 ? ? ? ? 8B E5 5D C2 ? ? CC CC CC CC CC 8B FF 55 8B EC 51 89 4D ? 8B 45 ? 05");
		patchJmp(pattern.get_first<void*>(), XACT_CGlobalSettings_GetCategoryVolumes);

	}
	void UpdateToNewerXACT()
	{
		int turn = UtilsGlobal::is_wine() ? 2 : 1;
		if (GameConfig::GetValue("Audio", "UseFixedXAudio", 1) >= turn) // Scanti the Goat
		{
			// Forces the game to use a newer version of XACT which in turn fixes all of the audio issues
			// in SR2 aside from 3D Panning.
			GUID xaudio = { 0x4c5e637a, 0x16c7, 0x4de3, 0x9c, 0x46, 0x5e, 0xd2, 0x21, 0x81, 0x96, 0x2d };        // version 2.3
			GUID ixaudio = { 0x8bcf1f58, 0x9fe7, 0x4583, 0x8a, 0xc6, 0xe2, 0xad, 0xc4, 0x65, 0xc8, 0xbb };
			SafeWriteBuf((0x00DD8A08), &xaudio, sizeof(xaudio));
			SafeWriteBuf((0x00DD8A18), &ixaudio, sizeof(ixaudio));

			//GUID xact_engine = { 0x94c1affa, 0x66e7, 0x4961, 0x95, 0x21, 0xcf, 0xde, 0xf3, 0x12, 0x8d, 0x4f };
			//SafeWriteBuf((0xDD89AC), &xact_engine, sizeof(xact_engine));
			Logger::TypedLog(CHN_MOD, "Forcing the use of a fixed XACT version.\n");

			patchCall((void*)0x51FC96, setup_audio_hook);

		}
	}
	uintptr_t X3DAudioInitialize_og;
	HRESULT _cdecl X3DAudioInitialize_hook(int SpeakerMask, float SpeedOfSound, int Instance) {
		printf("called agoksdkfljasfkg;asjfkladsjfkldasjf\n\n\n\n\n\n\n\n\n");
		SpeakerMask = 0x60F;

		return ((HRESULT(__cdecl*)(int, float, int))X3DAudioInitialize_og)(SpeakerMask, SpeedOfSound, Instance);
	}

	double apply_lpf_scalars(uintptr_t audio_channel, float a2)
	{
		double v2; // st6
		double v3; // st7

		v2 = 1.0;
		v3 = 1.0 - (1.0 - *(float*)(audio_channel + 0x30)) * (1.0 - *(float*)(audio_channel + 0x2C)) * (1.0 - a2);
		if (v3 > 0.0 && v3 >= 1.0)
			return (float)v2;
		v2 = 0.0;
		if (v3 <= 0.0)
			return (float)v2;
		else
			return (float)v3;
	}

	enum lpf_state : __int32
	{
		LPF_STATE_INSTANCE = 0x0,
		LPF_STATE_GLOBAL = 0x1,
		NUM_LPF_STATES = 0x2,
	};


	lpf_state xaudio_setup_dry_submix_lpf_path(uintptr_t a1, float a2) {

		lpf_state result;
		static uintptr_t xaudio_setup_dry_submix_lpf_path_addr = 0x482F40;
		__asm {
			mov edx, a1
			push a2
			call xaudio_setup_dry_submix_lpf_path_addr
			add esp,4
			mov result,eax
		}
		return result;
	}

	struct IXAudio2;

	struct IXAudio2_vt
	{
		HRESULT(__stdcall* QueryInterface)(IXAudio2* thisa, IID* riid, void** ppvInterface);
		unsigned int(__stdcall* AddRef)(IXAudio2* thisa);
		unsigned int(__stdcall* Release)(IXAudio2* thisa);
		HRESULT(__stdcall* GetDeviceCount)(IXAudio2* thisa, UINT32* pCount);
		HRESULT(__stdcall* GetDeviceDetails)(IXAudio2* thisa, UINT32 Index, XAUDIO2_DEVICE_DETAILS* pDeviceDetails);
		HRESULT(__stdcall* Initialize)(IXAudio2* thisa, UINT32 Flags, XAUDIO2_PROCESSOR XAudio2Processor);
		HRESULT(__stdcall* RegisterForCallbacks)(IXAudio2* thisa, IXAudio2EngineCallback* Callback);
		void(__stdcall* UnregisterForCallbacks)(IXAudio2* thisa, IXAudio2EngineCallback* Callback);
		HRESULT(__stdcall* CreateSourceVoice)(IXAudio2* thisa, IXAudio2SourceVoice** ppSourceVoice, WAVEFORMATEX* pSourceFormat, UINT32 Flags, float MaxFrequencyRatio, IXAudio2VoiceCallback* pCallback, XAUDIO2_VOICE_SENDS* pSendList, XAUDIO2_EFFECT_CHAIN* pEffectChain);
		HRESULT(__stdcall* CreateSubmixVoice)(IXAudio2* thisa, IXAudio2SubmixVoice** ppSubmixVoice, UINT32 InputChannels, UINT32 InputSampleRate, UINT32 Flags, UINT32 ProcessingStage, XAUDIO2_VOICE_SENDS* pSendList, XAUDIO2_EFFECT_CHAIN* pEffectChain);
		HRESULT(__stdcall* CreateMasteringVoice)(IXAudio2* thisa, IXAudio2MasteringVoice** ppMasteringVoice, UINT32 InputChannels, UINT32 InputSampleRate, UINT32 Flags, UINT32 DeviceIndex, XAUDIO2_EFFECT_CHAIN* pEffectChain);
		HRESULT(__stdcall* StartEngine)(IXAudio2* thisa);
		void(__stdcall* StopEngine)(IXAudio2* thisa);
		HRESULT(__stdcall* CommitChanges)(IXAudio2* thisa, UINT32 OperationSet);
		void(__stdcall* GetPerformanceData)(IXAudio2* thisa, XAUDIO2_PERFORMANCE_DATA* pPerfData);
		void(__stdcall* SetDebugConfiguration)(IXAudio2* thisa, XAUDIO2_DEBUG_CONFIGURATION* pDebugConfiguration, void* pReserved);
	};


	struct IXAudio2
	{
		IXAudio2_vt* vt;
	};


	void ChangeSpeakerCount()
	{
		X3DAudioInitialize_og = *(uintptr_t*)0xDB83AC;
		if (GameConfig::GetValue("Audio", "Fix3DAudio", 1))
		{
			Logger::TypedLog(CHN_AUDIO, "Using Fix3DAudio...\n");
			SafeWrite32(0xDB83AC, (uintptr_t)&X3DAudioInitialize_hook);
		}
		
		static auto huh = safetyhook::create_mid(0x4818EC, [](SafetyHookContext& ctx) {

			IXAudio2* pXAudio2 = *(IXAudio2**)0x00F52E34;
			XAUDIO2_DEVICE_DETAILS details{};
			pXAudio2->vt->GetDeviceDetails(pXAudio2, 0, &details);

			printf("channel mask from it is %p %p\n", details.OutputFormat.dwChannelMask,&details);
			//MessageBoxA(NULL, "FG", "FG", 0);


			});

		//static auto X360_LPF = safetyhook::create_mid(0x482296, [](SafetyHookContext& ctx) {

		//	uintptr_t audio_channel = ctx.esi;
		//	printf("audio_channel %p\n", audio_channel);
		//	if ((*(BYTE*)(audio_channel + 406) & 4) != 0)
		//	{
		//		float final_lpf = apply_lpf_scalars(audio_channel, 0.0);
		//		xaudio_setup_dry_submix_lpf_path(*(DWORD*)(audio_channel + 392), final_lpf);
		//		*(WORD*)(audio_channel + 406) &= ~4u;
		//	}

		//	});

		if (GameConfig::GetValue("Audio", "51Surround", 0) == 1)
		{
			Logger::TypedLog(CHN_AUDIO, "Using 5.1 Surround Sound...\n");
		}
		else {
			Logger::TypedLog(CHN_AUDIO, "Fixing Stereo Audio...\n");
			UINT32 number_of_speakers = 2;
			UINT32 frequency = 48000;

			//SafeWrite8(0x004818E3, number_of_speakers);         // Causes major audio glitches
			SafeWrite8(0x00482B08, number_of_speakers);
			SafeWrite8(0x00482B41, number_of_speakers);
			SafeWrite8(0x00482B96, number_of_speakers);

			SafeWrite32(0x00482B03, frequency);
			SafeWrite32(0x00482B3C, frequency);
			SafeWrite32(0x00482B91, frequency);
		}
	}

	void Init()
	{
		FixAudioHack();
		UpdateToNewerXACT();
		ChangeSpeakerCount();

	}
}