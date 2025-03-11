#include "MainHooks.h"
#include "UtilsGlobal.h"
#include "General/General.h"

#include "FileLogger.h"
#include "SafeWrite.h"
#include "Patches/All Patches.h"
#include "Patcher/patch.h"
#include "RPCHandler.h"
#include "ErrorManager.h"
#include "LuaHandler.h"
#include "DFEngine.h"
#include "Mem/Memory.h"

#include "UGC/Reloaded.h"
#include "UGC/InternalPrint.h"

#include "Player/Input.h"
#include "Player/Behavior.h"

#include "Math/Math.h"

#include "Render/Render3D.h"

#include "LUA/GLua.h"
#include "Game/Game.h"
#include "GameConfig.h"
#include "iat_functions.h"
#include <chrono>

#include "Audio/Audio.h"
#include "Audio/XACT.h"

#include <format>
#include <WinSock2.h>
#include <windows.h>
#include <codecvt>
#include "Render/Render2D.h"
#include "Network/Gamespy.h"
#include "UGC/Debug.h"

#include "BlingMenuInstall.h"
#include <shellapi.h>
#include "General/General.h"
using namespace General;
const char ServerNameSR2[] = "[Saints Row 2]";

BYTE useJuicedOSD = 0;
bool useExpandedOSD = false;
void PrintCoords(float x, float y, float z,bool showplayerorient);
void PrintFrametime();
void PrintGameFrametime();
void PrintFramerate();
void PrintUsername();
void PrintPartnerUsername();
void PrintLatestChunk();
void PrintDBGGarble();

float deltaTime;

char* executableDirectory[MAX_PATH];

bool lastFrameStates[256];
bool wasPressedThisFrame[256];

void SetDefaultGameSettings()
{
	patchBytesM((BYTE*)0x00774126, (BYTE*)"\xC6\x05\xAC\xA9\xF7\x01\x00", 7); // Force game into windowed on default settings.
#if RELOADED
	//char* playerName = (CHAR*)0x0212AB48;
	//char* GameName = reinterpret_cast<char*>(0x0212AA08);
	//strcpy(GameName, playerName);

	patchNop((BYTE*)0x0083FA3D, 22); // Removes the Unlim Score/Time check for MP.

	// change game version from 201 to 209 
	// 201 is Vanilla , 209 is Reloaded.
	//patchBytesM((BYTE*)0x008D01F6, (BYTE*)"\x68\xB1", 2);
#else
	char* GameName = reinterpret_cast<char*>(0x0212AA08);
	strcpy(GameName, (const char*)ServerNameSR2);
#endif

	// -- RAHHHHHHH I HATE RESOLUTION STUFF --
	/* patchNop((BYTE*)0x00775F24, 7);
	patchNop((BYTE*)0x00775F2A, 7);
	patchDWord((void*)(0x007EAEC3 + 2), (uint32_t)ResolutionY);
	patchDWord((void*)(0x007EAEB4 + 2), (uint32_t)ResolutionX); */
}

void UpdateKeys()
{
	for (int i = 0; i < 256; i++)
	{
		bool thisFrameState = GetKeyState(i) < 0;
		wasPressedThisFrame[i] = thisFrameState && !lastFrameStates[i];
		lastFrameStates[i] = thisFrameState;
	}
}

// Returns the address last in the chain, for example if value of ADDRESS (0x1)
// 0x1 + 0x2 = 0x4,
// and value of ADDRESS (0x4)
// 0x4 + 0x1 = 0x9, 
// then ReadPointer(0x1,{0x2,0x1}); will return 0x9.
uintptr_t ReadPointer(uintptr_t baseAddress, const std::vector<uintptr_t>& offsets) {
	uintptr_t address = baseAddress;

	if (address == 0) {
		return 0;
	}

	for (size_t i = 0; i < offsets.size(); ++i) {
		uintptr_t* nextAddress = reinterpret_cast<uintptr_t*>(address);
		if (nextAddress == nullptr || *nextAddress == 0) {
			return 0;
		}
		address = *nextAddress + offsets[i];
	}

	return address;
}

bool IsKeyPressed(unsigned char Key, bool Hold) // USE THIS FROM NOW ON
{
	if (General::IsSRFocused())
	{
		if (Hold)
		{
			return lastFrameStates[Key];
		}
		else
		{
			return wasPressedThisFrame[Key];
		}
	}
	return false;
}

const char* Path = "Software\\SR2Juiced";
const char* Name = "FirstBoot";
HKEY hKey;

