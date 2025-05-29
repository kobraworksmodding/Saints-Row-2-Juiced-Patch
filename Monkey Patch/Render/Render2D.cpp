// Render2D.cpp (uzis, Tervel, Clippy95)
// --------------------
// Created: 25/02/2025
#pragma warning( disable : 4834)
#pragma warning( disable : 4309)
#pragma warning( disable : 4244)
#pragma warning( disable : 4473)
#pragma warning( disable : 4996)
#include "../FileLogger.h"
#include "../Patcher/patch.h"
#include "../GameConfig.h"
#include "../SafeWrite.h"
#include "../UtilsGlobal.h"
#include "Render2D.h"
#include <safetyhook.hpp>
#include "..\General\General.h"
#include "Render3D.h"
#include "..\Ext\Hooking.Patterns.h"
#include <random>
namespace Render2D
{
	float* currentAR = (float*)0x022FD8EC;
	const float widescreenvalue = 1.777777791f;
	bool BetterChatTest = 0;
#if !JLITE
	bool IVRadarScaling = false;
	bool aspectratio_1610 = false;
	float RadarScale = 0.87272727272f;
	void RadarScaling() {
		float currentX = (float)(*(unsigned int*)0x022f63f8);
		float currentY = (float)(*(unsigned int*)0x022f63fc);

		float aspect = currentX / currentY;
		if (!IVRadarScaling && aspect < 1.45f)
			return;

		// vint scale
		auto pattern = hook::make_range_pattern(0x7A2C30, 0x007A37D2, "AC BF 2D 02");
		//Logger::TypedLog(CHN_DEBUG,"Count of vint %d \n", pattern.size());
		pattern.for_each_result([](hook::pattern_match match) {
			void* addr = match.get<void*>();
			SafeWrite32((UInt32)addr, (UInt32)&RadarScale);
			});
		// Height scale
		pattern = hook::make_range_pattern(0x7A2C30, 0x007A37D2, "A8 BF 2D 02");
		pattern.for_each_result([](hook::pattern_match match) {
			void* addr = match.get<void*>();
			SafeWrite32((UInt32)addr, (UInt32)&RadarScale);
			});

		// vint scale
		pattern = hook::make_range_pattern(0x79E970, 0x0079EB0A, "AC BF 2D 02");
		pattern.for_each_result([](hook::pattern_match match) {
			void* addr = match.get<void*>();
			SafeWrite32((UInt32)addr, (UInt32)&RadarScale);
			});

		// Height scale
		pattern = hook::make_range_pattern(0x79E970, 0x0079EB0A, "A8 BF 2D 02");
		pattern.for_each_result([](hook::pattern_match match) {
			void* addr = match.get<void*>();
			SafeWrite32((UInt32)addr, (UInt32)&RadarScale);
			});
		// vint scale
		pattern = hook::make_range_pattern(0x79EB10, 0x0079EDE3, "AC BF 2D 02");
		pattern.for_each_result([](hook::pattern_match match) {
			void* addr = match.get<void*>();
			SafeWrite32((UInt32)addr, (UInt32)&RadarScale);
			});

		// Height scale
		pattern = hook::make_range_pattern(0x79EB10, 0x0079EDE3, "A8 BF 2D 02");

		pattern.for_each_result([](hook::pattern_match match) {
			void* addr = match.get<void*>();
			SafeWrite32((UInt32)addr, (UInt32)&RadarScale);
			});

		// vint scale
		pattern = hook::make_range_pattern(0x00B877B0, 0x00B877D7, "AC BF 2D 02");

		pattern.for_each_result([](hook::pattern_match match) {
			void* addr = match.get<void*>();
			SafeWrite32((UInt32)addr, (UInt32)&RadarScale);
			});

		// height scale
		pattern = hook::make_range_pattern(0x00B877B0, 0x00B877D7, "A8 BF 2D 02");
		pattern.for_each_result([](hook::pattern_match match) {
			void* addr = match.get<void*>();
			SafeWrite32((UInt32)addr, (UInt32)&RadarScale);
			});

		// vint scale
		pattern = hook::make_range_pattern(0x79EB10, 0x0079EDE3, "AC BF 2D 02");
		pattern.for_each_result([](hook::pattern_match match) {
			void* addr = match.get<void*>();
			SafeWrite32((UInt32)addr, (UInt32)&RadarScale);
			});

		// height scale
		pattern = hook::make_range_pattern(0x79EB10, 0x0079EDE3, "A8 BF 2D 02");
		pattern.for_each_result([](hook::pattern_match match) {
			void* addr = match.get<void*>();
			SafeWrite32((UInt32)addr, (UInt32)&RadarScale);
			});

		// vint scale
		pattern = hook::make_range_pattern(0x79FEB0, 0x0079FFE5, "AC BF 2D 02");
		pattern.for_each_result([](hook::pattern_match match) {
			void* addr = match.get<void*>();
			SafeWrite32((UInt32)addr, (UInt32)&RadarScale);
			});

		// vint scale
		pattern = hook::make_range_pattern(0x79FEB0, 0x0079FFE5, "A8 BF 2D 02");
		pattern.for_each_result([](hook::pattern_match match) {
			void* addr = match.get<void*>();
			SafeWrite32((UInt32)addr, (UInt32)&RadarScale);
			});

		pattern = hook::make_range_pattern(0x5489F0, 0x00548C5C, "A? BF 2D 02");
		pattern.for_each_result([](hook::pattern_match match) {
			void* addr = match.get<void*>();
			SafeWrite32((UInt32)addr, (UInt32)&RadarScale);
			});

		pattern = hook::make_range_pattern(0x7A37E0, 0x007A3D2F, "AC BF 2D 02");
		pattern.for_each_result([](hook::pattern_match match) {
			void* addr = match.get<void*>();
			SafeWrite32((UInt32)addr, (UInt32)&RadarScale);
			});

		pattern = hook::make_range_pattern(0x7A37E0, 0x007A3D2F, "A8 BF 2D 02");
		pattern.for_each_result([](hook::pattern_match match) {
			void* addr = match.get<void*>();
			SafeWrite32((UInt32)addr, (UInt32)&RadarScale);
			});

		pattern = hook::make_range_pattern(0x0079FC92, 0x0079FD92, "AC BF 2D 02");
		pattern.for_each_result([](hook::pattern_match match) {
			void* addr = match.get<void*>();
			SafeWrite32((UInt32)addr, (UInt32)&RadarScale);
			});
		pattern = hook::make_range_pattern(0x0079FC92, 0x0079FD92, "A8 BF 2D 02");
		pattern.for_each_result([](hook::pattern_match match) {
			void* addr = match.get<void*>();
			SafeWrite32((UInt32)addr, (UInt32)&RadarScale);
			});
	}

