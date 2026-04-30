// CrashFixes.cpp (clippy95)
// --------------------
// Created: 20/4/2025
#include <safetyhook.hpp>
#include <wtypes.h>
#include "../GameConfig.h"
#include "../FileLogger.h"
#include <unordered_set>
#include "CrashFixes.h"
#include "../SafeWrite.h"
#include "../Patcher/patch.h"
#include "../General/General.h"
namespace AssertHandler {
	static std::unordered_set<std::string> ignored_asserts;
	static std::mutex assert_mutex;
	static std::unordered_set<std::string> active_asserts;
	static uint8_t CvarFixCrashes;
	inline void AssertOnce(const char* id, const char* message, bool hide_by_default) {
		const bool debugger_attached = IsDebuggerPresent();

		{
			std::lock_guard<std::mutex> lock(assert_mutex);
			if (ignored_asserts.count(id) || active_asserts.count(id))
				return;

			active_asserts.insert(id);
		}

		Logger::TypedLog(CHN_DEBUG, "[Assert ID: {}] {}", id, message);

		if (!debugger_attached && ((CvarFixCrashes >= 200) || (hide_by_default && (CvarFixCrashes != 201))))
		{
			std::lock_guard<std::mutex> lock(assert_mutex);
			active_asserts.erase(id);
			return;
		}

		std::string id_copy(id);
		std::string msg_copy(message);

		std::string full_message =
			"[Assert ID: " + id_copy + "]\n" +
			msg_copy +
			"\n\nYes = Break into debugger"
			"\nNo = Ignore once"
			"\nCancel = Ignore this warning for the session.";

		if (debugger_attached)
		{
			int result = MessageBoxA(
				nullptr,
				full_message.c_str(),
				"Assert Hit",
				MB_ICONWARNING | MB_YESNOCANCEL
			);

			{
				std::lock_guard<std::mutex> lock(assert_mutex);
				active_asserts.erase(id_copy);

				if (result == IDCANCEL)
					ignored_asserts.insert(id_copy);
			}

			if (result == IDYES)
				DebugBreak();

			return;
		}

		std::thread([id_copy, full_message]() {
			int result = MessageBoxA(
				nullptr,
				full_message.c_str(),
				"Error Occurred",
				MB_ICONWARNING | MB_YESNO
			);

			std::lock_guard<std::mutex> lock(assert_mutex);
			active_asserts.erase(id_copy);

			if (result == IDYES)
				ignored_asserts.insert(id_copy);
			}).detach();
	}
}
namespace CrashFixes {
	// sidokuthps had a crash while playing The Ronin - Rest In Peace, this section of code seems to only execute in missions? need to confirm, either ways they jump to an else if v0 is null but not if *v0 is null
// thus it crashes, here I check for that as well - Clippy95
	SAFETYHOOK_NOINLINE void FixAudioLoop_null_crash1(SafetyHookContext& ctx) {
		if (ctx.ebx) {
			if (*(int*)ctx.ebx == NULL) {
				// So Safetyhook converts the jnz short loc_46EE5B to a jne, so we'll jump by 6 bytes to keep on going.
				Logger::TypedLog(CHN_DEBUG, "Mission? audio crash prevented, 0x0046EE64\n");
				ctx.eip += 0x6;
			}
		}
	}
	SAFETYHOOK_NOINLINE void Fix_0x0055B681_crash(SafetyHookContext& ctx) {
		if (!ctx.eax)
			ctx.eip = 0x0055B68D;
	}
	SAFETYHOOK_NOINLINE void Fix_0x007B1D7E_crash_weaponstore(SafetyHookContext& ctx) {
		if (!ctx.edx)
			ctx.eip = 0x007B1E0D;
	}
	SAFETYHOOK_NOINLINE void Fix_0x007B510D_crash(SafetyHookContext& ctx) {
		if (!ctx.edx)
			ctx.eip = 0x007B5135;
	}
	SAFETYHOOK_NOINLINE void Fix_009AEDAD_crash_cs_start_characters_for_shot(SafetyHookContext& ctx) {
		if (ctx.ebx) {
			if (!(*(int*)(ctx.ebx + 0x10A4) + 0x18)) {
				ctx.eip = 0x9AEE79;
				goto crash_cs_start_characters_for_shot_009AEDAD;
			}
			else if(!*(int*)(*(int*)(ctx.ebx + 0x10A4) + 0x18)) {
				ctx.eip = 0x9AEE79;
				goto crash_cs_start_characters_for_shot_009AEDAD;
			}
			else if (!(*(int*)(*(int*)(ctx.ebx + 0x10A4) + 0x18) + 0x44)) {
				ctx.eip = 0x9AEE79;
				goto crash_cs_start_characters_for_shot_009AEDAD;
			}
			else if (!*(int*)(*(int*)(*(int*)(ctx.ebx + 0x10A4) + 0x18) + 0x44)) {
				ctx.eip = 0x9AEE79;
				goto crash_cs_start_characters_for_shot_009AEDAD;
			}
		}
		return;
	crash_cs_start_characters_for_shot_009AEDAD:
		AssertHandler::AssertOnce("cs_start_characters_for_shot_009AEDAD", "Crash prevented due to bad preload breaking cutscene objects, please fix your preload, game might still crash. \n");
	}
	SAFETYHOOK_NOINLINE void Fix_009AEE86_crash_cs_start_characters_for_shot(SafetyHookContext& ctx) {
		if (ctx.ebx) {
			if (!(*(int*)(ctx.ebx + 0x10AC) + 0x18)) {
				ctx.eip = 0x9AEF8C;
				goto crash_cs_start_characters_for_shot_009AEE86;
			}
			else if (!*(int*)(*(int*)(ctx.ebx + 0x10AC) + 0x18)) {
				ctx.eip = 0x9AEF8C;
				goto crash_cs_start_characters_for_shot_009AEE86;
			}
			else if (!(*(int*)(*(int*)(ctx.ebx + 0x10AC) + 0x18) + 0x44)) {
				ctx.eip = 0x9AEF8C;
				goto crash_cs_start_characters_for_shot_009AEE86;
			}
			else if (!*(int*)(*(int*)(*(int*)(ctx.ebx + 0x10AC) + 0x18) + 0x44)) {
				ctx.eip = 0x9AEF8C;
				goto crash_cs_start_characters_for_shot_009AEE86;
			}
		}
		return;
	crash_cs_start_characters_for_shot_009AEE86:
		AssertHandler::AssertOnce("cs_start_characters_for_shot_009AEE86", "Crash prevented due to bad preload breaking cutscene objects, please fix your preload, game might still crash \n");
	}

