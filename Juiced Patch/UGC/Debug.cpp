// Debug.cpp
// --------------------
// Created: 25/02/2025

#include "../FileLogger.h"
#include "../Patcher/patch.h"
#include "../GameConfig.h"
#include "../SafeWrite.h"
#include "../loose files.h"
#include "Debug.h"
#include "../Render/Render2D.h"
#include <safetyhook.hpp>

import OptionsManager;

namespace Debug
{
	bool addBindToggles = 0;
	bool LoadLastSave = 0;
	bool fixFrametime = 0;

	CMultiPatch CMPatches_DisableCheatFlag = {

	[](CMultiPatch& mp) {
		mp.AddPatchNop(0x00687e12,6);
	},

	[](CMultiPatch& mp) {
		mp.AddPatchNop(0x00687e18,6);
	},
	};

	void PatchDatafiles() {
		bool LooseCache = CreateCache((char*)"loose.txt");
		bool DLCCache = ScanDLCDir("DLC");
		if (LooseCache || DLCCache)
		{
			CacheConflicts();
			patchJmp((void*)0x0051DAC0, (void*)hook_loose_files);						// Allow the loading of loose files
			patchCall((void*)0x00BFD8F5, (void*)hook_raw_get_file_info_by_name);		// Add optional search in the ./loose directory
			static SafetyHookMid InsertHashes = safetyhook::create_mid(0x00C0A8E0, &InsertFileHashes);
		}
		else
			Logger::TypedLog(CHN_DLL, "Create loose file cache failed for both loose.txt & DLC.\n");
	}
	constexpr auto MEGABYTE = 1048576.0;
	int UseDynamicRenderDistance = false;
	bool DynamicRenderSetThisFrame = false;
	float MAX_RENDER = 5.f;
	float TRANSITION_SPEED = 2.f;
	int SIZE_MIN = 200;
	int MAX = 1400;
	void DynamicRenderDistance() {
		float* render_distance = (float*)0x00E996B4;
		float* frametime = (float*)0xE8437C;

		if (UseDynamicRenderDistance) {
			int size = **(int**)(0x52431A + 1);
			// Only make changes when size is at least 1
			if (size < 1) {
				return;
			}
			const float MIN_RENDER = 1.0f;

			int clamped_size = size;
			if (clamped_size < SIZE_MIN) clamped_size = SIZE_MIN;
			if (clamped_size > MAX) clamped_size = MAX;

			float normalized = 1.0f - (float)(clamped_size - SIZE_MIN) / (float)(MAX - SIZE_MIN);
			float target_render = MIN_RENDER + normalized * (MAX_RENDER - MIN_RENDER);
			float current_render = *render_distance;
			float delta = (*frametime) * TRANSITION_SPEED;

			if (target_render > current_render) {
				// Increasing render distance
				*render_distance = current_render + fminf(delta, target_render - current_render);
			}
			else {
				// Decreasing render distance
				*render_distance = current_render - fminf(delta, current_render - target_render);
			}
			DynamicRenderSetThisFrame = true;
		}
		else {
			if (DynamicRenderSetThisFrame) {
				*render_distance = 1.0f;
				DynamicRenderSetThisFrame = false;
			}
		}
	}
	void PrintMemoryUsage(int y) {
		char buffer[200];
		MEMORYSTATUSEX status;
		status.dwLength = sizeof(MEMORYSTATUSEX);

		if (GlobalMemoryStatusEx(&status)) {
			double v_total = status.ullTotalVirtual / MEGABYTE;
			double v_free = status.ullAvailVirtual / MEGABYTE;
			double p_total = status.ullTotalPhys / MEGABYTE;
			double p_free = status.ullAvailPhys / MEGABYTE;

			snprintf(buffer, sizeof(buffer), "Physical Memory: %5.2f / %5.2f (%5.2f free)",
				p_total - p_free, p_total, p_free);
			Render2D::ChangeTextColor(255, 255, 255, 255);

			__asm pushad
			Render2D::InGamePrint(buffer, y, Render2D::processtextwidth(0), 6);
			__asm popad

			snprintf(buffer, sizeof(buffer), "Virtual Memory: %5.2f / %5.2f (%5.2f free)",
				v_total - v_free, v_total, v_free);
			Render2D::ChangeTextColor(255, 255, 255, 255);

			__asm pushad
			Render2D::InGamePrint(buffer, y + 20, Render2D::processtextwidth(0), 6);
			__asm popad

			snprintf(buffer, sizeof(buffer), "render distance %2.4f render size %d", *(float*)0x00E996B4,**(int**)(0x52431A + 1));
			__asm pushad
			Render2D::InGamePrint(buffer, y + 40, Render2D::processtextwidth(0), 6);
			__asm popad

			// this is some stupid shit, someone make it better (clippy95)
			if (*(bool*)0x02527CF7 && *(int*)(0x02527D10)) {
				debug_cutscene_clock ct = *(debug_cutscene_clock*)0x027E66FC;
				debug_cutscene_info* const cs_info = (debug_cutscene_info*)(*(uintptr_t*)*(uintptr_t*)0x02527D10);
				// Get the first pointer (0x07B30000)
				uintptr_t ptr1 = *(uintptr_t*)0x02527D10;

				// Get the second pointer (0x07B9A230) from ptr1 + 0x1540
				uintptr_t ptr2 = NULL;
				ptr2 = *(uintptr_t*)(ptr1 + 0x1540);
				debug_cutscene_shot* cur_shot = NULL;
				// Finally, get the cutscene shot from ptr2
				if (ptr2)
					cur_shot = (debug_cutscene_shot*)ptr2;

				// Print cutscene timer info with newlines for each property
				int yOffset = y + 60; // Start below the previous text
				Render2D::ChangeTextColor(255, 255, 0, 255); // Yellow for cutscene info

				snprintf(buffer, sizeof(buffer), "CUTSCENE: elapsed: %.2fs / total: %.2fs",
					ct.elapsed_seconds, ct.total_seconds);
				__asm pushad
				Render2D::InGamePrint(buffer, yOffset, Render2D::processtextwidth(0), 6);
				__asm popad
				yOffset += 20;


				int current_frame_cs = (int)(ct.elapsed_seconds * 30.f);
				int total_frames_cs = (int)(ct.total_seconds * 30.f);

				snprintf(buffer, sizeof(buffer), "%u/%u frames",
					current_frame_cs, total_frames_cs);
				__asm pushad
				Render2D::InGamePrint(buffer, yOffset, Render2D::processtextwidth(0), 6);
				__asm popad
				yOffset += 20;


				snprintf(buffer, sizeof(buffer), "Shot: #%d elapsed: %.2fs / total: %.2fs",
					ct.current_shot_index, ct.current_shot_elapsed_seconds, ct.current_shot_total_seconds);
				__asm pushad
				Render2D::InGamePrint(buffer, yOffset, Render2D::processtextwidth(0), 6);
				__asm popad
				yOffset += 20;

				snprintf(buffer, sizeof(buffer), "Time: frame %.3fs intershot %.3fs",
					ct.frame_time_seconds, ct.intershot_frame_time_seconds);
				__asm pushad
				Render2D::InGamePrint(buffer, yOffset, Render2D::processtextwidth(0), 6);
				__asm popad
				yOffset += 20;

				snprintf(buffer, sizeof(buffer), "Ticks: start %u real_start %u last %u",
					ct.real_start_tick, ct.real_start_time, ct.real_last_time);
				__asm pushad
				Render2D::InGamePrint(buffer, yOffset, Render2D::processtextwidth(0), 6);
				__asm popad
				yOffset += 20;

				snprintf(buffer, sizeof(buffer), "Pause: count %d start %u total %u last %u",
					ct.paused_count, ct.pause_start_time, ct.pause_total_time, ct.last_frame_time);
				__asm pushad
				Render2D::InGamePrint(buffer, yOffset, Render2D::processtextwidth(0), 6);
				__asm popad
				yOffset += 20;

				// Print cutscene info structure information
				Render2D::ChangeTextColor(0, 255, 255, 255); // Cyan for cutscene info

				const char* name = cs_info->name ? cs_info->name : "NULL";
				snprintf(buffer, sizeof(buffer), "CS Name: %s", name);
				__asm pushad
				Render2D::InGamePrint(buffer, yOffset, Render2D::processtextwidth(0), 6);
				__asm popad
				yOffset += 20;

				// Check if display_name is NULL before trying to print it
				if (cs_info->display_name) {
					// Convert wchar_t to char if needed (simplified approach)
					char displayName[100] = "";
					for (int i = 0; i < 99 && cs_info->display_name[i]; i++) {
						displayName[i] = (char)cs_info->display_name[i]; // Simple conversion, may not work for all characters
					}

					snprintf(buffer, sizeof(buffer), "Display Name: %s", displayName);
					__asm pushad
					Render2D::InGamePrint(buffer, yOffset, Render2D::processtextwidth(0), 6);
					__asm popad
					yOffset += 20;
				}
				else {
					snprintf(buffer, sizeof(buffer), "Display Name: NULL");
					__asm pushad
					Render2D::InGamePrint(buffer, yOffset, Render2D::processtextwidth(0), 6);
					__asm popad
					yOffset += 20;
				}

				snprintf(buffer, sizeof(buffer), "Viewed: %s", cs_info->viewed ? "Yes" : "No");
				__asm pushad
				Render2D::InGamePrint(buffer, yOffset, Render2D::processtextwidth(0), 6);
				__asm popad
				yOffset += 20;

				// Print cutscene shot structure information
				if (cur_shot) {
					Render2D::ChangeTextColor(0, 255, 0, 255); // Green for cutscene shot

					const char* shotName = cur_shot->shot_name ? cur_shot->shot_name : "NULL";
					snprintf(buffer, sizeof(buffer), "Shot Name: %s", shotName);
					__asm pushad
					Render2D::InGamePrint(buffer, yOffset, Render2D::processtextwidth(0), 6);
					__asm popad
					yOffset += 20;

					snprintf(buffer, sizeof(buffer), "Shot Index: %d", cur_shot->shot_index);
					__asm pushad
					Render2D::InGamePrint(buffer, yOffset, Render2D::processtextwidth(0), 6);
					__asm popad
					yOffset += 20;

					snprintf(buffer, sizeof(buffer), "Start Time: %.2fs", cur_shot->start_time_seconds);
					__asm pushad
					Render2D::InGamePrint(buffer, yOffset, Render2D::processtextwidth(0), 6);
					__asm popad
					yOffset += 20;
					int current_frame = (int)(ct.current_shot_elapsed_seconds * 30.f);
					snprintf(buffer, sizeof(buffer), "%u/%u frames", current_frame, cur_shot->duration_frames);
					__asm pushad
					Render2D::InGamePrint(buffer, yOffset, Render2D::processtextwidth(0), 6);
					__asm popad
					yOffset += 20;

					snprintf(buffer, sizeof(buffer), "Fade Out: %.2fs from end, %.2fs duration",
						cur_shot->fade_out_time_from_end_seconds, cur_shot->fade_out_duration_seconds);
					__asm pushad
					Render2D::InGamePrint(buffer, yOffset, Render2D::processtextwidth(0), 6);
					__asm popad
					yOffset += 20;

					snprintf(buffer, sizeof(buffer), "Fade Letterbox: %s",
						cur_shot->fade_out_letterbox ? "Yes" : "No");
					__asm pushad
					Render2D::InGamePrint(buffer, yOffset, Render2D::processtextwidth(0), 6);
					__asm popad
				}
			}

		}
	}

