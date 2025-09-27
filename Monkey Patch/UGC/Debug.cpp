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
		if (CreateCache((char*)"loose.txt"))
		{
			CacheConflicts();
			patchJmp((void*)0x0051DAC0, (void*)hook_loose_files);						// Allow the loading of loose files
			patchCall((void*)0x00BFD8F5, (void*)hook_raw_get_file_info_by_name);		// Add optional search in the ./loose directory
			static SafetyHookMid InsertHashes = safetyhook::create_mid(0x00C0A8E0, &InsertFileHashes);
		}
		else
			Logger::TypedLog(CHN_DLL, "Create loose file cache failed.\n");
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
		}
	}

	int __cdecl printf_mempool_hook(const char* const Format, ...) {
		va_list args;
		va_list args_copy;
		int result = 0;
		va_start(args, Format);
		va_copy(args_copy, args);
		int size = vsnprintf(NULL, 0, Format, args) + 1;

		if (size > 0) {
			char* buffer = (char*)malloc(size);
			if (buffer) {

				vsnprintf(buffer, size, Format, args_copy);

				const char* extra_prefix = "[MEMPOOL ERROR]\n";
				const char* extra_suffix = "\nMost likely caused by an installed mod, please attempt to increase original mempool size, expect issues otherwise.";

				int extended_size = strlen(extra_prefix) + strlen(buffer) + strlen(extra_suffix) + 1;
				char* extended_buffer = (char*)malloc(extended_size);

				if (extended_buffer) {
					snprintf(extended_buffer, extended_size, "%s%s%s",
						extra_prefix, buffer, extra_suffix);


					MessageBoxA(NULL, extended_buffer, "Juiced Patch", MB_OK | MB_ICONERROR);

					free(extended_buffer);
				}
				else {

					MessageBoxA(NULL, buffer, "Juiced Patch", MB_OK | MB_ICONERROR);
				}


				result = printf("%s", buffer);

				free(buffer);
			}
		}

		va_end(args_copy);
		va_end(args);
		return result;
	}

	void Init() {
		OptionsManager::registerOption("Graphics", "DynamicRenderDistance", (int*)&UseDynamicRenderDistance, 0);
#if !JLITE
		if (GameConfig::GetValue("Gameplay", "SkipIntros", 0)) // can't stop Tervel won't stop Tervel
		{
			Logger::TypedLog(CHN_DLL, "Skipping intros & legal disclaimers.\n");
			patchNop((BYTE*)(0x005207B4), 6); // prevent intros from triggering
			patchBytesM((BYTE*)0x0068C740, (BYTE*)"\x96\xC5\x68\x00", 4); // replace case 0 with case 4 to skip legal disclaimers
		}

		if (GameConfig::GetValue("Gameplay", "DisableCheatFlag", 0))
		{
			//patchNop((BYTE*)0x00687e12, 6);
			//patchNop((BYTE*)0x00687e18, 6);
			CMPatches_DisableCheatFlag.Apply();
		}


		if (GameConfig::GetValue("Debug", "AddBindToggles", 1))
		{
			Logger::TypedLog(CHN_DEBUG, "Adding Custom Key Toggles...\n");
			addBindToggles = 1;
			patchNop((BYTE*)0x0051FEB0, 7); // nop to prevent the game from locking the camera roll in slew
			patchBytesM((BYTE*)0x00C01B52, (BYTE*)"\xD9\x1D\xF8\x2C\x7B\x02", 6); // slew roll patch, makes the game write to a random unallocated float instead to prevent issues
			patchBytesM((BYTE*)0x00C01AC8, (BYTE*)"\xDC\x64\x24\x20", 4); // invert Y axis in slew 
		}

		if (GameConfig::GetValue("Gameplay", "LoadLastSave", 0)) // great for testing stuff faster and also for an optional feature in gen
		{
			LoadLastSave = 1;
			Logger::TypedLog(CHN_DEBUG, "Skipping main menu...\n");
		}

#endif 
		if (GameConfig::GetValue("Debug", "FixFrametime", 1))
		{
			Logger::TypedLog(CHN_DEBUG, "Fixing Frametime issues...\n");
			fixFrametime = 1;
		}

		if (GameConfig::GetValue("Debug", "mempool_print_error", 1) != 0) {
			patchCall((void*)0xBFCA9E, printf_mempool_hook);
			patchCall((void*)0xBFCB5F, printf_mempool_hook);
			patchCall((void*)0xBFCBBE, printf_mempool_hook);
			patchCall((void*)0xC00E4A, printf_mempool_hook);
		}

	}
}