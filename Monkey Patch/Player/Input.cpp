// Input.cpp (uzis, Clippy95)
// --------------------
// Created: 22/02/2025

import OptionsManager;
#include "../FileLogger.h"
#include "../Patcher/patch.h"
#include "../GameConfig.h"
#include "../General/General.h"

#include "Xinput.h"
#pragma comment(lib, "Xinput.lib")

#include "Input.h"
#include "../UtilsGlobal.h"
#include <safetyhook.hpp>
#include "../Game/Game.h"
#include <Hooking.Patterns.h>
#include <SDL3/SDL_gamepad.h>
#include <SDL3/SDL_init.h>
bool IsKeyPressed(unsigned char Key, bool Hold);
int __fastcall subT_6218F0(DWORD* a1);
namespace Input {
	// DO NOT CHANGE WHILE GAME IS RUNNING, only meant to disable on runtime. if it causes any issues (hopefully none) 
	BYTE EnableDynamicPrompts = 2;
	static SDL_Gamepad* g_gamepad = NULL;
	bool allow_hacked_inventory_KBM;
	int HoldFineAim = false;

	static bool SDL_isPlayStationController() {
		if (!g_gamepad) return false;

		SDL_GamepadType type = SDL_GetGamepadType(g_gamepad);

		if (type == SDL_GAMEPAD_TYPE_PS3 || type == SDL_GAMEPAD_TYPE_PS4 || type == SDL_GAMEPAD_TYPE_PS5) {
			return true;
		}

		Uint16 vendor = SDL_GetGamepadVendor(g_gamepad);
		if (vendor == 0x054C) {  // Sony vendor ID
			return true;
		}

		return false;
	}
	int usePS3Prompts = false;

	ControllerType sdlToControllerType(SDL_GamepadType sdlType) {
		switch (sdlType) {
		case SDL_GAMEPAD_TYPE_XBOXONE:
			return XboxSeriesX;

		case SDL_GAMEPAD_TYPE_PS5:
			return PS5;

		case SDL_GAMEPAD_TYPE_PS4:
		case SDL_GAMEPAD_TYPE_PS3:
			return PS3;

		case SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_PRO:
		case SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_JOYCON_LEFT:
		case SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_JOYCON_RIGHT:
		case SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_JOYCON_PAIR:
			return nx;

		case SDL_GAMEPAD_TYPE_STANDARD:
			return Xbox360;

		case SDL_GAMEPAD_TYPE_XBOX360:
			return Xbox360;


		case SDL_GAMEPAD_TYPE_UNKNOWN:
		default:
			return Xbox360;
		}
	}

	ControllerType forced_current_controller_type;

	ControllerType GetControllerType() {
		if (forced_current_controller_type != NONE)
			return forced_current_controller_type;

		if (g_gamepad)
			return sdlToControllerType(SDL_GetGamepadType(g_gamepad));

	}

	bool UsePS3Prompts() {
		auto cur = GetControllerType();
		if (cur == PS3 || cur == PS5)
			return true;
		if (!g_gamepad)
			return false;

		return SDL_isPlayStationController();
	}