	void VintScaleIV() {
		char buffer[256]{};
		const char* lua_command = "vint_set_property(vint_object_find(\"%s\", 0, vint_document_find(\"%s\")), \"%s\", %f, %f)";
		snprintf(buffer, sizeof(buffer), lua_command, "map_grp", "hud", "scale",
			Render2D::RadarScale, Render2D::RadarScale);
		General::VintExecute(buffer);
		snprintf(buffer, sizeof(buffer), lua_command, "map_grp", "hud", "anchor",
			50.f, 710.f);
		General::VintExecute(buffer);
		snprintf(buffer, sizeof(buffer), lua_command, "msg_diversion_anchor", "hud_msg", "scale",
			Render2D::RadarScale, Render2D::RadarScale);
		General::VintExecute(buffer);
		snprintf(buffer, sizeof(buffer), lua_command, "msg_diversion_anchor", "hud_msg", "anchor",
			75.f, 520.f);
		General::VintExecute(buffer);
	}

#endif
	ChangeTextColorT ChangeTextColor = (ChangeTextColorT)0xD14840;
	// What I use in BlingMenu to clamp mouse cursor, maybe a bit too much for Ultrawide? who's going to have an ultrawide under 720p?
	void get_vint_res_limit(float* x, float* y) {
		if (!x || !y) return;

		float currentX = (float)(*(unsigned int*)0x022F63F8);
		float currentY = (float)(*(unsigned int*)0x022F63FC);

		if (currentX <= 0 || currentY <= 0) return;

		float aspect_ratio = currentX / currentY;
		float targetX = 1280.0f;
		float targetY = 720.0f;

		if (currentX > targetX || currentY > targetY) {
			targetX = currentY * aspect_ratio;
			if (targetX > currentX) {
				targetX = currentX;
				targetY = currentX / aspect_ratio;
			}
		}
		else {
			targetX = currentX;
			targetY = currentY;
		}
		if (targetX < 1280.0f) targetX = 1280.0f;
		if (targetY < 720.0f) targetY = 720.0f;

		if (currentX < 1280.0f || currentY < 720.0f) {
			targetX = currentX;
			targetY = currentY;
		}

		*x = targetX;
		*y = targetY;
	}

