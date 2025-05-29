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
bool IsKeyPressed(unsigned char Key, bool Hold);
namespace Input {
	// DO NOT CHANGE WHILE GAME IS RUNNING, only meant to disable on runtime. if it causes any issues (hopefully none) 
	BYTE EnableDynamicPrompts = 2;
	bool allow_hacked_inventory_KBM;
	int HoldFineAim = false;
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
		if (LeftStickX != 0.f || LeftStickY != 0.f)
			g_lastInput = CONTROLLER;

		if (mouse().getXdelta() || mouse().getYdelta())
			g_lastInput = MOUSE;

		if (g_lastInputPrevFrame != g_lastInput)
			refreshVintStrings();
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
	volatile float aim_assist_empty_buffer[18]{};
	SafetyHookMid player_autoaim_do_assisted_aiming_midhook;
	BYTE disable_aim_assist_noMatterInput;
	SAFETYHOOK_NOINLINE void player_autoaim_do_assisted_aiming_midhookfunc_disableaimassistmouse(safetyhook::Context32& ctx) {
		if(disable_aim_assist_noMatterInput == 2 ||LastInput() == GAME_LAST_INPUT::MOUSE)
		ctx.esi = (uintptr_t)&aim_assist_empty_buffer;
	}

	volatile char KEY_inventory_up = 'W';
	volatile char KEY_inventory_down = 'S';
	volatile char KEY_inventory_left = 'A';
	volatile char KEY_inventory_right = 'D';
	void Process_Inventory_Hack_KBM() {
#define ARROW_UP (bool*)0x23494E0
#define ARROW_DOWN (bool*)0x2349540
#define ARROW_RIGHT (bool*)0x0234951C 
#define ARROW_LEFT (bool*)0x02349504
		static bool was_up_pressed = false;
		static bool was_down_pressed = false;
		static bool was_left_pressed = false;
		static bool was_right_pressed = false;

		if ((*(byte*)0x00E863C8 == 36)) {
			// Handle UP key
			bool is_up_pressed_now = IsKeyPressed(KEY_inventory_up, true);
			if (is_up_pressed_now) {
				*ARROW_UP = 1;
				was_up_pressed = true;
			}
			else if (was_up_pressed) {
				*ARROW_UP = 0;
				was_up_pressed = false;
			}


			bool is_down_pressed_now = IsKeyPressed(KEY_inventory_down, true);
			if (is_down_pressed_now) {
				*ARROW_DOWN = 1;
				was_down_pressed = true;
			}
			else if (was_down_pressed) {
				*ARROW_DOWN = 0;
				was_down_pressed = false;
			}


			bool is_left_pressed_now = IsKeyPressed(KEY_inventory_left, true);
			if (is_left_pressed_now) {
				*ARROW_LEFT = 1;
				was_left_pressed = true;
			}
			else if (was_left_pressed) {
				*ARROW_LEFT = 0;
				was_left_pressed = false;
			}


			bool is_right_pressed_now = IsKeyPressed(KEY_inventory_right, true);
			if (is_right_pressed_now) {
				*ARROW_RIGHT = 1;
				was_right_pressed = true;
			}
			else if (was_right_pressed) {
				*ARROW_RIGHT = 0;
				was_right_pressed = false;
			}
		}
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
	};