	bool __declspec(naked) key_held(int keycode) {
		static const DWORD func_addr = 0xC111D0;
		__asm {
			push ebp
			mov ebp, esp
			sub esp, __LOCAL_SIZE
			mov     eax, keycode
			call func_addr
			mov esp, ebp
			pop ebp
			ret
		}
	}
	inline int refreshVintStrings() {
		return ((int(*)(void))0x00BA2F90)();
	}
	GAME_LAST_INPUT g_lastInputPrevFrame = MOUSE;
	GAME_LAST_INPUT g_lastInput = UNKNOWN;
	bool* is_controller_connect = (bool*)(0x0252A58E);
	GAME_LAST_INPUT LastInput() {
		using namespace UtilsGlobal;
		if (!*is_controller_connect) {
			g_lastInput = MOUSE;
			return g_lastInput;
		}
		if (*(uintptr_t*)0x2348810) {
			for (int c = 0; c <= 19; ++c) {
				BYTE* buttons = (BYTE*)*(uintptr_t*)0x2348810;
				if (buttons[c] != 0) {
					g_lastInput = CONTROLLER;
					break;
				}
				
			}
		}
		float LeftStickX = *(float*)0x23485F4;
		float LeftStickY = *(float*)0x23485F8;
		float RightStickX = *(float*)0x023485B4;
		float RightStickY = *(float*)0x023485B8;

		const float deadzone = 0.01f;

		if (abs(LeftStickX) > deadzone || abs(LeftStickY) > deadzone ||
			abs(RightStickX) > deadzone || abs(RightStickY) > deadzone)
			g_lastInput = CONTROLLER;

		if (mouse().getXdelta() || mouse().getYdelta())
			g_lastInput = MOUSE;

		if (g_lastInputPrevFrame != g_lastInput)
			//refreshVintStrings();
		if (EnableDynamicPrompts >= 2) {
			for (int i = 0; i < 103; i++) {
				if (key_held(i)) {
					g_lastInput = MOUSE;
					break;
				}

			}
		}
		g_lastInputPrevFrame = g_lastInput;

		return g_lastInput;

	}
	int ForceInput = 0;
	GAME_LAST_INPUT LastInputUI() {
		switch (ForceInput) {
		case 1:
			return GAME_LAST_INPUT::CONTROLLER;
		case 2:
			return GAME_LAST_INPUT::MOUSE;
		default:
			return LastInput();
		}
	}
#pragma warning( disable : 4995)
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
	DWORD WINAPI LastInputCheck(LPVOID lpParameter)
	{
		while (true) {
			LastInput();
			SleepEx(33, 0);
		}
		return 0;
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
	// If we need an empty global buffer we could use that, but it has to be 0.
	//volatile float aim_assist_empty_buffer[19]{};
	SafetyHookMid player_autoaim_do_assisted_aiming_midhook;
	BYTE disable_aim_assist_noMatterInput;
	SAFETYHOOK_NOINLINE void player_autoaim_do_assisted_aiming_midhookfunc_disableaimassistmouse(safetyhook::Context32& ctx) {
		if((disable_aim_assist_noMatterInput >= 1 && g_lastInput == GAME_LAST_INPUT::MOUSE) || disable_aim_assist_noMatterInput >= 2)
		ctx.eip = 0x9D7757;
	}
	struct PC_port_controller_keys
	{
		// idk wtf is this
		int keyboard_button;
		int controller_button;
	};
	PC_port_controller_keys* PC_port_key_for_controler_assignments = (PC_port_controller_keys*)0x234E768;
	SafetyHookInline getpckeyboardimage_T{};
	wchar_t prompt_image_buffer[10000]{};
	int prompt_image_buffer_index = 0;
	struct ControllerMapping {
		const wchar_t* xbox;
		const wchar_t* ps3;
		const wchar_t* steamdeck;
		const wchar_t* xboxseriesx;
		const wchar_t* nx;
		const wchar_t* ps5;
	};



	std::unordered_map<controller_values, ControllerMappingSDL> char_SDLButtons_lua = {
		{a_button, {"ui_ctrl_sdk_btn_a", "ui_ctrl_xsx_btn_a", "ui_ctrl_nx_btn_b", "ui_ctrl_ps5_btn_cross"}},
		{b_button, {"ui_ctrl_sdk_btn_b", "ui_ctrl_xsx_btn_b", "ui_ctrl_nx_btn_a", "ui_ctrl_ps5_btn_circle"}},
		{x_button, {"ui_ctrl_sdk_btn_x", "ui_ctrl_xsx_btn_x", "ui_ctrl_nx_btn_y", "ui_ctrl_ps5_btn_square"}},
		{y_button, {"ui_ctrl_sdk_btn_y", "ui_ctrl_xsx_btn_y", "ui_ctrl_nx_btn_x", "ui_ctrl_ps5_btn_triangle"}},
		{right_stick, {"ui_ctrl_sdk_R3", "ui_ctrl_xsx_RS", "ui_ctrl_nx_RS", "ui_ctrl_ps5_R3"}},
		{lt_button, {"ui_ctrl_sdk_L1", "ui_ctrl_xsx_LB", "ui_ctrl_nx_L", "ui_ctrl_ps5_L1"}}, // PC PORT AHH, defualt bindings are RB/LB and are only used in horz menus
		{rt_button, {"ui_ctrl_sdk_R1", "ui_ctrl_xsx_LB", "ui_ctrl_nx_R", "ui_ctrl_ps5_R1"}}, // PC PORT AHH
		{lb_button, {"ui_ctrl_sdk_L1", "ui_ctrl_xsx_LB", "ui_ctrl_nx_L", "ui_ctrl_ps5_L1"}},
		{rb_button, {"ui_ctrl_sdk_R1", "ui_ctrl_xsx_RB", "ui_ctrl_nx_R", "ui_ctrl_ps5_R1"}},
		{left_right, {"ui_ctrl_sdk_dpad_lr", "ui_ctrl_xsx_dpad_lr", "ui_ctrl_nx_dpad_lr", "ui_ctrl_ps5_dpad_lr"}},
		{up_down, {"ui_ctrl_sdk_dpad_ud", "ui_ctrl_xsx_dpad_ud", "ui_ctrl_nx_dpad_ud", "ui_ctrl_ps5_dpad_ud"}},
		{left_trigger, {"ui_ctrl_sdk_L2", "ui_ctrl_xsx_LT", "ui_ctrl_nx_ZL", "ui_ctrl_ps5_L2"}},
		{right_trigger, {"ui_ctrl_sdk_R2", "ui_ctrl_xsx_RT", "ui_ctrl_nx_ZR", "ui_ctrl_ps5_R2"}},
		{dpad_image, {"ui_ctrl_sdk_dpad", "ui_ctrl_xsx_dpad", "ui_ctrl_nx_dpad", "ui_ctrl_ps5_dpad"}},
		{dpad_lr_image, {"ui_ctrl_sdk_dpad_lr", "ui_ctrl_xsx_dpad_lr", "ui_ctrl_nx_dpad_lr", "ui_ctrl_ps5_dpad_lr"}},
		{dpad_ud_image, {"ui_ctrl_sdk_dpad_ud", "ui_ctrl_xsx_dpad_ud", "ui_ctrl_nx_dpad_ud", "ui_ctrl_ps5_dpad_ud"}},
		{control_stick_base, {"ui_ctrl_sdk_S", "ui_ctrl_xsx_S", "ui_ctrl_nx_S", "ui_ctrl_ps5_S"}},
		{control_stick_thumb, {"ui_ctrl_sdk_S", "ui_ctrl_xsx_S", "ui_ctrl_nx_S", "ui_ctrl_ps5_S"}}
	};

	std::unordered_map<int, ControllerMapping> padButtonMaps = {
		{0, {L"ui_ctrl_360_btn_a", L"ui_ctrl_ps3_btn_cross", L"ui_ctrl_sdk_btn_a", L"ui_ctrl_xsx_btn_a", L"ui_ctrl_nx_btn_b", L"ui_ctrl_ps5_btn_cross"}}, // A/Cross (Nintendo B is bottom)
		{1, {L"ui_ctrl_360_btn_b", L"ui_ctrl_ps3_btn_circle", L"ui_ctrl_sdk_btn_b", L"ui_ctrl_xsx_btn_b", L"ui_ctrl_nx_btn_a", L"ui_ctrl_ps5_btn_circle"}}, // B/Circle (Nintendo A is right)
		{2, {L"ui_ctrl_360_btn_x", L"ui_ctrl_ps3_btn_square", L"ui_ctrl_sdk_btn_x", L"ui_ctrl_xsx_btn_x", L"ui_ctrl_nx_btn_y", L"ui_ctrl_ps5_btn_square"}}, // X/Square (Nintendo Y is left)
		{3, {L"ui_ctrl_360_btn_y", L"ui_ctrl_ps3_btn_triangle", L"ui_ctrl_sdk_btn_y", L"ui_ctrl_xsx_btn_y", L"ui_ctrl_nx_btn_x", L"ui_ctrl_ps5_btn_triangle"}}, // Y/Triangle (Nintendo X is top)
		{4, {L"ui_ctrl_360_btn_lb", L"ui_ctrl_ps3_btn_l1", L"ui_ctrl_sdk_L1", L"ui_ctrl_xsx_LB", L"ui_ctrl_nx_L", L"ui_ctrl_ps5_L1"}}, // Left Bumper/L1
		{5, {L"ui_ctrl_360_btn_rb", L"ui_ctrl_ps3_btn_r1", L"ui_ctrl_sdk_R1", L"ui_ctrl_xsx_RB", L"ui_ctrl_nx_R", L"ui_ctrl_ps5_R1"}}, // Right Bumper/R1
		{6, {L"ui_ctrl_360_btn_back", L"ui_ctrl_ps3_btn_select", L"ui_ctrl_sdk_select", L"ui_ctrl_xsx_select", L"ui_ctrl_nx_select", L"ui_ctrl_ps5_select"}}, // Back/Select
		{7, {L"ui_ctrl_360_btn_start", L"ui_ctrl_ps3_btn_start", L"ui_ctrl_sdk_start", L"ui_ctrl_xsx_start", L"ui_ctrl_nx_start", L"ui_ctrl_ps5_start"}}, // Start
		{8, {L"ui_ctrl_360_btn_ls", L"ui_ctrl_ps3_btn_l3", L"ui_ctrl_sdk_L3", L"ui_ctrl_xsx_LS", L"ui_ctrl_nx_LS", L"ui_ctrl_ps5_L3"}}, // Left Stick Click
		{9, {L"ui_ctrl_360_btn_rs", L"ui_ctrl_ps3_btn_r3", L"ui_ctrl_sdk_R3", L"ui_ctrl_xsx_RS", L"ui_ctrl_nx_RS", L"ui_ctrl_ps5_R3"}}, // Right Stick Click
		{10, {L"ui_ctrl_360_btn_lt", L"ui_ctrl_ps3_btn_l2", L"ui_ctrl_sdk_L2", L"ui_ctrl_xsx_LT", L"ui_ctrl_nx_ZL", L"ui_ctrl_ps5_L2"}}, // Left Trigger/L2/ZL
		{11, {L"ui_ctrl_360_btn_rt", L"ui_ctrl_ps3_btn_r2", L"ui_ctrl_sdk_R2", L"ui_ctrl_xsx_RT", L"ui_ctrl_nx_ZR", L"ui_ctrl_ps5_R2"}}, // Right Trigger/R2/ZR
		{16, {L"ui_ctrl_360_dpad_r", L"ui_ctrl_ps3_dpad_r", L"ui_ctrl_sdk_dpad_right", L"ui_ctrl_xsx_dpad_right", L"ui_ctrl_nx_dpad_right", L"ui_ctrl_ps5_dpad_right"}}, // D-pad Right
		{17, {L"ui_ctrl_360_dpad_u", L"ui_ctrl_ps3_dpad_u", L"ui_ctrl_sdk_dpad_up", L"ui_ctrl_xsx_dpad_up", L"ui_ctrl_nx_dpad_up", L"ui_ctrl_ps5_dpad_up"}}, // D-pad Up
		{18, {L"ui_ctrl_360_dpad_l", L"ui_ctrl_ps3_dpad_l", L"ui_ctrl_sdk_dpad_left", L"ui_ctrl_xsx_dpad_left", L"ui_ctrl_nx_dpad_left", L"ui_ctrl_ps5_dpad_left"}}, // D-pad Left
		{19, {L"ui_ctrl_360_dpad_d", L"ui_ctrl_ps3_dpad_d", L"ui_ctrl_sdk_dpad_down", L"ui_ctrl_xsx_dpad_down", L"ui_ctrl_nx_dpad_down", L"ui_ctrl_ps5_dpad_down"}} // D-pad Down
	};

	std::unordered_map<int, const wchar_t*> padButtonToText = {
	{0, L"A"},
	{1, L"B"},
	{2, L"X"},
	{3, L"Y"},
	{4, L"LB"},
	{5, L"RB"},
	{6, L"Back"},
	{7, L"Start"},
	{8, L"LS"},
	{9, L"RS"},
	{10, L"LT"},
	{11, L"RT"},
	{16, L"dPadRight"},
	{17, L"dPadUp"},
	{18, L"dPadLeft"},
	{19, L"dPadDown"}
	};

	std::unordered_map<int, ControllerMapping> actionsToController = {
		{0, {L"ui_ctrl_360_btn_ls", L"ui_ctrl_ps3_btn_l3", L"ui_ctrl_sdk_L3", L"ui_ctrl_xsx_LS", L"ui_ctrl_nx_LS", L"ui_ctrl_ps5_L3"}},
		{2, {L"ui_ctrl_360_btn_ls", L"ui_ctrl_ps3_btn_l3", L"ui_ctrl_sdk_L3", L"ui_ctrl_xsx_LS", L"ui_ctrl_nx_LS", L"ui_ctrl_ps5_L3"}},
		{3, {L"ui_ctrl_360_btn_ls", L"ui_ctrl_ps3_btn_l3", L"ui_ctrl_sdk_L3", L"ui_ctrl_xsx_LS", L"ui_ctrl_nx_LS", L"ui_ctrl_ps5_L3"}},
		{12, {L"ui_ctrl_360_btn_a", L"ui_ctrl_ps3_btn_cross", L"ui_ctrl_sdk_btn_a", L"ui_ctrl_xsx_btn_a", L"ui_ctrl_nx_btn_b", L"ui_ctrl_ps5_btn_cross"}},
	};

	bool useTextPrompts = false;

	

	wchar_t* __cdecl getpckeyboardimage_hook(uint32_t* action_index, int mouse) {
		if (LastInputUI() == GAME_LAST_INPUT::CONTROLLER) {
			ControllerType controller_type = GetControllerType();

			// Default to Xbox360 if NONE
			if (controller_type == NONE) {
				controller_type = Xbox360;
			}

			if (prompt_image_buffer_index > 9500) {
				prompt_image_buffer_index = 0;
			}
			int start_index = prompt_image_buffer_index;

			if (!action_index) {
				auto action_controller_pc = (mouse == 1) ? padButtonMaps.find(9) : padButtonMaps.find(8);
				if (action_controller_pc != padButtonMaps.end()) {
					const wchar_t* buttonImage = nullptr;

					switch (controller_type) {
					case Xbox360:
						buttonImage = action_controller_pc->second.xbox;
						break;
					case PS3:
						buttonImage = action_controller_pc->second.ps3;
						break;
					case SteamDeck:
						buttonImage = action_controller_pc->second.steamdeck;
						break;
					case XboxSeriesX:
						buttonImage = action_controller_pc->second.xboxseriesx;
						break;
					case nx:
						buttonImage = action_controller_pc->second.nx;
						break;
					case PS5:
						buttonImage = action_controller_pc->second.ps5;
						break;
					default:
						buttonImage = action_controller_pc->second.xbox; // Default to Xbox360
						break;
					}

					wsprintf(&prompt_image_buffer[prompt_image_buffer_index], L"[format][scale:1.0][image:%s][/format]", buttonImage);
					prompt_image_buffer_index += wcslen(&prompt_image_buffer[start_index]) + 1;
					return &prompt_image_buffer[start_index];
				}
				else
					return getpckeyboardimage_T.ccall<wchar_t*>(action_index, mouse);
			}

			int controller_key = PC_port_key_for_controler_assignments[*action_index + 2].controller_button;
			int keyboard_key = PC_port_key_for_controler_assignments[*action_index + 2].keyboard_button;
			auto it = padButtonMaps.find(controller_key);

			if (it != padButtonMaps.end()) {
				const wchar_t* buttonImage = nullptr;

				switch (controller_type) {
				case Xbox360:
					buttonImage = it->second.xbox;
					break;
				case PS3:
					buttonImage = it->second.ps3;
					break;
				case SteamDeck:
					buttonImage = it->second.steamdeck;
					break;
				case XboxSeriesX:
					buttonImage = it->second.xboxseriesx;
					break;
				case nx:
					buttonImage = it->second.nx;
					break;
				case PS5:
					buttonImage = it->second.ps5;
					break;
				default:
					buttonImage = it->second.xbox; // Default to Xbox360
					break;
				}

				wsprintf(&prompt_image_buffer[prompt_image_buffer_index], L"[format][scale:1.0][image:%s][/format]", buttonImage);
			}
			else if (controller_key == -1) {
				auto actionfinder = actionsToController.find(*action_index);
				if (actionfinder != actionsToController.end()) {
					const wchar_t* buttonImageActions = nullptr;

					switch (controller_type) {
					case Xbox360:
						buttonImageActions = actionfinder->second.xbox;
						break;
					case PS3:
						buttonImageActions = actionfinder->second.ps3;
						break;
					case SteamDeck:
						buttonImageActions = actionfinder->second.steamdeck;
						break;
					case XboxSeriesX:
						buttonImageActions = actionfinder->second.xboxseriesx;
						break;
					case nx:
						buttonImageActions = actionfinder->second.nx;
						break;
					case PS5:
						buttonImageActions = actionfinder->second.ps5;
						break;
					default:
						buttonImageActions = actionfinder->second.xbox;
						break;
					}

					wsprintf(&prompt_image_buffer[prompt_image_buffer_index], L"[format][scale:1.0][image:%s][/format]", buttonImageActions);
				}
				else {
					wsprintf(&prompt_image_buffer[prompt_image_buffer_index], L"[format][scale:0.7]Action %d[/format]", *action_index);
				}
			}
			else {
				wsprintf(&prompt_image_buffer[prompt_image_buffer_index], L"[format][scale:0.7]Pad %d[/format]", controller_key + 1);
				Logger::TypedLog("ERROR", "Was supposed to give an item out! pad_button %d actions %d button %d ,mouse %d\n", controller_key, *action_index, keyboard_key, mouse);
			}

			prompt_image_buffer_index += wcslen(&prompt_image_buffer[start_index]) + 1;
			return &prompt_image_buffer[start_index];
		}
		else if (action_index) {
			int keyboard_key = PC_port_key_for_controler_assignments[*action_index + 2].keyboard_button;
			if (keyboard_key == 0x103 || keyboard_key == 0x104) {
				if (prompt_image_buffer_index > 9500) {
					prompt_image_buffer_index = 0;
				}
				int start_index = prompt_image_buffer_index;
				if (keyboard_key == 0x104)
					wsprintf(&prompt_image_buffer[prompt_image_buffer_index], L"[format][color:purple]%s[/format]", L"Mouse 5");
				else if (keyboard_key == 0x103)
					wsprintf(&prompt_image_buffer[prompt_image_buffer_index], L"[format][color:purple]%s[/format]", L"Mouse 4");
				prompt_image_buffer_index += wcslen(&prompt_image_buffer[start_index]) + 1;
				return &prompt_image_buffer[start_index];
			}
		}
		return getpckeyboardimage_T.ccall<wchar_t*>(action_index, mouse);
	}
	SafetyHookInline pc_get_action_pad_pure_text_T{};
	// This is wrapped around by the "pc_get_action_pad_pure_text" lua function the game has, vanilla returns Pad Button: %d and the index for it, there's a pc_get_action_key_pure_text to return keyboards.
	// Here we'll just modify it to return Xbox buttons so it's more readable in the menus.
	wchar_t* __cdecl pc_get_action_pad_pure_text_hook(int action_index) {
		if (prompt_image_buffer_index > 9500) {
			prompt_image_buffer_index = 0;
		}
		int start_index = prompt_image_buffer_index;
		if (prompt_image_buffer_index > 9500) {
			prompt_image_buffer_index = 0;
		}
		int pad_button = PC_port_key_for_controler_assignments[action_index + 2].controller_button; // No +1
		auto it = padButtonToText.find(pad_button);
		if (it != padButtonToText.end()) {
			wsprintf(&prompt_image_buffer[prompt_image_buffer_index], L"%s : %d", it->second,pad_button + 1);
		}
		else {
			pc_get_action_pad_pure_text_T.ccall<wchar_t*>(action_index);
		}
		prompt_image_buffer_index += wcslen(&prompt_image_buffer[start_index]) + 1;

		return &prompt_image_buffer[start_index];
	}
	int* input_capturing = (int*)0xE82F6C;
	int __declspec(naked) pc_port_action_register_maybe(int button) {
		static const DWORD pc_port_action_register_maybe_addr = 0xC12450;
		__asm pushad
		__asm {
			push ebp
			mov ebp, esp
			sub esp, __LOCAL_SIZE
			mov     esi, button
			call pc_port_action_register_maybe_addr
			mov esp, ebp
			pop ebp
			popad
			ret
		}
	}
	static inline void setAL(SafetyHookContext& ctx, int val)
	{
		ctx.eax = (ctx.eax & 0xFFFFFF00) | (val & 0xFF);
	}

	SAFETYHOOK_NOINLINE void special_mouse_cases_midhook1(SafetyHookContext& ctx) {
		if (IsKeyPressed(VK_XBUTTON1, true)) {
			pc_port_action_register_maybe(0x103);
			PC_port_key_for_controler_assignments[*input_capturing + 2].keyboard_button = 0x103;
			setAL(ctx, true);
		}
		if (IsKeyPressed(VK_XBUTTON2, true)) {
			pc_port_action_register_maybe(0x104);
			PC_port_key_for_controler_assignments[*input_capturing + 2].keyboard_button = 0x104;
			setAL(ctx, true);
		}
	}
	struct SR2_SPECIAL_KEYBINDINGS
	{
		const char* localization_name;
		DWORD key_code;
	};
	struct SR2_SPECIAL_KEYBINDINGS specialKeys[] = {
	{"PCKEY_ARROW_LEFT", 0xCB},
	{"PCKEY_ARROW_RIGHT", 0xCD},
	{"PCKEY_ARROW_UP", 0xC8},
	{"PCKEY_ARROW_DOWN", 0xD0},
	{"PCKEY_LEFT_MOUSE_BUTTON", 0x100},
	{"PCKEY_RIGHT_MOUSE_BUTTON", 0x101},
	{"PCKEY_MIDDLE_MOUSE_BUTTON", 0x102},
	{"PCKEY_ENTER", 0x1C},
	{"PCKEY_ESC", 0x01},
	{"PCKEY_TAB", 0x0F},
	{"PCKEY_BACKSPACE", 0x0E},
	{"PCKEY_SPACEBAR", 0x39},
	{"PCKEY_LEFT_SHIFT", 0x2A},
	{"PCKEY_RIGHT_SHIFT", 0x36},
	{"PCKEY_LEFT_CONTROL", 0x1D},
	{"PCKEY_RIGHT_CONTROL", 0x9D},
	{"PCKEY_LEFT_ALT", 0x38},
	{"PCKEY_RIGHT_ALT", 0xB8},
	{"PCKEY_INSERT", 0xD2},
	{"PCKEY_DELETE", 0xD3},
	{"PCKEY_HOME", 0xC7},
	{"PCKEY_END", 0xCF},
	{"PCKEY_PAGE_UP", 0xC9},
	{"PCKEY_PAGE_DOWN", 0xD1},
	{"PCKEY_NUM_ENTER", 0x9C},
	{"PCKEY_NUM_LOCK", 0xC5},
	{"PCKEY_NUM_SLASH", 0xB5},
	{"PCKEY_NUM_MINUS", 0x4A},
	{"PCKEY_NUM_STAR", 0x37},
	{"PCKEY_NUM_PLUS", 0x4E},
	{"PCKEY_NUM_DOT", 0x53},
	{"MOUSE 4", 0x103},
	{"MOUSE 5", 0x104},
	{"PCKEY_UNASSIGNED", 0xFFFFFFFF}
	};

	SAFETYHOOK_NOINLINE void key_held_midhook_special_mousecase_midhook1(SafetyHookContext& ctx) {
		int key = ctx.eax;
		if (key == 0x103) {
			setAL(ctx, IsKeyPressed(VK_XBUTTON1, true));
			// do a ret
			ctx.eip = 0xC11220;
		} else if (key == 0x104) {
			setAL(ctx, IsKeyPressed(VK_XBUTTON2, true));
			// do a ret
			ctx.eip = 0xC11220;
		}

	}
	volatile char KEY_inventory_up = 'W';
	volatile char KEY_inventory_down = 'S';
	volatile char KEY_inventory_left = 'A';
	volatile char KEY_inventory_right = 'D';
	inline void patch_zoom_aware_interior_pause_map() {
		patchNop((void*)0x771DB7, 0x2E);
		static auto zoom_aware_interior_pause_map = safetyhook::create_mid(0x771DB7, [](SafetyHookContext& ctx) {
			using namespace UtilsGlobal;
			vector2* pause_map_crosshair = (vector2*)(0x29C8E1C);
			float zoom_modifier = 1.f;
			if (*(bool*)0x2528269)
				zoom_modifier = (*(float*)0x1F7A8CC / *(float*)0x00E8DFA8) * 0.8f;
			pause_map_crosshair->x -= ((float)mouse().getXdelta() * mouse().getMouseX_sens()) * zoom_modifier;
			pause_map_crosshair->y += ((float)mouse().getYdelta() * mouse().getMouseY_sens()) * zoom_modifier;
			});
	}

	inline void controller_disconect_dialog_prevframe() {
		*(bool*)0xE8DFBF = false;
	}

	int __declspec(naked) controller_flushasm(controller* a1) {
		static const DWORD controllerflush_addr = 0x75C700;
		__asm {
			push ebp
			mov ebp, esp
			sub esp, __LOCAL_SIZE
			mov     eax, a1
			call controllerflush_addr
			mov esp, ebp
			pop ebp
			ret
		}
	}

	void controller_flush(controller* a1) {
		__asm pushad
		__asm pushfd
		controller_flushasm(a1);
		__asm popfd
		__asm popad
	}

	int __declspec(naked) set_deadzoneasm(float* x, float* y, float deadzone) {
		static const DWORD set_deadzone_addr = 0xC13A90;
		__asm {
			push ebp
			mov ebp, esp
			sub esp, __LOCAL_SIZE

			mov     edi, x

			mov     esi, y

			push deadzone

			call set_deadzone_addr
			mov esp, ebp
			pop ebp
			ret
		}
	}

	void __stdcall set_deadzone(float* x, float* y, float deadzone) {
		__asm pushad
		__asm pushfd
		set_deadzoneasm(x, y, deadzone);
		__asm popfd
		__asm popad
	}

	void __cdecl input_pc_poll_sdl(controller* cntrl)
	{
		bool& pad_is_connected = *(bool*)0x0252A58E;
		SDL_Event event;
		const char* GamepadName = NULL;
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_EVENT_GAMEPAD_ADDED) {
				if (!g_gamepad) {
					g_gamepad = SDL_OpenGamepad(event.gdevice.which);
					if (g_gamepad) {
						if (!pad_is_connected) {
							controller_disconect_dialog_prevframe();
						}
						pad_is_connected = 1;
						GamepadName = SDL_GetGamepadName(g_gamepad);
						if(GamepadName)
						Logger::TypedLog("SDL", "%s connected\n", GamepadName);
					}
				}
			}
			else if (event.type == SDL_EVENT_GAMEPAD_REMOVED) {
				if (g_gamepad && event.gdevice.which == SDL_GetGamepadID(g_gamepad)) {
					GamepadName = SDL_GetGamepadName(g_gamepad);
					if (GamepadName)
						Logger::TypedLog("SDL", "%s disconnected\n", GamepadName);
					SDL_CloseGamepad(g_gamepad);
					g_gamepad = NULL;
					if (pad_is_connected) {
						controller_flush(cntrl);
					}
					pad_is_connected = 0;
				}
			}
		}

