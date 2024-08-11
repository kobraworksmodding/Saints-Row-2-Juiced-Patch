#include "MainHooks.h"
#include "FileLogger.h"
#include "SafeWrite.h"
#include "Patches/All Patches.h"
#include "Patcher/patch.h"
#include "RPCHandler.h"
#include "ErrorManager.h"
#include "LuaHandler.h"

#include "GameConfig.h"
#include <chrono>

#include <format>

char* executableDirectory[MAX_PATH];
const char mus2xtbl[] = "music2.xtbl";
const char ServerNameRL[] = "[SR2 RELOADED]";
//char ServerNameJUI = "[JUICED]";


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
	Logger::TypedLog(CHN_DLL, " --- Welcome to Saints Row 2 JUICED Version: 5.1.1 ---\n");
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
		UInt32 winmaindata=*((UInt32*)0x00520ba0);
		if(winmaindata==0x83ec8b55)
		{
			// The Steam version of the executable is now unencrypted, so we can start patching.


			Logger::TypedLog(CHN_DLL, "Hooking WinMain.\n");
			WriteRelCall(0x00c9e1c0,(UInt32)&Hook_WinMain);

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

typedef int(__fastcall* pauseT)();
pauseT pause = (pauseT)0x00502A70;

typedef void(__cdecl* unpauseT)();
unpauseT unpause = (unpauseT)0x00503090;

typedef int(__cdecl* slewleftoverT)();
slewleftoverT slewleftover = (slewleftoverT)0x00501220;

bool slew = false;
bool pausetest = false;
bool ARfov = 0;
bool ARCutscene = 0;
double FOVMultiplier = 1;

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
				const double currentCFOV = *(double*)0x00e5c3f0;
				double correctCFOV = currentCFOV * ((double)currentAR / (double)a169);
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

float getDeltaTime() {
	static DWORD lastTime = GetTickCount();
	DWORD currentTime = GetTickCount();
	DWORD elapsedMillis = currentTime - lastTime;
	lastTime = currentTime;
	return elapsedMillis / 1000.0f;
}

void slewtest() {
	float deltaTime = getDeltaTime();
	float fovSpeed = 15.0f;
	float& fov = *(float*)0x25F5BA8;

	if (slew) {
		slewleftover();

		if (GetAsyncKeyState(VK_UP) & 0x8000) {
			fov -= fovSpeed * deltaTime;
			fov = max(fov, 10.0f);
		}
		if (GetAsyncKeyState(VK_DOWN) & 0x8000) {
			fov += fovSpeed * deltaTime;
			fov = min(fov, 120.0f);
		}
	}
}

void havokFrameTicker() { // Proper Frametime ticker by Terval
	float currentFps = *(float*)0x00E84388;
	static DWORD lastTick = 0;

	DWORD currentTick = GetTickCount();

	float newFrametime = 1.0f / (currentFps * 2);
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

void cus_FrameToggles() {
	float delay = 0.0f;
	float duration = 1.5f;
	float whateverthefuck = 0.0f;
	static bool uglyMode = false;
	static bool DetachCam = false;
	static bool FPSCounter = false;
	static bool HUDTogg = false;
	static uint8_t ogAA;


	if (GetAsyncKeyState(VK_F1) & 1) { // F1

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

	if (GetAsyncKeyState(VK_F4) & 1) { // F4

		*(bool*)(0x252740E) = 1; // Ins Fraud Sound

		slew = !slew;

		std::wstring subtitles = L"Slew Mode:[format][color:purple]";
		subtitles += slew ? L" ON" : L" OFF";
		subtitles += L"[/format]";
		addsubtitles(subtitles.c_str(), delay, duration, whateverthefuck);

		if (*(uint8_t*)(0x2527D14) == 1) {
			*(int*)(0x25F5AE8) = (slew ? 2 : 5);
		}
		else {
			*(int*)(0x25F5AE8) = (slew ? 2 : 0);
		}
	}

	if (GetAsyncKeyState(VK_F3) & 1) { // F3

		*(bool*)(0x252740E) = 1; // Ins Fraud Sound

		FPSCounter = !FPSCounter;
		std::wstring subtitles = L"Debugging Info:[format][color:purple]";
		subtitles += FPSCounter ? L" ON" : L" OFF";
		subtitles += L"[/format]";
		addsubtitles(subtitles.c_str(), delay, duration, whateverthefuck);
		*(BYTE*)(0x0252734B) = FPSCounter ? 0x1 : 0x0;

	}

	if (GetAsyncKeyState(VK_F2) & 1) { // F2

		*(bool*)(0x252740E) = 1; // Ins Fraud Sound

		HUDTogg = !HUDTogg;
		std::wstring subtitles = L"HUD Toggle:[format][color:purple]";
		subtitles += HUDTogg ? L" ON" : L" OFF";
		subtitles += L"[/format]";
		addsubtitles(subtitles.c_str(), delay, duration, whateverthefuck);
		*(BYTE*)(0x0252737C) = HUDTogg ? 0x1 : 0x0;

	}

	if (GetAsyncKeyState(VK_F9) & 1) { // F9
		FOVMultiplier += 0.1;
		AspectRatioFix();
		Logger::TypedLog(CHN_DEBUG, "+FOV Multiplier: %f,\n", FOVMultiplier);
		GameConfig::SetDoubleValue("Gameplay", "FOVMultiplier", FOVMultiplier);
	}

	if (GetAsyncKeyState(VK_F8) & 1) { // F8
		FOVMultiplier -= 0.1;
		AspectRatioFix();
		Logger::TypedLog(CHN_DEBUG, "-FOV Multiplier: %f,\n", FOVMultiplier);
		GameConfig::SetDoubleValue("Gameplay", "FOVMultiplier", FOVMultiplier);

	}

	if (GetAsyncKeyState(VK_F5) & 1) { // F5
		FLOAT* hkg_playerPosition = (FLOAT*)0x00FA6DB0;
		FLOAT* hkg_camOrient = (FLOAT*)0x025F5B5C; // ???? maybe
		*(bool*)(0x252740E) = 1; // Ins Fraud Sound

		std::wstring subtitles = (L"Player Position & Orient Printed to Console!");
		addsubtitles(subtitles.c_str(), delay, duration, whateverthefuck);
		Logger::TypedLog(CHN_DEBUG, "Player Pos + Orient: <%0.6f %0.6f %0.6f> [%0.6f]\n", hkg_playerPosition[0], hkg_playerPosition[1], hkg_playerPosition[2], hkg_camOrient[0]);
	}
	if (RPCHandler::IsCoopOrSP == true) 
	{
		if (GetAsyncKeyState(VK_F6) & 1) {
			pausetest = !pausetest;

			if (pausetest) {
				pause();
			}
			else {
				unpause();
			}
		}
	}

}

typedef int __cdecl RenderLoopStuff_Native();
RenderLoopStuff_Native* UpdateRenderLoopStuff = (RenderLoopStuff_Native*)(0x00C063D0); //0x00BD4A80

bool fixFrametime = 0;
bool addBindToggles = 0;

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
	    cus_FrameToggles();
	    slewtest();

	if (ARfov)
		AspectRatioFix();


	// Call original func
	return UpdateRenderLoopStuff();
}

void SetDefaultGameSettings()
{
	patchBytesM((BYTE*)0x00774126, (BYTE*)"\xC6\x05\xAC\xA9\xF7\x01\x00", 7); // Force game into windowed on default settings.
	//patchDWord((void*)(0x00753544 + 4), *(const char*)ServerNameRL);
	//*(CHAR**)0x0212AA08 = (char*)"[SR2 RELOADED]";
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

char* lobby_list[2] = {
		const_cast<char*>("sr2_mp_gb_frat01"),
		const_cast<char*>("sr2_mp_lobby")
};

int WINAPI Hook_WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	ErrorManager::Initialize();

	char NameBuffer[260];
	PIMAGE_DOS_HEADER dos_header;
	PIMAGE_NT_HEADERS nt_header;
	MEMORYSTATUSEX memory_info;
	HMODULE main_handle;
	
	Logger::TypedLog(CHN_DLL, "Calling Hooked WinMain.\n");

	main_handle=GetModuleHandleA(NULL);
	GetModuleFileNameA(main_handle,NameBuffer,260);
	Logger::TypedLog(CHN_DLL, "Module name = %s\n",NameBuffer);
	dos_header=(PIMAGE_DOS_HEADER)main_handle;
	nt_header=(PIMAGE_NT_HEADERS)((DWORD)main_handle+dos_header->e_lfanew);
	

	memory_info.dwLength=sizeof(memory_info);
	GlobalMemoryStatusEx(&memory_info);
	Logger::TypedLog(CHN_DLL, "Memory allocated to process at startup = %I64dMB, memory free = %I64dMB.\n",memory_info.ullTotalVirtual/1048576,memory_info.ullAvailVirtual/1048576);

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

#if RELOADED
	// patch in some stuff at run time, maybe even add exclusive reloaded toggles.
	if (GameConfig::GetValue("Debug", "ReloadedRuntimeFiles", 1))
	{
		patchBytesM((BYTE*)0x00E06CC4, (BYTE*)"\x72\x65\x65\x6C", 4); // reeload.tbl
		patchBytesM((BYTE*)0x00E06CD0, (BYTE*)"\x72\x65\x65\x6C", 4); // reeload_anims.tbl
	}
	// patch music2.xtbl
	if (GameConfig::GetValue("Audio", "NoMenuMusic", 0))
	{
		Logger::TypedLog(CHN_DLL, "Removing Menu Music...\n");
		patchDWord((void*)0x00DD87FC, (uint32_t)&mus2xtbl);
	}
	// Sidoku tint desat because he keeps crying kek
	if (GameConfig::GetValue("Graphics", "Tint", 0)) 
	{
		// HDR Tint Desat.

		Logger::TypedLog(CHN_DLL, "Removing HDR Tint...\n");
		patchNop((BYTE*)0x0051756A, 25); // nop normal tint

	}

#endif

	if (GameConfig::GetValue("Debug", "DisableXInput", 0))
	{
		patchNop((BYTE*)0x00BF9F40, 7);
		patchNop((BYTE*)0x00BF9F50, 7);
		patchNop((BYTE*)0x00BFA090, 2);
		patchNop((BYTE*)0x00BFA099, 5);
		patchNop((BYTE*)0x00BFA0C8, 7);
		patchNop((BYTE*)0x00C13A60, 7);
		patchNop((BYTE*)0x00C13A70, 7);
		patchNop((BYTE*)0x00C147D5, 5);
		patchNop((BYTE*)0x00C147DC, 5);
		patchNop((BYTE*)0x00C14A06, 5);
		Logger::TypedLog(CHN_DEBUG, "XInput Disabled.\n");
	}

	if (GameConfig::GetValue("Gameplay", "FixUltrawideFOV", 1))
	{
		ARfov = 1;
	}

	if (GameConfig::GetValue("Gameplay", "FixUltrawideCutsceneFOV", 1))
	{
		ARCutscene = 1;
	}

	if (GameConfig::GetDoubleValue("Gameplay", "FOVMultiplier", 1.0)) // 1.0 isn't go anywhere.
	{
		FOVMultiplier = GameConfig::GetDoubleValue("Gameplay", "FOVMultiplier", FOVMultiplier);
		if (FOVMultiplier > 1.0) {
			ARfov = 1;
			Logger::TypedLog(CHN_DEBUG, "Applying FOV Multiplier.\n");
		}
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
		lobby_list[0] = newLobby1;
		lobby_list[1] = newLobby2;

		patchDWord((void*)(0x0073EABA + 3), (int)&lobby_list);
		patchDWord((void*)(0x0073EA0B + 3), (int)&lobby_list);
		patchDWord((void*)(0x007E131A + 3), (int)&lobby_list);
		patchDWord((void*)(0x007E161E + 3), (int)&lobby_list);
		patchDWord((void*)(0x007E7670 + 3), (int)&lobby_list);
		patchDWord((void*)(0x007E774F + 3), (int)&lobby_list);
		patchDWord((void*)(0x0082F2E9 + 3), (int)&lobby_list);
		patchDWord((void*)(0x0082F4CC + 3), (int)&lobby_list);
		patchDWord((void*)(0x00842497 + 3), (int)&lobby_list);
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
		patchBytesM((BYTE*)0x0050124B, (BYTE*)"\xD9\x05\xA4\x7D\x52\x02", 6); // slew cam patch 1
		patchBytesM((BYTE*)0x00501238, (BYTE*)"\xD9\x05\x97\x2C\x7B\x02", 6); // slew camera float patch 2 (needed as this float is broken on PC, it's supposed to change dynamically but it never does)
		patchBytesM((BYTE*)0x00502A8F, (BYTE*)"\xBF\x02\x00\x00\x00", 5); // change pause type
		patchBytesM((BYTE*)0x00502A9C, (BYTE*)"\xE9\x3C\x00\x00\x00", 5); // Hopefully make func skip all unneccesary code.
		patchBytesM((BYTE*)0x005030AB, (BYTE*)"\xBE\x02\x00\x00\x00", 5); // change unpause type
		patchBytesM((BYTE*)0x00C01AC8, (BYTE*)"\xDC\x64\x24\x20", 4); // invert Y axis in slew 
		patchBytesM((BYTE*)0x0050134D, (BYTE*)"\xE9\xB9\x02\x00\x00\x00", 6); // ignore collision in slew
	}

	if (GameConfig::GetValue("Graphics", "VanillaFXPlus", 0))
	{
		Logger::TypedLog(CHN_DEBUG, "Patching VanillaFXPlus...\n");
		patchNop((BYTE*)0x00773797, 5); // prevent the game from disabling/enabling the tint.
		patchBytesM((BYTE*)0x0051A952, (BYTE*)"\xD9\x05\x7F\x2C\x7B\x02", 6); // new brightness address
		patchBytesM((BYTE*)0x0051A997, (BYTE*)"\xD9\x05\x83\x2C\x7B\x02", 6); // new sat address patch
		patchBytesM((BYTE*)0x0051A980, (BYTE*)"\xD9\x05\x87\x2C\x7B\x02", 6); // Hopefully make func skip all unneccesary code.
		patchBytesM((BYTE*)0x005030AB, (BYTE*)"\xBE\x02\x00\x00\x00", 5); // change unpause type
		patchByte((BYTE*)0x00E9787F, 0x00); // force HDR off because it's 1 by default
		patchNop((BYTE*)0x00773792, 5); // prevent the game from turning HDR on/off

		patchFloat((BYTE*)0x027B2C7F, 1.3f); //Bright
		patchFloat((BYTE*)0x027B2C83, 0.8f); //Sat
		patchFloat((BYTE*)0x027B2C87, 1.62f); //Contr
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

	if (GameConfig::GetValue("Gameplay", "BetterMovementBehaviour", 0))
	{
		// Majority of the SR2 movement sluggishness is due to the fact that certain walking anims add an
		// increased latency to walking generally and 180 anims tend to play constantly when trying to strafe, 180 anims didnt exist in SR1.
		Logger::TypedLog(CHN_MOD, "Patching In Better Movement Behavior...\n");
		patchNop((BYTE*)0x00E9216C, 3); // Jog180
		patchNop((BYTE*)0x00E9213C, 3); // Stand180
		patchNop((BYTE*)0x00E92130, 3); // WalkStop
		patchNop((BYTE*)0x00E9237C, 3); // StandToRun
		patchNop((BYTE*)0x00E92370, 3); // RunToStop
		patchNop((BYTE*)0x00E92364, 3); // WalkToStop
		patchNop((BYTE*)0x00E92394, 3); // WalkToStand
		patchNop((BYTE*)0x00E92388, 3); // StandToWalk

	}

	if (GameConfig::GetValue("Gameplay", "FastDoors", 0)) // removes the anim for kicking or opening doors.
	{
		Logger::TypedLog(CHN_DEBUG, "Patching Fast Doors...\n");
		patchNop((BYTE*)0x00E92268, 3);
		patchNop((BYTE*)0x00E9225C, 3);
	}

	if (GameConfig::GetValue("Gameplay", "BetterDriveByCam", 1)) // Fixes Car CAM Axis while doing drive-bys.
	{
		Logger::TypedLog(CHN_DEBUG, "Patching Better Drive-by Cam...\n");
		patchBytesM((BYTE*)0x00498689 + 2, (BYTE*)"\x71\x5D", 2);
	}

	if (GameConfig::GetValue("Gameplay", "BetterHandbrakeCam", 0)) // Fixes Car CAM Axis while doing handbrakes.
	{
		Logger::TypedLog(CHN_DEBUG, "Patching Better Handbrake Cam...\n");
		patchBytesM((BYTE*)0x004992a2 + 2, (BYTE*)"\x71\x5D", 2);
	}

	if (GameConfig::GetValue("Graphics", "RemoveBloom", 0)) // Removes a shader call to create bloom on bright objects.
	{
		Logger::TypedLog(CHN_MOD, "Removing Bloom...\n");
		patchNop((BYTE*)0x005174FA, 30);
		patchNop((BYTE*)0x005178F6, 25); // hopefully this new entry works on shitty nvidia gpus
	}

	if (GameConfig::GetValue("Gameplay", "DisableAimAssist", 0))
	{
		Logger::TypedLog(CHN_MOD, "Disabling Aim Assist...\n");
		patchNop((BYTE*)0x00E3CC80, 16); // nop aim_assist.xtbl
	}

	if (GameConfig::GetValue("Graphics", "RemoveVignette", 0))
	{
		Logger::TypedLog(CHN_MOD, "Disabling Vignette...\n");
		patchNop((BYTE*)0x00E0C62C, 9); // nop aVignette
	}

	if (GameConfig::GetValue("Graphics", "BetterAmbientOcclusion", 1)) 
	{
		Logger::TypedLog(CHN_MOD, "Making AO Better...\n");
		patchFloat((BYTE*)0x00E9898C, 0.05);

	}

	if (GameConfig::GetValue("Graphics", "DisableScreenBlur", 1))
	{
		Logger::TypedLog(CHN_MOD, "Disabling Screen Blur...\n");
		patchByte((BYTE*)0x02527297, 0x0);
	}
	else 
	{
		Logger::TypedLog(CHN_MOD, "Enabling Screen Blur...\n");
		patchByte((BYTE*)0x02527297, 0x1);

	}

	if (GameConfig::GetValue("Debug", "AltTabFPS", 1)) // Removes a sleep call in main render loop, this one seems to slow the game to below 25 fps when the game is alt-tabbed.
	{
		Logger::TypedLog(CHN_DLL, "Making ALT-TAB smoother...\n");
		patchNop((BYTE*)0x005226F3, 8); // Bye bye sleep call.
	}

	if (GameConfig::GetValue("Debug", "UncapFPS", 0)) // Removes a sleep call in main render loop, this one seems to slow the game to below 25 fps when the game is alt-tabbed.
	{  // Uncapping frames can lead to broken doors among other issues not yet noted.
		Logger::TypedLog(CHN_DLL, "Uncapping FPS...\n");
		patchNop((BYTE*)0x00D20E3E, 7);
	}

	// Removes all necessary sleep calls in the game, doubles fps and mitigates stutter, tanks CPU usage.
	if (GameConfig::GetValue("Debug", "SleepHack", 0) == 1) // LOW patch
	{
		// Woohoo, this is a dirty patch, but we'll include it for people who want it and CAN actually run it.
		// This will destroy older and weaker pcs, but we'll make sure to let the people who are on that, know that.

		// This is the lower spec version of the patch, the things that will cause the LEAST cpu usage.

		Logger::TypedLog(CHN_DLL, "Removing a Very Safe Amount of Sleep Calls...\n");
		patchNop((BYTE*)0x0052108C, 3); // patch win main sleep call
		patchNop((BYTE*)0x00521FC0, 4); // wait call in a threaded function, i think
		patchNop((BYTE*)0x00521FE5, 4); // same with this one
		patchNop((BYTE*)0x005285A2, 4); // this ones a doozy, this is some weird threaded exchange function, for each something, sleep. 

	}
	if (GameConfig::GetValue("Debug", "SleepHack", 0) == 2) // MEDIUM patch
	{
		Logger::TypedLog(CHN_DLL, "Removing a Safe Amount of Sleep Calls...\n");
		patchNop((BYTE*)0x0052108C, 3); // patch win main sleep call
		patchNop((BYTE*)0x00521FC0, 4); // wait call in a threaded function, i think
		patchNop((BYTE*)0x00521FE5, 4); // same with this one
		patchNop((BYTE*)0x005285A2, 4); // this ones a doozy, this is some weird threaded exchange function, for each something, sleep. 
		patchNop((BYTE*)0x0052847C, 8); //make the shadow pool less sleepy
	}
	
	if (GameConfig::GetValue("Graphics", "ExtendedRenderDistance", 0))
	{
		// Increases the Render Distance by x1.85
		// Might be glitchy, would've loved to increase this to x3.00 or x4.00 but the LOD starts bugging out
		Logger::TypedLog(CHN_MEMORY, "Increasing LOD Distance by x1.85. (Only a slight increase.)\n");
		patchBytesM((BYTE*)0x00E996B4, (BYTE*)"\x00\x00\xEC\x3F", 4);
	}

	if (GameConfig::GetValue("Graphics", "ExtendedTreeFadeDistance", 0))
	{
		// Increases the Tree Fade Distance from 250000 to 500000
		Logger::TypedLog(CHN_MEMORY, "Increasing Tree Fade Distance to 500000.\n");
		patchFloat((BYTE*)0x0252A058, 500000);
	}

	if (GameConfig::GetValue("Graphics", "ExtendedShadowRenderDistance", 0))
	{
		// Increases the Shadow Render Distance from 125 to 255, Actually a considerable difference.
		Logger::TypedLog(CHN_MEMORY, "Increasing Shadow Render Distance to 255.\n");
		patchDWord((void*)0x0279778C, 255); // Day_ShadowRenderDist > 255
		patchDWord((void*)0x02797790, 255); // Night_ShadowRenderDist > 255
		// im a bit scared about these nops but it works?... ~ NOPs calls that set these values from xtbl
		patchNop((BYTE*)0x0054DFEE, 15);
		patchNop((BYTE*)0x0054DFDE, 15);
	}

	if (GameConfig::GetValue("Graphics", "RemoveBlackBars", 0)) // Another Tervel moment
	{
		Logger::TypedLog(CHN_DLL, "Removing Black Bars.\n");
		patchNop((BYTE*)(0x0075A265), 5);
	}

	// Continue to the program's WinMain.

	WinMain_Type OldWinMain=(WinMain_Type)0x00520ba0;
	return (OldWinMain(hInstance, hPrevInstance, lpCmdLine,nShowCmd));
}

