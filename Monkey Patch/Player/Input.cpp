// Input.cpp (uzis, Clippy95)
// --------------------
// Created: 22/02/2025

#include "../FileLogger.h"
#include "../Patcher/patch.h"
#include "../GameConfig.h"
#include "../General/General.h"

#include "Xinput.h"
#pragma comment(lib, "Xinput.lib")

#include "Input.h"
#include "../UtilsGlobal.h"
#include <safetyhook.hpp>

namespace Input {
	GAME_LAST_INPUT g_lastInput = UNKNOWN;
	GAME_LAST_INPUT LastInput() {
		using namespace UtilsGlobal;
		float LeftStickX = *(float*)0x23485F4;
		float LeftStickY = *(float*)0x23485F8;
		if (LeftStickX != 0.f || LeftStickY != 0.f)
			g_lastInput = CONTROLLER;

		if (mouse().getXdelta() || mouse().getYdelta())
			g_lastInput = MOUSE;
		return g_lastInput;

	}

	typedef int(__stdcall* XInputEnableT)(bool Enable); // this is a deprecated feature and I couldn't get it to register through the XInput lib
	XInputEnableT XInputEnable = (XInputEnableT)0x00CCD4F8;

	int controllerConnected[4] = { 1, 1, 1, 1 };
	bool NoControllers = false;
	bool XInputEnabled = true;

	typedef DWORD(WINAPI* XInputGetCapabilitiesFunc)(DWORD dwUserIndex, DWORD dwFlags, XINPUT_CAPABILITIES* pCapabilities);

	typedef struct {
		HMODULE handle;
		XInputGetCapabilitiesFunc GetCapabilities;
	} XInputDLL;

