#pragma once
#include "../Patcher/CMultiPatch.h"
namespace Debug {
	extern void Init();
	extern void PatchDatafiles();
	extern bool addBindToggles;
	extern bool LoadLastSave;
	extern bool fixFrametime;
	extern int UseDynamicRenderDistance;
	extern float MAX_RENDER;
	extern float TRANSITION_SPEED;
	extern int SIZE_MIN;
	extern int MAX;
	extern CMultiPatch CMPatches_DisableCheatFlag;
	void DynamicRenderDistance();
	void PrintMemoryUsage(int y);
}

struct debug_cutscene_clock {
	float elapsed_seconds;
	float total_seconds;
	unsigned int real_start_tick;
	unsigned int real_start_time;
	int current_shot_index;
	float current_shot_elapsed_seconds;
	float current_shot_total_seconds;
	float frame_time_seconds;
	float intershot_frame_time_seconds;
	int paused_count;
	unsigned int pause_start_time;
	unsigned int last_frame_time;
	unsigned int pause_total_time;
	unsigned int real_last_time;
};

struct debug_cutscene_info
{
	const char* name;
	const wchar_t* display_name;
	bool viewed;
	char pad_09[0x3];
	char pad_0C[0x8];
};

struct __declspec(align(4)) debug_cutscene_shot
{
	const char* shot_name;
	char pad_04[0x4];
	int shot_index;
	char pad_0C[0x14C];
	float start_time_seconds;
	unsigned int duration_frames;
	char pad_160[0x4];
	float fade_out_time_from_end_seconds;
	float fade_out_duration_seconds;
	bool fade_out_letterbox;
	char pad_16D[0x3];
};

static_assert(sizeof(debug_cutscene_clock) == 0x38);
static_assert(sizeof(debug_cutscene_info) == 0x14);
static_assert(sizeof(debug_cutscene_shot) == 0x170);
