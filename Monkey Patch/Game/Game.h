#pragma once
enum vint_message_type {
	top_of_screen = 0,
	middle_of_screen = 1,
	top_of_radar = 2,
	bottom_of_screen = 3,

};
struct vint_message_struct
{
	float timeduration_seconds = 1.5f;
	float field_4 = 0.f;
	float field_8 = 0.f;
	float fade_out_time = 0.5f;
	int field_10 = 0;
	vint_message_type pos = top_of_radar;
	int field_18 = 0;
	char play_sound_effect = 0;
	char padding1 = 0 ;
	__int16 field_1E = 0;
	int field_20 = 0;
	int field_24 = 0;
};

namespace Game
{
	namespace HUD {
		extern int vint_message(wchar_t* message_text, vint_message_struct *a2);
	};
};