		// Check if we need to find/reconnect a gamepad
		if (!g_gamepad) {
			// Try to find and open the first available gamepad
			int num_joysticks;
			SDL_JoystickID* joysticks = SDL_GetJoysticks(&num_joysticks);
			if (joysticks) {
				for (int i = 0; i < num_joysticks; i++) {
					if (SDL_IsGamepad(joysticks[i])) {
						g_gamepad = SDL_OpenGamepad(joysticks[i]);
						if (g_gamepad) {
							if (!pad_is_connected) {
								controller_disconect_dialog_prevframe();
							}
							pad_is_connected = 1;
							SDL_free(joysticks);
							break;
						}
					}
				}
				SDL_free(joysticks);
			}

			// No gamepad found
			if (!g_gamepad) {
				if (pad_is_connected) {
					controller_flush(cntrl);
				}
				pad_is_connected = 0;
				return;
			}
		}

		// Gamepad is connected, read input
		if (!pad_is_connected) {
			controller_disconect_dialog_prevframe();
		}
		pad_is_connected = 1;
		cntrl->di_buttons[0] = SDL_GetGamepadButton(g_gamepad, SDL_GAMEPAD_BUTTON_SOUTH) ? 128 : 0;  // A
		cntrl->di_buttons[1] = SDL_GetGamepadButton(g_gamepad, SDL_GAMEPAD_BUTTON_EAST) ? 128 : 0;   // B
		cntrl->di_buttons[2] = SDL_GetGamepadButton(g_gamepad, SDL_GAMEPAD_BUTTON_WEST) ? 128 : 0;   // X
		cntrl->di_buttons[3] = SDL_GetGamepadButton(g_gamepad, SDL_GAMEPAD_BUTTON_NORTH) ? 128 : 0;  // Y
		cntrl->di_buttons[4] = SDL_GetGamepadButton(g_gamepad, SDL_GAMEPAD_BUTTON_LEFT_SHOULDER) ? 128 : 0; // LB
		cntrl->di_buttons[5] = SDL_GetGamepadButton(g_gamepad, SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER) ? 128 : 0;// RB
		cntrl->di_buttons[6] = SDL_GetGamepadButton(g_gamepad, SDL_GAMEPAD_BUTTON_BACK) ? 128 : 0;         // Back
		cntrl->di_buttons[7] = SDL_GetGamepadButton(g_gamepad, SDL_GAMEPAD_BUTTON_START) ? 128 : 0;        // Start
		cntrl->di_buttons[8] = SDL_GetGamepadButton(g_gamepad, SDL_GAMEPAD_BUTTON_LEFT_STICK) ? 128 : 0;   // LS
		cntrl->di_buttons[9] = SDL_GetGamepadButton(g_gamepad, SDL_GAMEPAD_BUTTON_RIGHT_STICK) ? 128 : 0;  // RS