	int processtextwidth(int width) {
		return width * (*currentAR / 1.77777777778f);

	}

	inline bool is_aspect_1610() {
		return *currentAR == 1.6f;
	}

	float get_vint_x_resolution() {
		if (*currentAR >= 1.77777777778f)
			return *currentAR * 720;
		// Hack to fix odd weird res widescreens like 1.6f during display changes 
		// between ultrawide and non-ultrawide, only sometimes so it's still super buggy and weird.
		else return 1280.f; 
	}
	float get_vint_1610_hack_scale() {
		if (!is_aspect_1610())
			return 0.f;
		return 80.f;
	}
	void __declspec(naked) InGamePrintASM(const char* Text, int x, int y, int font) {
		__asm {
			push ebp
			mov ebp, esp
			sub esp, __LOCAL_SIZE

			push edi
			push esi
			push eax

			mov edi, font
			mov esi, Text
			push x
			push y

			mov eax, 0xD15DC0
			call eax

			pop eax
			pop esi
			pop edi

			mov esp, ebp
			pop ebp
			ret
		}
	}

	void InGamePrint(const char* Text, int x, int y, int font) {
		int game_setting_language = *(int*)0x00E98AF8;
		switch (game_setting_language) {
		case LANG_JAPANESE:
		case LANG_CHINESE:

			font = 0; // Fixes crash for Japanese and Chinese SR2 and our InGamePrint
			break;
		}
		InGamePrintASM(Text, x, y, font);

	}

	typedef void SomeMMFunc_Native();
	SomeMMFunc_Native* UpdateSomeMMFunc = (SomeMMFunc_Native*)(0x0075B270);

	typedef void SomePMFunc_Native();
	SomePMFunc_Native* UpdateSomePMFunc = (SomePMFunc_Native*)(0x00B99DB0);
	const char* JuicedText = "JUICED ";
	void SomeMMFunc_Hacked()
	{
#if JLITE
		if (*(BYTE*)0x02527B75 == 1 && *(BYTE*)0xE8D56B == 1) {
			ChangeTextColor(160, 160, 160, 128);
			__asm pushad
			InGamePrint(("JUICED LITE " + std::string(UtilsGlobal::juicedversion)).c_str(), 680, processtextwidth(1070), 2);
			__asm popad
		}
#else
#if !RELOADED
		if (*(BYTE*)0x02527B75 == 1 && *(BYTE*)0xE8D56B == 1) {
			ChangeTextColor(160, 160, 160, 128);
			__asm pushad
			InGamePrint((JuicedText + std::string(UtilsGlobal::juicedversion)).c_str(), 680, processtextwidth(1120), 2);
			__asm popad
		}
#else
		if (*(BYTE*)0xE8D56B == 1) {
			ChangeTextColor(160, 160, 160, 128);
			__asm pushad
			InGamePrint(("THAROW " + std::string(UtilsGlobal::thaRowmenuversion)).c_str(), 680, processtextwidth(1120), 2);
			__asm popad
		}
#endif
#endif

		// Call original func
		return UpdateSomeMMFunc();
	}
	/*
	void SomePMFunc_Hacked()
	{

		if (menustatus(menustatus::pausemenu) || menustatus(menustatus::pausemenuphone) || menustatus(menustatus::pausemenuscroll2) || menustatus(menustatus::pausemenescroll1) || menustatus(menustatus::pausemenuphonebook)) {
			ChangeTextColor(160, 160, 160, 128);
			__asm pushad
			InGamePrint(("JUICED " + std::string(juicedversion)).c_str(), 680, 160, 6);
			__asm popad
		}

		// Call original func
		return UpdateSomePMFunc();
	}
	*/

