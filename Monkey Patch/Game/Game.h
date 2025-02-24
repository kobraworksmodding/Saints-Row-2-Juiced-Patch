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

struct xtbl_node
{
	char* name;
	xtbl_node* next; // next_node
	xtbl_node* elements; // branch_node
	char* text;
};

namespace Game
{
	namespace HUD {
		extern int vint_message(wchar_t* message_text, vint_message_struct *a2);
	};
	namespace InLoop {
		extern bool IsCoopOrSP;
		extern bool ShouldFixStereo;
		extern char* ClanTag[3];
		extern bool UsingClanTag;
#if RELOADED
		extern void RLServerNameUpdateOnce();
#endif
		extern void FrameChecks();
	}
	namespace xml {
		typedef int(__cdecl* read_and_parse_fileT)(char* xtbl_filename, int* xtbl_mempool_loc);
		extern read_and_parse_fileT read_and_parse_file;
		extern xtbl_node* parse_table_node(const char* filename, int* override_xtbl_mempool);
		extern uint32_t checksum(xtbl_node* root, uint32_t accumulator);
	}
	namespace utils {
		typedef uint32_t(__thiscall* crc_strT)(char* text);
		extern crc_strT str_to_hash;
	}
};

