#include "DLCSupport.h"
#include "../SafeWrite.h"
#include <safetyhook.hpp>

void LoadDLCPersonaVoice(SafetyHookContext& ctx) {
	char* str = (char*)(ctx.ebp);
	const char* DLCBank = "SR2_DLC";
	if (strcmp(str, "SR2_VOC_DL") == 0) { // this is a bit of a hack because it assumes you'll be using a voice_pc that marks the DLC audio as DL
		ctx.ebp = (uintptr_t)DLCBank;
	}
}

void IncreaseMemPool() {
	SafeWrite32((UInt32)0x006AD138, 2049100); // doubled size to avoid the game crashing if you merge voice_pc with dlc_voice_xbox2
	SafeWrite32((UInt32)0x006AD151, 2049100);
}

void DLCSetup() {
	static SafetyHookMid DLCVoice = safetyhook::create_mid(0x0047AD09, &LoadDLCPersonaVoice);
	IncreaseMemPool();
}