	void InitMenVerNum() {
		if (GameConfig::GetValue("Debug", "MenuVersionNumber", 1))
		{
			Logger::TypedLog(CHN_MOD, "Patching MenuVersionNumber...\n");
			//patchCall((void*)0x0052050C, (void*)SomeMMFunc_Hacked);

			std::random_device rd;
			std::mt19937 gen(rd());

			std::bernoulli_distribution chance(0.01);
			if (chance(gen))
				JuicedText = "JUICER ";
			patchCall((void*)0x0073CE0D, (void*)SomeMMFunc_Hacked);
			//patchCall((void*)0x00B995D5, (void*)SomePMFunc_Hacked);
		}
	}

	void SetupBorderless()
	{
		int l_IsBorderless = GameConfig::GetValue("Graphics", "Borderless", 0);
		uint32_t window_style = l_IsBorderless ? (WS_POPUP) : (WS_CAPTION | WS_BORDER);
		patchDWord((void*)(0x00BFA35A + 4), window_style);
		patchBytesM((BYTE*)0x00BFA494, (BYTE*)"\x6A\x03", 2); //Maximise Borderless so it fits perfectly.
	}

	void SetupBetterWindowed()
	{
		int l_IsBetterWindowed = GameConfig::GetValue("Graphics", "BetterWindowed", 0);
		uint32_t windowed_style = l_IsBetterWindowed ? (WS_CAPTION | WS_OVERLAPPED | WS_SYSMENU | WS_DLGFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SIZEBOX) : (WS_DLGFRAME);
		patchDWord((void*)(0x00BFA35A + 4), windowed_style);
	}

