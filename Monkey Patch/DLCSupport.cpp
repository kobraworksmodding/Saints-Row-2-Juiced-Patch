#include "DLCSupport.h"
#include "../SafeWrite.h"
#include <safetyhook.hpp>

void LoadDLCPersonaVoice(SafetyHookContext& ctx) {
	char* str = (char*)(ctx.esp + 76);
	if (strcmp(str, "VOC_DL") == 0) { // this is a bit of a hack because it assumes you'll be using a voice_pc that marks the DLC audio as DL
		strcpy_s(str, 4, "DLC");
	}
}

void IncreaseMemPool() {
	SafeWrite32((UInt32)0x006AD138, 2049100); // doubled size to avoid the game crashing if you merge voice_pc with dlc_voice_xbox2
	SafeWrite32((UInt32)0x006AD151, 2049100);
}

void DLCSetup() {
	static SafetyHookMid DLCVoice = safetyhook::create_mid(0x006AD231, &LoadDLCPersonaVoice);
	IncreaseMemPool();
}