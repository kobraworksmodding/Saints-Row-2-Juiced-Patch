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

	enum vint_variant_type : __int32
	{
		VINT_PROP_TYPE_NONE = 0x0,
		VINT_PROP_TYPE_INT = 0x1,
		VINT_PROP_TYPE_UINT = 0x2,
		VINT_PROP_TYPE_FLOAT = 0x3,
		VINT_PROP_TYPE_STRING = 0x4,
		VINT_PROP_TYPE_BOOL = 0x5,
		VINT_PROP_TYPE_COLOR = 0x6,
		VINT_PROP_TYPE_VECTOR2F = 0x7,
		VINT_PROP_TYPE_CALLBACK = 0x8,
		VINT_PROP_TYPE_DOCUMENT = 0x9,
		VINT_PROP_TYPE_BITMAP = 0xA,
		VINT_PROP_TYPE_FONT = 0xB,
		VINT_PROP_TYPE_SOUND = 0xC,
		VINT_PROP_LAST_VALID_FOR_VARIANT = 0xC,
		VINT_PROP_TYPE_ENUM = 0xD,
		VINT_PROP_TYPE_VARIABLE = 0xE,
		NUM_VINT_PROP_TYPES = 0xF,
	};

	union __declspec(align(4)) vint_variant_values
	{
		float x;
		float y;
		// and more.
	};
	struct vint_variant {
		vint_variant_type type;
		vint_variant_values values;
	};
	extern const float widescreenvalue;
	extern float* currentAR;
	extern bool UltrawideFix;
	extern float get_vint_x_resolution();
	extern char SR2Ultrawide_HUDScale();
	extern SafetyHookMid vint_create_process_hook;
	extern void create_process_hook(safetyhook::Context32& ctx);
}