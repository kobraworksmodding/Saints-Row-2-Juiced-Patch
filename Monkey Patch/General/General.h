#pragma once
#include "../Patcher/CPatch.h"

namespace General {
	extern void TopWinMain();
	extern void BottomWinMain();

	typedef int* (__thiscall* GetCharacterIDT)(const char* Name);
	extern GetCharacterIDT GetCharacterID;

	typedef int(__cdecl* ChangeCharacterT)(int** a1); // IDA believes it's int* on PC however that didn't work so I copied ** from 360, it can also take a second arg that isn't needed and I've no idea what it'd do
	extern ChangeCharacterT ChangeCharacter;

	typedef void(__cdecl* ResetCharacterT)(char a1,char a2); // no idea what it expects as the first arg, on 360 I can call it without one and it works but here it dies
	extern ResetCharacterT ResetCharacter;

	typedef int(__thiscall* DeleteNPCT)(int a1, int a2);
	extern DeleteNPCT DeleteNPC;

	extern void NPCSpawner(const char* Name);
	extern void YeetAllNPCs();
	extern bool IsSRFocused();
	extern CPatch CFixHorizontalMouseSensitivity;

}