	struct mempool_to_ini_s {
		const char* internal_name;
		const char* ini_name;
		bool updated_this_session = false;
	};

	mempool_to_ini_s mempool_to_ini[] = { {"perm mesh cpu","perm_mesh_cpu"},{ "audio - wavebank","audio_wavebank"},{"animation","animation"} };

	int __cdecl printf_mempool_hook(const char* const Format, ...) {
		static bool ignore_session = false;

		va_list args;
		va_list args_copy;
		int result = 0;

		if (ignore_session) {
			va_start(args, Format);
			result = vprintf(Format, args);
			va_end(args);
			return result;
		}

		va_start(args, Format);
		va_copy(args_copy, args);
		int size = vsnprintf(NULL, 0, Format, args) + 1;

		if (size > 0) {
			char* buffer = (char*)malloc(size);
			if (buffer) {
				vsnprintf(buffer, size, Format, args_copy);
				const char* extra_prefix = "[MEMPOOL ERROR/WARNING]\n";
				const char* extra_suffix = "Most likely caused by an installed mod, please attempt to increase original mempool size, expect issues otherwise.\n\nIgnore future errors this session?";

				std::string auto_update_string = "\n";

				for (int i = 0; i < std::size(mempool_to_ini); i++) {
					if (std::string_view(buffer).contains(mempool_to_ini[i].internal_name) && !mempool_to_ini[i].updated_this_session) {
						auto old_value = GameConfig::GetValue("Mempool", mempool_to_ini[i].ini_name, 0);
						if (old_value > 0) {
							mempool_to_ini[i].updated_this_session = true;
							const char* internal_name = mempool_to_ini[i].internal_name;
							auto_update_string = "\n\nAs Juiced already has a way to increase this mempool it automatically the updated size of ";
							auto_update_string += internal_name;
							auto_update_string += " with the .ini name of ";
							auto_update_string += mempool_to_ini[i].ini_name;


							auto new_value = (uint32_t)(old_value * 1.5);

							auto_update_string += " from " + std::to_string(old_value) + " to " + std::to_string(new_value);

							GameConfig::SetValue("Mempool", mempool_to_ini[i].ini_name, new_value);

							auto_update_string += "\nThis will require a restart to apply.";
							break;
						}
						else {
							break;
						}
					}
				}

				int extended_size = strlen(extra_prefix) + strlen(buffer) + strlen(extra_suffix) + auto_update_string.length() + 1;
				char* extended_buffer = (char*)malloc(extended_size);

				if (extended_buffer) {
					snprintf(extended_buffer, extended_size, "%s%s%s%s",
						extra_prefix, buffer, extra_suffix, auto_update_string.c_str());

					int response = MessageBoxA(NULL, extended_buffer, "Juiced Patch", MB_YESNO | MB_ICONERROR);

					if (response == IDYES) {
						ignore_session = true;
					}

					free(extended_buffer);
				}
				else {
					MessageBoxA(NULL, buffer, "Juiced Patch (restored error message)", MB_OK | MB_ICONERROR);
				}

	result = printf("%s", buffer);
				free(buffer);
			}
		}

		va_end(args_copy);
		va_end(args);
		return result;
	}
	int sprintf_messagebox_mesh(char* buffer, const char* message, const char* mesh)
	{
		auto result = sprintf_s(buffer, 1024,"The game failed trying to preload %s", mesh);
		MessageBoxA(0, buffer, "Juiced Patch (restored error message)", MB_OK | MB_ICONERROR);
		return result;
	}

