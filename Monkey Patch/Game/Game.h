#pragma once
#include "../Patcher/CPatch.h"
#include <safetyhook.hpp>
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
	int field_24 = 1;
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
	extern CPatch CDisable_Tutorials;
	extern void Init();
	namespace Timer {
		typedef double (*havok_get_time_this_frameT)();
		extern havok_get_time_this_frameT havok_get_time_this_frame;

		extern float GetFrameTime();
		extern float GetFrameTimeOver33ms_Fix();
		extern float Get33msOverFrameTime_Fix();
		extern float GetHavokFrameTime();
		extern float GetHavokFrameTimeOver16ms_Fix();
		extern int GetFrameCount();
	}

	class hzlimiter {
		float m_accumulator = 0.0f;
		float m_interval;
		unsigned int m_last_frame = 0;
		bool m_tick_this_frame = false;


	public:
		hzlimiter(float target_fps) : m_interval(1.0f / target_fps) {}
		bool b_enabled = true;

		bool ShouldTick() {

			if (!b_enabled) {
				return true;
			}

			unsigned int frame = Timer::GetFrameCount();
			if (m_last_frame != frame) {
				m_last_frame = frame;
				m_accumulator += Timer::GetFrameTime();
				if (m_accumulator >= m_interval) {
					m_accumulator = 0.0f;
					m_tick_this_frame = true;
				}
				else {
					m_tick_this_frame = false;
				}
			}
			return m_tick_this_frame;
		}
	};

	namespace HUD {
		extern int vint_message(const wchar_t* message_text, vint_message_struct *a2);
	};
	namespace InLoop {
		extern bool IsCoopOrSP;
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
		typedef uint32_t(__thiscall* crc_strT)(const char* text);
		extern crc_strT str_to_hash;
	}
	extern SafetyHookMid FixFrametimeVehicleSkids;
	struct XTBLScanStatus {
		unsigned __int8 bitmap_sheets_scanned : 1;
		unsigned __int8 outfits_scanned : 1;
		unsigned __int8 cheats_scanned : 1;
		unsigned __int8 gotr_detected : 1;
		unsigned __int8 audio_constants_scanned : 2;
		unsigned __int8 trees_scanned : 2;
		bool overhauled_stilwater_detected() {
			return (!gotr_detected && audio_constants_scanned == 2 && trees_scanned == 2);
		}
	};
	extern XTBLScanStatus xtbl_scan_status;
	extern int s_CursorPosition;
	extern CPatch DisableDistantPeds;
	extern CPatch DisableDistantVehicles;
	extern bool bFixTireDirtFPS;
	extern void SetTauCallMe();
	extern bool FrametimeTauDampingHack;
	extern bool b_train_havok_frametime_extra_fix;
};

