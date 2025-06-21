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

void __declspec(naked) AddInterfacePeg()
{
    static const char* DLC = "ui_dlc.peg";
    static int jmp_continue = 0x00520803;
    __asm {
        mov edi, dword ptr[0x522450]
        call edi
        mov edx, 0x27716E4
        mov ecx, DLC
        call edi
        jmp jmp_continue
    }
}

void DLCSetup() {
    WriteRelJump(0x005207FE, (UInt32)&AddInterfacePeg);
	static SafetyHookMid DLCVoice = safetyhook::create_mid(0x0047AD09, &LoadDLCPersonaVoice);
	IncreaseMemPool();
}