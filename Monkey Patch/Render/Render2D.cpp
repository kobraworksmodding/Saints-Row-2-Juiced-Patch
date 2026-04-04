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
#include "../loose files.h"
namespace Render2D
{
	float* currentAR = (float*)0x022FD8EC;
	const float widescreenvalue = 1.777777791f;
	bool BetterChatTest = 0;
#if !JLITE
	bool IVRadarScaling = false;

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
	if (*currentAR >= 1.77777777778f) {
		int offset = (int)(*currentAR * 720);
		offset -= 1280;
		if (offset != 0) {
			width += offset / 2;
		}
	}
	return width;

}

	float get_vint_x_resolution() {
		if (*currentAR >= 1.77777777778f)
			return *currentAR * 720;
		// Hack to fix odd weird res widescreens like 1.6f during display changes 
		// between ultrawide and non-ultrawide, only sometimes so it's still super buggy and weird.
		else return 1280.f; 
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

	void __declspec(naked) InGamePrintASMSS(int a1, const char* a2, int a3, int a4, float a5) {
		__asm {
			push ebp
			mov ebp, esp
			sub esp, __LOCAL_SIZE

			push edi
			push esi
			push eax

			mov edi, a1
			mov esi, a2
			push a5
			push a4
			push a3

			mov eax, 0xD15D00
			call eax

			pop eax
			pop esi
			pop edi

			mov esp, ebp
			pop ebp
			ret
		}
	}

	void InGamePrintScale(int font, const char* a2, int a3, int a4, float a5) {
		if (font == 2 || font == 0) {
			font = *(int*)0x00E98A90;
		}
		else if (font == 6) {
			font = *(int*)0x00E98A24;
		}
		InGamePrintASMSS(font, a2, a3, a4, a5);
	}

	void InGamePrint(const char* Text, int x, int y, int font) {
		if (font == 2 || font == 0) {
			font = *(int*)0x00E98A90;
		}
		else if (font == 6) {
			font = *(int*)0x00E98A24;
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
		bool& r_is_widescreen = *(bool*)0x025272DD;
#if !RELOADED
		if (*(BYTE*)0x02527B75 == 1 && *(BYTE*)0xE8D56B == 1) {

			int x = 0;
			int y = 0;
			if (r_is_widescreen) {
				x = processtextwidth(1120);
				y = 680;
			}

			ChangeTextColor(160, 160, 160, 128);

#ifdef GITHUB_ACTIONS
			__asm pushad
			InGamePrint(("NIGHTLY " + UtilsGlobal::getShortCommitHash()).c_str(),35, 0, 6);
			__asm popad
#endif
			__asm pushad
			InGamePrint((JuicedText + std::string(UtilsGlobal::juicedversion)).c_str(), y, x, 2);
			__asm popad

			if (*(BYTE*)0x02529334 == 4) {
				ChangeTextColor(160, 160, 160, 128);
				__asm pushad
				InGamePrint(UtilsGlobal::mynat_type.c_str(), y + 15, Render2D::processtextwidth(10), 6);
				__asm popad
			}
		}
#else
		if (*(BYTE*)0xE8D56B == 1) {

			int x = 0;
			int y = 0;
			if (r_is_widescreen) {
				x = processtextwidth(1120);
				y = 640;
			}

			ChangeTextColor(160, 160, 160, 128);
			__asm pushad
			InGamePrint(("THAROW " + std::string(UtilsGlobal::thaRowmenuversion)).c_str(), y, x, 2);
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
	if (GameConfig::GetValue("Debug", "MenuVersionNumber", 1, "Shows Version Number on Main Menu and/or Pause Menu. (Tervel)\n1 = on\n0 = off"))
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
	int l_IsBorderless = GameConfig::GetValue("Graphics", "Borderless", 1, "Borderless windowed, this replaces Windowed mode in-game.");
		uint32_t window_style = l_IsBorderless ? (WS_POPUP) : (WS_CAPTION | WS_BORDER);
		patchDWord((void*)(0x00BFA35A + 4), window_style);
		patchBytesM((BYTE*)0x00BFA494, (BYTE*)"\x6A\x03", 2); //Maximise Borderless so it fits perfectly.
	}

	void SetupBetterWindowed()
	{
		uint32_t windowed_style = (WS_CAPTION | WS_OVERLAPPED | WS_SYSMENU | WS_DLGFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SIZEBOX);
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
		snprintf(buffer, sizeof(buffer), lua_command,"safe_frame", cached_str,"anchor", (get_vint_x_resolution() - 1280) / 2.f,0.f);
		//Logger::TypedLog(CHN_DEBUG, "%s \n", possible_str);
		General::VintExecute(buffer);

		if (strcmp(cached_str, "hud") == 0) {

			snprintf(buffer, sizeof(buffer), lua_command, "extra_homie", "hud", "anchor", (get_vint_x_resolution() - 1280) / 2.f, -500.f);
			General::VintExecute(buffer);

			snprintf(buffer, sizeof(buffer), lua_command, "mp_snatch_john", "hud", "anchor", (get_vint_x_resolution() - 1280) / 2.f, -500.f);
			General::VintExecute(buffer);

			snprintf(buffer, sizeof(buffer), lua_command, "health_mini_grp", "hud", "anchor", (get_vint_x_resolution() - 1280) / 2.f, -500.f);
			General::VintExecute(buffer);


			snprintf(buffer, sizeof(buffer), lua_command, "health_large_grp", "hud", "anchor", (get_vint_x_resolution() - 1280) / 2.f, -500.f);
			General::VintExecute(buffer);

			float weirdscale = 1.f / (widescreenvalue / *currentAR);
			//snprintf(buffer, sizeof(buffer), lua_command, "mayhem_grp", "hud", "scale", weirdscale, 1.f);
			//General::VintExecute(buffer);

			snprintf(buffer, sizeof(buffer), lua_command, "cash_txt", "hud", "scale",
				0.f, 0.f);

			snprintf(buffer, sizeof(buffer), lua_command, "multiplier_txt", "hud", "scale",
				0.f, 0.f);

			snprintf(buffer, sizeof(buffer), lua_command, "vignettes", "hud", "anchor", -((get_vint_x_resolution() - 1280) / 2.f), 0.f);
			General::VintExecute(buffer);

			snprintf(buffer, sizeof(buffer), lua_command, "vignettes", "hud", "scale", weirdscale, 1.f);
			General::VintExecute(buffer);

		}
	}

bool UltrawideFix = false;
// Clippy TODO, maybe handle 16:10?
std::thread RefreshHUD_thread;
void RefreshHUD_loop() {
	Logger::TypedLog(CHN_DEBUG, "SR2Ultrawide Refreshing HUD {}\n",2);
	vint_create_process_hook.enable();
	std::this_thread::sleep_for(std::chrono::seconds(4));
	vint_create_process_hook.disable();
}

char SR2Ultrawide_HUDScale() {
	Logger::TypedLog(CHN_DEBUG, "SR2Ultrawide Refreshing HUD {}\n", 1);
	float currentX = (float)(*(unsigned int*)0x022f63f8);
	float currentY = (float)(*(unsigned int*)0x022f63fc);
	char result;

	float aspectRatio = currentX / currentY;
	// Cutscene black bars
	SafeWrite32((0x00755C49 + 1), 1280);
	Render3D::AspectRatioFix(true);
	SafeWrite32((0x75F1F6 + 4), processtextwidth(387));
	if (aspectRatio >= 1.77) {
		SafeWrite32((0x00755C49 + 1), (uint32_t)(aspectRatio * 720));
		// Fix reflections being broken at ultrawide.
		*(float*)(0x0E86388) = aspectRatio;
#if !JLITE
	if (GameConfig::GetValue("Graphics", "IVRadarScaling", 0, "Makes the minimap scaling more in line with GTA IV (Clippy95)")) {
			IVRadarScaling = true;
			RadarScaling();
		}
#endif

		// Fucking tagging system cause yeah lets hard code the anchor for it?
	int var = (int)(aspectRatio * 720.f);
	static int var2;
	 var2 = (int)(aspectRatio * 360.f);
	SafeWrite32(0x00622571 + 1, var);
	SafeWrite32(0x00625A2B + 2, var);
	//SafeWrite32(0x00625F70 + 1, var);
	//SafeWrite32(0x00755A21 + 1, var);
	//SafeWrite32(0x00755C49 + 1, var);
	//SafeWrite32(0x00B87313 + 1, var2);
	//SafeWrite32(0x00B87313 + 1, var2);
	SafeWrite32(0x00625D09 + 2, (UInt32)&var2);
	SafeWrite32(0x0062597F + 2, (UInt32)&var2);
	
		Logger::TypedLog(CHN_DEBUG, "SR2Ultrawide Refreshing HUD {}\n", 3);
		RefreshHUD_thread = std::thread(RefreshHUD_loop);
		RefreshHUD_thread.detach();
	}
	if ((GameConfig::GetValue("Graphics", "FixUltrawideHUD", 1) == 1)) {
		if (aspectRatio <= 1.79777777778f && aspectRatio != 1.5f) {

			UltrawideFix = false;
			General::CleanupModifiedScript();
			return ((char(*)())0xD1C910)(); // Original HUD scale function.
			
		}
		else {

			Logger::TypedLog(CHN_DEBUG, "SR2Ultrawide Refreshing HUD {}\n", 4);
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
	Logger::TypedLog(CHN_MOD, "SR2Ultrawide patched HUD scale X: {:f} Y: {:f} bool: {} \n", adjustedX, currentY / 720.0f, UltrawideFix);
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

typedef int(__cdecl* bink_renderT)(float l, float r,float w,float h);
bink_renderT bink_render = (bink_renderT)0x4923F0;


// A fix for bink videos being vert- (zoomed in) on aspect ratios that aren't matched to the .bik videos.
int __cdecl bink_render_hook(float l, float r, float w, float h) {
	BINK* bink_handle = *(BINK**)(0x140E670);
	int display_w = *(int*)0x22FDC1C;
	int display_h = *(int*)0x22FDC20;

	if (!bink_handle || display_w < 1 || display_h < 1) {
		return bink_render(l, r, w, h);
	}

	float display_aspect = (float)display_w / (float)display_h;
	float movie_aspect = (float)bink_handle->Width / (float)bink_handle->Height;

	if (display_aspect > movie_aspect) {
		float corrected_w = (float)display_h * movie_aspect;
		float corrected_l = ((float)display_w - corrected_w) * 0.5f;

		return bink_render(corrected_l, 0.0f, corrected_w, (float)display_h);
	}

	return bink_render(l, r, w, h);
}



char __cdecl bitmap_minimap_render_player(
	uint32_t id,
	float x,
	float y,
	float angle,
	float scale,
	DWORD unk) {

	if (*(bool*)0x2528615)
		return ((char(__cdecl*)(uint32_t, float, float, float, float, DWORD))0xB87C10)(id, x, y, angle, scale, unk);

	if (UtilsGlobal::getplayer()) {
		float* PlayerSin = (float*)(UtilsGlobal::getplayer() + 0x38);
		float* PlayerCos = (float*)(UtilsGlobal::getplayer() + 0x40);

		angle = -angle;
		float playerAngle = -atan2f(*PlayerSin, *PlayerCos) + M_PI;
		angle = angle - playerAngle;

		if (isfinite(angle)) {
			const float twoPi = 2.0f * static_cast<float>(M_PI);
			angle = fmodf(angle, twoPi);
			if (angle < 0) angle += twoPi;
		}
	}

	return ((char(__cdecl*)(uint32_t, float, float, float, float, DWORD))0xB87C10)(id, x, y, angle, scale, unk);
}

char __cdecl bitmap_pause_map_render_player(
	uint32_t id,
	float x,
	float y,
	float angle,
	float scale,
	DWORD unk) {


	if (UtilsGlobal::getplayer()) {
		float* PlayerSin = (float*)(UtilsGlobal::getplayer() + 0x38);
		float* PlayerCos = (float*)(UtilsGlobal::getplayer() + 0x40);

		float playerAngle = -atan2f(*PlayerSin, *PlayerCos) + M_PI;
		angle = -playerAngle;

		if (isfinite(angle)) {
			const float twoPi = 2.0f * static_cast<float>(M_PI);
			angle = fmodf(angle, twoPi);
			if (angle < 0) angle += twoPi;
		}
	}
		return ((char(__cdecl*)(uint32_t, float, float, float, float, DWORD))0xB87C10)(id, x, y, angle, scale, unk);
	
}

CMultiPatch CMPatches_ProperPlayerCursor = {

	[](CMultiPatch& mp) {
		mp.AddWriteRelCall(0x7A3C8B,(uintptr_t)&bitmap_minimap_render_player);
		mp.AddWriteRelCall(0x7704CC, (uintptr_t)&bitmap_pause_map_render_player);
		mp.AddWriteRelJump(0x7A3C95, 0x7A3C11);
	},

};

void __fastcall vint_sr2_render(void* thisa) {
	((void(__thiscall*)(void*))0x7F33B0)(thisa);

	if (!loaded_files_to_render.empty()) {
		std::string display_text = loaded_files_to_render + "[JUICED] These are loose files loaded during THIS loading screen.";
		ChangeTextColor(238, 130, 238, 255);

		InGamePrintScale(6, display_text.c_str(), processtextwidth(0), 0, 0.7f);
	}
}

	void Init() {
	if(GameConfig::GetValue("Debug","DisplayLooseFilesLoading",1, "Renders the loose files that load during the initial loading screen (Clippy95)"))
		patchCall((void*)0x68C607, vint_sr2_render);
	if(GameConfig::GetValue("Graphics","mini_pause_map_PlayerRotation",1, "Ties player cursor in minimap and pause map to the actual player orientation rather than camera when using the \"Minimap view: Rotational\" setting (Clippy95)"))
		CMPatches_ProperPlayerCursor.Apply();
		patchCall((void*)0x688C7A, bink_render_hook);
		// Fix vint UI speeding up at 1000?+ FPS
		fix_screen_fade_notint();
		patchNop((void*)0x00B8BC6B, 6);
		final_2d_render = safetyhook::create_mid(0xD1DFAA, [](SafetyHookContext& ctx) {
			texture_2d* pass = (texture_2d*)ctx.eax;
			ApplyX360Gamma(pass->color_info);
			},safetyhook::MidHook::StartDisabled);
	if (GameConfig::GetValue("Graphics", "X360GammaUI", 0, "Applies XBOX 360 gamma on HUD elements. (Clippy95)"))
			final_2d_render.enable();
	if (GameConfig::GetValue("Graphics", "Borderless", 1, "Borderless windowed, this replaces Windowed mode in-game."))
		{
			SetupBorderless();
			Logger::TypedLog(CHN_DEBUG, "Enabling Borderless Windowed.\n");
		}
		else
		{
			SetupBetterWindowed();
			Logger::TypedLog(CHN_DEBUG, "Fixing Windowed Mode.\n");
		}

	if (GameConfig::GetValue("Gameplay", "BetterChat", 1, "Increases character limit for Text Chat in Multiplayer (Creds to Tervel)")) // changes char limit from 64 to 128 and formats the input after the 64th character
		{
			BetterChatTest = 1;
			patchBytesM((BYTE*)0x0075C91E, (BYTE*)"\xC7\x05\x1C\x69\xF7\x01\x80\x00\00\x00", 10); // change chat char limit from 64 to 128
			patchBytesM((BYTE*)0x0075CCF7, (BYTE*)"\x6A\x82", 2);  // change chat print limit from 64 to 130 (extra 2 characters to account for formatted input with - and newline)
			patchBytesM((BYTE*)0x0075CDEA, (BYTE*)"\x68\xFF\x92\x20\x02", 5); // new chat read address for entered message
			Logger::TypedLog(CHN_DEBUG, "Enabling better chat...\n");
		}
	}
}