	void __declspec(naked) crash_0x0071447C_fix() {
		static const DWORD null_jump = 0x714396;
		static const DWORD continue_jump = 0x714483;

		__asm {
			test edx, edx
			jz null_case

			movaps xmm0, xmmword ptr[edx + 110h]
			jmp continue_jump

			null_case :
			jmp null_jump
		}
	}
	uintptr_t load_all_addr = 0x695F00;

	// ignore modded saves and let me crash
	bool SAVE_HURT_ME_PLENTY = false;
	void __cdecl UserWantsItToBlowSaveUp(int Unk, int SelectedOption, int Action) {

		if (Action == 2 && SelectedOption == 1) {
			SAVE_HURT_ME_PLENTY = true;
		}
	}

	void load_all_og(uintptr_t savegame_ptr, BOOL idk)
	{
		__asm mov eax, savegame_ptr
		__asm push idk

		if (!SAVE_HURT_ME_PLENTY) {
			__try {
				__asm call load_all_addr
			}
			__except (EXCEPTION_EXECUTE_HANDLER) {
				
				// aborts save no matter what
				*(bool*)0x2527E80 = true;

				const wchar_t* Options[] = { L"OKAY", L"[format][color:#D41111]Ignore for next load (allow crash to occur)[/format]\n\n" };
				auto vint = General::AddMessageCustomized(L"JUICED", L"Save file could not be loaded due to corruption or this saved game was saved using mods that are no longer installed.", Options, _countof(Options));

				*(void**)(vint + 0x930) = &UserWantsItToBlowSaveUp;

			}
		}
		// SAVE_HURT_ME_PLENTY IS TRUE PATH
		else {
			// I TAKE THE RISKS!!!
			__asm call load_all_addr
		}


		__asm add esp,4

	}
	void load_all_og_hook(BOOL idk)
	{
		uintptr_t saveptr;
		__asm mov saveptr,eax
		load_all_og(saveptr, idk);
	}
	SAFETYHOOK_NOINLINE void Fix_0x00BD8662_buffer_double_release(SafetyHookContext& ctx) {
		// a deregister releases extra divided-bones vbuffers, then
		// releases vertex_buffer normally. Some meshes can possibily alias both.
		const uintptr_t block_bones = ctx.edi;
		if (!block_bones)
			return;

		const uintptr_t block_vbuffer = *(uintptr_t*)(block_bones + 0x10);
		const uintptr_t block_bones_remap = *(uintptr_t*)(block_bones + 0x14);
		const uintptr_t verts = *(uintptr_t*)(ctx.esp + 0x1C);
		if (!block_vbuffer || !verts)
			return;

		const uintptr_t base_vbuffer = *(uintptr_t*)(verts + 0x0C);
		if (block_vbuffer != base_vbuffer)
			return;

		AssertHandler::AssertOnce("Fix_0x00BD8662_buffer_double_release", "vertex buffer double-release 0x00D07244\n", true);

		ctx.edi = block_bones_remap;
		ctx.eip = 0x00D07249;
	}
	void Init() {
		AssertHandler::CvarFixCrashes = GameConfig::GetValue("Debug", "FixCrashes", 2);
		static auto FixAudioLoop_null_crash1_hook = safetyhook::create_mid(0x0046EE64, &FixAudioLoop_null_crash1);
		static auto player_vehicle_C4_crash = safetyhook::create_mid(0x4FA07B, [](SafetyHookContext& ctx) {
			if (ctx.eax == NULL)
				ctx.eip = 0x4FA09E;
			});
		// The one above should work always..
		if (GameConfig::GetValue("Debug", "FixCrashes", 2)) {	
			static auto Fix_0x0055B681_hook = safetyhook::create_mid(0x0055B681, &Fix_0x0055B681_crash);
			static auto Fix_0x007B1D7E_hook = safetyhook::create_mid(0x007B1D7E, &Fix_0x007B1D7E_crash_weaponstore);
			static auto Fix_0x007B510D_hook = safetyhook::create_mid(0x007B510D, &Fix_0x007B510D_crash);
			static auto Fix_0x004B58B2_hook = safetyhook::create_mid(0x004B58B2, [](SafetyHookContext& ctx) {
				if (ctx.ecx == NULL) {
					ctx.eip = 0x4B58E9;
					AssertHandler::AssertOnce("Fix_0x004B58B2_hook", "Crash prevented but still unsafe? (most likely cause is a modded install) ecx\n");
					return;
				}
				uintptr_t ptr = *(uintptr_t*)ctx.ecx;
				uintptr_t ptr1 = *(uintptr_t*)(ptr + 0x20);
				if (ptr1 == NULL) {
					ctx.eip = 0x4B58E9;
					AssertHandler::AssertOnce("Fix_0x004B58B2_hook1", "Crash prevented but still unsafe? (most likely cause is a modded install) ecx\n");
					return;
				}
				});

			static auto Fix_0x0x943711_hook = safetyhook::create_mid(0x943711, [](SafetyHookContext& ctx) {
				if (!ctx.ebp) {
					ctx.eip = 0x9436CA;
					Logger::TypedLog(CHN_DEBUG, "Avoided 0x943711 crash?\n");
					}
				});

			WriteRelJump(0x0071447C, (int)&crash_0x0071447C_fix);

			//static auto fix_0x0071447C_hook = safetyhook::create_mid(0x0071447C, [](SafetyHookContext& ctx) {
			//	if(ctx.edx == NULL)
			//	ctx.eip = 0x714396;
			//	});

		}
		// I kind of want these seperated, idk why but let's say 2+ is confirmed by mods or modded setups?
		if (GameConfig::GetValue("Debug", "FixCrashes", 1) >= 2) {
			// Yes this is a modded crash, don't mess with preloads please. - this was caused by SidokuTHPS's modded preload.
			static auto Fix_0x009AEDAD_hook = safetyhook::create_mid(0x009AED73, &Fix_009AEDAD_crash_cs_start_characters_for_shot);
			// This one hasn't really crashed but it seems to be sameish?
			static auto Fix_0x009AEE86_hook = safetyhook::create_mid(0x009AEE86, &Fix_009AEE86_crash_cs_start_characters_for_shot);
		}

		if (GameConfig::GetValue("Debug", "ExceptionHandler_Saves", 1,"Kick to the main menu when attempting to load a save game that cannot be loaded (clippy95)")) {

			patchCall((void*)0x6A03BF, load_all_og_hook);
			patchCall((void*)0x69600B, load_all_og_hook);

		}

		static auto Fix_0x00BD8662_buffer_double_release_hook = safetyhook::create_mid(0x00D0721E, &Fix_0x00BD8662_buffer_double_release);


		patchNop((void*)0x00695BE7, 15); // these 2 lines of code for freeing the saving mempool only exist on PC for some reason and they cause the game to crash when saving during a mission replay
	}
}