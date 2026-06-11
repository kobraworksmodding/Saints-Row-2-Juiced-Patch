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
	if (GameConfig::GetValue("Debug", "FixAudio", 0, "[EXPERIMENTAL] Fixes stereo output in cutscenes, very experimental in its current state.")) // solid workaround for making cutscenes play with directional/3D audio (breaks the volume sliders)
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
			Logger::TypedLog(CHN_XACT,"xactengine3_2.dll is loaded at: {}\n", fmt::ptr(hXAct32));
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
	if (static_cast<int>(GameConfig::GetValue("Audio", "UseFixedXAudio", 1, "Fixes audio playback across the board, Replaces the ingame XAudio 2.2 to XAudio 2.3. (Creds to Scanti)\nSetting this to 1 enables it for Windows Systems, requires 2 for Linux/Wine and proper XACT install (use winetricks or protontricks)")) >= turn) // Scanti the Goat
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
	void ChangeSpeakerCount()
	{
	if (GameConfig::GetValue("Audio", "51Surround", 0, "Toggles the games in-built 5.1 Surround Support. (Scanti)") == 1)
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

		// removes "FILE_FLAG_NO_BUFFERING" from XACT soundbank createfileA fixes 4kn loading (clippy95)
		Patch<uint32_t>(0x481183 + 1, FILE_FLAG_OVERLAPPED);
	}
}
