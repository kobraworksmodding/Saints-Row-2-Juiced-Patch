#pragma once
namespace Input {
	extern void Init();
    
	inline bool betterTags;
}
enum GAME_LAST_INPUT: BYTE{
	UNKNOWN,
	MOUSE,
	CONTROLLER,
};