	XInputDLL g_XInputDLL = { 0 };
	bool LoadXInputDLL() {
		const wchar_t* XInputLibraryNames[] = {
			L"xinput1_4.dll",   // Windows 8+
			L"xinput1_3.dll",   // DirectX SDK
			L"xinput9_1_0.dll", // Windows Vista, Windows 7
			L"xinput1_2.dll",   // DirectX SDK
			L"xinput1_1.dll",   // DirectX SDK
			NULL,
		};

		// Try to load each DLL in order until one succeeds
		for (int i = 0; XInputLibraryNames[i] != NULL; i++) {
			g_XInputDLL.handle = LoadLibraryW(XInputLibraryNames[i]);
			if (g_XInputDLL.handle != NULL) {
				Logger::TypedLog(CHN_DLL,"Successfully loaded XInput DLL: %ls\n", XInputLibraryNames[i]);
				break;
			}
		}

		if (g_XInputDLL.handle == NULL) {
			Logger::TypedLog(CHN_DLL, "Failed to load any XInput DLL\n");
			return false;
		}

		g_XInputDLL.GetCapabilities = (XInputGetCapabilitiesFunc)GetProcAddress(g_XInputDLL.handle, "XInputGetCapabilities");

		if (g_XInputDLL.GetCapabilities == NULL) {
			Logger::TypedLog(CHN_DLL, "Failed to load required XInput function\n");
			FreeLibrary(g_XInputDLL.handle);
			g_XInputDLL.handle = NULL;
			return false;
		}

		return true;
	}
	void UnloadXInputDLL() {
		if (g_XInputDLL.handle != NULL) {
			FreeLibrary(g_XInputDLL.handle);
			g_XInputDLL.handle = NULL;
			g_XInputDLL.GetCapabilities = NULL;
		}
	}
	DWORD XInput_GetCapabilities(DWORD dwUserIndex, DWORD dwFlags, XINPUT_CAPABILITIES* pCapabilities) {
		if (g_XInputDLL.GetCapabilities) {
			return g_XInputDLL.GetCapabilities(dwUserIndex, dwFlags, pCapabilities);
		}
		return ERROR_DEVICE_NOT_CONNECTED;
	}
	bool IsControllerConnected(int controllerIndex)
	{
		XINPUT_CAPABILITIES capabilities;
		return XInput_GetCapabilities(controllerIndex, 0, &capabilities); // perhaps a little more lightweight than GetState?
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

			bool inFocus = General::IsSRFocused(); // calling it less = better for performance; local variable

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

	void DisableXInput() {
		if (GameConfig::GetValue("Debug", "DisableXInput", 0))
		{
			patchBytesM((BYTE*)0x00BFA090, (BYTE*)"\x6A\x00", 2);
			patchBytesM((BYTE*)0x00BFA0C8, (BYTE*)"\x6A\x00", 2);
			Logger::TypedLog(CHN_DEBUG, "XInput Disabled.\n");
		}

		else { // if XInput is not disabled completely, we just force our polling fix
			patchNop((BYTE*)0x00BFA090, 2);
			patchNop((BYTE*)0x00BFA099, 5);
			patchNop((BYTE*)0x00BFA0C8, 2);
			patchNop((BYTE*)0x00BFA0CA, 5);
			CreateThread(0, 0, XInputCheck, 0, 0, 0);
		}

	}

	void ForceNoVibration()
	{
		if (GameConfig::GetValue("Debug", "ForceDisableVibration", 0)) // Fixes load/new save insta-crash due to broken / shitty joystick drivers.
		{
			patchBytesM((BYTE*)0x00C14930, (BYTE*)"\xC3\x00", 2);
			Logger::TypedLog(CHN_DEBUG, "Vibration Forced to OFF.\n");
		}
	}

	typedef int __cdecl PlayerSpin(float a1);
	PlayerSpin* UpdatePlayerSpin = (PlayerSpin*)(0x0073FB20); //0x00BD4A80

	int NativeMouse_clothing_store(float a1) {
		if (UtilsGlobal::isPaused)
			return UpdatePlayerSpin(a1);
		// Same implementation as Saints Row 1 Mousehook 
		// expect for vehicle spinning as it's already implemented.

		// Only current issue as of now is that the tickrate of the function we're replacing is quite low so it doesn't detect small mouse movements, 
		// not an issue of the mouse struct as if we move this to the main game loop it's actually quite smooth! also it's pretty slow that it can't even detect scroll wheel :(


		int8_t& busy = *(int8_t*)(0x00E8D57F);
		UtilsGlobal::mouse mouse;
		//float wheeldelta = mouse.getWheeldelta() * 7.5f;
		float deltax = mouse.getXdelta() / 7.5f;
		// Unlike SR1, SR2 doesn't like it when you play with the players rotation directly so clamp the delta otherwise the player gets squished, it still does but not as bad.
		deltax = UtilsGlobal::clamp(deltax, -40.f, 40.f);
		int baseplayer = UtilsGlobal::getplayer();
		if (deltax != 0.f && !busy) {
			float* x_player_cos = (float*)(baseplayer + 0x38);
			float* x_player_sin = (float*)(baseplayer + 0x40);
			float x = atan2(*x_player_sin, *x_player_cos);
			x = UtilsGlobal::RadianstoDegree(x);
			if (UtilsGlobal::invert)
				x += deltax;
			else
				x -= deltax;
			x = fmod(x + 180.0f, 360.0f);
			if (x < 0) x += 360.0f;
			x -= 180.0f;
			x = UtilsGlobal::DegreetoRadians(x);
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
	// If we need an empty global buffer we could use that, but it has to be 0.
	volatile float aim_assist_empty_buffer[18]{};
	SafetyHookMid player_autoaim_do_assisted_aiming_midhook;
	BYTE disable_aim_assist_noMatterInput;
	SAFETYHOOK_NOINLINE void player_autoaim_do_assisted_aiming_midhookfunc_disableaimassistmouse(safetyhook::Context32& ctx) {
		if(disable_aim_assist_noMatterInput == 2 ||LastInput() == GAME_LAST_INPUT::MOUSE)
		ctx.esi = (uintptr_t)&aim_assist_empty_buffer;
	}

	void Init() {
		LoadXInputDLL();
		if (GameConfig::GetValue("Gameplay", "DisableAimAssist", 1) == 1)
		{
			player_autoaim_do_assisted_aiming_midhook = safetyhook::create_mid(0x009E28B3, &player_autoaim_do_assisted_aiming_midhookfunc_disableaimassistmouse);
			Logger::TypedLog(CHN_MOD, "Disabling Aim Assist while using mouse...\n");
		}
		else if (GameConfig::GetValue("Gameplay", "DisableAimAssist", 1) >= 2) {
			disable_aim_assist_noMatterInput = 3; // can't use hook.
			Logger::TypedLog(CHN_MOD, "Disabling Aim Assist completely...\n");
			patchNop((BYTE*)0x00E3CC80, 16); // nop aim_assist.xtbl
		}
		
		betterTags = 0;

		DisableXInput();
		ForceNoVibration();

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
				UtilsGlobal::invert = true;
			}
			patchCall((int*)0x007CE170, (int*)NativeMouse_clothing_store);
			Logger::TypedLog(CHN_DEBUG,
				"Native mouse to player rotation in clothing store menus: %s.\n",
				UtilsGlobal::invert ? "inverted direction" : "normal rotation");
		}

		if (GameConfig::GetValue("Gameplay", "SwapScrollWheel", 0))
		{
			// TODO: maybe have optional options for when to swap scroll? like map only or weapon wheel only, mouse function is done at 0x00C1F320.

			patchBytesM((BYTE*)0x00C1F0F7, (BYTE*)"\x29", 1); // opcode for sub, add previously.
		}

	}
}