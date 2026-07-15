#pragma once
#include "../Patcher/CPatch.h"
#include "../Patcher/CMultiPatch.h"
#include <safetyhook.hpp>
#include <functional>
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
	extern bool DeletionMode;
	extern bool* EnterPressed;
	extern bool IsQuickSaving;
	extern bool* InCutscene;
	extern bool* InMultiplayer;
	extern char* GameLoaded;
	extern char* InMission;
	extern char* CurrentGamemode;
	extern bool& IsWidescreen;
	extern char* LobbyCheck;
	int GetNPCWeaponType(int NPCPointer);
	bool isCoop();
	bool isMissionCompleted(const char* Name);
	bool isMissionUnlocked(const char* Name);
	void LoadCTS(const char* Name);
	void LoadBitmapTable(const char* FileName);
	int AddMessage(const wchar_t* Title, const wchar_t* Desc);
	int AddMessageCustomized(const wchar_t* Title, const wchar_t* Desc, const wchar_t* Options[], int OptionCount);
	void AddInfoMessage(wchar_t* String, int* Data);
	void UnlockPhoneNumber(unsigned int* StringHash, bool Unk);
	bool VintGetGlobalBool(const char* Name);
	bool VintSetGlobalBool(const char* Name, bool Value);
	bool IsInSaveMenu();
	int LuaExecute(const char* Command);
	int VintExecute(const char* Command);
	void NewSave();
	wchar_t* RequestString(const wchar_t* Dest, const char* Label);
	extern CMultiPatch CMPatches_TervelTextureCrashWorkaround_be_as_pe;
	extern void CleanupModifiedScript();
	extern SafetyHookMid cleanupBufferHook;
	extern SafetyHookMid luaLoadBuffHook;
	extern std::function<void()> D3D9_create;
	extern UINT16* GameResX;
	extern UINT16* GameResY;
}