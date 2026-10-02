// Audio.cpp (uzis)
// --------------------
// Created: 22/02/2025

#include "../FileLogger.h"
#include "../Patcher/patch.h"
#include "../GameConfig.h"
#include "../SafeWrite.h"
#include "usercaller.hpp"
#include "safetyhook_inline.hpp"

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
	using vehicle_get_lpfABI = uc::abi<
		uc::st0_ret<double>,
		uc::eax_arg<int>,
		uc::stack_arg<uintptr_t>,
		uc::stack_arg<int>
	>;
	static uc::inline_hook<vehicle_get_lpfABI> vehicle_get_lpfD;

	double UC_CDECL vehicle_get_lpf_hook(int a1, uintptr_t vehicle, int a3)
	{
		if (vehicle)
		{
			auto info = *(uintptr_t*)(vehicle + 0x84E4);
			if (info)
			{
				// if bike
				if (*(uintptr_t*)(info + 0x9C) == 1)
					return 0.0;
			}
		}
		return vehicle_get_lpfD.unsafe_call_original(a1, vehicle, a3);
	}

	void Init()
	{
		// so theres an oversight where bikes return a value as if it's a normal car rather than 0.0 (convertibles return 0.0!!!)
		// causing music/radio to be muffled rather than not, not a PC bug also skipped over on console
		// this of course does nothing if LPF isn't fixed which currently is broken on the PC release so there's really no difference - clippy95
		vehicle_get_lpfD.create(0x483B00, vehicle_get_lpf_hook);
		FixAudioDeviceAssignment();

	}
}