bool FirstBootCheck() {

	if (RegOpenKeyExA(HKEY_CURRENT_USER, Path, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
	{
		DWORD value = 0, size = sizeof(value);
		if (RegQueryValueExA(hKey, Name, nullptr, nullptr, (LPBYTE)&value, &size) == ERROR_SUCCESS && value == 1)
		{
			RegCloseKey(hKey);
			return false;
		}
		RegCloseKey(hKey);
	}
	return true;
}

void FirstBootFlag() {

	if (RegCreateKeyExA(HKEY_CURRENT_USER, Path, 0, nullptr, 0, KEY_WRITE, nullptr, &hKey, nullptr) == ERROR_SUCCESS)
	{
		DWORD value = 1;
		RegSetValueExA(hKey, Name, 0, REG_DWORD, (const BYTE*)&value, sizeof(value));
		RegCloseKey(hKey);
	}
}

void __cdecl WelcomeCallback(int Unk, int SelectedOption, int Action) {

	if (Action == 2 && SelectedOption == 1) {
		ShellExecute(0, 0, L"https://discord.com/invite/HEMaT2mUpU", 0, 0, SW_SHOW);
	}
}

BOOL __stdcall Hook_GetVersionExA(LPOSVERSIONINFOA lpVersionInformation)
{
    char timeString[200];
    std::time_t newt = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::strftime(timeString, sizeof(timeString), "%a %d %b %Y - %I:%M:%S%p", std::localtime(&newt));
    
    GetModuleFileNameA(NULL, (LPSTR)&executableDirectory, MAX_PATH);
    
    char* exe = strrchr((LPSTR)executableDirectory, '\\');
	if (exe) {
		*(exe + 1) = '\0';
	}
#if !RELOADED
	#if JLITE
	Logger::TypedLog(CHN_DLL, (" --- Welcome to Saints Row 2 JUICED LITE Version: " + std::string(UtilsGlobal::juicedversion) + " ---\n").c_str());
    #else
	Logger::TypedLog(CHN_DLL, (" --- Welcome to Saints Row 2 JUICED Version: " + std::string(UtilsGlobal::juicedversion) + " ---\n").c_str());
    #endif
#else
	Logger::TypedLog(CHN_DLL, " --- Welcome to Saints Row 2 RELOADED ---\n");
#endif
	Logger::TypedLog(CHN_DLL, "RUNNING DIRECTORY: %s\n", &executableDirectory);
	Logger::TypedLog(CHN_DLL, "LOG FILE CREATED: %s\n", &timeString);
	Logger::TypedLog(CHN_DLL, "--- DLL Based on MonkeyPatch by scanti, additional work by Uzis, Tervel, jason098 and Clippy95. ---\n");
	
	if(GetVersionExAFirstRun)
	{
		GetVersionExAFirstRun=false;
		Logger::TypedLog(CHN_DLL, "Calling hooked GetVersionExA.\n");
		UInt32 winmaindata=*((UInt32*)offset_addr(0x00520ba0));
		if(winmaindata==0x83ec8b55)
		{
			// The Steam version of the executable is now unencrypted, so we can start patching.


			Logger::TypedLog(CHN_DLL, "Hooking WinMain.\n");
			WriteRelCall(offset_addr(0x00c9e1c0),(UInt32)&Hook_WinMain);

			// Add patch routines here for patches that need to be run at crt startup, usually for patching constructors.
			// Be very careful you can break things easily.

			// The game timer constructor sets up the game's timing so this needs to patch before the constructor is called.
			PatchQueryPerformance();
		}
		else
		{
			Logger::TypedLog(CHN_DLL, "WinMain sanity check failed in GetVersionExA.\n");
		}
	}
	else
	{
		Logger::TypedLog(CHN_DLL, "Calling hooked GetVersionExA more than once. Skipping patch code.");
	}

	return(GetVersionExA(lpVersionInformation));
}

void __declspec(naked) pauseGame() {
	__asm {

		push ebp
		mov ebp, esp
		sub esp, __LOCAL_SIZE

		push 1
		mov  edi, 5
		mov  esi, 0x699840
		call esi

		mov esp, ebp
		pop ebp
		ret
	}
}

void __declspec(naked) unpauseGame() {
	__asm {
		mov  esi, 2
		mov  edi, 0x699910
		call edi
		ret
	}
}

typedef void(__cdecl* SlewCameraT)(float* Pos, float* Orient, float Frametime, int idk1, bool idk2);
SlewCameraT SlewCamera = (SlewCameraT)0xC01650;

typedef void(__cdecl* UpdateCameraT)();
UpdateCameraT UpdateCamera = (UpdateCameraT)0x4932F0;

typedef bool(*isCoopT)();
isCoopT isCoop = (isCoopT)0x007F7AD0;

typedef void(__cdecl* CoopRemotePauseT)(char pause);
CoopRemotePauseT CoopRemotePause = (CoopRemotePauseT)0x008CB140;

typedef char(__cdecl* LoadContinueT)();
LoadContinueT LoadContinue = (LoadContinueT)0x7790E0;

typedef void(*ChunkStrT)();
ChunkStrT ChunkStr = (ChunkStrT)0xA7B880;

typedef void(*VegStrT)();
VegStrT VegStr = (VegStrT)0x4E66A0;

bool slewMode = false;

void RawTags() {
	// CLIPPY TODO: Figure out what's wrong with X axis sensitivty at low speed, maybe use mouse struct instead of reading from overall ingame delta?
	BYTE menuStatus = *(BYTE*)(0x00EBE860);
	if (menuStatus != 2) return;
	/*
	mouse mouseread;
	int32_t x = mouseread.getXdelta();
	int32_t y = mouseread.getYdelta();*/
	float XDelta = *(float*)0x2348534;
	float yDelta = *(float*)0x2348538;
	float LeftStickX = *(float*)0x23485F4;
	float LeftStickY = *(float*)0x23485F8;
	float RightStickX = *(float*)0x023485B4;
	float RightStickY = *(float*)0x023485B8;
	uint16_t yTag = *(uint16_t*)0x027A3F6C;
	uint16_t xTag = *(uint16_t*)0x027A3F68;

	int16_t newYTag = yTag;
	int16_t newXTag = xTag;
	float divisor;
	if (!XDelta && !yDelta && !LeftStickX && !LeftStickY)
		return;
	if (RightStickX != 0 && RightStickY != 0) {
		divisor = 29.f;
	}
	else {
		divisor = 4.f;
	}
	newYTag -= static_cast<int16_t>(((yDelta / divisor) + (LeftStickY / 29.f)) * 128.0f);

	newXTag += static_cast<int16_t>(((XDelta / divisor) + (LeftStickX / 29.f)) * 76.8f);


	// Ensure yTag stays within the range otherwise it'll break
	if (newYTag > 255) {
		newYTag = 255;
	}
	else if (newYTag < 0) {
		newYTag = 0;
	}

	// Ensure xTag stays within the range otherwise it'll break
	if (newXTag > 511) {
		newXTag = 511;
	}
	else if (newXTag < 0) {
		newXTag = 0;
	}

	*(uint16_t*)0x027A3F6C = static_cast<uint16_t>(newYTag);
	*(uint16_t*)0x027A3F68 = static_cast<uint16_t>(newXTag);
}

void AspectRatioFix() {
	float currentAR = *(float*)0x022FD8EC;
		const float a169 = 1.777777791;
		const double defaultFOV = 1.33333337306976;
		//double currentFOV = *(double*)0x0E5C808;
		double correctFOV = (defaultFOV * ((double)currentAR / (double)a169));
		if (currentAR > a169 && Render3D::ARfov) { // otherwise causes issues for odd ARs like 16:10/5:4 and the common 4:3.
			patchDouble((BYTE*)0x00E5C808, correctFOV);
			patchNop((BYTE*)0x00797181, 6); // Crosshair location that is read from FOV, we'll replace with our own logic below.
			patchFloat((BYTE*)0x00EC2614, correctFOV);
			Logger::TypedLog(CHN_DEBUG, "Aspect Ratio FOV fixed...\n");
			//ARfov = 0;// stop this thread 

			if (Render3D::ARCutscene) {
				const double currentCFOV = *(double*)0x00e5c3f0; // default 57.2957795131, this is (180 / pi).
				double correctCFOV = currentCFOV * ((double)currentAR / (double)a169);
				if (correctCFOV > 125) {
					correctCFOV = 125; // arbiratry number close to 32:9 CFOV, 
					//this will stop most scenes from going upside down in 48:9, we need a beter address for cutscenes similiar to world FOV.
				}
				patchDouble((BYTE*)0x00e5c3f0, correctCFOV);
				Logger::TypedLog(CHN_DEBUG, "Aspect Ratio Cutscenes (might break above 21:9) hack...\n");
				Render3D::ARCutscene = 0;

			}
		}
		if (Render3D::FOVMultiplier >= 1.01 || !Render3D::ARfov) { // Not mixed above due to 16:10 and 4:3
			double multipliedFOV = (currentAR > a169) ? correctFOV * Render3D::FOVMultiplier : defaultFOV * Render3D::FOVMultiplier;
			patchDouble((BYTE*)0x00E5C808, multipliedFOV);
			patchNop((BYTE*)0x00797181, 6);
			patchFloat((BYTE*)0x00EC2614, (float)multipliedFOV);
			Render3D::ARfov = 0;
		}
		else {
			Render3D::ARfov = 0;
			Render3D::ARCutscene = 0;
		}
		return;

}

void getDeltaTime() {
	static LARGE_INTEGER tpsFreq;
	static LARGE_INTEGER lastTickCount;
	LARGE_INTEGER currentTickCount;

	QueryPerformanceFrequency(&tpsFreq);
	QueryPerformanceCounter(&currentTickCount);
	float delta = (float)(currentTickCount.QuadPart - lastTickCount.QuadPart) / tpsFreq.QuadPart;
	lastTickCount = currentTickCount;
	// do not kill me if this is wrong thanks, couldn't find too many examples for doing it
	deltaTime = delta;
}

void FogTest() {
	float ogFogStrength1 = *(float*)(0x00E989A0);
	float ogFogStrength2 = *(float*)(0x00E989A4);
	*(float*)(0x027B2CBA) = max(ogFogStrength1 / 1.5, 0.3f);
	*(float*)(0x027B2CBE) = max(ogFogStrength2 / 1.5, 0.3f);
}

bool DOFEnabled;
float DOFBlur = 8.0f;
float DOFRadius = 16.0f;
float DOFDistance = 50.0f;

void Slew() {
	UtilsGlobal::mouse Mouse;
	float Speed = 15.0f;
	float* CameraPos = (float*)(0x25F5B20);
	float* CameraOrient = (float*)(0x25F5B5C);
	float& FOV = *(float*)0x25F5BA8;
	float& Roll = *(float*)0x33DA350;

	if (slewMode) {

		SlewCamera(CameraPos, CameraOrient, deltaTime, 0, false);

		if (UtilsGlobal::isPaused) {
			UpdateCamera();
			ChunkStr();
			VegStr();
			*(float*)0x02F9B7F8 = *(float*)0x025F5B1C;
			*(float*)0x02F9B7F4 = *(float*)0x025F5B18;
			*(float*)0x02F9B7F0 = *(float*)0x025F5B14;
			*(float*)0x02F9B7EC = *(float*)0x025F5B1C;
			*(float*)0x02F9B7E8 = *(float*)0x025F5B18;
			*(float*)0x02F9B7E4 = *(float*)0x025F5B14;
		}

		if (IsKeyPressed(VK_MBUTTON, false)) {
			DOFEnabled =! DOFEnabled;
		}

		else if ((IsKeyPressed(VK_XBUTTON1, true)) && Mouse.getWheeldelta()) {
			DOFDistance += (((float)Mouse.getWheeldelta()) / 10.0f);
			DOFDistance = UtilsGlobal::clamp(DOFDistance, 5.0f, 200.0f);
		}

		else if ((IsKeyPressed(VK_XBUTTON2, true)) && Mouse.getWheeldelta()) {
			DOFBlur += (((float)Mouse.getWheeldelta()) / 240.0f);
			DOFBlur = UtilsGlobal::clamp(DOFBlur, 1.0f, 10.0f);
			DOFRadius = DOFBlur * 2;
		}

		else if (Mouse.getWheeldelta()) {
			FOV -= (((float)Mouse.getWheeldelta()) / 120.0f);
			FOV = UtilsGlobal::clamp(FOV, 1.0f, 120.0f);
		}

		else if (IsKeyPressed(0x31, true)) {
			Roll = 1.0f;
		}

		else if (IsKeyPressed(0x33, true)) {
			Roll = -1.0f;
		}

		else {
			Roll = 0.0f;
		}

	}
}

void SlewScrollWheelSmoothing() {
	UtilsGlobal::mouse mouse;
	int wheel_delta = mouse.getWheeldelta();
		if (wheel_delta) {
			float* smoothing = (float*)(0x00E83E1C);
			*smoothing = UtilsGlobal::clamp(*smoothing + ( (float)wheel_delta / 2850.f), 0.f, 1.3f);
			//Logger::TypedLog(CHN_DEBUG, "Mouse slew smoothing: %f,\n", *smoothing);
		}
}

void __declspec(naked) SlewScrollWheelSmoothingASMHelp() {
	static int jmp_continue = 0x00C013E8;
	__asm {
		pushad
		pushfd
	}
	__asm {
		call SlewScrollWheelSmoothing
	}
	__asm {
		popfd
		popad
		mov eax,edi
		cdq
		mov ecx,eax

		jmp jmp_continue
	}

}

 void FixandImproveSlewMouseRuntimePatch() {
	patchNop((BYTE*)0x0051FEA4, 6); // Allow mouse mode to be toggled by default.
	patchFloat((BYTE*)0x00E83E1C, 0.3f); // Increase smoothing a bit by default for mouse.
	WriteRelJump(0x00C013E3, (UInt32)&SlewScrollWheelSmoothingASMHelp);
}

void havokFrameTicker() { // Proper Frametime ticker by Tervel

	if (*(float*)0xE84380 <= 0.03333333333f)
		*(float*)(0x02527DA4) = *(float*)0xE84380 / 2;
	else
		*(float*)(0x02527DA4) = 0.01666666666f;

}

typedef int(__cdecl* addsubs_t)(int a1, float a2, float a3, int a4);
addsubs_t addsubs = (addsubs_t)0x6D9610;

int addsubtitles(const wchar_t* subtitles, float delay, float duration, float whateverthefuck) { // Tervel W
	int result = addsubs((int)subtitles, delay, duration, whateverthefuck);
	return result;
}

void coopPauseLoop() {
	bool CoopCheck = isCoop();
	static bool PauseRestored = false;
	BYTE* IsPaused = (BYTE*)(0x027B2CF6);
	BYTE IsPausedOriginal = *(BYTE*)(0x02527C08);
	BYTE IsPauseMenuOpen = *(BYTE*)(0x00EBE860);
	BYTE ThankYouVolition = *(BYTE*)(0x00E8CF80); // Some UI mode shit, hopefully can be used to check if you passed/failed a mission to restore OG pause

	/*float delay = 0.0f;
	float duration = 1.5f;
	float whateverthefuck = 0.0f;

	if (IsKeyPressed('P', 1)) {

		*(bool*)(0x252740E) = 1;

		PauseRestored = !PauseRestored;

		std::wstring subtitles = L"Pause Restored:[format][color:purple]";
		subtitles += PauseRestored ? L" ON" : L" OFF";
		subtitles += L"[/format]";
		addsubtitles(subtitles.c_str(), delay, duration, whateverthefuck);

		patchBytesM((BYTE*)0x00779C5E, PauseRestored ? (BYTE*)"\xE8\xDD\x14\x15\x00" : (BYTE*)"\x90\x90\x90\x90\x90", 5);

		if (IsPauseMenuOpen == 20) {
			CoopRemotePause(PauseRestored ? 1 : 0);
		}
	}*/

	// Either nop or restore Zombie Uprising pausing based on whether you're in SP or co-op
	patchBytesM((BYTE*)0x005B8246, CoopCheck ? (BYTE*)"\x90\x90\x90\x90\x90" : (BYTE*)"\xE8\x85\xBE\x19\x00", 5);

	if (CoopCheck) {
		patchBytesM((BYTE*)0x00520531, IsPausedOriginal ? (BYTE*)"\x90\x90\x90\x90\x90" : (BYTE*)"\xE8\x2A\xE2\xFD\xFF", 5);
	}
	if (CoopCheck && !PauseRestored && ThankYouVolition != 15) {
		*IsPaused = 0;
	}
	else if (!CoopCheck || PauseRestored || ThankYouVolition == 15) {
		*IsPaused = IsPausedOriginal;
	}
}

void LessRetardedChat() {

	wchar_t* ACDProjektBlackSpecial = reinterpret_cast<wchar_t*>(0x01F76948);
	wchar_t* FixThatShit = reinterpret_cast<wchar_t*>(0x022092FF);

	size_t Length = 0;
	for (Length = 0; Length < 127 && ACDProjektBlackSpecial[Length] != L'\0'; ++Length);

	wchar_t temp[130] = { 0 };

	wcsncpy(temp, ACDProjektBlackSpecial, Length < 127 ? Length : 127);

	if (Length > 64) {
		temp[64] = (ACDProjektBlackSpecial[63] == L' ') ? L'\n' : L'-';
		if (temp[64] == L'-') temp[65] = L'\n';
		wcsncpy(temp + (temp[64] == L'-' ? 66 : 65), ACDProjektBlackSpecial + 64, Length - 64);
	}

	wcscpy(FixThatShit, temp);
}

void SkipMainMenu() {
	static bool ShouldSkip = true;
	static DWORD lastTick = 0;

	if (ShouldSkip) {
		DWORD currentTick = GetTickCount();

		if (lastTick == 0) {
			lastTick = currentTick; 
		}

		if (currentTick - lastTick >= 1) { // very small delay, otherwise it black screens the game in juiced?? worked fine in my code w/o that
			memset((void*)0x025283BC, 1, 3);
			LoadContinue();
			ShouldSkip = false;
		}
	}
}

typedef int(__cdecl* VehicleSpawnT)(int a1);
VehicleSpawnT VehicleSpawn = (VehicleSpawnT)0x00AE4AE0;

typedef int(__thiscall* GetVehIndexT)(const char* Vehicle);
GetVehIndexT GetVehIndex = (GetVehIndexT)0x00AE4090;

typedef bool(__thiscall* isMissionCompletedT)(const char* mission_name);
isMissionCompletedT IsMissionCompleted = (isMissionCompletedT)0x6A6E50;

typedef int(__fastcall* GetPointerT)(int VehiclePointer);
GetPointerT GetPointer = (GetPointerT)0x00AE28F0;

typedef int(*TeleportPlayerT)();
TeleportPlayerT TeleportPlayer = (TeleportPlayerT)0x9D3C50;

typedef int(__thiscall* DeleteVehT)(int a1, int a2);
DeleteVehT DeleteVeh = (DeleteVehT)0xAA4490;

typedef char(__cdecl* StopRagdollT)(int a1, int a2);
StopRagdollT StopRagdoll = (StopRagdollT)0x9ACB10;

typedef void(__stdcall* DisableRagdollT)(int Pointer, bool Enable);
DisableRagdollT DisableRagdoll = (DisableRagdollT)0x965B50;

typedef char(*ExitVehicleT)();
ExitVehicleT ExitVehicle = (ExitVehicleT)0x5E8140;

typedef char(__cdecl* ExitFineAimT)(int PlayerPointer);
ExitFineAimT ExitFineAim = (ExitFineAimT)0x9D9FD0;

typedef int(__cdecl* GetAnimStateT)(const char* Name);
GetAnimStateT GetAnimState = (GetAnimStateT)0x6EF510;

typedef void(__thiscall* SetAnimStateT)(int Pointer, int* Anim);
SetAnimStateT SetAnimState = (SetAnimStateT)0x9695F0;

typedef void(__thiscall* CollisionTestT)(int Pointer, char Idk1, int Idk2);
CollisionTestT CollisionTest = (CollisionTestT)0x960800; // npc_enable_human_collision lead me to this, it's needed to fix an issue w the noclip

void tpCoords(float x, float y, float z) {
	float* xyz = reinterpret_cast<float*>(0x027B305A);
	xyz[0] = x;
	xyz[1] = y;
	xyz[2] = z;
	TeleportPlayer();
}

void __declspec(naked) FadeIn(int Veh, int Duration) {
	__asm {

		mov eax, [esp + 4]
		mov edx, [esp + 8]
		mov esi, 0xAABC80
		call esi
		ret
	}
}

void __declspec(naked) EnterVeh(int VehPointer, int PlayerPointer, int SeatIndex) {
	__asm {

		push ebp
		mov ebp, esp
		sub esp, __LOCAL_SIZE

		push    SeatIndex
		mov     esi, PlayerPointer
		mov     edi, VehPointer
		mov		ebx, 0x597C40
		call    ebx

		mov esp, ebp
		pop ebp
		ret
	}
}

int __declspec(naked) GetVarIndex(int VehData, const char* VariantName) {
	__asm {

		push ebp
		mov ebp, esp
		sub esp, __LOCAL_SIZE

		push	esi
		mov		esi, VehData
		mov     eax, esi
		imul    eax, 2000
		mov		ebx, 0x2FAD1F8
		add     eax, ebx
		mov	    edi, VariantName
		mov     ecx, edi
		mov		ebx, 0xAC7420
		call    ebx

		mov esp, ebp
		pop ebp
		ret
	}
}

void VehicleSpawner(const char* Name, const char* Var) {

	static int VehPointer;
	static int CurVehPointer;

	int& VehFromSpawner = *(int*)(0x0252A0E0);
	int& PlayerOffset = *(int*)0x21703D4;
	int& CurrentVeh = *(int*)(PlayerOffset + 0xD74);

	if (CurrentVeh > 0 && CurrentVeh != VehFromSpawner) {
		CurVehPointer = GetPointer(CurrentVeh);
		DeleteVeh(CurVehPointer, 0);
	}

	int VehIndex = GetVehIndex(Name);
	_asm pushad
	int VarIndex = GetVarIndex(VehIndex, Var);
	_asm popad

	SafeWrite8((UInt32)0x00AE4BE8, (UInt32)VarIndex);

	if (VehIndex != -1) {
		VehicleSpawn(VehIndex);
		VehPointer = GetPointer(VehFromSpawner);
		FadeIn(VehPointer, 500);
		StopRagdoll(PlayerOffset, 0);
		_asm pushad
		EnterVeh(VehPointer, PlayerOffset, 0);
		_asm popad
	}
}

int __declspec(naked) AddMessage(const wchar_t* Title, const wchar_t* Desc) { // we can use this for juiced-related info prompts
	__asm {
		push ebp
		mov ebp, esp
		sub esp, __LOCAL_SIZE


		mov edx, Desc
		push edx
		mov esi, Title
		push esi

		mov eax, 0x7E6250
		call eax

		mov esp, ebp
		pop ebp
		ret
	}
}

int __declspec(naked) AddMessageCustomized(const wchar_t* Title, const wchar_t* Desc, const wchar_t* Options[], int OptionCount) { // same thing as above except we have freedom over everything
	__asm {
		push ebp
		mov ebp, esp
		sub esp, __LOCAL_SIZE

		push 0
		push 1
		push 2
		push OptionCount
		mov edx, Options
		push edx
		push Desc
		mov eax, Title
		push eax
		or eax, -1

		mov ecx, 0x753080
		call ecx

		mov esp, ebp
		pop ebp
		ret
	}
}

bool hasCheatMessageBeenSeen = 0;

typedef int(__cdecl* ShowPauseDialogT)(bool a1, bool a2, bool a3, bool a4);
ShowPauseDialogT ShowPauseDialog = (ShowPauseDialogT)0x7540D0;

typedef void(*RemovePauseDialogT)();
RemovePauseDialogT RemovePauseDialog = (RemovePauseDialogT)0x754270;

void SlewModeToggle() {
	float delay = 0.0f;
	float duration = 1.5f;
	float whateverthefuck = 0.0f;
	bool InCutscene = *(bool*)(0x2527D14);
	*(bool*)(0x252740E) = 1; // Ins Fraud Sound

	slewMode = !slewMode;

	std::wstring subtitles = L"Slew Mode:[format][color:purple]";
	subtitles += slewMode ? L" ON" : L" OFF";
	subtitles += L"[/format]";
	addsubtitles(subtitles.c_str(), delay, duration, whateverthefuck);

	if (InCutscene) {
		*(int*)(0x25F5AE8) = (slewMode ? 2 : 5);
	}
	else {
		*(int*)(0x25F5AE8) = (slewMode ? 2 : 0);
	}

	SafeWrite32(0x0051A8BD + 3, slewMode ? (UInt32)&DOFEnabled : (UInt32)0x276D00C);
	SafeWrite32(0x0051A9C4 + 2, slewMode ? (UInt32)&DOFBlur : (UInt32)0x276D010);
	SafeWrite32(0x0051A9D2 + 2, slewMode ? (UInt32)&DOFRadius : (UInt32)0x276D014);
	SafeWrite32(0x0051A8F8 + 1, slewMode ? (UInt32)&DOFDistance : (UInt32)0x276D024);

}

void TeleportToWaypoint() {
	if (!UtilsGlobal::getplayer())
		return;
	if ((*(int*)(0x1F7A418) != 0)) { // check if there's a waypoint
		if (!Debug::CMPatches_DisableCheatFlag.IsApplied()) {
			*(BYTE*)0x02527B5A = 0x1;
			*(BYTE*)0x02527BE6 = 0x1;
		}
		*(bool*)(0x252740E) = 1; // Ins Fraud Sound
		std::wstring subtitles = (L"Teleported to waypoint!");
		addsubtitles(subtitles.c_str(), 0.0f, 1.5f, 0.0f);
		tpCoords(*(float*)0x29B9CD0, *(float*)0x29B9CD4, *(float*)0x29B9CD8);

	}
}

void cus_FrameToggles() {
#if !JLITE
	float delay = 0.0f;
	float duration = 1.5f;
	float whateverthefuck = 0.0f;
	static bool uglyMode = false;
	static bool DetachCam = false;
	static bool FPSCounter = false;
	static uint8_t ogAA;
	bool InCutscene = *(bool*)(0x2527D14);
	static bool CutscenePaused;


	if (IsKeyPressed(VK_F1, false)) { // F1

		*(bool*)(0x252740E) = 1; // Ins Fraud Sound

		uglyMode = !uglyMode;
		if (uglyMode) {
			ogAA = *(uint8_t*)(0x252A2DC);
			*(uint8_t*)(0x252A2DC) = 0;
		}
		else {
			*(uint8_t*)(0x252A2DC) = ogAA;
		}
		std::wstring subtitles = L"Ugly Mode:[format][color:purple]";
		subtitles += uglyMode ? L" ON" : L" OFF";
		subtitles += L"[/format]";
		addsubtitles(subtitles.c_str(), delay, duration, whateverthefuck);
		*(uint8_t*)(0x25273B4) = uglyMode ? 1 : 0;
		*(float*)(0xE98988) = uglyMode ? 400.0f : 20000.0f;
	}

	if (IsKeyPressed(VK_F4, false)) { // F4
		SlewModeToggle();
	}

	if (IsKeyPressed(VK_F3, false)) { // F3

		*(bool*)(0x252740E) = 1; // Ins Fraud Sound

		useJuicedOSD = (useJuicedOSD + 1) % 4;
		std::wstring subtitles = L"Debugging Info:[format][color:purple]";
		switch (useJuicedOSD) {
		case 0:
			subtitles += L" OFF";
			break;
		case 1:
			subtitles += L" 1";
			break;
		case 2:
			subtitles += L" 2";
			break;
		case 3:
			subtitles += L" 3";
			break;
		}
		subtitles += L"[/format]";
		addsubtitles(subtitles.c_str(), delay, duration, whateverthefuck);
	}

	if (IsKeyPressed(VK_F2, false)) { // F2

		*(bool*)(0x252740E) = 1; // Ins Fraud Sound
		std::wstring subtitles = L"HUD Toggle:[format][color:purple]";
		subtitles += *(BYTE*)(0x0252737C) ? L" ON" : L" OFF";
		subtitles += L"[/format]";
		addsubtitles(subtitles.c_str(), delay, duration, whateverthefuck);
		*(BYTE*)(0x0252737C) = !(*(BYTE*)(0x0252737C));

	}

	if (IsKeyPressed(VK_F9, false)) { // F9
		Render3D::FOVMultiplier += 0.1;
		AspectRatioFix();
		Logger::TypedLog(CHN_DEBUG, "+FOV Multiplier: %f,\n", Render3D::FOVMultiplier);
		GameConfig::SetDoubleValue("Gameplay", "FOVMultiplier", Render3D::FOVMultiplier);
	}

	if (IsKeyPressed(VK_F8, false)) { // F8
		Render3D::FOVMultiplier -= 0.1;
		AspectRatioFix();
		Logger::TypedLog(CHN_DEBUG, "-FOV Multiplier: %f,\n", Render3D::FOVMultiplier);
		GameConfig::SetDoubleValue("Gameplay", "FOVMultiplier", Render3D::FOVMultiplier);

	}

	if (IsKeyPressed(VK_TAB, false) && InCutscene && !isCoop()) {
		CutscenePaused = !CutscenePaused;
		CutscenePaused ? ShowPauseDialog(true, false, false, false) : RemovePauseDialog();
	}

	if (IsKeyPressed(VK_F5, false)) { // F5
		FLOAT* hkg_playerPosition = (FLOAT*)0x00FA6DB0;
		float xAngle = *(float*)0x025F5B50;
		float zAngle = *(float*)0x025F5B58;
		float hkg_camOrient = atan2(xAngle, zAngle);
		*(bool*)(0x252740E) = 1; // Ins Fraud Sound

		std::wstring subtitles = (L"Player Position & Orient Printed to Console!");
		addsubtitles(subtitles.c_str(), delay, duration, whateverthefuck);
		Logger::TypedLog(CHN_DEBUG, "Player Pos + Orient: <%0.6f %0.6f %0.6f> [%0.6f]\n", hkg_playerPosition[0], hkg_playerPosition[1], hkg_playerPosition[2], hkg_camOrient);
	}

	if (IsKeyPressed(VK_F7, false)) {

		if (hasCheatMessageBeenSeen == 1 || Debug::CMPatches_DisableCheatFlag.IsApplied())
			TeleportToWaypoint();
			
		

		if (!Debug::CMPatches_DisableCheatFlag.IsApplied()) {
			if (hasCheatMessageBeenSeen == 0) {
				const wchar_t* JuicedF7Cheat =
					L"The F7 key hosts a command that allows you to teleport to your waypoint on your map.\n"
					L"Pressing F7 again will teleport you to your Waypoint destination and flag your save as cheated.";
				__asm pushad
				AddMessage(L"Juiced", JuicedF7Cheat);
				__asm popad

				hasCheatMessageBeenSeen = 1;
			}
		}
	}

	if (IsKeyPressed(VK_DELETE, false)) {
		DeletionMode = true;
		*EnterPressed = true;
	}

	if (Game::InLoop::IsCoopOrSP == true)
	{
		if (IsKeyPressed(VK_F6, false)) {
			UtilsGlobal::isPaused = !UtilsGlobal::isPaused;
			UtilsGlobal::isPaused ? pauseGame() : unpauseGame();
		}
	}
#endif
}

typedef int(__cdecl* chatWindowT)();
chatWindowT chatWindow = (chatWindowT)0x75C8F0;

int __declspec(naked) LuaExecute(const char* command)
{
	_asm {
		push ebp
		mov ebp, esp
		sub esp, __LOCAL_SIZE


		mov esi, ds:0x0252983C // Lua State
		mov eax, command
		mov edx, 0xCDA000
		call edx

		mov esi, ds : 0x0252A1B8 // Vint State
		mov eax, command
		mov edx, 0xCDA000
		call edx


		mov esp, ebp
		pop ebp
		ret
	}
}

bool hasCheatMessageBeenSeen2 = 0;

bool NoclipEnabled = false;

typedef void(__stdcall* PlayerHolsterT)(int Player, bool Holster);
PlayerHolsterT PlayerHolster = (PlayerHolsterT)0x9661E0;

typedef void(__stdcall* SetInvulnerableT)(int Pointer, bool Enable);
SetInvulnerableT SetInvulnerable = (SetInvulnerableT)0x965F40;

#if !RELOADED
void ResetYVel() {
	uintptr_t YVelBase = ReadPointer(UtilsGlobal::getplayer(true), { 0x570 });
	float* YVelPositive = (float*)(*(int*)YVelBase + 0x164);
	float* YVelNegative = (float*)(*(int*)YVelBase + 0x144);
	*YVelPositive = 0.0f;
	*YVelNegative = 0.0f;
}

void ToggleNoclip() {
	if (!UtilsGlobal::getplayer())
		return;

	NoclipEnabled = !NoclipEnabled;
	CollisionTest(UtilsGlobal::getplayer(), 1, 0);
	ResetYVel();
	patchByte((BYTE*)0x4FAA90, NoclipEnabled ? 0xC3 : 0x55);

	if (NoclipEnabled) {
		patchNop((BYTE*)0x00C1370F, 5);
		patchNop((BYTE*)0x00C13745, 5);
		patchNop((BYTE*)0x00C1375B, 5);
		patchNop((BYTE*)0x00C13776, 5);
	}
	else {
		patchBytesM((BYTE*)0x00C1370F, (BYTE*)"\xA1\x78\xE7\x34\x02", 5);
		patchBytesM((BYTE*)0x00C13745, (BYTE*)"\xA1\x80\xE7\x34\x02", 5);
		patchBytesM((BYTE*)0x00C1375B, (BYTE*)"\xA1\x90\xE7\x34\x02", 5);
		patchBytesM((BYTE*)0x00C13776, (BYTE*)"\xA1\x88\xE7\x34\x02", 5);
	}

	int AnimState = NoclipEnabled ? GetAnimState("streaking stand") : -1;
	SetAnimState(UtilsGlobal::getplayer(), &AnimState);
	ExitFineAim(UtilsGlobal::getplayer());
	ExitVehicle();

	DWORD old;
	VirtualProtect((LPVOID)0xDD04F4, sizeof(float), PAGE_READWRITE, &old);
	*(float*)0xDD04F4 = NoclipEnabled ? 0.0f : 1.5f;
	VirtualProtect((LPVOID)0xDD04F4, sizeof(float), old, &old);

	StopRagdoll(UtilsGlobal::getplayer(), 0);
	DisableRagdoll(UtilsGlobal::getplayer(), NoclipEnabled ? true : false);
	PlayerHolster(UtilsGlobal::getplayer(), NoclipEnabled ? true : false);
	SetInvulnerable(UtilsGlobal::getplayer(), NoclipEnabled ? true : false);

	*(bool*)(0x252740E) = 1; // Ins Fraud Sound
	std::wstring subtitles = L"Noclip:[format][color:purple]";
	subtitles += NoclipEnabled ? L" ON" : L" OFF";
	subtitles += L"[/format]";
	addsubtitles(subtitles.c_str(), 0.0f, 1.5f, 0.0f);
}

bool IsWaiting = false;
/*
xtbl_node* test = nullptr;
void xtbl_parse_test() {
	test = Game::xml::parse_table_node("weapons.xtbl",0);
	uint32_t checksum = Game::xml::checksum(test, 0);
	printf("\n addr: 0x%X", &test);
	if(test)
	printf("\n test: 0x%X", test);
	printf("\n checksum: 0x%X \n", checksum);
}
*/
void Noclip() {
	int PlayerBase = *(int*)0x21703D4;
	if (!PlayerBase)
		return; // Noclip will still be enabled if this passes.. - Clippy95.
	float MovementSpeed = (IsKeyPressed(VK_SHIFT, true) ? 80.0f : 40.0f);
	float xAngle = *(float*)0x025F5B50;
	float yAngle = *(float*)0x025F5B54;
	float zAngle = *(float*)0x025F5B58;
	float* PlayerSin = (float*)(PlayerBase + 0x38);
	float* PlayerCos = (float*)(PlayerBase + 0x40);

	if (NoclipEnabled && !slewMode && !IsWaiting) {
		uintptr_t CoordsPointer = ReadPointer(UtilsGlobal::getplayer(true), { 0x570,0x8,0x40,0x18 });
		if (!CoordsPointer) {
			return; // Noclip will still be enabled if this passes.. - Clippy95.
		}
		float* x = (float*)(*(int*)CoordsPointer + 0x30);
		float* y = (float*)(*(int*)CoordsPointer + 0x34);
		float* z = (float*)(*(int*)CoordsPointer + 0x38);
		*PlayerSin = xAngle;
		*PlayerCos = zAngle;

		if (IsKeyPressed('W', true)) {
			*x += MovementSpeed * xAngle * deltaTime;
			*y += MovementSpeed * yAngle * deltaTime;
			*z += MovementSpeed * zAngle * deltaTime;
		}

		if (IsKeyPressed('S', true)) {
			*x -= MovementSpeed * xAngle * deltaTime;
			*y -= MovementSpeed * yAngle * deltaTime;
			*z -= MovementSpeed * zAngle * deltaTime;
		}

		if (IsKeyPressed('A', true)) {
			*x -= MovementSpeed * zAngle * deltaTime;
			*z += MovementSpeed * xAngle * deltaTime;
		}

		if (IsKeyPressed('D', true)) {
			*x += MovementSpeed * zAngle * deltaTime;
			*z -= MovementSpeed * xAngle * deltaTime;
		}

		if (IsKeyPressed(VK_SPACE, true)) {
			*y += MovementSpeed * deltaTime;
		}

		if (IsKeyPressed('E', true)) {
			*y -= MovementSpeed * deltaTime;
		}
	}

}
#endif
typedef void(*LoadLevelT)();
LoadLevelT LoadLevel = (LoadLevelT)0x73C000;
#if !RELOADED
void LuaExecutor() {
	BYTE CurrentGamemode = *(BYTE*)0x00E8B210; 
	BYTE LobbyCheck = *(BYTE*)0x02528C14; // Copied from Rich Presence stuff, just using it so we can limit LUA Executor to SP/CO-OP.
	BYTE AreWeLoaded = *(BYTE*)0x00E94D3E;
	static bool OpenedByExecutor = false;
	BOOL* IsOpen = (BOOL*)(0x0252A5B3);
	wchar_t* ChatInput = reinterpret_cast<wchar_t*>(0x01F76948);
	wchar_t* NameFormat = reinterpret_cast<wchar_t*>(0x027B303A);
	static std::wstring cmdLog[10]; // feel free to increase or decrease this but make sure to edit all the code below to not break it
	static int cmdN = 0, cmdIndex = -1;

	wcsncpy_s(NameFormat, 16, OpenedByExecutor ? L"%sConsole> %s" : L"%s> %s", 16);

	if (AreWeLoaded == 0x1 && !LobbyCheck == 0x0 && CurrentGamemode == 0xFF) { // If SP/CO-OP allow executor... hopefully.

		if (IsKeyPressed(GameConfig::GetValue("Debug", "ExecutorBind", VK_INSERT), false)) {
			if (hasCheatMessageBeenSeen2 == 1 || Debug::CMPatches_DisableCheatFlag.IsApplied()) {
				if (*IsOpen && OpenedByExecutor) {
					*(BYTE*)(0x2349849) = 1;
					OpenedByExecutor = false;
					IsWaiting = false;
				}
				else if (!*IsOpen && !IsWaiting) {
					IsWaiting = true;
					*(BYTE*)(0x1F76944) = 3;
					chatWindow();
					//*(wchar_t*)(0x1F76948) = 0; // idea was to flush the textbox to allow other keys to be used other than VK_INSERT, seems useless as the game already does this. - Clippy95
					OpenedByExecutor = true;
				}
			}
			if (!Debug::CMPatches_DisableCheatFlag.IsApplied()) {
				if (hasCheatMessageBeenSeen2 == 0) {
					const wchar_t* LUAExeCheat =
						L"The LUA Executor console allows you to do a LOT of things not possible in the vanilla game.\n"
						L"Therefore executing any command will immediately flag your save game as a cheated save.\n\n"
						L"Press Insert again to launch the LUA Executor console.";
					__asm pushad
					AddMessage(L"Juiced", LUAExeCheat);
					__asm popad

					hasCheatMessageBeenSeen2 = 1;
				}
			}
		}

		if (IsKeyPressed(VK_ESCAPE, false)) {
			IsWaiting = false;
			OpenedByExecutor = false;
		}

		if (IsWaiting && IsKeyPressed(VK_RETURN, false)) {
			IsWaiting = false;
			char Arg1[128], Arg2[128];
			float x, y, z;


			std::wstring wstr(ChatInput);
			if (!wstr.empty()) { // no need to add empty strings to the log/history

				if (cmdN < 10) {
					cmdLog[cmdN++] = wstr;
				}
				else {
					for (int i = 1; i < 10; i++) {
						cmdLog[i - 1] = cmdLog[i];
					}
					cmdLog[9] = wstr;
				}
			}

			cmdIndex = -1;
			std::string Converted(wstr.begin(), wstr.end());

			if (sscanf(Converted.c_str(), "spawn_car %s %[^\n]", Arg1, Arg2) >= 1) {
				VehicleSpawner(Arg1, sscanf(Converted.c_str(), "spawn_car %s %[^\n]", Arg1, Arg2) == 1 ? "-1" : Arg2);
			}

			else if (sscanf_s(Converted.c_str(), "tp_player %f %f %f", &x, &y, &z) == 3) {
				tpCoords(x, y, z);
			}

			else if (sscanf_s(Converted.c_str(), "play_as %s", Arg1) == 1) {
				int* Character = General::GetCharacterID(Arg1);
				General::ChangeCharacter(&Character); // pointer to pointer because of **
			}

			else if (Converted == "reset_player") {
				General::ResetCharacter(0,255); // passing 0 to the unknown arg to avoid crashing
			}

			else if (sscanf_s(Converted.c_str(), "spawn_npc %s", Arg1) == 1) {
				General::NPCSpawner(Arg1);
			}

			else if (sscanf_s(Converted.c_str(), "level %s", Arg1) == 1) {
				char* ConsoleString = (char*)0x02345A60;
				strcpy_s(ConsoleString, 128, Arg1);
				LoadLevel();
				ConsoleString = NULL;
			}

			else if (Converted == "delete_npcs") {
				General::YeetAllNPCs();
			}

			else if (Converted == "noclip") {
				ToggleNoclip();
			}
			/*
			else if (Converted == "xtbl") {

				xtbl_parse_test();

			}
			*/
			else {
				LuaExecute(Converted.c_str());
			}
			if (!Debug::CMPatches_DisableCheatFlag.IsApplied()) {
				*(BYTE*)0x02527B5A = 0x1;
				*(BYTE*)0x02527BE6 = 0x1;
			}
		}

		if (IsWaiting) {

			if (IsKeyPressed(VK_CONTROL, true) && IsKeyPressed('V', false)) { // using ctrl + shift for now because either the game or windows = stupid??
				// also using both getasynckeystate & my wrapper to properly check if ctrl is being held while
				// only triggering if the game is in focus
				if (OpenClipboard(nullptr)) {
					HANDLE hData = GetClipboardData(CF_UNICODETEXT);
					if (hData) {
						wchar_t* clipboardInput = static_cast<wchar_t*>(GlobalLock(hData));

						if (clipboardInput) {
							size_t curLength = wcsnlen(ChatInput, 128);
							size_t remSpace = 128 - curLength - 1;

							if (remSpace > 0) {
								wcsncat_s(ChatInput, 128, clipboardInput, remSpace);
							}

							GlobalUnlock(hData);
						}
					}
					CloseClipboard();
				}
			}

			else if (IsKeyPressed(VK_UP, false) && (cmdIndex + 1 < cmdN)) {

				cmdIndex++;
				wmemset(ChatInput, L'\0', 128);
				wcsncpy_s(ChatInput, 128, cmdLog[cmdN - 1 - cmdIndex].c_str(), 127);

			}

			else if (IsKeyPressed(VK_DOWN, false)) {

				if (cmdIndex > 0) {
					cmdIndex--;
					wmemset(ChatInput, L'\0', 128);
					wcsncpy_s(ChatInput, 128, cmdLog[cmdN - 1 - cmdIndex].c_str(), 127);
				}

				else if (cmdIndex == 0) { // this is needed, otherwise you can have infinite negative indexes (unless you limit indexes somewhere)

					cmdIndex--;
					wmemset(ChatInput, L'\0', 128);
				}
			}
		}

		if (!*IsOpen) {
			OpenedByExecutor = false;
			IsWaiting = false;
		}
	}
}
#endif
#if !RELOADED
inline void ModpackWarning(const wchar_t* Warning) {
	__asm pushad
	AddMessage(L"Juiced", Warning);
	__asm popad

}
#endif

typedef int __cdecl RenderLoopStuff_Native();
RenderLoopStuff_Native* UpdateRenderLoopStuff = (RenderLoopStuff_Native*)(0x00C063D0); //0x00BD4A80

#if !RELOADED
static bool modpackread = 0;
#endif
int RenderLoopStuff_Hacked()
{
#if !JLITE
	if (useJuicedOSD) {
		PrintFrametime();
		PrintGameFrametime();
		PrintFramerate();
		if (useJuicedOSD >= 2) {
			PrintCoords(*(float*)0x25F5BB4, *(float*)0x25F5BBC, *(float*)0x25F5BB8, (useJuicedOSD >= 3)); // z = height? even though it's most likely Y since they are in X,Y,Z
			PrintUsername();
			PrintPartnerUsername();
			PrintLatestChunk();
			PrintDBGGarble();
		}
	}

	Game::InLoop::FrameChecks();

	if (RPCHandler::Enabled) 
	{
		RPCHandler::DiscordCallbacks();
		RPCHandler::UpdateDiscordParams();
	}

	if (!ErrorManager::b_HandlerAssigned)
		ErrorManager::AssignHandler();

	if (Debug::addBindToggles)
		UpdateKeys();
	    cus_FrameToggles();
	    Slew();

    #if !RELOADED
		LuaExecutor();
		Noclip();
    #endif
	if (Render3D::VFXP_fixFog)
		FogTest();

	if (Gamespy::coopPausePatch)
		coopPauseLoop();

	if (Debug::LoadLastSave)
		SkipMainMenu();

	if (Render3D::useFPSCam) {
		Render3D::FPSCamHack();
	}

	if (FirstBootCheck()) {
		const wchar_t* JuicedWelcome =
			L"Welcome to [format][color:#B200FF]Juiced[/format]! Thank you for installing the patch.\n"
			L"If you're in need of support, head over to our [format][color:#5864F6]Discord[/format].\n"
			L"- [format][color:#B200FF]Juiced Team[/format]"
			L"[format][scale:1.0][image:ui_hud_inv_d_ginjuice][/format]";
		const wchar_t* Options[] = { L"OK", L"Join the [format][color:#5864F6]Discord[/format]\n\n" };
		const wchar_t* Title = L"Juiced";
		int Result = AddMessageCustomized(Title, JuicedWelcome, Options, _countof(Options));
		*(void**)(Result + 0x930) = &WelcomeCallback;
		FirstBootFlag();
	}
#else // what the fuck is the point of this someone needs to explain it to me because I'm confused??
	if (FirstBootCheck()) {
		const wchar_t* JuicedWelcome =
			L"Welcome to [format][color:#B200FF]Juiced[/format]! Thank you for installing the patch.\n"
			L"If you're in need of support, head over to our [format][color:#5864F6]Discord[/format].\n"
			L"- [format][color:#B200FF]Juiced Team[/format]"
			L"[format][scale:1.0][image:ui_hud_inv_d_ginjuice][/format]";
		const wchar_t* Options[] = { L"OK", L"Join the [format][color:#5864F6]Discord[/format]\n\n" };
		const wchar_t* Title = L"Juiced";
		int Result = AddMessageCustomized(Title, JuicedWelcome, Options, _countof(Options));
		*(void**)(Result + 0x930) = &WelcomeCallback;
		FirstBootFlag();
	}
#endif
	if (Debug::fixFrametime)
	    havokFrameTicker();

	if (Render3D::ARfov)
		AspectRatioFix();

	if (*(uint8_t*)(0x00E87B4F) == 0 && Input::betterTags)
		RawTags();

	if (Render2D::BetterChatTest) {
		LessRetardedChat();
	}

#if !RELOADED
	/*if (!modpackread && GOTR()) {
		ModpackWarning(L"Create a [format][color:#B200FF]gotr.txt[/format] file in the Saints Row 2 directory to stop receiving this message.");
		ModpackWarning(L"[format][color:#B200FF]Juiced[/format] has detected Gentlemen of the Row, while there are no incompatibilities between the 2\n"
			L"it should be noted that Gentlemen of the Row is a mod compilation that overhauls the game and does not improve game stability.\n"
			L"Or fix game [format][color:#FF0000]crashes[/format] on modern operating systems!"
			L"");

			modpackread = true;
	}*/
#endif
	getDeltaTime();

	// Call original func
	return UpdateRenderLoopStuff();
}

void PrintCoords(float x, float z,float y, bool showplayerorient) {
	int baseplayer = UtilsGlobal::getplayer();
	char buffer[90];
	snprintf(buffer, sizeof(buffer), "UserCoords: (X: %.1f, Y: %.1f, Z: %.1f)", x, y, z);
	 if(showplayerorient && baseplayer) {
			float x_player_sin = *(float*)(baseplayer + 0x38);
			float x_player_cos = *(float*)(baseplayer + 0x40);
			float orient_player = atan2(x_player_sin, x_player_cos);
			float xAngle = *(float*)0x025F5B50;
			float zAngle = *(float*)0x025F5B58;
			float orient_camera = atan2(xAngle, zAngle);
		snprintf(buffer, sizeof(buffer), "UserCoords: (X: %.4f, Y: %.4f, Z: %.4f, C: %.4f, P: %.4f)", x, y, z, orient_camera, orient_player);
	}
	Render2D::ChangeTextColor(255, 255, 255, 255);
	__asm pushad
	Render2D::InGamePrint(buffer, 60, Render2D::processtextwidth(0), 6);
	__asm popad
}

void PrintFrametime() {
	char buffer[50];
	int fr = 1.0f / *(float*)(0xE84380);
	int frms = 1.0f / fr * 1000;
	snprintf(buffer, sizeof(buffer), "RenderMS: %i", frms);
	Render2D::ChangeTextColor(255, 255, 255, 255);
	__asm pushad
	Render2D::InGamePrint(buffer, 20, Render2D::processtextwidth(0), 6);
	__asm popad
}

void PrintGameFrametime() {
	char buffer[50];
	int ft = *(float*)(0x02527DA4) * 1000;
	snprintf(buffer, sizeof(buffer), "GameMS: %i", ft);
	Render2D::ChangeTextColor(255, 255, 255, 255);
	__asm pushad
	Render2D::InGamePrint(buffer, 40, Render2D::processtextwidth(0), 6);
	__asm popad
}

void PrintFramerate() {
	char buffer[50];
	int fr = 1.0f / *(float*)(0xE84380);
	snprintf(buffer, sizeof(buffer), "FPS: %i", fr);
	if (fr < 20.0) {
		Render2D::ChangeTextColor(255, 5, 5, 255);
	}
	else {
		if (fr < 35.0) {
			Render2D::ChangeTextColor(255, 255, 0, 255);
			__asm pushad
			Render2D::InGamePrint(buffer, 0, Render2D::processtextwidth(0), 6);
			__asm popad
			return;
		}
		Render2D::ChangeTextColor(255, 255, 255, 255);
	}
	__asm pushad
	Render2D::InGamePrint(buffer, 0, Render2D::processtextwidth(0), 6);
	__asm popad
}
void PrintUsername() {
	BYTE GamespyStatus = *(BYTE*)0x02529334;
	if (GamespyStatus == 4) {
		char buffer[50];
		char* playerName = (CHAR*)0x0212AB48;
		snprintf(buffer, sizeof(buffer), "GS Username: %s", playerName);
		Render2D::ChangeTextColor(255, 255, 255, 255);
		__asm pushad
		Render2D::InGamePrint(buffer, 120, Render2D::processtextwidth(0), 6);
		__asm popad
	}
}

void PrintLatestChunk() {
	char buffer[50];
	char* latestChunk = (CHAR*)0x00EB865C;
	snprintf(buffer, sizeof(buffer), "NewChunkStreamed: % s", latestChunk);
	Render2D::ChangeTextColor(255, 255, 255, 255);
	__asm pushad
	Render2D::InGamePrint(buffer, 80, Render2D::processtextwidth(0), 6);
	__asm popad
}

void PrintDBGGarble() {
	char buffer[50];
	char* dbgg = (CHAR*)0x023460E0;
	snprintf(buffer, sizeof(buffer), "DBGGarble: % s", dbgg);
	Render2D::ChangeTextColor(255, 255, 255, 255);
	__asm pushad
	Render2D::InGamePrint(buffer, 100, Render2D::processtextwidth(0), 6);
	__asm popad
}

std::string wstring_to_string(const std::wstring& wstr) {
	// Create a wide-to-narrow converter
	std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
	return converter.to_bytes(wstr);
}

void PrintPartnerUsername() {
	BYTE GamespyStatus = *(BYTE*)0x02529334;
	if (GamespyStatus == 4) {
		char buffer[50];
		wchar_t* partnerName = (WCHAR*)0x02CD1870;
		std::wstring wPartnerName = partnerName; // parse co-op partner name to a wstring
		std::string f_PartnerName = wstring_to_string(wPartnerName); // THEN to a string
		snprintf(buffer, sizeof(buffer), "Recently played with in CO-OP: %s", f_PartnerName.c_str());
		Render2D::ChangeTextColor(255, 255, 255, 255);
		__asm pushad
		Render2D::InGamePrint(buffer, 140, Render2D::processtextwidth(0), 6);
		__asm popad
	}
}

bool FileExists(const char* fileName) {
	WIN32_FIND_DATAA findFileData;
	HANDLE handle = FindFirstFileA(fileName, &findFileData);
	bool found = (handle != INVALID_HANDLE_VALUE);
	if (found) {
		FindClose(handle);
	}
	return found;
}

int WINAPI Hook_WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	General::TopWinMain();
	ErrorManager::Initialize();

	char NameBuffer[260];
	PIMAGE_DOS_HEADER dos_header;
	PIMAGE_NT_HEADERS nt_header;
	MEMORYSTATUSEX memory_info;
	HMODULE main_handle;

	Logger::TypedLog(CHN_DLL, "Calling Hooked WinMain.\n");

	main_handle = GetModuleHandleA(NULL);
	GetModuleFileNameA(main_handle, NameBuffer, 260);
	Logger::TypedLog(CHN_DLL, "Module name = %s\n", NameBuffer);
	dos_header = (PIMAGE_DOS_HEADER)main_handle;
	nt_header = (PIMAGE_NT_HEADERS)((DWORD)main_handle + dos_header->e_lfanew);


	memory_info.dwLength = sizeof(memory_info);
	GlobalMemoryStatusEx(&memory_info);
	Logger::TypedLog(CHN_DLL, "Memory allocated to process at startup = %I64dMB, memory free = %I64dMB.\n", memory_info.ullTotalVirtual / 1048576, memory_info.ullAvailVirtual / 1048576);

	for (int i = 1; i < *pargc; i++)
	{
		if (!strcmp(pargv[0][i], "keepfpslimit"))
		{
			Logger::TypedLog(CHN_DLL, "keepfpslimit - Keeping GOG FPS limiter.\n");
			keepfpslimit = true;
		}
	}

	// Probably Add SR2 Reloaded patch routines here, used to be OS and FPS patch from Monkey here.

	PatchOpenSpy();
	patch_metrics();

	if (GameConfig::GetValue("Debug", "PatchGameLoop", 1)) //THIS IS REQUIRED FOR RICH PRESENCE AND ERROR HANDLER
	{
		Logger::TypedLog(CHN_DEBUG, "Patching GameRenderLoop...\n");
		patchCall((void*)0x0052050C, (void*)RenderLoopStuff_Hacked); // Patch stuff into Render loop, we could do game loop but this was easier to find, and works i guess.
	}

#if !JLITE
	InternalPrint::Init();
	RPCHandler::Init();

#if !RELOADED
	BlingMenuInstall::AddOptions();
	Debug::PatchDatafiles();
#endif

#endif
	Gamespy::Init();
	Input::Init();
	Behavior::Init();
	Memory::Init();
	Render3D::Init();
	Render2D::Init();
	GLua::Init();
	Audio::Init();
	XACT::Init();
	Debug::Init();

#if RELOADED
	Reloaded::PatchTables();
	Reloaded::Init();
	Behavior::BetterMovement();
#else
	Render2D::InitMenVerNum();
#endif

	SetDefaultGameSettings(); // Set SR2 Reloaded Modernized Default Settings

	if (!keepfpslimit)
		PatchGOGNoFPSLimit();

#if !JLITE
	General::BottomWinMain();
#endif

	// Continue to the program's WinMain.

	WinMain_Type OldWinMain = (WinMain_Type)offset_addr(0x00520ba0);
	return (OldWinMain(hInstance, hPrevInstance, lpCmdLine, nShowCmd));
}

