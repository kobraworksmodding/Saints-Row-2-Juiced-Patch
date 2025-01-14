#include "MainHooks.h"
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
#include "Player/Behavior.h"
#include "Render/Render3D.h"
#include "InternalPrint.h"

#include "GameConfig.h"
#include "iat_functions.h"
#include <chrono>

#include <format>
#include <WinSock2.h>
#include "Xinput.h"
#pragma comment(lib, "Xinput.lib")

float deltaTime;

const char* juicedversion = "7.1.0";

char* executableDirectory[MAX_PATH];
const char ServerNameRL[] = "[RELOADED Mod 1.0.0b4]";
const char ServerNameSR2[] = "[Saints Row 2]";
// - UNUSED
float AOQuality = 0.05;
float AOSmoothness = -4.0;
int ResolutionX = 1920;
int ResolutionY = 1080;
// --------
bool CheatFlagDisabled = 0;

bool lastFrameStates[256];
bool wasPressedThisFrame[256];

void UpdateKeys()
{
	for (int i = 0; i < 256; i++)
	{
		bool thisFrameState = GetKeyState(i) < 0;
		wasPressedThisFrame[i] = thisFrameState && !lastFrameStates[i];
		lastFrameStates[i] = thisFrameState;
	}
}

bool IsSRFocused()
{
	DWORD pid;
	HWND Window = GetForegroundWindow();
	GetWindowThreadProcessId(GetForegroundWindow(), &pid);
	if (Window != ConsoleWindow) {
		return pid == GetCurrentProcessId();
	}
	return false;
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
	if (IsSRFocused())
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
	Logger::TypedLog(CHN_DLL, (" --- Welcome to Saints Row 2 JUICED Version: " + std::string(juicedversion) + " ---\n").c_str());
	Logger::TypedLog(CHN_DLL, "RUNNING DIRECTORY: %s\n", &executableDirectory);
    Logger::TypedLog(CHN_DLL, "LOG FILE CREATED: %s\n", &timeString);
	Logger::TypedLog(CHN_DLL, "--- Based on MonkeyPatch by scanti, additional fixes by Uzis, Tervel, jason098 and Clippy95. ---\n");
    #else
	Logger::TypedLog(CHN_DLL, " --- Welcome to Saints Row 2 RELOADED ---\n");
	Logger::TypedLog(CHN_DLL, "RUNNING DIRECTORY: %s\n", &executableDirectory);
	Logger::TypedLog(CHN_DLL, "LOG FILE CREATED: %s\n", &timeString);
	Logger::TypedLog(CHN_DLL, "--- DLL Based on MonkeyPatch by scanti, additional work by Uzis, Tervel, jason098 and Clippy95. ---\n");
    #endif
	
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
bool isPaused = false;
bool ARfov = 0;
bool ARCutscene = 0;
double FOVMultiplier = 1;
bool betterTags = 0;

typedef int __cdecl PlayerSpin(float a1);
PlayerSpin* UpdatePlayerSpin = (PlayerSpin*)(0x0073FB20); //0x00BD4A80

static bool invert;
int NativeMouse_clothing_store(float a1) {
	// Same implementation as Saints Row 1 Mousehook 
	// expect for vehicle spinning as it's already implemented.

	// Only current issue as of now is that the tickrate of the function we're replacing is quite low so it doesn't detect small mouse movements, 
	// not an issue of the mouse struct as if we move this to the main game loop it's actually quite smooth! also it's pretty slow that it can't even detect scroll wheel :(


	int8_t& busy = *(int8_t*)(0x00E8D57F);
	mouse mouse;
	//float wheeldelta = mouse.getWheeldelta() * 7.5f;
	float deltax = mouse.getXdelta() / 7.5f;
	// Unlike SR1, SR2 doesn't like it when you play with the players rotation directly so clamp the delta otherwise the player gets squished, it still does but not as bad.
	deltax = clamp(deltax, -40.f, 40.f);
	int baseplayer = getplayer();
	if (deltax != 0.f && !busy) {
		float* x_player_cos = (float*)(baseplayer + 0x38);
		float* x_player_sin = (float*)(baseplayer + 0x40);
		float x = atan2(*x_player_sin, *x_player_cos);
		x = RadianstoDegree(x);
		if (invert)
			x += deltax;
		else
			x -= deltax;
		x = fmod(x + 180.0f, 360.0f);
		if (x < 0) x += 360.0f;
		x -= 180.0f;
		x = DegreetoRadians(x);
		*x_player_cos = cos(x);
		*x_player_sin = sin(x);
		/*
		float* zoom_level = (float*)0x00E997E0;
		float zoom = *zoom_level;
		zoom += wheeldelta;
		zoom = clamp(zoom, 0.75f, 3.f);
		*zoom_level = zoom;
		*/
		return 1;
	}
	else
		return UpdatePlayerSpin(a1);


}
static float X_divisor = 30.f;
// Not a proper fix but final outcome should look like this lol, ideally find out why vehicles fuck up the sens when reading
void WorkAroundHorizontalMouseSensitivity() {
	BYTE PlayerStatus = *(BYTE*)0x00E9A5BC;
	enum status {
		vehicle = 3,
		boat = 5,
		helicopter = 6,
		plane = 8,
	};
	switch (PlayerStatus) {
	case vehicle:
	case boat:
	case helicopter:
	case plane:
		X_divisor = 10.f;
		break;
	default:
		X_divisor = 30.f;
		break;
	}


}

void __declspec(naked) WorkAroundHorizontalMouseSensitivityASMHelper() {
	static int jmp_continue = 0x00C13720;

	__asm {
		pushad
		pushfd
	}
	__asm {
		call WorkAroundHorizontalMouseSensitivity
	}
	__asm {
		popfd
		popad
		fdiv ds: X_divisor
		jmp jmp_continue
	}

}

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
		if (currentAR > a169 && ARfov) { // otherwise causes issues for odd ARs like 16:10/5:4 and the common 4:3.
			patchDouble((BYTE*)0x00E5C808, correctFOV);
			patchNop((BYTE*)0x00797181, 6); // Crosshair location that is read from FOV, we'll replace with our own logic below.
			patchFloat((BYTE*)0x00EC2614, correctFOV);
			Logger::TypedLog(CHN_DEBUG, "Aspect Ratio FOV fixed...\n");
			//ARfov = 0;// stop this thread 

			if (ARCutscene) {
				const double currentCFOV = *(double*)0x00e5c3f0; // default 57.2957795131, this is (180 / pi).
				double correctCFOV = currentCFOV * ((double)currentAR / (double)a169);
				if (correctCFOV > 125) {
					correctCFOV = 125; // arbiratry number close to 32:9 CFOV, 
					//this will stop most scenes from going upside down in 48:9, we need a beter address for cutscenes similiar to world FOV.
				}
				patchDouble((BYTE*)0x00e5c3f0, correctCFOV);
				Logger::TypedLog(CHN_DEBUG, "Aspect Ratio Cutscenes (might break above 21:9) hack...\n");
				ARCutscene = 0;

			}
		}
		if (FOVMultiplier >= 1.01 || !ARfov) { // Not mixed above due to 16:10 and 4:3
			double multipliedFOV = (currentAR > a169) ? correctFOV * FOVMultiplier : defaultFOV * FOVMultiplier;
			patchDouble((BYTE*)0x00E5C808, multipliedFOV);
			patchNop((BYTE*)0x00797181, 6);
			patchFloat((BYTE*)0x00EC2614, (float)multipliedFOV);
			ARfov = 0;
		}
		else {
			ARfov = 0;
			ARCutscene = 0;
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

void Slew() {
	float fovSpeed = 15.0f;
	float* camPos = (float*)(0x25F5B20);
	float* camOrient = (float*)(0x25F5B5C);
	float& fov = *(float*)0x25F5BA8;
	float& roll = *(float*)0x33DA350;


	if (slewMode) {

		SlewCamera(camPos, camOrient, deltaTime, 0, false);

		if (isPaused) {
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

		if (IsKeyPressed(VK_UP, true)) {
			fov -= fovSpeed * deltaTime;
			fov = max(fov, 10.0f);
		}

		if (IsKeyPressed(VK_DOWN, true)) {
			fov += fovSpeed * deltaTime;
			fov = min(fov, 120.0f);
		}

		if (IsKeyPressed(0x31, true)) {
			roll = 1.0f;
		}

		else if (IsKeyPressed(0x33, true)) { // number 3 key
			roll = -1.0f;
		}

		else {
			roll = 0.0f;
		}
	}
}

void SlewScrollWheelSmoothing() {
	mouse mouse;
	int wheel_delta = mouse.getWheeldelta();
		if (wheel_delta) {
			float* smoothing = (float*)(0x00E83E1C);
			*smoothing = clamp(*smoothing + ( (float)wheel_delta / 2850.f), 0.f, 1.3f);
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

void havokFrameTicker() { // Proper Frametime ticker by Terval
	float currentFps = *(float*)0x00E84388;
	static DWORD lastTick = 0;

	DWORD currentTick = GetTickCount();

	// Game does calcuate frametime internally we can use that instead of calculating it here.

	float newFrametime;
	if (currentFps > 30.f)
		newFrametime = 1.0f / (currentFps * 2);
	else
		newFrametime = 0.01666666666f;
	float newFrametime2 = 1.0f / currentFps;

	if (currentTick - lastTick >= 100) {
		lastTick = currentTick;

		*(float*)(0x02527DA4) = newFrametime;
		*(float*)(0x027B2C97) = newFrametime2;

		//Logger::TypedLog(CHN_DEBUG, "CurrentFPS: %f, Frametime: %f\n", currentFps, newFrametime);
	}
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

	DWORD old;
	VirtualProtect((LPVOID)0x00AE4BE8, sizeof(int), PAGE_READWRITE, &old);
	*(unsigned char*)(0x00AE4BE8) = VarIndex;
	VirtualProtect((LPVOID)0x00AE4BE8, sizeof(int), old, &old);

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

bool hasCheatMessageBeenSeen = 0;

void cus_FrameToggles() {
	float delay = 0.0f;
	float duration = 1.5f;
	float whateverthefuck = 0.0f;
	static bool uglyMode = false;
	static bool DetachCam = false;
	static bool FPSCounter = false;
	static bool HUDTogg = false;
	static uint8_t ogAA;


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

		*(bool*)(0x252740E) = 1; // Ins Fraud Sound

		slewMode = !slewMode;

		std::wstring subtitles = L"Slew Mode:[format][color:purple]";
		subtitles += slewMode ? L" ON" : L" OFF";
		subtitles += L"[/format]";
		addsubtitles(subtitles.c_str(), delay, duration, whateverthefuck);

		if (*(uint8_t*)(0x2527D14) == 1) {
			*(int*)(0x25F5AE8) = (slewMode ? 2 : 5);
		}
		else {
			*(int*)(0x25F5AE8) = (slewMode ? 2 : 0);
		}
	}

	if (IsKeyPressed(VK_F3, false)) { // F3

		*(bool*)(0x252740E) = 1; // Ins Fraud Sound

		FPSCounter = !FPSCounter;
		std::wstring subtitles = L"Debugging Info:[format][color:purple]";
		subtitles += FPSCounter ? L" ON" : L" OFF";
		subtitles += L"[/format]";
		addsubtitles(subtitles.c_str(), delay, duration, whateverthefuck);
		*(BYTE*)(0x0252734B) = FPSCounter ? 0x1 : 0x0;

	}

	if (IsKeyPressed(VK_F2, false)) { // F2

		*(bool*)(0x252740E) = 1; // Ins Fraud Sound

		HUDTogg = !HUDTogg;
		std::wstring subtitles = L"HUD Toggle:[format][color:purple]";
		subtitles += HUDTogg ? L" ON" : L" OFF";
		subtitles += L"[/format]";
		addsubtitles(subtitles.c_str(), delay, duration, whateverthefuck);
		*(BYTE*)(0x0252737C) = HUDTogg ? 0x0 : 0x1; // why was it 0x1 : 0x0 previously? The game starts off with the HUD on and then you enable something that's already on

	}

	if (IsKeyPressed(VK_F9, false)) { // F9
		FOVMultiplier += 0.1;
		AspectRatioFix();
		Logger::TypedLog(CHN_DEBUG, "+FOV Multiplier: %f,\n", FOVMultiplier);
		GameConfig::SetDoubleValue("Gameplay", "FOVMultiplier", FOVMultiplier);
	}

	if (IsKeyPressed(VK_F8, false)) { // F8
		FOVMultiplier -= 0.1;
		AspectRatioFix();
		Logger::TypedLog(CHN_DEBUG, "-FOV Multiplier: %f,\n", FOVMultiplier);
		GameConfig::SetDoubleValue("Gameplay", "FOVMultiplier", FOVMultiplier);

	}

	if (IsKeyPressed(VK_F5, false)) { // F5
		FLOAT* hkg_playerPosition = (FLOAT*)0x00FA6DB0;
		FLOAT* hkg_camOrient = (FLOAT*)0x025F5B5C; // ???? maybe
		*(bool*)(0x252740E) = 1; // Ins Fraud Sound

		std::wstring subtitles = (L"Player Position & Orient Printed to Console!");
		addsubtitles(subtitles.c_str(), delay, duration, whateverthefuck);
		Logger::TypedLog(CHN_DEBUG, "Player Pos + Orient: <%0.6f %0.6f %0.6f> [%0.6f]\n", hkg_playerPosition[0], hkg_playerPosition[1], hkg_playerPosition[2], hkg_camOrient[0]);
	}

	if (IsKeyPressed(VK_F7, false)) {

		if (hasCheatMessageBeenSeen == 1 || CheatFlagDisabled == 1) {
			if ((*(int*)(0x1F7A418) != 0)) { // check if there's a waypoint
				if (CheatFlagDisabled != 1) {
					*(BYTE*)0x02527B5A = 0x1;
					*(BYTE*)0x02527BE6 = 0x1;
				}
				*(bool*)(0x252740E) = 1; // Ins Fraud Sound
				std::wstring subtitles = (L"Teleported to waypoint!");
				addsubtitles(subtitles.c_str(), delay, duration, whateverthefuck);
				tpCoords(*(float*)0x29B9CD0, *(float*)0x29B9CD4, *(float*)0x29B9CD8);

			}
		}

		if (CheatFlagDisabled != 1) {
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

	if (RPCHandler::IsCoopOrSP == true) 
	{
		if (IsKeyPressed(VK_F6, false)) {
			isPaused = !isPaused;
			isPaused ? pauseGame() : unpauseGame();
		}
	}
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


typedef int* (__thiscall* GetCharacterIDT)(const char* Name);
GetCharacterIDT GetCharacterID = (GetCharacterIDT)0x4A5A90;

typedef int(__cdecl* ChangeCharacterT)(int** a1); // IDA believes it's int* on PC however that didn't work so I copied ** from 360, it can also take a second arg that isn't needed and I've no idea what it'd do
ChangeCharacterT ChangeCharacter = (ChangeCharacterT)0x6856A0;

typedef void(__cdecl* ResetCharacterT)(int a1); // no idea what it expects as the first arg, on 360 I can call it without one and it works but here it dies
ResetCharacterT ResetCharacter = (ResetCharacterT)0x685D50;

typedef int(__thiscall* DeleteNPCT)(int a1, int a2);
DeleteNPCT DeleteNPC = (DeleteNPCT)0x960240;

bool IsSpawning = false;
int CurrentNPC = 0;
int SpawnedNPCs[10] = { 0 }; // we could make this a vector maybe, i don't mind it being like this though

char __declspec(naked) SpawnNPC(int NPCPointer) {
	__asm {
		push ebp
		mov ebp, esp
		sub esp, __LOCAL_SIZE

		mov     eax, NPCPointer
		push	eax
		mov     eax, ds: 0x21703D4
		mov     ecx, 0x98E400
		call    ecx

		mov esp, ebp
		pop ebp
		ret
	}
}

void NPCSpawner(const char* Name) {
	static int Index = 0;
	int CharID = (int)GetCharacterID(Name);
	if (CharID != NULL) {
		IsSpawning = true;
		if (SpawnedNPCs[Index] != 0) {
			DeleteNPC(SpawnedNPCs[Index], 0); // deletes the oldest npc once you go past 10 (feel free to change the max amount)
		}
		SpawnNPC(CharID);
		SpawnedNPCs[Index] = CurrentNPC;
		Index = (Index + 1) % 10;
		IsSpawning = false;
	}
}

void YeetAllNPCs() {
	for (int i = 0; i < 10; i++) {
		if (SpawnedNPCs[i] != 0) {
			DeleteNPC(SpawnedNPCs[i], 0);
			SpawnedNPCs[i] = 0;
		}
	}
}

void __declspec(naked) SpawningCheck()
{
	static int jmp_skip = 0x0098EE3D;
	static int jmp_continue = 0x0098EE11;

	__asm {
		cmp		IsSpawning, 0
		jnz		skip
		mov		eax, [esi + 3132]
		jmp		jmp_continue

		skip :
		mov		edx, [esi + 68]
		jmp		jmp_skip
	}
}

void __declspec(naked) StoreNPCPointer()
{
	static int jmp_continue = 0x0098E498;
	__asm {
		mov		ecx, 0x9CFCE0
		call	ecx
		mov		CurrentNPC, eax
		jmp		jmp_continue
	}
}

bool NoclipEnabled = false;

typedef void(__stdcall* PlayerHolsterT)(int Player, bool Holster);
PlayerHolsterT PlayerHolster = (PlayerHolsterT)0x9661E0;


void ResetYVel() {
	uintptr_t YVelBase = ReadPointer(getplayer(true), { 0x570 });
	float* YVelPositive = (float*)(*(int*)YVelBase + 0x164);
	float* YVelNegative = (float*)(*(int*)YVelBase + 0x144);
	*YVelPositive = 0.0f;
	*YVelNegative = 0.0f;
}

void ToggleNoclip() {
	NoclipEnabled = !NoclipEnabled;
	CollisionTest(getplayer(), 1, 0);
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
	SetAnimState(getplayer(), &AnimState);
	ExitFineAim(getplayer());
	ExitVehicle();

	DWORD old;
	VirtualProtect((LPVOID)0xDD04F4, sizeof(float), PAGE_READWRITE, &old);
	*(float*)0xDD04F4 = NoclipEnabled ? 0.0f : 1.5f;
	VirtualProtect((LPVOID)0xDD04F4, sizeof(float), old, &old);

	StopRagdoll(getplayer(), 0);
	DisableRagdoll(getplayer(), NoclipEnabled ? true : false);
	PlayerHolster(getplayer(), NoclipEnabled ? true : false);

	*(bool*)(0x252740E) = 1; // Ins Fraud Sound
	std::wstring subtitles = L"Noclip:[format][color:purple]";
	subtitles += NoclipEnabled ? L" ON" : L" OFF";
	subtitles += L"[/format]";
	addsubtitles(subtitles.c_str(), 0.0f, 1.5f, 0.0f);
}

bool IsWaiting = false;

void Noclip() {
	int PlayerBase = *(int*)0x21703D4;
	float MovementSpeed = (IsKeyPressed(VK_SHIFT, true) ? 80.0f : 40.0f);
	float xAngle = *(float*)0x025F5B50;
	float yAngle = *(float*)0x025F5B54;
	float zAngle = *(float*)0x025F5B58;
	float* PlayerSin = (float*)(PlayerBase + 0x38);
	float* PlayerCos = (float*)(PlayerBase + 0x40);

	if (NoclipEnabled && !slewMode && !IsWaiting) {
		uintptr_t CoordsPointer = ReadPointer(getplayer(true), { 0x570,0x8,0x40,0x18 });
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

typedef void(*LoadLevelT)();
LoadLevelT LoadLevel = (LoadLevelT)0x73C000;

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
			if (hasCheatMessageBeenSeen2 == 1 || CheatFlagDisabled == 1) {
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
			if (CheatFlagDisabled != 1) {
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
				int* Character = GetCharacterID(Arg1);
				ChangeCharacter(&Character); // pointer to pointer because of **
			}

			else if (Converted == "reset_player") {
				ResetCharacter(0); // passing 0 to the unknown arg to avoid crashing
			}

			else if (sscanf_s(Converted.c_str(), "spawn_npc %s", Arg1) == 1) {
				NPCSpawner(Arg1);
			}

			else if (sscanf_s(Converted.c_str(), "level %s", Arg1) == 1) {
				char* ConsoleString = (char*)0x02345A60;
				strcpy_s(ConsoleString, 128, Arg1);
				LoadLevel();
				ConsoleString = NULL;
			}

			else if (Converted == "delete_npcs") {
				YeetAllNPCs();
			}

			else if (Converted == "noclip") {
				ToggleNoclip();
			}

			else {
				LuaExecute(Converted.c_str());
			}
			if (CheatFlagDisabled != 1) {
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

typedef int __cdecl RenderLoopStuff_Native();
RenderLoopStuff_Native* UpdateRenderLoopStuff = (RenderLoopStuff_Native*)(0x00C063D0); //0x00BD4A80

bool fixFrametime = 0;
bool addBindToggles = 0;
bool coopPausePatch = 0;
bool LoadLastSave = 0;
bool BetterChatTest = 0;

int RenderLoopStuff_Hacked()
{
	if (RPCHandler::Enabled) 
	{
		RPCHandler::DiscordCallbacks();
		RPCHandler::UpdateDiscordParams();
	}
	else 
	{
		RPCHandler::UpdateNoDiscParams();
	}

	if (!ErrorManager::b_HandlerAssigned)
		ErrorManager::AssignHandler();

	if (fixFrametime)
	    havokFrameTicker();

	if (addBindToggles)
		UpdateKeys();
	    cus_FrameToggles();
	    Slew();
		LuaExecutor();
		Noclip();

	if (Render3D::VFXP_fixFog)
	   FogTest();

	if (ARfov)
		AspectRatioFix();

	if (coopPausePatch)
		coopPauseLoop();

	if (LoadLastSave)
		SkipMainMenu();

	if (*(uint8_t*)(0x00E87B4F) == 0 && betterTags)
		RawTags();

	if (BetterChatTest) {
		LessRetardedChat();
	}

	if (Render3D::useFPSCam) {
		Render3D::FPSCamHack();
	}

	if (FirstBootCheck()) {
		const wchar_t* JuicedWelcome =
			L"Welcome to [format][color:#B200FF]Juiced[/format]! Thank you for installing the patch.\n"
			L"If you're in need of support, head over to our [format][color:#5864F6]Discord[/format]:\n\n"
			L"[format][color:#4F9EFF]discord.com/invite/TkurdZJQ[/format]\n\n"
			L"- [format][color:#B200FF]Juiced Team[/format]"
			L"[format][scale:1.0][image:ui_hud_inv_d_ginjuice][/format]";
		__asm pushad
		AddMessage(L"Juiced", JuicedWelcome);
		__asm popad
		FirstBootFlag();
	}
	
	getDeltaTime();

	// Call original func
	return UpdateRenderLoopStuff();
}

typedef float(__cdecl* ChangeTextColorT)(int R, int G, int B, int Alpha);
ChangeTextColorT ChangeTextColor = (ChangeTextColorT)0xD14840;

void __declspec(naked) InGamePrint(const char* Text, int y, int x, int font) {
	__asm {
		push ebp
		mov ebp, esp
		sub esp, __LOCAL_SIZE

		push edi
		push esi
		push eax

		mov edi, font
		mov esi, Text
		push y
		push x

		mov eax, 0xD15DC0
		call eax

		pop eax
		pop esi
		pop edi

		mov esp, ebp
		pop ebp
		ret
	}
}

typedef void SomeMMFunc_Native();
SomeMMFunc_Native* UpdateSomeMMFunc = (SomeMMFunc_Native*)(0x0075B270);

typedef void SomePMFunc_Native();
SomePMFunc_Native* UpdateSomePMFunc = (SomePMFunc_Native*)(0x77A3C0);

void SomeMMFunc_Hacked()
{
	if (*(BYTE*)0x02527B75 == 1 && *(BYTE*)0xE8D56B == 1) {
		ChangeTextColor(160, 160, 160, 128);
		__asm pushad
		InGamePrint(("JUICED " + std::string(juicedversion)).c_str(), 680, 1120, 2);
		__asm popad
	}

	// Call original func
	return UpdateSomeMMFunc();
}

void SomePMFunc_Hacked()
{

	if (menustatus(menustatus::pausemenu) || menustatus(menustatus::pausemenuphone) || menustatus(menustatus::pausemenuscroll2) || menustatus(menustatus::pausemenescroll1) || menustatus(menustatus::pausemenuphonebook)) {
		ChangeTextColor(255, 255, 255, 128);
		__asm pushad
		InGamePrint(("JUICED " + std::string(juicedversion)).c_str(), 690, 1280, 3);
		InGamePrint(("JUICED " + std::string(juicedversion)).c_str(), 690, 1280, 2);
		__asm popad
	}
	// Call original func
	return UpdateSomePMFunc();
}


int(__stdcall* theirbind)(SOCKET, const struct sockaddr_in*, int) = nullptr;

int __stdcall bindWrapper(SOCKET socket, struct sockaddr_in* address, int namelen) {
	address->sin_addr.S_un.S_addr = INADDR_ANY; // bind to all interfaces instead of just one

	int result = theirbind(socket, address, namelen);

	//Logger::TypedLog(CHN_NET, "Redirecting network bind to %i\n", result);

	return result;
}

void patchNetworkBind() { // Binds local ip to any instead of 127.0.0.1 which fucks up if you have more than 1 network adapter.
	uint32_t* bindaddr = reinterpret_cast<uint32_t*>(0x00C1BC76 + 1);

	theirbind = reinterpret_cast<int(_stdcall*)(SOCKET, const struct sockaddr_in*, int)>(reinterpret_cast<intptr_t>(bindaddr) + 4 + *bindaddr);

	patchCall((BYTE*)0x00C1BC76, bindWrapper); // join
	patchCall((BYTE*)0x00D2526F, bindWrapper); // host
	//patchCall((BYTE*)0x0090A779, bindWrapper);
	//patchCall((BYTE*)0x008E7681, bindWrapper);
}

void SetDefaultGameSettings()
{
	patchBytesM((BYTE*)0x00774126, (BYTE*)"\xC6\x05\xAC\xA9\xF7\x01\x00", 7); // Force game into windowed on default settings.
#if RELOADED
	char* GameName = reinterpret_cast<char*>(0x0212AA08);
	strcpy(GameName, (const char*)ServerNameRL);

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

void SetupBorderless()
{
	int l_IsBorderless = GameConfig::GetValue("Graphics", "Borderless", 0);
	uint32_t window_style = l_IsBorderless ? (WS_POPUP) : (WS_CAPTION | WS_BORDER);
	patchDWord((void*)(0x00BFA35A + 4), window_style);
	patchBytesM((BYTE*)0x00BFA494, (BYTE*)"\x6A\x03", 2); //Maximise Borderless so it fits perfectly.
}

void SetupBetterWindowed()
{
	int l_IsBetterWindowed = GameConfig::GetValue("Graphics", "BetterWindowed", 0);
	uint32_t windowed_style = l_IsBetterWindowed ? (WS_CAPTION | WS_OVERLAPPED | WS_SYSMENU | WS_DLGFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SIZEBOX) : (WS_DLGFRAME);
	patchDWord((void*)(0x00BFA35A + 4), windowed_style);
}

void replace_all( // taken from https://stackoverflow.com/questions/5878775/how-to-find-and-replace-string - we could code our own if there's a problem
	std::string& s,
	std::string const& toReplace,
	std::string const& replaceWith
) {
	std::string buf;
	std::size_t pos = 0;
	std::size_t prevPos;

	buf.reserve(s.size());

	while (true) {
		prevPos = pos;
		pos = s.find(toReplace, pos);
		if (pos == std::string::npos)
			break;
		buf.append(s, prevPos, pos - prevPos);
		buf += replaceWith;
		pos += toReplace.size();
	}

	buf.append(s, prevPos, s.size() - prevPos);
	s.swap(buf);
}

int userResX = GetSystemMetrics(SM_CXSCREEN);
int userResY = GetSystemMetrics(SM_CYSCREEN);
std::string patchedRes;

bool resFound = false;

typedef int __cdecl luaLoadBufferOrig_T(void* L, const char* buff, size_t sz, const char* name);
luaLoadBufferOrig_T* luaLoadBufferOrig = (luaLoadBufferOrig_T*)(0xCDCFB0);

int luaLoadBuff(void* L, const char* buff, size_t sz, const char* name) {

	__asm pushad

	std::string convertedBuff(buff);

	int* resX = (int*)(0xE8DF14);
	int* resY = (int*)(0xE8DF4C);

	patchedRes = std::to_string(resX[13]) + "x" + std::to_string(resY[13]);
	std::string searchAA = "adv_antiali_slider_values \t\t\t= { [0] = { label = \"CONTROL_NO\" }, [1] = { label = \"2x\" },\t\t\t\t[2] = { label = \"4x\" },\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\tnum_values = 3, cur_value = 0 }";
	std::string newAA = "adv_antiali_slider_values = { [0] = { label = \"CONTROL_NO\" }, [1] = { label = \"2x\" }, [2] = { label = \"4x\" }, [3] = { label = \"8x\" }, num_values = 4, cur_value = 0 }";
	// removed unnecessary tabs and spaces to make the extra label fit in without breaking the buffer
	std::string sLibSuperUI =
		"audio_play(\"SYS_RACE_FAIL\")\n\t"
		"local error_message = \"attempted to read undefined global variable '\"..k..\"'\"\n\t"
		"debug_print(error_message..\"\\n\")\n\t"
		"mission_help_table(\"[format][color:red]\"..tostring(error_message)..\"[/format]\")\n\t"
		"error(error_message)";

	std::string blankLib(sLibSuperUI.length(), ' ');

	if (buff) {
		for (int i = 0; i < 14; ++i) { // parses the hardcoded array to check if your current resolution exists in it
			if (userResX == resX[i] && userResY == resY[i]) {
				resFound = true;
				break;
			}
		}

		if (!resFound) {
			resX[13] = userResX;
			resY[13] = userResY;
		}

		replace_all(convertedBuff, searchAA, newAA);
		replace_all(convertedBuff, "Fullscreen_Antialiasing", "MSAA                   "); // extra spaces for padding otherwise it'll break the buffer
		replace_all(convertedBuff, "2048x1536", patchedRes); // easier to do it this way than to only patch if the user's res isn't found
		replace_all(convertedBuff, sLibSuperUI, blankLib); // fixes the error logger from SuperUI in system_lib.lua from crashing our executor, if nclok fixes it we'll get rid of this

		if (*(BYTE*)(0xE8C470) == 0) { // only patch these if the game's running in English
			replace_all(convertedBuff, "MENU_BLUR\",\t\t", "Pause Blur\",\t");
			replace_all(convertedBuff, "MENU_DEPTH_OF_FIELD", "Depth of Field     ");
			replace_all(convertedBuff, "ANISOTROPY_FILTERING\",\t\t", "Anisotropic Filtering\",\t");
			replace_all(convertedBuff, "CONTROLS_MINIMAP_VIEW", "Minimap View         ");
			replace_all(convertedBuff, "MENU_VSYNC\",\t\t\t\t\t\t", "Fullscreen VSync\",");
			replace_all(convertedBuff, "Shadow_Maps", "Shadows    ");
		}

		sz = convertedBuff.length();

		strncpy(const_cast<char*>(buff), convertedBuff.c_str(), sz);
		const_cast<char*>(buff)[sz] = '\0';

		__asm popad

		return luaLoadBufferOrig(L, buff, sz, name);
	}
}

void __declspec(naked) MSAA()
{
	static int jmp_continue = 0x007737E4;
	__asm {
		mov ds:dword ptr[0x252A2DC], 0
		sub eax, 1
		jz MSAA8
		jmp jmp_continue

		MSAA8:
		mov ds:dword ptr[0x252A2DC], 8
		jmp jmp_continue
	}
}

void __declspec(naked) MouseFix()
{
	static int jmp_continue = 0x00C1F4F2;
	__asm {
		mov ds : dword ptr[0x234F460], eax
		mov ds : dword ptr[0x0347B2F4], eax // reset the missing old delta to fix ghost scrolling when tabbing in and out of the game
		jmp jmp_continue
	}
}

BOOL __declspec(naked) ValidCharFix()
{
	static int jmp_continue = 0x0075C8D5;
	static int jmp_xor = 0x0075C8E7;

	__asm {
		mov ax, [esp + 4]
		cmp ax, 0x20
		jb short skip
		jmp jmp_continue

		skip:
		jmp jmp_xor
	}
}


typedef void __cdecl HudControlT(bool Hide);
HudControlT* HudControl = (HudControlT*)(0x793D60);

void IdleFix(bool Hide) {

	patchByte((BYTE*)0x004F81EE, Hide ? 0x00 : 0x32);
	patchByte((BYTE*)0x004F81CE, Hide ? 0x00 : 0x33);

	return HudControl(Hide);
}

typedef int __cdecl TextureTestT(int idk1, int idk2);
TextureTestT* TextureTest = (TextureTestT*)(0xC080C0);

int TextureCrashFix(int idk1, int idk2) {

	__asm pushad
	idk1 = *(int*)(idk2); // making the first arg be the same as the second seems to not break the game and could maybe fix the crash
	__asm popad
	return TextureTest(idk1, idk2);
}

bool IsControllerConnected(int controllerIndex)
{
	XINPUT_CAPABILITIES capabilities;
	return XInputGetCapabilities(controllerIndex, 0, &capabilities); // perhaps a little more lightweight than GetState?
}

typedef int(__stdcall* XInputEnableT)(bool Enable); // this is a deprecated feature and I couldn't get it to register through the XInput lib
XInputEnableT XInputEnable = (XInputEnableT)0x00CCD4F8;

int controllerConnected[4] = { 1, 1, 1, 1 };
bool NoControllers = false;
bool XInputEnabled = true;

void __declspec(naked) RestoreFiltering()
{
	static int jmp_continue = 0x00515974;
	__asm {
		fstp st(1)
		push esp
		add dword ptr[esp], 8
		fstp st
		jmp jmp_continue
	}
}

DWORD WINAPI XInputCheck(LPVOID lpParameter)
{
	while (true) {
		for (int i = 0; i < 4; ++i)
		{
			controllerConnected[i] = IsControllerConnected(i);
		}

		for (int i = 0; i < 4; ++i) {
			if (controllerConnected[i] == 0) {
				NoControllers = false;
				break;
			}
			else {
				NoControllers = true;
			}
		}

		bool inFocus = IsSRFocused(); // calling it less = better for performance; local variable

		static bool focusedLast = true;


		if (inFocus && NoControllers && XInputEnabled) {
			XInputEnable(false);
			XInputEnabled = false;
		}

		else if (inFocus && !NoControllers && !XInputEnabled) {
			XInputEnable(true);
			XInputEnabled = true;
		}

		else if (focusedLast && !inFocus) {
			XInputEnabled = false;
			// we set to false so it knows to re-enable but we do nothing else as we let the game flush for us out of focus (which disables XInput)
		}

		focusedLast = inFocus;

		SleepEx(500, 0); // feel free to decrease or increase; using SleepEx to make it independent from our third sleep hack
	}
	return 0;
}

int WINAPI Hook_WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	patchBytesM((BYTE*)0x004CBFEE, (BYTE*)"\xD9\x05\x00\x5C\x5F\x02", 6); // change the motion blur to directly read the current frametime (fix strength above 30 fps)
	patchBytesM((BYTE*)0x004CBFF4, (BYTE*)"\xEB\x13", 2); // jump over the stupid checks
	patchBytesM((BYTE*)0x0053818F, (BYTE*)"\xA1\x94\x89\xE9\x00", 5); // make shadow maps check shadows instead of shadow map type
	patchBytesM((BYTE*)0x00538194, (BYTE*)"\x83\xE8\x02", 3); // make it check if full shadows are enabled (so none = no shadows, simple = stencil and full = stencil & s. maps)
	patchNop((BYTE*)0x0077376D, 3); // force full stencil shadows with the simple setting, removes the == 2 check
	patchNop((BYTE*)0x006C5FE0, 10); // fix cutscenes resetting shadows
	patchNop((BYTE*)0x0073C01B, 6); // remove the command check from the level function
	patchCall((void*)0x00458646, (void*)IdleFix); // prevents you from being able to use the scroll wheel when idling
	patchCall((void*)0x009A3D8E, (void*)IdleFix);
	patchCall((void*)0x00C0900D, (void*)TextureCrashFix); // WIP (unknown if it fixes it or not just yet)
	patchCall((void*)0x00C08493, (void*)TextureCrashFix);
	if (GameConfig::GetValue("Debug", "PatchPauseMenuLua", 1))
	patchCall((void*)0x00CD9FE8, (void*)luaLoadBuff); // used to intercept the pause menu lua before compiled, needed for full 8x MSAA support + custom res
	WriteRelJump(0x007737DA, (UInt32)&MSAA); // 8x MSAA support; requires modded pause_menu.lua but won't cause issues without
	WriteRelJump(0x0075C8D0, (UInt32)&ValidCharFix); // add check for control keys to avoid pasting issues in the executor
	WriteRelJump(0x00C1F4ED, (UInt32)&MouseFix); // fix ghost mouse scroll inputs when tabbing in and out
	if (GameConfig::GetValue("Gameplay", "FixHorizontalMouseSensitivity", 1))
	WriteRelJump(0x00C1371A, (UInt32)&WorkAroundHorizontalMouseSensitivityASMHelper); // attempt to fix Horizontal sens being 3x faster compared to vertical while on foot
	FixandImproveSlewMouseRuntimePatch();
	WriteRelJump(0x0098E493, (UInt32)&StoreNPCPointer);
	WriteRelJump(0x0098EE0B, (UInt32)&SpawningCheck);
	WriteRelJump(0x0051596E, (UInt32)&RestoreFiltering);
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

	// LUA EXECUTE
	patchBytesM((BYTE*)0x0075D5D6, (BYTE*)"\x68\x3A\x30\x7B\x02", 5);
	patchBytesM((BYTE*)0x0075D5B5, (BYTE*)"\x68\x3A\x30\x7B\x02", 5);

	// Adds Clan tag to name
	if (GameConfig::GetValue("Multiplayer", "FixNetworkBinding", 1))
	{
		Logger::TypedLog(CHN_NET, "Fixing Network Adapter Binding...\n");
		patchNetworkBind();
	}

	PatchOpenSpy();

	patch_metrics();

	InternalPrint::Init();
	Behavior::Init();
	Memory::Init();
	Render3D::Init();

	// Experiment to give CO-OP a higher player limit.
	//patchByte((BYTE*)0x007F750D + 1, 0x01); // max gb count (we'll use this for modified co-op)
	//patchDWord((void*)(0x0086ACF5 + 6), 12); // overwrite the co-op maxplayerslobby to 12
	//patchBytesM((BYTE*)0x007F7095, (BYTE*)"\xBD\xC", 2);
	//patchNop((BYTE*)0x007F71EC, 12); // nop coop max and max lobby
	//patchBytesM((BYTE*)0x007F7A31, (BYTE*)"\xB8\x02", 2);
	//patchBytesM((BYTE*)0x0051E502, (BYTE*)"\x68\x00\x60\x77\x01", 5);
	//patchBytesM((BYTE*)0x00826B8B, (BYTE*)"\xB8\xC", 2);
	//patchBytesM((BYTE*)0x0051E535, (BYTE*)"\x68\xB0\xEB\x92\x00", 5);
	//patchBytesM((BYTE*)0x0051E8A7, (BYTE*)"\x68\00\x70\x24\x0B", 5);

	//0051E502

#if RELOADED

	Reloaded::PatchTables();
	Reloaded::Init();
	Behavior::BetterMovement();

#else

	if (GameConfig::GetValue("Debug", "MenuVersionNumber", 1))
	{
		Logger::TypedLog(CHN_MOD, "Patching MenuVersionNumber...\n");
		//patchCall((void*)0x0052050C, (void*)SomeMMFunc_Hacked);
		patchCall((void*)0x0073CE0D, (void*)SomeMMFunc_Hacked);
		if (2 <= GameConfig::GetValue("Debug", "MenuVersionNumber", 1))
			SafeWrite32(0x00E8D060, (UInt32)&SomePMFunc_Hacked);
	}

	if (GameConfig::GetValue("Gameplay", "LoadLastSave", 0)) // great for testing stuff faster and also for an optional feature in gen
	{
		LoadLastSave = 1;
		Logger::TypedLog(CHN_DEBUG, "Skipping main menu...\n");
	}

#endif

	//Logger::TypedLog(CHN_DEBUG, "Increasing Customization Memory...\n");
    // Hopefully increase customization_items.xtbl limit from 1050 items to 1150
    //patchBytesM((BYTE*)0x007BBAC6 + 1, (BYTE*)"\x7E\x04", 2);
    //patchBytesM((BYTE*)0x007BCC14 + 6, (BYTE*)"\x7E\x04", 2);


	if (GameConfig::GetValue("Debug", "DisableXInput", 0))
	{
		patchBytesM((BYTE*)0x00BFA090, (BYTE*)"\x6A\x00", 2);
		patchBytesM((BYTE*)0x00BFA0C8, (BYTE*)"\x6A\x00", 2);
		Logger::TypedLog(CHN_DEBUG, "XInput Disabled.\n");
	}

	else { // if XInput is not disabled completely, we just force our fix
		patchNop((BYTE*)0x00BFA090, 2);
		patchNop((BYTE*)0x00BFA099, 5);
		patchNop((BYTE*)0x00BFA0C8, 2);
		patchNop((BYTE*)0x00BFA0CA, 5);
		CreateThread(0, 0, XInputCheck, 0, 0, 0);
	}

	if (GameConfig::GetValue("Debug", "ForceDisableVibration", 0)) // Fixes load/new save insta-crash due to broken / shitty joystick drivers.
	{
		patchBytesM((BYTE*)0x00C14930, (BYTE*)"\xC3\x00", 2);
		Logger::TypedLog(CHN_DEBUG, "Vibration Forced to OFF.\n");
	}


	if (GameConfig::GetValue("Audio", "UseFixedXACT", 1)) // Scanti the Goat
	{
		// Forces the game to use a newer version of XACT which in turn fixes all of the audio issues
		// in SR2 aside from 3D Panning.
		GUID xaudio = { 0x4c5e637a, 0x16c7, 0x4de3, 0x9c, 0x46, 0x5e, 0xd2, 0x21, 0x81, 0x96, 0x2d };        // version 2.3
		GUID ixaudio = { 0x8bcf1f58, 0x9fe7, 0x4583, 0x8a, 0xc6, 0xe2, 0xad, 0xc4, 0x65, 0xc8, 0xbb };
		SafeWriteBuf((0x00DD8A08), &xaudio, sizeof(xaudio));
		SafeWriteBuf((0x00DD8A18), &ixaudio, sizeof(ixaudio));
		Logger::TypedLog(CHN_MOD, "Forcing the use of a fixed XACT version.\n");
	}

	if (GameConfig::GetValue("Gameplay", "FixUltrawideFOV", 1))
	{
		ARfov = 1;
	}

	if (GameConfig::GetValue("Gameplay", "FixUltrawideCutsceneFOV", 1))
	{
		ARCutscene = 1;
	}

	if (GameConfig::GetValue("Gameplay", "DisableCheatFlag", 0))
	{
		patchNop((BYTE*)0x00687e12, 6);
		patchNop((BYTE*)0x00687e18, 6);
		CheatFlagDisabled = 1;
	}

	if (GameConfig::GetDoubleValue("Gameplay", "FOVMultiplier", 1.0)) // 1.0 isn't go anywhere.
	{
		FOVMultiplier = GameConfig::GetDoubleValue("Gameplay", "FOVMultiplier", FOVMultiplier);
		if (FOVMultiplier > 1.0) {
			ARfov = 1;
			Logger::TypedLog(CHN_DEBUG, "Applying FOV Multiplier.\n");
		}
		patchBytesM((BYTE*)0x00AA5648, (BYTE*)"\xDC\x0D\x9B\x2C\x7B\x02", 6); // patch vehicle turning radius, this read from the FOV and the radius gets smaller if FOV is lower than 4/3
		*(double*)(0x027B2C9B) = 1.333333373069763; // code / data cave for wheel radius to read from.
		Logger::TypedLog(CHN_DEBUG, "FOV Multiplier: %f,\n", FOVMultiplier);
	}

	if (GameConfig::GetValue("Multiplayer", "NewLobbyList", 1))
	{
		char newLobby1[MAX_PATH];
		char newLobby2[MAX_PATH];

		Logger::TypedLog(CHN_DEBUG, "Changing Lobby List...\n");

		GameConfig::GetStringValue("Multiplayer", "Lobby1", "sr2_mp_lobby03", newLobby1);
		GameConfig::GetStringValue("Multiplayer", "Lobby2", "sr2_mp_lobby02", newLobby2);

		Logger::TypedLog(CHN_DEBUG, "Lobby Map 1 Found: %s\n", newLobby1);
		Logger::TypedLog(CHN_DEBUG, "Lobby Map 2 Found: %s\n", newLobby2);
		Reloaded::lobby_list[0] = newLobby1;
		Reloaded::lobby_list[1] = newLobby2;

		patchDWord((void*)(0x0073EABA + 3), (int)&Reloaded::lobby_list);
		patchDWord((void*)(0x0073EA0B + 3), (int)&Reloaded::lobby_list);
		patchDWord((void*)(0x007E131A + 3), (int)&Reloaded::lobby_list);
		patchDWord((void*)(0x007E161E + 3), (int)&Reloaded::lobby_list);
		patchDWord((void*)(0x007E7670 + 3), (int)&Reloaded::lobby_list);
		patchDWord((void*)(0x007E774F + 3), (int)&Reloaded::lobby_list);
		patchDWord((void*)(0x0082F2E9 + 3), (int)&Reloaded::lobby_list);
		patchDWord((void*)(0x0082F4CC + 3), (int)&Reloaded::lobby_list);
		patchDWord((void*)(0x00842497 + 3), (int)&Reloaded::lobby_list);
	}

	if (GameConfig::GetValue("Multiplayer", "FreeMPClothing", 1))
	{
		Logger::TypedLog(CHN_DEBUG, "Making MP Clothing Free...\n");
		patchNop((BYTE*)0x0088DDBD, 5); // nop the float call from xtbl > exe that parses clothing prices.
	}

	if (GameConfig::GetValue("Debug", "FixFrametime", 1))
	{
		Logger::TypedLog(CHN_DEBUG, "Fixing Frametime issues...\n");
		fixFrametime = 1;
	}

	if (GameConfig::GetValue("Debug", "FixAudio", 0))
	{
		Logger::TypedLog(CHN_DEBUG, "Patching Stereo Cutscenes (EXPERIMENTAL)...\n");
		RPCHandler::ShouldFixStereo = true;
	}

	if (GameConfig::GetValue("Debug", "AddBindToggles", 1))
	{
		Logger::TypedLog(CHN_DEBUG, "Adding Custom Key Toggles...\n");
		addBindToggles = 1;
		patchNop((BYTE*)0x0051FEB0, 7); // nop to prevent the game from locking the camera roll in slew
		patchBytesM((BYTE*)0x00C01B52, (BYTE*)"\xD9\x1D\xF8\x2C\x7B\x02", 6); // slew roll patch, makes the game write to a random unallocated float instead to prevent issues
		patchBytesM((BYTE*)0x00C01AC8, (BYTE*)"\xDC\x64\x24\x20", 4); // invert Y axis in slew 
	} 

	if (GameConfig::GetValue("Debug", "LUADebugPrintF", 1)) // Rewrites the DebugPrint LUA function to our own.
	{
		Logger::TypedLog(CHN_DEBUG, "Re-writing Debug_Print...\n");
		copyFunc((uint32_t)0x00D74BA0, 0x00D74BD8, HookedDebugPrint); // Overwrite vanilla debug_print LUA function with ours.
	}

	if (GameConfig::GetValue("Debug", "PatchGameLoop", 1)) //THIS IS REQUIRED FOR RICH PRESENCE AND ERROR HANDLER
	{
		Logger::TypedLog(CHN_DEBUG, "Patching GameRenderLoop...\n");
		patchCall((void*)0x0052050C, (void*)RenderLoopStuff_Hacked); // Patch stuff into Render loop, we could do game loop but this was easier to find, and works i guess.
	}

	SetDefaultGameSettings(); // Set SR2 Reloaded Modernized Default Settings

	if (GameConfig::GetValue("Graphics", "BetterWindowed", 0) && (!GameConfig::GetValue("Graphics", "Borderless", 0)))
	{
		SetupBetterWindowed();
		Logger::TypedLog(CHN_DEBUG, "Fixing Windowed Mode.\n");
	}
	else
	{
		SetupBorderless();
		Logger::TypedLog(CHN_DEBUG, "Enabling Borderless Windowed.\n");
	}

	RPCHandler::Enabled = GameConfig::GetValue("Misc", "RichPresence", 0);

	if (RPCHandler::Enabled)
	{
		Logger::TypedLog(CHN_RPC, "Attempting to initialize Discord RPC...\n");
		RPCHandler::InitRPC();
		//RPCHandler::DiscordCallbacks(); callbacks needs to be hooked into a game loop
	}

	if (!keepfpslimit)
		PatchGOGNoFPSLimit();

	// VFX+ already disables bloom + this does it incorrectly which breaks the item glow
	/*if (GameConfig::GetValue("Graphics", "RemoveBloom", 0)) // Removes a shader call to create bloom on bright objects.
	{
		Logger::TypedLog(CHN_MOD, "Removing Bloom...\n");
		patchNop((BYTE*)0x005174FA, 30);
		patchNop((BYTE*)0x005178F6, 25); // hopefully this new entry works on shitty nvidia gpus
	}*/

	if (GameConfig::GetValue("Gameplay", "DisableAimAssist", 0))
	{
		Logger::TypedLog(CHN_MOD, "Disabling Aim Assist...\n");
		patchNop((BYTE*)0x00E3CC80, 16); // nop aim_assist.xtbl
	}

	if (GameConfig::GetValue("Audio", "FixAudioDeviceAssign", 1))
	{
		Logger::TypedLog(CHN_MOD, "Fixing Audio Device Assignment.\n");
		// fixes, or attempts to fix the incorrect GUID assigning for BINK related stuff in SR2.
		patchBytesM((BYTE*)0x00DBA69C, (BYTE*)"\x00\x00\x00\x00", 4);
	}

	if (GameConfig::GetValue("Graphics", "RemoveBlackBars", 0)) // Another Tervel moment
	{
		Logger::TypedLog(CHN_DLL, "Removing Black Bars.\n");
		patchNop((BYTE*)(0x0075A265), 5);
	}

	if (GameConfig::GetValue("Gameplay", "SkipIntros", 0)) // can't stop Tervel won't stop Tervel
	{
		Logger::TypedLog(CHN_DLL, "Skipping intros & legal disclaimers.\n");
		patchNop((BYTE*)(0x005207B4), 6); // prevent intros from triggering
		patchBytesM((BYTE*)0x0068C740, (BYTE*)"\x96\xC5\x68\x00", 4); // replace case 0 with case 4 to skip legal disclaimers
	}

	// Disabled by default FOR NOW, too many issues arise when dealing with missions. Otherwise it works beautifully in freeroam.
	if (GameConfig::GetValue("Gameplay", "coopPausePatch", 1)) // Tervel W streak
	{
		Logger::TypedLog(CHN_DEBUG, "Disabling CO-OP pause...\n");
		coopPausePatch = 1;
		patchNop((BYTE*)0x00779C5E, 5); // Prevent the game from pausing your co-op partner if you open the pause menu.
		patchBytesM((BYTE*)0x0068CA79, (BYTE*)"\x83\x3D\xF6\x2C\x7B\x02\x00", 7); // new pause check address
		patchBytesM((BYTE*)0x00BF0A1B, (BYTE*)"\x83\x3D\xF6\x2C\x7B\x02\x00", 7); // new particle pause check address
		patchBytesM((BYTE*)0x00BDCFFD, (BYTE*)"\x83\x3D\xF6\x2C\x7B\x02\x00", 7);  // new particle pause check address 2
		patchBytesM((BYTE*)0x006B793F, (BYTE*)"\x83\x3D\xF6\x2C\x7B\x02\x00", 7);  // new particle pause check address 3
	}

	if (GameConfig::GetValue("Audio", "51Surround", 0) == 1)
	{
		Logger::TypedLog(CHN_AUDIO, "Using 5.1 Surround Sound...\n");
	}
	else {
		Logger::TypedLog(CHN_AUDIO, "Fixing Stereo Audio...\n");
		UINT32 number_of_speakers = 2;
		UINT32 frequency = 48000;

		//SafeWrite8(0x004818E3, number_of_speakers);         // Causes major audio glitches
		SafeWrite8(0x00482B08, number_of_speakers);
		SafeWrite8(0x00482B41, number_of_speakers);
		SafeWrite8(0x00482B96, number_of_speakers);

		SafeWrite32(0x00482B03, frequency);
		SafeWrite32(0x00482B3C, frequency);
		SafeWrite32(0x00482B91, frequency);
	}

	if (GameConfig::GetValue("Gameplay", "BetterChat", 1)) // changes char limit from 64 to 128 and formats the input after the 64th character
	{
		BetterChatTest = 1;
		patchBytesM((BYTE*)0x0075C91E, (BYTE*)"\xC7\x05\x1C\x69\xF7\x01\x80\x00\00\x00", 10); // change chat char limit from 64 to 128
		patchBytesM((BYTE*)0x0075CCF7, (BYTE*)"\x6A\x82", 2);  // change chat print limit from 64 to 130 (extra 2 characters to account for formatted input with - and newline)
		patchBytesM((BYTE*)0x0075CDEA, (BYTE*)"\x68\xFF\x92\x20\x02", 5); // new chat read address for entered message
		Logger::TypedLog(CHN_DEBUG, "Enabling better chat...\n");
	}

	if (GameConfig::GetValue("Gameplay", "TagsHook", 1))
	{
		betterTags = 1;
		patchNop((BYTE*)0x006221AA, 6); // Original stores for Tags, X and Y.
		patchNop((BYTE*)0x00622189, 6);
		Logger::TypedLog(CHN_DEBUG, "Replaced Tags controls with BetterTags\n");
	}

	if (uint8_t direction = GameConfig::GetValue("Gameplay", "NativeMousetoPlayerWardrobe", 0))
	{
		if (direction >= 2) { // invert player rotation in wardrobe if 2 or above.
			invert = true;
		}
		patchCall((int*)0x007CE170, (int*)NativeMouse_clothing_store);
		Logger::TypedLog(CHN_DEBUG,
			"Native mouse to player rotation in clothing store menus: %s.\n",
			invert ? "inverted direction" : "normal rotation");
	}

	if (GameConfig::GetValue("Gameplay", "SwapScrollWheel", 0))
	{
		// TODO: maybe have optional options for when to swap scroll? like map only or weapon wheel only, mouse function is done at 0x00C1F320.

		patchBytesM((BYTE*)0x00C1F0F7, (BYTE*)"\x29", 1); // opcode for sub, add previously.
	}
	patchNop((BYTE*)0x004D6795, 5); // Fix for the sun flare disappearing upon reloading a save. Prevents the game from deallocating the flare.

#if !RELOADED
	if (CreateCache("loose.txt"))
	{
		CacheConflicts();
		patchJmp((void*)0x0051DAC0, (void*)hook_loose_files);						// Allow the loading of loose files
		patchCall((void*)0x00BFD8F5, (void*)hook_raw_get_file_info_by_name);		// Add optional search in the ./loose directory
	}
	else
		Logger::TypedLog(CHN_DLL, "Create loose file cache failed.\n");

#endif

	// Continue to the program's WinMain.

	patchBytesM((BYTE*)0x009D3C70, (BYTE*)"\xD9\x05\x5A\x30\x7B\x02", 6); // TP X
	patchBytesM((BYTE*)0x009D3C83, (BYTE*)"\xD9\x05\x5E\x30\x7B\x02", 6); // TP Y
	patchBytesM((BYTE*)0x009D3CAE, (BYTE*)"\xD9\x05\x62\x30\x7B\x02", 6); // TP Z
	patchBytesM((BYTE*)0x00BE1B50, (BYTE*)"\xC3", 1); // return - avoid crashing from the unused broken debug console variable checker
	patchNop((BYTE*)0x009D3C65, 2); // nop out the command check so TP works without it


	patchBytesM((BYTE*)0x0068579B, (BYTE*)"\x6A\x06", 2);
	patchBytesM((BYTE*)0x006857CB, (BYTE*)"\x6A\x06", 2);
	patchBytesM((BYTE*)0x0068571F, (BYTE*)"\x6A\x06", 2);
	patchBytesM((BYTE*)0x0068574F, (BYTE*)"\x6A\x06", 2);
	patchBytesM((BYTE*)0x00685E12, (BYTE*)"\x6A\x06", 2);
	patchBytesM((BYTE*)0x00685E1E, (BYTE*)"\x6A\x06", 2);
	patchBytesM((BYTE*)0x00685DC7, (BYTE*)"\x6A\x06", 2);
	patchBytesM((BYTE*)0x00687C32, (BYTE*)"\x6A\x06", 2);
	patchBytesM((BYTE*)0x00687BC2, (BYTE*)"\x6A\x06", 2);
	patchBytesM((BYTE*)0x00687CAB, (BYTE*)"\x6A\x06", 2);
	// this should increase the stream priority for the character swap cheat - on 360, the loading times are much bigger so there are no issues there but here this might be needed
	patchNop((BYTE*)0x00684C84, 5); // get rid of the loading screen with the cheat, remove the nop if there are any issues but it should be fine?

	WinMain_Type OldWinMain=(WinMain_Type)offset_addr(0x00520ba0);
	return (OldWinMain(hInstance, hPrevInstance, lpCmdLine,nShowCmd));
}

