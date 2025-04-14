#pragma once
#include <safetyhook.hpp>
namespace Input {
	extern void Init();
	enum GAME_LAST_INPUT : BYTE {
		UNKNOWN,
		MOUSE,
		CONTROLLER,
	};
	inline bool betterTags;
	extern void UnloadXInputDLL();
	extern BYTE disable_aim_assist_noMatterInput;
	extern SafetyHookMid player_autoaim_do_assisted_aiming_midhook;
}