#pragma once
#include <safetyhook.hpp>
namespace Input {
	extern bool useTextPrompts;
	extern void Init();
	enum GAME_LAST_INPUT : BYTE {
		UNKNOWN,
		MOUSE,
		CONTROLLER,
	};
	extern void Process_Inventory_Hack_KBM();
	extern bool allow_hacked_inventory_KBM;
	inline bool betterTags;
	extern void UnloadXInputDLL();
	extern BYTE disable_aim_assist_noMatterInput;
	extern SafetyHookMid player_autoaim_do_assisted_aiming_midhook;
	extern GAME_LAST_INPUT LastInput();
	extern GAME_LAST_INPUT LastInputUI();
	extern int usePS3Prompts;
	extern int ForceInput;
	extern BYTE EnableDynamicPrompts;
	extern int HoldFineAim;
}