#pragma once
#include <safetyhook.hpp>

struct __declspec(align(4)) scroll_info
{
	int start_time;
	int last_scroll;
	bool scrolled;
};


struct joy
{
	float x_val;
	float y_val;
	float deadzone;
	bool invert_x;
	bool invert_y;
	bool masked_x;
	bool masked_y;
	scroll_info scroll[4];
};


struct __declspec(align(4)) button
{
	float value;
	float last_value;
	float deadzone;
	int repeat_rate;
	int repeat_delay;
	int time_down;
	int last_repeat;
	bool just_pressed;
	bool repeated;
	bool just_released;
	bool was_down;
	bool masked;
};


enum pc_controller_type : __int32
{
	PC_CNTL_TYPE_XBOX = 0x0,
	PC_CNTL_TYPE_DIRECTIUNPUT1 = 0x1,
	PC_CNTL_TYPE_DIRECTIUNPUT2 = 0x2,
	PC_CNTL_TYPE_XINPUT = 0x3,
	PC_CNTL_TYPE_DIRECTINPUT3 = 0x4,
	PC_NUM_CNTL_TYPES,
};

struct DIEFFECT
{
	unsigned int dwSize;
	unsigned int dwFlags;
	unsigned int dwDuration;
	unsigned int dwSamplePeriod;
	unsigned int dwGain;
	unsigned int dwTriggerButton;
	unsigned int dwTriggerRepeatInterval;
	unsigned int cAxes;
	unsigned int* rgdwAxes;
	int* rglDirection;
	void* lpEnvelope;
	unsigned int cbTypeSpecificParams;
	void* lpvTypeSpecificParams;
	unsigned int dwStartDelay;
};

struct DICONSTANTFORCE
{
	int lMagnitude;
};

// this is size 0x488 in SR4 but 104C in SR2 due to joy and butons?
struct __declspec(align(4)) controller
{
	_GUID guidInstance;
	button buttons[87];
	joy joys[8];
	float idle_time;
	bool active;
	pc_controller_type controller_type;
	void* di_interface;
	unsigned int di_data_size;
	unsigned int di_data_buffer[64];
	unsigned __int8* di_data;
	unsigned int di_num_buttons;
	unsigned __int8* di_buttons;
	unsigned int di_button_instance_ids[16];
	unsigned int di_num_axes;
	unsigned int* di_axes;
	unsigned int di_axis_instance_ids[16];
	unsigned int di_num_povs;
	unsigned int* di_povs;
	unsigned int di_pov_instance_ids[1];
	bool di_forcefeedback;
	void* di_ffeffect;
	DIEFFECT di_effect_struct;
	DICONSTANTFORCE di_constant_force;
	int xi_index;
	bool xi_connected;
};


namespace Input {
	extern bool useTextPrompts;
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
	extern GAME_LAST_INPUT LastInput();
	extern GAME_LAST_INPUT LastInputUI();
	extern GAME_LAST_INPUT g_lastInput;
	extern int usePS3Prompts;
	extern int ForceInput;
	extern BYTE EnableDynamicPrompts;
	extern int HoldFineAim;
	extern bool UsePS3Prompts();
}