	//SafetyHookInline static_mesh_loadD;
	//void* __fastcall static_mesh_load(char* filename_in, void* thisa, char permanent, char a4)
	//{
	//	static char meshbuffer[256]{};
	//	auto result = static_mesh_loadD.unsafe_fastcall<void*>(filename_in, thisa, permanent, a4);
	//	if (!result && filename_in) {
	//		sprintf_s(meshbuffer, sizeof(meshbuffer), "The game failed trying to load %s", filename_in);
	//		MessageBoxA(0, meshbuffer, "Juiced Patch", MB_OK | MB_ICONERROR);
	//	}
	//	return result;
	//}

	void Init() {
		//InjectHook(0x6E4F87, sprintf_messagebox_mesh);
		//InjectHook(0x6E4FC7, sprintf_messagebox_mesh);

		//static_mesh_loadD = safetyhook::create_inline(0x6E5760, static_mesh_load);

		OptionsManager::registerOption("Graphics", "DynamicRenderDistance", (int*)&UseDynamicRenderDistance, 0);
#if !JLITE
	if (GameConfig::GetValue("Gameplay", "SkipIntros", 0, "Skips Startup Logos and Disclaimers (Creds to Tervel)")) // can't stop Tervel won't stop Tervel
		{
			Logger::TypedLog(CHN_DLL, "Skipping intros & legal disclaimers.\n");
			patchNop((BYTE*)(0x005207B4), 6); // prevent intros from triggering
			patchBytesM((BYTE*)0x0068C740, (BYTE*)"\x96\xC5\x68\x00", 4); // replace case 0 with case 4 to skip legal disclaimers
		}

	if (GameConfig::GetValue("Gameplay", "DisableCheatFlag", 0, "Disables Cheat Flagging (Creds to Clippy95)"))
		{
			//patchNop((BYTE*)0x00687e12, 6);
			//patchNop((BYTE*)0x00687e18, 6);
			CMPatches_DisableCheatFlag.Apply();
		}


	if (GameConfig::GetValue("Debug", "AddBindToggles", 0, "Adds Custom Option Binds to the F Keys [F1-F9]"))
		{
			Logger::TypedLog(CHN_DEBUG, "Adding Custom Key Toggles...\n");
			addBindToggles = 1;
			patchNop((BYTE*)0x0051FEB0, 7); // nop to prevent the game from locking the camera roll in slew
			patchBytesM((BYTE*)0x00C01B52, (BYTE*)"\xD9\x1D\xF8\x2C\x7B\x02", 6); // slew roll patch, makes the game write to a random unallocated float instead to prevent issues
			patchBytesM((BYTE*)0x00C01AC8, (BYTE*)"\xDC\x64\x24\x20", 4); // invert Y axis in slew 
		}

	if (GameConfig::GetValue("Gameplay", "LoadLastSave", 0, "Automatically Loads your last saved game upon main menu boot up. (Creds to Tervel)")) // great for testing stuff faster and also for an optional feature in gen
		{
			LoadLastSave = 1;
			Logger::TypedLog(CHN_DEBUG, "Skipping main menu...\n");
		}

#endif 
	if (GameConfig::GetValue("Debug", "FixFrametime", 1, "Fixes Cutscene Object Syncage and Doors teleporting on high fps."))
		{
			Logger::TypedLog(CHN_DEBUG, "Fixing Frametime issues...\n");
			fixFrametime = 1;
		}

	if (GameConfig::GetValue("Mempool", "mempool_print_error", 1, "Opens a popup when mempool alloc fails (Clippy95)") != 0) {
			patchCall((void*)0xBFCA9E, printf_mempool_hook);
			patchCall((void*)0xBFCB5F, printf_mempool_hook);
			patchCall((void*)0xBFCBBE, printf_mempool_hook);
			patchCall((void*)0xC00E4A, printf_mempool_hook);
		}

	}
}
