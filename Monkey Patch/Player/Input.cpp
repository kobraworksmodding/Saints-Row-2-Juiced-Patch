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
#include "../SafeWrite.h"
#include <safetyhook.hpp>
extern bool IsKeyPressed(unsigned char Key, bool Hold);
namespace Input {
	GAME_LAST_INPUT g_lastInput = UNKNOWN;
	GAME_LAST_INPUT LastInput() {
		using namespace UtilsGlobal;
		float LeftStickX = *(float*)0x23485F4;
		float LeftStickY = *(float*)0x23485F8;
		if (LeftStickX != 0.f || LeftStickY != 0.f)
			g_lastInput = CONTROLLER;
	
		if(mouse().getXdelta() || mouse().getYdelta())
			g_lastInput = MOUSE;
		return g_lastInput;
	
	}
	typedef int(__stdcall* XInputEnableT)(bool Enable); // this is a deprecated feature and I couldn't get it to register through the XInput lib
	XInputEnableT XInputEnable = (XInputEnableT)0x00CCD4F8;

	int controllerConnected[4] = { 1, 1, 1, 1 };
	bool NoControllers = false;
	bool XInputEnabled = true;

	bool IsControllerConnected(int controllerIndex)
	{
		XINPUT_CAPABILITIES capabilities;
		return XInputGetCapabilities(controllerIndex, 0, &capabilities); // perhaps a little more lightweight than GetState?
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

	typedef char(*CDBP_mouse_poolT)();
	CDBP_mouse_poolT CDBP_mouse_pool = (CDBP_mouse_poolT)0xC13660;
	static float wheel_x = 0.0f;
	static float wheel_y = 0.0f;
	volatile char KEY_inventory_up = 'W';
	volatile char KEY_inventory_down = 'S';
	volatile char KEY_inventory_left = 'A';
	volatile char KEY_inventory_right = 'D';
	void reset_weapon_wheel_mousefix_pos() {
		wheel_x = 0.f;
		wheel_y = 0.f;
	}
#pragma optimize("", off)
	static bool was_up_pressed = false;
	static bool was_down_pressed = false;
	static bool was_left_pressed = false;
	static bool was_right_pressed = false;
	__declspec(noinline) char CDBP_mouse_poll_hook() {
		using namespace UtilsGlobal;
		char result = CDBP_mouse_pool();
		byte* invopen = (byte*)0x00E863C8;
		float* inventory_x = (float*)0x2348574;
		float* inventory_y = (float*)0x2348578;

		if (*invopen == 36) {
			// Get mouse movement deltas
			int dx = mouse().getXdelta();
			int dy = mouse().getYdelta();

			// Convert to float and apply sensitivity factor (adjust as needed)
			const float sensitivity = 0.5f;
			float fdx = dx * sensitivity;
			float fdy = dy * sensitivity;


			wheel_x += fdx;
			wheel_y += fdy;

			SafeWrite8(0x23494E0, IsKeyPressed('W', true));

			// Normalize to keep within -1.0 to 1.0 circle
			float length = sqrt(wheel_x * wheel_x + wheel_y * wheel_y);
			if (length > 1.0f) {
				wheel_x /= length;
				wheel_y /= length;
			}
			printf("wheel_x %f, wheel_y %f \n", wheel_x, wheel_y);
			// Only update the game's inventory position when inventory is open


			if (IsKeyPressed(KEY_inventory_up, true) || IsKeyPressed(KEY_inventory_down, true) || IsKeyPressed(KEY_inventory_right, true) || IsKeyPressed(KEY_inventory_left, true))
				reset_weapon_wheel_mousefix_pos();

			if (LastInput() == MOUSE) {
				*inventory_x = wheel_x;
				*inventory_y = -wheel_y;
			}
		}
		return result;
	}
#pragma optimize("", on)

	typedef int(*Input_LoopT)();
	Input_LoopT input_loop = (Input_LoopT)0x00C11710;

	int input_loop_hook() {
#define ARROW_UP (bool*)0x23494E0
#define ARROW_DOWN (bool*)0x2349540
#define ARROW_RIGHT (bool*)0x0234951C 
#define ARROW_LEFT (bool*)0x02349504

		int result = input_loop();

		if ((*(byte*)0x00E863C8 == 36)) {
			// Handle UP key
			bool is_up_pressed_now = IsKeyPressed(KEY_inventory_up, true);
			if (is_up_pressed_now) {
				reset_weapon_wheel_mousefix_pos();
				*ARROW_UP = 1;
				was_up_pressed = true;
			}
			else if (was_up_pressed) {
				*ARROW_UP = 0;
				was_up_pressed = false;
			}


			bool is_down_pressed_now = IsKeyPressed(KEY_inventory_down, true);
			if (is_down_pressed_now) {
				reset_weapon_wheel_mousefix_pos();
				*ARROW_DOWN = 1;
				was_down_pressed = true;
			}
			else if (was_down_pressed) {
				*ARROW_DOWN = 0;
				was_down_pressed = false;
			}


			bool is_left_pressed_now = IsKeyPressed(KEY_inventory_left, true);
			if (is_left_pressed_now) {
				reset_weapon_wheel_mousefix_pos();
				*ARROW_LEFT = 1;
				was_left_pressed = true;
			}
			else if (was_left_pressed) {
				*ARROW_LEFT = 0;
				was_left_pressed = false;
			}


			bool is_right_pressed_now = IsKeyPressed(KEY_inventory_right, true);
			if (is_right_pressed_now) {
				reset_weapon_wheel_mousefix_pos();
				*ARROW_RIGHT = 1;
				was_right_pressed = true;
			}
			else if (was_right_pressed) {
				*ARROW_RIGHT = 0;
				was_right_pressed = false;
			}
		}

		return result;
	}
	SafetyHookMid NoMixedInput;
	SAFETYHOOK_NOINLINE void NoMixedInput_mid(safetyhook::Context32& ctx) {
#define CONTROLLER_STATUS_ADDR (int*)0x23486FC
		if (*CONTROLLER_STATUS_ADDR == 3);

	}
	void Init() {

		//WriteRelCall(0x00C147A5, (UInt32)&CDBP_mouse_poll_hook);
		KEY_inventory_up = (char)GameConfig::GetValue("Input", "KEY_inventory_up", 'W');
		KEY_inventory_down = (char)GameConfig::GetValue("Input", "KEY_inventory_down", 'S');
		KEY_inventory_left = (char)GameConfig::GetValue("Input", "KEY_inventory_left", 'A');
		KEY_inventory_right = (char)GameConfig::GetValue("Input", "KEY_inventory_right", 'D');
		patchCall((void*)0x00758C03, input_loop_hook);
		patchCall((void*)0x00758C15, input_loop_hook);
		patchCall((void*)0x00C1478E, input_loop_hook);
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