	std::unordered_map<int, ControllerMapping> padButtonMaps = {
		{0, {L"ui_ctrl_360_btn_a", L"ui_ctrl_ps3_btn_cross"}},
		{1, {L"ui_ctrl_360_btn_b", L"ui_ctrl_ps3_btn_circle"}},
		{2, {L"ui_ctrl_360_btn_x", L"ui_ctrl_ps3_btn_square"}},
		{3, {L"ui_ctrl_360_btn_y", L"ui_ctrl_ps3_btn_triangle"}},
		{4, {L"ui_ctrl_360_btn_lb", L"ui_ctrl_ps3_btn_l1"}},
		{5, {L"ui_ctrl_360_btn_rb", L"ui_ctrl_ps3_btn_r1"}},
		{6, {L"ui_ctrl_360_btn_back", L"ui_ctrl_ps3_btn_select"}},
		{7, {L"ui_ctrl_360_btn_start", L"ui_ctrl_ps3_btn_start"}},
		{8, {L"ui_ctrl_360_btn_ls", L"ui_ctrl_ps3_btn_l3"}},
		{9, {L"ui_ctrl_360_btn_rs", L"ui_ctrl_ps3_btn_r3"}},
		{10, {L"ui_ctrl_360_btn_lt", L"ui_ctrl_ps3_btn_l2"}},
		{11, {L"ui_ctrl_360_btn_rt", L"ui_ctrl_ps3_btn_r2"}},
		{16, {L"ui_ctrl_360_dpad_r", L"ui_ctrl_ps3_dpad_r"}},
		{17, {L"ui_ctrl_360_dpad_u", L"ui_ctrl_ps3_dpad_u"}},
		{18, {L"ui_ctrl_360_dpad_l", L"ui_ctrl_ps3_dpad_l"}},
		{19, {L"ui_ctrl_360_dpad_d", L"ui_ctrl_ps3_dpad_d"}}
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

	std::unordered_map<int, const wchar_t*> actionsToXbox = {
	{2, L"ui_ctrl_360_btn_ls"},
	{3, L"ui_ctrl_360_btn_ls"},
	};

	std::unordered_map<int, const wchar_t*> actionsToPS3 = {
	{2, L"ui_ctrl_ps3_btn_l3"},
	{3, L"ui_ctrl_ps3_btn_l3"},
	};

	bool useTextPrompts = false;
	int usePS3Prompts = false;
	wchar_t* __cdecl getpckeyboardimage_hook(uint32_t* action_index, int mouse) {
		if (LastInputUI() == GAME_LAST_INPUT::CONTROLLER) {
			if (prompt_image_buffer_index > 9500) {
				prompt_image_buffer_index = 0;
			}
			int start_index = prompt_image_buffer_index;
			if (!action_index) {
				if (mouse == 1) {
					const wchar_t* buttonImage = usePS3Prompts ? L"ui_ctrl_ps3_btn_r3" : L"ui_ctrl_360_btn_rs";
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
				const wchar_t* buttonImage = usePS3Prompts ? it->second.ps3 : it->second.xbox;
				wsprintf(&prompt_image_buffer[prompt_image_buffer_index], L"[format][scale:1.0][image:%s][/format]", buttonImage);
			}
			else if (controller_key == -1) {
				auto& actionsMap = usePS3Prompts ? actionsToPS3 : actionsToXbox;
				auto actionfinder = actionsMap.find(*action_index);
				if (actionfinder != actionsMap.end())
					wsprintf(&prompt_image_buffer[prompt_image_buffer_index], L"[format][scale:1.0][image:%s][/format]", actionfinder->second);
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
		else {
			return getpckeyboardimage_T.ccall<wchar_t*>(action_index, mouse);
		}
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
	void Init() {
		OptionsManager::registerOption("Input", "HoldFineAim", &HoldFineAim);
		if (EnableDynamicPrompts) {
			OptionsManager::registerOption("Input", "ForceInputPrompt", &ForceInput, 0);
			OptionsManager::registerOption("Input", "usePS3Prompts", &usePS3Prompts, 0);
			//SetThreadPriority(CreateThread(0, 0, LastInputCheck, 0, 0, 0),-1);
			pc_get_action_pad_pure_text_T = safetyhook::create_inline(0xC11A90, &pc_get_action_pad_pure_text_hook);
			getpckeyboardimage_T = safetyhook::create_inline(0xC11C00, &getpckeyboardimage_hook);
		}
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
		allow_hacked_inventory_KBM = GameConfig::GetValue("Gameplay", "allow_hacked_inventory_KBM", 1);
		DisableXInput();
		ForceNoVibration();

		if (GameConfig::GetValue("Gameplay", "TagsHook", 1))
		{
			betterTags = 1;
			patchNop((BYTE*)0x006221AA, 6); // Original stores for Tags, X and Y.
			patchNop((BYTE*)0x00622189, 6);
			Logger::TypedLog(CHN_DEBUG, "Replaced Tags controls with BetterTags\n");
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

	}
}