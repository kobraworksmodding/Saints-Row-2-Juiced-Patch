#pragma once
#include <safetyhook.hpp>
namespace Render2D
{
    typedef float(__cdecl* ChangeTextColorT)(int R, int G, int B, int Alpha);
    extern ChangeTextColorT ChangeTextColor;
    extern int processtextwidth(int width);

    extern void Init();
    extern void InitMenVerNum();
    extern void InGamePrint(const char* Text, int x, int y, int font);

    extern bool BetterChatTest;
	extern bool IVRadarScaling;
	extern float RadarScale;
	extern const float widescreenvalue;
	extern float* currentAR;
	extern bool UltrawideFix;
	extern float get_vint_x_resolution();
	extern char SR2Ultrawide_HUDScale();
	extern SafetyHookMid vint_create_process_hook;
	extern void create_process_hook(safetyhook::Context32& ctx);
	extern void RadarScaling();
	extern void VintScaleIV();
	struct color
	{
		unsigned __int8 b;
		unsigned __int8 g;
		unsigned __int8 r;
		unsigned __int8 a;
		int unk11;
	};

	struct texture_2d
	{
		float x;
		float y;
		float z;
		color color_info;
		float unk1;
		float unk2;
	};
	extern SafetyHookMid final_2d_render;
	extern float get_vint_1610_hack_scale();
	extern inline bool is_aspect_1610();
}