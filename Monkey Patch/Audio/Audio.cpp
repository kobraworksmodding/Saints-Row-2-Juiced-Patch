// Audio.cpp (uzis)
// --------------------
// Created: 22/02/2025

#include "../FileLogger.h"
#include "../Patcher/patch.h"
#include "../GameConfig.h"
#include "../SafeWrite.h"

namespace Audio
{
	void FixAudioDeviceAssignment()
	{
		if (GameConfig::GetValue("Audio", "FixAudioDeviceAssign", 1))
		{
			Logger::TypedLog(CHN_MOD, "Fixing Audio Device Assignment.\n");
			// fixes, or attempts to fix the incorrect GUID assigning for BINK related stuff in SR2.
			patchBytesM((BYTE*)0x00DBA69C, (BYTE*)"\x00\x00\x00\x00", 4);
		}
	}

	void Init()
	{
		FixAudioDeviceAssignment();

	}
}