		auto left_trigger = SDL_GetGamepadAxis(g_gamepad, SDL_GAMEPAD_AXIS_LEFT_TRIGGER);
		auto right_trigger = SDL_GetGamepadAxis(g_gamepad, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER);

		cntrl->di_buttons[10] = (left_trigger > 3932) ? (left_trigger * 128) / 32767 : 0;  // ~3932 = 30/255 * 32767 deadzone
		cntrl->di_buttons[11] = (right_trigger > 3932) ? (right_trigger * 128) / 32767 : 0;

		// D-pad
		cntrl->di_buttons[16] = SDL_GetGamepadButton(g_gamepad, SDL_GAMEPAD_BUTTON_DPAD_RIGHT) ? 128 : 0;  // bit 8 = RIGHT
		cntrl->di_buttons[17] = SDL_GetGamepadButton(g_gamepad, SDL_GAMEPAD_BUTTON_DPAD_UP) ? 128 : 0;     // bit 1 = UP
		cntrl->di_buttons[18] = SDL_GetGamepadButton(g_gamepad, SDL_GAMEPAD_BUTTON_DPAD_LEFT) ? 128 : 0;   // bit 4 = LEFT  
		cntrl->di_buttons[19] = SDL_GetGamepadButton(g_gamepad, SDL_GAMEPAD_BUTTON_DPAD_DOWN) ? 128 : 0;   // bit 2 = DOWN
		//printf("cntrl all di_buttons %p\n", cntrl);
		bool& g_CapturePadInput = *(bool*)0x252A5B0;
		bool* g_PadCaptureStates = (bool*)0x2349808;
		if (g_CapturePadInput) {
			for (unsigned int i = 0; i < cntrl->di_num_buttons; ++i) {
				g_PadCaptureStates[i] = cntrl->di_buttons[i] != 0;
			}
			*(bool*)0x2349818 = cntrl->di_buttons[16] != 0;  // Up
			*(bool*)0x2349819 = cntrl->di_buttons[17] != 0;  // Down
			*(bool*)0x234981A = cntrl->di_buttons[18] != 0;  // Left
			*(bool*)0x234981B = cntrl->di_buttons[19] != 0;  // Right
		}
		else {
			float axis_reading[4]{};
			axis_reading[0] = -(float)SDL_GetGamepadAxis(g_gamepad, SDL_GAMEPAD_AXIS_LEFTY) / 32767.0f;    // Left Y (inverted)
			axis_reading[1] = (float)SDL_GetGamepadAxis(g_gamepad, SDL_GAMEPAD_AXIS_LEFTX) / 32767.0f;     // Left X
			axis_reading[2] = -(float)SDL_GetGamepadAxis(g_gamepad, SDL_GAMEPAD_AXIS_RIGHTY) / 32767.0f;   // Right Y (inverted)
			axis_reading[3] = (float)SDL_GetGamepadAxis(g_gamepad, SDL_GAMEPAD_AXIS_RIGHTX) / 32767.0f;    // Right X

			int* axis_bind = (int*)0x234E768;

			cntrl->joys[4].x_val = axis_reading[axis_bind[0]];
			cntrl->joys[4].y_val = axis_reading[axis_bind[1]];
			cntrl->joys[3].x_val = axis_reading[axis_bind[2]];
			cntrl->joys[3].y_val = axis_reading[axis_bind[3]];

			// Apply deadzones
			set_deadzone(&cntrl->joys[4].x_val, &cntrl->joys[4].y_val, cntrl->joys[4].deadzone);
			set_deadzone(&cntrl->joys[3].x_val, &cntrl->joys[3].y_val, cntrl->joys[3].deadzone);

			// Fallback assignments
			if (cntrl->joys[0].x_val == 0.0f) cntrl->joys[0].x_val = cntrl->joys[4].x_val;
			if (cntrl->joys[0].y_val == 0.0f) cntrl->joys[0].y_val = cntrl->joys[4].y_val;
			if (cntrl->joys[1].x_val == 0.0f) cntrl->joys[1].x_val = cntrl->joys[3].x_val;
			if (cntrl->joys[1].y_val == 0.0f) cntrl->joys[1].y_val = cntrl->joys[3].y_val;
			if (cntrl->joys[6].x_val == 0.0f) cntrl->joys[6].x_val = cntrl->joys[3].x_val;
			if (cntrl->joys[6].y_val == 0.0f) cntrl->joys[6].y_val = cntrl->joys[3].y_val;
			if (cntrl->joys[7].x_val == 0.0f) cntrl->joys[7].x_val = cntrl->joys[4].x_val;
			if (cntrl->joys[7].y_val == 0.0f) cntrl->joys[7].y_val = cntrl->joys[4].y_val;
			if (cntrl->joys[2].x_val == 0.0f) cntrl->joys[2].x_val = cntrl->joys[0].x_val;
			if (cntrl->joys[2].y_val == 0.0f) cntrl->joys[2].y_val = cntrl->joys[0].y_val;

			if (!cntrl->di_buttons) {
				printf("ERROR: di_buttons became NULL, skipping button mapping addr %p value %p cntrl %p\n", &cntrl->di_buttons, cntrl->di_buttons,cntrl);
				return;
			}


			for (int i = 0; i < 87; ++i) {
				int controller_button = PC_port_key_for_controler_assignments[i + 2].controller_button;


				if (controller_button != -1 && cntrl->buttons && cntrl->buttons[i].value == 0.0f) {
					cntrl->buttons[i].value = (float)cntrl->di_buttons[controller_button] / 128.0f;
				}
			}
		}
	}

	int __stdcall XInputSetState_SDL_replace(int dwUserIndex, XINPUT_VIBRATION* pVibration) {
		// Ignore dwUserIndex since we only have one gamepad
		if (!g_gamepad || !pVibration) {
			return -1;  // Error
		}


		return SDL_RumbleGamepad(g_gamepad, pVibration->wLeftMotorSpeed, pVibration->wRightMotorSpeed, 0);
	}


	int input_pc_init_sdl()
	{
		if (SDL_AddGamepadMappingsFromFile("gamecontrollerdb.txt") == -1) {
			Logger::TypedLog("SDL", "Could not load gamecontrollerdb.txt: %s\n", SDL_GetError());
		}
		patchNop((void*)0xC131CF, 2);
		patchNop((void*)0xC133A2, 0xA);
		patchNop((void*)0xC133C0, 0xA);
		patchNop((void*)0xC133DE, 0xA);
		patchCall((void*)0xC14A06, &XInputSetState_SDL_replace);
		return SDL_Init(SDL_INIT_GAMEPAD);

	}

	void Init() {
		if (GameConfig::GetValue("Input", "SDL", 1) != 0) {
			input_pc_init_sdl();
			patchJmp((void*)0xC13C80, &input_pc_poll_sdl);
		}
		patch_zoom_aware_interior_pause_map();
		static auto tag_shake_frametimefix = safetyhook::create_mid(0x621C04, [](SafetyHookContext& ctx) {
			if (g_lastInput == MOUSE) {
				*(float*)(ctx.esp + 0x10) *= Game::Timer::Get33msOverFrameTime_Fix();
			}
			});
		if (GameConfig::GetValue("Gameplay", "allow_reloading_in_cars", 0) != 0) {
			patchDWord((void*)(0x4F61D6 + 6), 0xC20100);
		}
		KEY_inventory_up = GameConfig::GetChar("Input", "KEY_inventory_up", KEY_inventory_up);
		KEY_inventory_down = GameConfig::GetChar("Input", "KEY_inventory_down", KEY_inventory_down);
		KEY_inventory_left = GameConfig::GetChar("Input", "KEY_inventory_left", KEY_inventory_left);
		KEY_inventory_right = GameConfig::GetChar("Input", "KEY_inventory_right", KEY_inventory_right);
		if (GameConfig::GetValue("Input", "better_inventory_keyboard", 1)) {
			static auto properInventory_keyboard = safetyhook::create_mid(0xB997F3, [](SafetyHookContext& ctx) {
				if (!ctx.ebx || *is_controller_connect) // shouldn't allow this to work while a controller is connected, vanilla game bug but having a controller connected + using WASD will move the weapon wheel as if it's LS. -- Clippy95
					return;
				float& value = *(float*)(ctx.esp + 0x10);
				const char* event = (const char*)*(uintptr_t*)(ctx.ebx + 0x14);
				if (!event)
					return;
				if (strcmp(event, "inventory_up") == 0)
					value = IsKeyPressed(KEY_inventory_up, true) ? 1.0f : value;
				else if (strcmp(event, "inventory_down") == 0)
					value = IsKeyPressed(KEY_inventory_down, true) ? 1.0f : value;
				else if (strcmp(event, "inventory_left") == 0)
					value = IsKeyPressed(KEY_inventory_left, true) ? 1.0f : value;
				else if (strcmp(event, "inventory_right") == 0)
					value = IsKeyPressed(KEY_inventory_right, true) ? 1.0f : value;
				});
		}
		OptionsManager::registerOption("Input", "HoldFineAim", &HoldFineAim);
		if (EnableDynamicPrompts) {
			OptionsManager::registerOption("Input", "ForceInputPrompt", &ForceInput, 0);
			OptionsManager::registerOption("Input", "ForcedControllerPrompts", (int*)&forced_current_controller_type, 0);
			//SetThreadPriority(CreateThread(0, 0, LastInputCheck, 0, 0, 0),-1);
			pc_get_action_pad_pure_text_T = safetyhook::create_inline(0xC11A90, &pc_get_action_pad_pure_text_hook);
			getpckeyboardimage_T = safetyhook::create_inline(0xC11C00, &getpckeyboardimage_hook);

			// currently I add mouse4 and mouse5 support, requires getpckeyboardimage_T hook.
			static auto key_held_hook = safetyhook::create_mid(0xC11214, &key_held_midhook_special_mousecase_midhook1);
			static auto special_mouse_cases_midhook = safetyhook::create_mid(0xC12A74, &special_mouse_cases_midhook1);
			//patchDWord((void*)(0xC119B0 + 2), (uint32_t)&specialKeys[0].key_code);
			//patchDWord((void*)(0xC119C2 + 2), (uint32_t)&specialKeys[0].localization_name);
			//patchDWord((void*)(0xC119CD + 3), (uint32_t)&specialKeys[0].localization_name);

			//auto pattern = hook::pattern("A0 37 E8 00");
			//pattern.for_each_result([](hook::pattern_match match) {
			//	void* addr = match.get<void*>();
			//	patchDWord((void*)addr, (uint32_t)&specialKeys[0].localization_name);
			//	});

			//pattern = hook::pattern("A4 37 E8 00");
			//pattern.for_each_result([](hook::pattern_match match) {
			//	void* addr = match.get<void*>();
			//	patchDWord((void*)addr, (uint32_t)&specialKeys[0].key_code);
			//	});

		}
		LoadXInputDLL();

			player_autoaim_do_assisted_aiming_midhook = safetyhook::create_mid(0x009D7752, &player_autoaim_do_assisted_aiming_midhookfunc_disableaimassistmouse);
			Logger::TypedLog(CHN_MOD, "Disabling Aim Assist while using mouse...\n");
			disable_aim_assist_noMatterInput = (BYTE)std::clamp((int)GameConfig::GetValue("Gameplay", "DisableAimAssist", 1), 0, 2);
		DisableXInput();
		ForceNoVibration();

		if (GameConfig::GetValue("Debug", "DisableDInput", 1))
		{
			patchByte((void*)0xC147D4, 0xC); // jnz 0xC147E1
		}

		if (GameConfig::GetValue("Gameplay", "TagHook", 1))
		{
			patchDWord((void*)0xDF77FC, (uint32_t)&subT_6218F0);
			patchNop((BYTE*)0x006221AA, 6); // Original stores for Tags, X and Y.
			patchNop((BYTE*)0x00622189, 6);
			Logger::TypedLog(CHN_DEBUG, "Replaced Tags controls with TagHook\n");
		}

		if (GameConfig::GetValue("Gameplay", "BetterPlayerWardrobeRotate", 1))
		{
			Logger::TypedLog(CHN_MOD, "Patching better player rotation for wardrobes.\n");
			patchCall((int*)0x007CE170, (int*)0x0073FA80);
			patchNop((int*)0x7CE168, 2);
		}

		if (GameConfig::GetValue("Gameplay", "SwapScrollWheel", 0))
		{
			// TODO: maybe have optional options for when to swap scroll? like map only or weapon wheel only, mouse function is done at 0x00C1F320.

			patchBytesM((BYTE*)0x00C1F0F7, (BYTE*)"\x29", 1); // opcode for sub, add previously.
		}
		static auto map_zoom_fps_controller_fix = safetyhook::create_mid(0x770D5A, [](SafetyHookContext& ctx) {
			using namespace Game::Timer;
			if (Input::LastInput() != Input::CONTROLLER)
				return;
			float* zoom_delta = (float*)(ctx.esp + 0x10);
			*zoom_delta = *zoom_delta * GetFrameTimeOver33ms_Fix();
			});
	}
}