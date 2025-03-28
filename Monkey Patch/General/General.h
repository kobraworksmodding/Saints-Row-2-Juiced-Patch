#pragma once
#include "../Patcher/CPatch.h"
#include "../Patcher/CMultiPatch.h"
#include <safetyhook.hpp>
namespace General {
	void TopWinMain();
	void BottomWinMain();

	typedef int* (__thiscall* GetCharacterIDT)(const char* Name);
	extern GetCharacterIDT GetCharacterID;

	typedef int(__cdecl* ChangeCharacterT)(int** a1); // IDA believes it's int* on PC however that didn't work so I copied ** from 360, it can also take a second arg that isn't needed and I've no idea what it'd do
	extern ChangeCharacterT ChangeCharacter;

	typedef void(__cdecl* ResetCharacterT)(char a1,char a2); // no idea what it expects as the first arg, on 360 I can call it without one and it works but here it dies
	extern ResetCharacterT ResetCharacter;

	typedef int(__thiscall* DeleteNPCT)(int a1, int a2);
	extern DeleteNPCT DeleteNPC;

	void NPCSpawner(const char* Name);
	void YeetAllNPCs();
	bool IsSRFocused();
	extern CPatch CFixHorizontalMouseSensitivity;
	extern bool DeletionMode;
	extern bool* EnterPressed;
	bool VintGetGlobalBool(const char* Name);
	bool VintSetGlobalBool(const char* Name, bool Value);
	bool IsInSaveMenu();
	int LuaExecute(const char* Command);
	int VintExecute(const char* Command);
	wchar_t* RequestString(const wchar_t* Dest, const char* Label);
	extern CMultiPatch CMPatches_TervelTextureCrashWorkaround_be_as_pe;
	extern void CleanupModifiedScript();
	extern SafetyHookMid cleanupBufferHook;
}