	SafetyHookMid vint_create_process_hook;
	void create_process_hook(safetyhook::Context32& ctx) {
		const char* target_str = "safe_frame";
		uintptr_t* vint_document = (uintptr_t*)ctx.esi;
		const char* possible_str = reinterpret_cast<const char*>(&vint_document[10]);
		char cached_str[64];
		strncpy(cached_str, possible_str, sizeof(cached_str) - 1);
		cached_str[sizeof(cached_str) - 1] = '\0';
		char buffer[256];
		const char* lua_command = "vint_set_property(vint_object_find(\"%s\", 0, vint_document_find(\"%s\")), \"%s\", %f, %f)";
		const char* lua_command_visbile = "vint_set_property(vint_object_find(\"%s\", 0, vint_document_find(\"%s\")), \"%s\", %s)";
		snprintf(buffer, sizeof(buffer), lua_command,"safe_frame", cached_str,"anchor", (get_vint_x_resolution() - 1280) / 2.f, get_vint_1610_hack_scale() / 2.f);
		//Logger::TypedLog(CHN_DEBUG, "%s \n", possible_str);
		General::VintExecute(buffer);

		if (strcmp(cached_str, "hud") == 0) {

			snprintf(buffer, sizeof(buffer), lua_command_visbile, "extra_homie", "hud", "visible", "false");
			General::VintExecute(buffer);

			snprintf(buffer, sizeof(buffer), lua_command_visbile, "mp_snatch_john", "hud", "visible", "false");
			General::VintExecute(buffer);

			snprintf(buffer, sizeof(buffer), lua_command_visbile, "health_mini_grp", "hud", "visible", "false");
			General::VintExecute(buffer);


			snprintf(buffer, sizeof(buffer), lua_command_visbile, "health_large_grp", "hud", "visible", "false");
			General::VintExecute(buffer);
			snprintf(buffer, sizeof(buffer), lua_command, "mayhem_grp", "hud", "anchor", -((get_vint_x_resolution() - 1280) / 2.f), -(get_vint_1610_hack_scale() / 2.f));
			General::VintExecute(buffer);

			float weirdscale = 1.f / (widescreenvalue / *currentAR);
			float weirdscale_y = 0.f;
			if (Render2D::is_aspect_1610()) {
				weirdscale = 1.f / (*Render2D::currentAR / Render2D::widescreenvalue);
				weirdscale_y = weirdscale;
			}
				snprintf(buffer, sizeof(buffer), lua_command, "mayhem_grp", "hud", "scale", weirdscale, weirdscale_y);
				General::VintExecute(buffer);
			
			snprintf(buffer, sizeof(buffer), lua_command, "vignettes", "hud", "anchor", -((get_vint_x_resolution() - 1280) / 2.f), -(get_vint_1610_hack_scale() / 2.f));
			General::VintExecute(buffer);

			snprintf(buffer, sizeof(buffer), lua_command, "vignettes", "hud", "scale", weirdscale, weirdscale_y);
			General::VintExecute(buffer);

		}
	}

bool UltrawideFix = false;

std::thread RefreshHUD_thread;
void RefreshHUD_loop() {
	Logger::TypedLog(CHN_DEBUG, "SR2Ultrawide Refreshing HUD %d\n",2);
	vint_create_process_hook.enable();
	std::this_thread::sleep_for(std::chrono::seconds(4));
	vint_create_process_hook.disable();
}

char SR2Ultrawide_HUDScale() {
	Logger::TypedLog(CHN_DEBUG, "SR2Ultrawide Refreshing HUD %d\n", 1);
	Render3D::ChangeShaderOptions();
	float currentX = (float)(*(unsigned int*)0x022f63f8);
	float currentY = (float)(*(unsigned int*)0x022f63fc);
	char result;

	float aspectRatio = currentX / currentY;
	// Cutscene black bars
	SafeWrite32((0x00755C49 + 1), 1280);
	Render3D::AspectRatioFix(true);
	if (aspectRatio >= 1.58f) {
		SafeWrite32((0x00755C49 + 1), (uint32_t)(aspectRatio * 720));
		// Fix reflections being broken at ultrawide.
		*(float*)(0x0E86388) = aspectRatio;
#if !JLITE
		if (GameConfig::GetValue("Graphics", "IVRadarScaling", 0)) {
			IVRadarScaling = true;
			RadarScaling();
		}
#endif

		// Fucking tagging system cause yeah lets hard code the anchor for it?
	int var = (int)(aspectRatio * 720.f);
	static int var2 = (int)(aspectRatio * 360.f);
	if (aspectRatio != 1.6f) {
		SafeWrite32(0x00622571 + 1, var);
		SafeWrite32(0x00625A2B + 2, var);
		SafeWrite32(0x00625D09 + 2, (UInt32)&var2);
		SafeWrite32(0x0062597F + 2, (UInt32)&var2);
	}
	SafeWrite32(0x00622555 + 2, 720);
	SafeWrite32(0x00625A44 + 1, 720);

	SafeWrite32(0x00625D5D + 2, 0x22DBF90);
	SafeWrite32(0x006259FC + 2, 0x22DBF90);
	if (aspectRatio == 1.6f) {
		int aspect_1610 = 800;
		SafeWrite32(0x00622555 + 2, aspect_1610);
		SafeWrite32(0x00625A44 + 1, aspect_1610);
		static int aspect_1610_width = 400;
		SafeWrite32(0x00625D5D + 2, (UInt32)&aspect_1610_width);
		SafeWrite32(0x006259FC + 2, (UInt32)&aspect_1610_width);
	}
		Logger::TypedLog(CHN_DEBUG, "SR2Ultrawide Refreshing HUD %d\n", 3);
		RefreshHUD_thread = std::thread(RefreshHUD_loop);
		RefreshHUD_thread.detach();
	}
	if ((GameConfig::GetValue("Graphics", "FixUltrawideHUD", 1) == 1)) {
		if (aspectRatio <= 1.79777777778f && aspectRatio != 1.5f && aspectRatio != 1.6f) {

			UltrawideFix = false;
			General::CleanupModifiedScript();
			return ((char(*)())0xD1C910)(); // Original HUD scale function.
			
		}
		else {

			Logger::TypedLog(CHN_DEBUG, "SR2Ultrawide Refreshing HUD %d\n", 4);
			if(aspectRatio != 1.5f)
			UltrawideFix = true;
			if (aspectRatio == 1.5f) {
				General::CleanupModifiedScript();
			}
		}
	}

	float correctionFactor = 1.777777777777778f / aspectRatio;

	float stretchedX = currentX / 1280.0f;
	float adjustedX = stretchedX * correctionFactor;
	aspectratio_1610 = false;
	if (aspectRatio <= 1.59f) {
		result = 0;
		*(uint8_t*)0x0213c383 = 0;
		*(uint8_t*)0x025272dd = 0;
		*(float*)0x022fdcc0 = currentX / 640.0;
		*(float*)0x022fdcbc = currentY / 480.0f;
	}
	else {
		result = 1;
		*(uint8_t*)0x0213c383 = 1;
		*(uint8_t*)0x025272dd = 1;
		*(float*)0x022fdcc0 = adjustedX;
		*(float*)0x022fdcbc = currentY / 720.0f;
	}
	// Okay this shit is pissing me off, I really have to find a universal way of calculating HUD for all aspect ratios than hard coding this garbage in - Clippy95
	if (aspectRatio == 1.6f) {
		result = 1;
		*(uint8_t*)0x0213c383 = 1;
		*(uint8_t*)0x025272dd = 1;
		*(float*)0x022fdcc0 = currentX / 1280.0f;
		*(float*)0x022fdcbc = currentY / 800.0f; 
		aspectratio_1610 = true;
	}
	Logger::TypedLog(CHN_MOD, "SR2Ultrawide patched HUD scale X: %f Y: %f bool: %d, aspect ratio %f \n", adjustedX, *(float*)0x022fdcbc, UltrawideFix, aspectRatio);
	return result;
}
float saturate(float x) {
	return std::clamp(x, 0.0f, 1.0f);
}


float X360GammaApprox(float x) {
	const float A = 0.541901f;
	const float B = 1.13465f;
	const float C = 13.53054f;
	const float D = 6.56649f;
	const float E = 0.311465f;
	float f1 = A * x;
	float f2 = std::pow(x, B) * (1.0f - std::exp2(-C * x));
	float f3 = saturate(x * D + E);

	return std::lerp(f1, f2, f3);
}


void ApplyX360Gamma(color& col) {
	float r = col.r / 255.0f;
	float g = col.g / 255.0f;
	float b = col.b / 255.0f;

	r = X360GammaApprox(r);
	g = X360GammaApprox(g);
	b = X360GammaApprox(b);

	col.r = static_cast<unsigned __int8>(saturate(r) * 255.0f);
	col.g = static_cast<unsigned __int8>(saturate(g) * 255.0f);
	col.b = static_cast<unsigned __int8>(saturate(b) * 255.0f);
}

void fix_screen_fade_notint() {
	static auto screen_fade_notint_fix = safetyhook::create_mid(0x518F39, [](SafetyHookContext& ctx) {
		vector3* tint = (vector3*)(ctx.eax + 0xC);
		vector3* fade = (vector3*)0x00E9D670;
		*tint *= *fade;
		});
}

SafetyHookMid final_2d_render{};
	void Init() {
		// Fix vint UI speeding up at 1000?+ FPS
		fix_screen_fade_notint();
		patchNop((void*)0x00B8BC6B, 6);
		final_2d_render = safetyhook::create_mid(0xD1DFAA, [](SafetyHookContext& ctx) {
			texture_2d* pass = (texture_2d*)ctx.eax;
			ApplyX360Gamma(pass->color_info);
			},safetyhook::MidHook::StartDisabled);
		if (GameConfig::GetValue("Graphics", "X360GammaUI", 0))
			final_2d_render.enable();
		if (GameConfig::GetValue("Graphics", "Borderless", 0))
		{
			SetupBorderless();
			Logger::TypedLog(CHN_DEBUG, "Enabling Borderless Windowed.\n");
		}
		else
		{
			SetupBetterWindowed();
			Logger::TypedLog(CHN_DEBUG, "Fixing Windowed Mode.\n");
		}

		if (GameConfig::GetValue("Gameplay", "BetterChat", 1)) // changes char limit from 64 to 128 and formats the input after the 64th character
		{
			BetterChatTest = 1;
			patchBytesM((BYTE*)0x0075C91E, (BYTE*)"\xC7\x05\x1C\x69\xF7\x01\x80\x00\00\x00", 10); // change chat char limit from 64 to 128
			patchBytesM((BYTE*)0x0075CCF7, (BYTE*)"\x6A\x82", 2);  // change chat print limit from 64 to 130 (extra 2 characters to account for formatted input with - and newline)
			patchBytesM((BYTE*)0x0075CDEA, (BYTE*)"\x68\xFF\x92\x20\x02", 5); // new chat read address for entered message
			Logger::TypedLog(CHN_DEBUG, "Enabling better chat...\n");
		}
	}
}