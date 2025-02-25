// Debug.cpp
// --------------------
// Created: 25/02/2025

#include "../FileLogger.h"
#include "../Patcher/patch.h"
#include "../GameConfig.h"
#include "../SafeWrite.h"
#include "../loose files.h"
#include "Debug.h"

namespace Debug
{
	bool CheatFlagDisabled = 0;
	bool addBindToggles = 0;
	bool LoadLastSave = 0;
	bool fixFrametime = 0;

	void PatchDatafiles() {
		if (CreateCache("loose.txt"))
		{
			CacheConflicts();
			patchJmp((void*)0x0051DAC0, (void*)hook_loose_files);						// Allow the loading of loose files
			patchCall((void*)0x00BFD8F5, (void*)hook_raw_get_file_info_by_name);		// Add optional search in the ./loose directory
		}
		else
			Logger::TypedLog(CHN_DLL, "Create loose file cache failed.\n");
	}

	void Init() {
#if !JLITE
		if (GameConfig::GetValue("Gameplay", "SkipIntros", 0)) // can't stop Tervel won't stop Tervel
		{
			Logger::TypedLog(CHN_DLL, "Skipping intros & legal disclaimers.\n");
			patchNop((BYTE*)(0x005207B4), 6); // prevent intros from triggering
			patchBytesM((BYTE*)0x0068C740, (BYTE*)"\x96\xC5\x68\x00", 4); // replace case 0 with case 4 to skip legal disclaimers
		}

		if (GameConfig::GetValue("Gameplay", "DisableCheatFlag", 0))
		{
			patchNop((BYTE*)0x00687e12, 6);
			patchNop((BYTE*)0x00687e18, 6);
			CheatFlagDisabled = 1;
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

		if (GameConfig::GetValue("Gameplay", "DisableAimAssist", 0))
		{
			Logger::TypedLog(CHN_MOD, "Disabling Aim Assist...\n");
			patchNop((BYTE*)0x00E3CC80, 16); // nop aim_assist.xtbl
		}

	}
}