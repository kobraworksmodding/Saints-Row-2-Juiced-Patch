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
#include <fstream>
#include <sstream>
#include "../loose files.h"
#include "../Game/Game.h"
#include <ankerl/unordered_dense.h>
#include "../Hooker.h"
#include "../DLCSupport.h"
import component;
typedef char(__cdecl* gr_rectT)(int x1, int y1, int w, int h, int* state);
gr_rectT gr_rect = (gr_rectT)0x00D0B980;

namespace Render2D
{
	float* currentAR = (float*)0x022FD8EC_g;
	const float widescreenvalue = 1.777777791f;
	bool BetterChatTest = 0;
#if !JLITE
	bool IVRadarScaling = false;
	int IVRadarScalingDyn;
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

	// 16:10 and other screens between 4:3 and 16:9, the HUD keeps its 16:9 shape and is centred vertically
	bool TallHUDFix = false;
	float TallHUDHeight = 720.f;
	float tall_safearea_y = 0.f;

	float get_vint_y_resolution() {
		if (TallHUDFix)
			return TallHUDHeight;
		return 720.f;
	}

	// where safe_frame goes, the 1280x720 frame centred in the vint screen
	float get_safe_frame_x() {
		return (get_vint_x_resolution() - 1280.f) / 2.f;
	}
	float get_safe_frame_y() {
		return (get_vint_y_resolution() - 720.f) / 2.f;
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
	bool& r_is_widescreen = *(bool*)0x025272DD;
	static bool hasChangedLobbyName = false;
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
				if (!hasChangedLobbyName) {
					char* playerName = (CHAR*)0x0212AB48;
					char* GameName = reinterpret_cast<char*>(0x0212AA08);

					snprintf(GameName, 256, "[%s]", playerName);
					DLC::SetDLCNameFlagOnline();
					hasChangedLobbyName = true;
				}
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

	// --- window mode, borderless or a normal window, can be switched while the game runs ---------

	static bool g_borderless = true;
	static bool g_window_was_borderless = false;
	static SafetyHookMid g_window_shown_hook{};
	static SafetyHookInline g_change_screen_hook{};

	constexpr DWORD kBorderlessStyle = WS_POPUP | WS_VISIBLE;
	constexpr DWORD kWindowedStyle = WS_OVERLAPPEDWINDOW | WS_VISIBLE; // caption, resizable, min/max

	static bool GameIsWindowed()
	{
		return *(BOOL*)0x022F6418_g != FALSE; // d3d_present_params.Windowed
	}

	// borderless covers the whole monitor (taskbar too), windowed gets a client area of w x h
	static void ApplyWindowMode(bool windowed, int w, int h, bool center)
	{
		HWND hwnd = *(HWND*)0x0252A410_g;
		if (!hwnd || !windowed)
			return; // exclusive fullscreen keeps the game's window as is

		MONITORINFO mi{};
		mi.cbSize = sizeof(mi);
		if (!GetMonitorInfoA(MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST), &mi))
			return;
		const LONG ex = GetWindowLongA(hwnd, GWL_EXSTYLE) &
			~(WS_EX_TOPMOST | WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE | WS_EX_DLGMODALFRAME | WS_EX_STATICEDGE);

		if (g_borderless) {
			const RECT& r = mi.rcMonitor;
			SetWindowLongA(hwnd, GWL_STYLE, kBorderlessStyle);
			SetWindowLongA(hwnd, GWL_EXSTYLE, ex);
			SetWindowPos(hwnd, HWND_NOTOPMOST, r.left, r.top, r.right - r.left, r.bottom - r.top,
				SWP_FRAMECHANGED | SWP_NOOWNERZORDER | SWP_SHOWWINDOW);
			g_window_was_borderless = true;
			return;
		}

		if (w <= 0 || h <= 0)
			return;
		RECT rc{ 0, 0, w, h };
		AdjustWindowRectEx(&rc, kWindowedStyle, FALSE, ex);
		const int ww = rc.right - rc.left;
		const int wh = rc.bottom - rc.top;
		const RECT& wa = mi.rcWork;
		int x = wa.left + ((wa.right - wa.left) - ww) / 2;
		int y = wa.top + ((wa.bottom - wa.top) - wh) / 2;
		RECT cur;
		if (!center && !g_window_was_borderless && GetWindowRect(hwnd, &cur)) {
			// resolution change, keep the window where the player put it
			x = cur.left;
			y = cur.top;
		}
		if (x < wa.left)
			x = wa.left;
		if (y < wa.top)
			y = wa.top; // keep the title bar reachable
		SetWindowLongA(hwnd, GWL_STYLE, kWindowedStyle);
		SetWindowLongA(hwnd, GWL_EXSTYLE, ex);
		SetWindowPos(hwnd, HWND_NOTOPMOST, x, y, ww, wh, SWP_FRAMECHANGED | SWP_NOOWNERZORDER | SWP_SHOWWINDOW);
		g_window_was_borderless = false;
	}

	// pc_change_d3d_screen, resizes the window with its own style on every resolution / mode change
	static char __cdecl ChangeScreenHook(int w, int h, char fullscreen, char vsync)
	{
		const char result = g_change_screen_hook.unsafe_ccall<char>(w, h, fullscreen, vsync);
		ApplyWindowMode(GameIsWindowed(), *(int*)0x022F63F8_g, *(int*)0x022F63FC_g, false);
		return result;
	}

	bool IsBorderless()
	{
		return g_borderless;
	}

	void SetBorderless(bool borderless)
	{
		if (borderless == g_borderless)
			return;
		g_borderless = borderless;
		GameConfig::SetValue("Graphics", "Borderless", borderless ? 1 : 0);
		ApplyWindowMode(GameIsWindowed(), *(int*)0x022F63F8_g, *(int*)0x022F63FC_g, true);
	}

	void SetupWindowMode()
	{
		g_borderless = GameConfig::GetValue("Graphics", "Borderless", 1, "Borderless windowed, this replaces Windowed mode in-game.") != 0;
		// style the window is created with, saves a flicker before the first ApplyWindowMode
		patchDWord((void*)(0x00BFA35A + 4), (g_borderless ? kBorderlessStyle : kWindowedStyle) & ~WS_VISIBLE);
		// after ShowWindow in the window setup, esi = launch settings (w, h, ..., +0x10 fullscreen)
		g_window_shown_hook = safetyhook::create_mid(0x00BFA49D_g, [](SafetyHookContext& ctx) {
			const uintptr_t settings = ctx.esi;
			const bool fullscreen = *(uint8_t*)(settings + 0x10) != 0;
			ApplyWindowMode(!fullscreen, *(int*)settings, *(int*)(settings + 4), true);
			});
		g_change_screen_hook = safetyhook::create_inline(0x00D1FF60_g, &ChangeScreenHook);
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
		snprintf(buffer, sizeof(buffer), lua_command,"safe_frame", cached_str,"anchor", get_safe_frame_x(), get_safe_frame_y());
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

			// vignettes cover the whole screen, not just the safe frame
			const float vignette_scale_x = get_vint_x_resolution() / 1280.f;
			const float vignette_scale_y = get_vint_y_resolution() / 720.f;
			//snprintf(buffer, sizeof(buffer), lua_command, "mayhem_grp", "hud", "scale", weirdscale, 1.f);
			//General::VintExecute(buffer);

			snprintf(buffer, sizeof(buffer), lua_command, "cash_txt", "hud", "scale",
				0.f, 0.f);

			snprintf(buffer, sizeof(buffer), lua_command, "multiplier_txt", "hud", "scale",
				0.f, 0.f);

			snprintf(buffer, sizeof(buffer), lua_command, "vignettes", "hud", "anchor", -get_safe_frame_x(), -get_safe_frame_y());
			General::VintExecute(buffer);

			snprintf(buffer, sizeof(buffer), lua_command, "vignettes", "hud", "scale", vignette_scale_x, vignette_scale_y);
			General::VintExecute(buffer);

		}
	}

bool UltrawideFix = false;
bool bSmartCutsceneBorder = true;
std::thread RefreshHUD_thread;
void RefreshHUD_loop() {
	Logger::TypedLog(CHN_DEBUG, "SR2Ultrawide Refreshing HUD {}\n",2);
	vint_create_process_hook.enable();
	std::this_thread::sleep_for(std::chrono::seconds(4));
	vint_create_process_hook.disable();
}
uintptr_t gr_rect_og;
int letterbox_height = *(int*)0xE98960_g;
static void draw_black_rect(int x, int y, int w, int h, int* state)
{
	if (w <= 0 || h <= 0)
		return;

	cdecl_call<void>(gr_rect_og, x, y, w, h, state);
}



#include <algorithm>
#include <cmath>

struct SmartCutsceneFrame
{
	int screenW;
	int screenH;

	int viewX;
	int viewY;
	int viewW;
	int viewH;

	int top;
	int bottom;
	int left;
	int right;

	float screenAspect;
	float cameraAspect;
};

static SmartCutsceneFrame GetSmartCutsceneFrame()
{
	int screenW = *(int*)0x022f63f8_g;
	int screenH = *(int*)0x022f63fc_g;

	// Your game is using 720p UI space.
	// If these addresses return real resolution, normalize it to 720p space.
	if (screenH != 720 && screenH > 0)
	{
		float scale = 720.0f / (float)screenH;
		screenW = (int)std::round((float)screenW * scale);
		screenH = 720;
	}

	if (screenW <= 0)
		screenW = 1280;

	if (screenH <= 0)
		screenH = 720;

	constexpr float targetAspect = 24.0f / 10.0f; // 2.40:1

	SmartCutsceneFrame out{};
	out.screenW = screenW;
	out.screenH = screenH;
	out.screenAspect = (float)screenW / (float)screenH;

	// Border frame calculation.
	// If screen is narrower than 24:10, use top/bottom bars.
	// If screen is wider than 24:10, use side bars.
	if (out.screenAspect < targetAspect)
	{
		out.viewW = screenW;
		out.viewH = (int)std::round((float)screenW / targetAspect);

		out.viewX = 0;
		out.viewY = (screenH - out.viewH) / 2;
	}
	else
	{
		out.viewH = screenH;
		out.viewW = (int)std::round((float)screenH * targetAspect);

		out.viewX = (screenW - out.viewW) / 2;
		out.viewY = 0;
	}

	out.top = out.viewY;
	out.bottom = screenH - (out.viewY + out.viewH);
	out.left = out.viewX;
	out.right = screenW - (out.viewX + out.viewW);

	if (out.top < 0) out.top = 0;
	if (out.bottom < 0) out.bottom = 0;
	if (out.left < 0) out.left = 0;
	if (out.right < 0) out.right = 0;

	// Important:
	// FOV is based on the camera area, not the black-bar frame.
	// 16:9 stays 16:9.
	// 21:9 uses 21:9.
	// 32:9 gets clamped to 24:10.
	out.cameraAspect = min(out.screenAspect, targetAspect);

	return out;
}

static void DrawBorderRect(int x, int y, int w, int h, int* state)
{
	if (w <= 0 || h <= 0)
		return;

	cdecl_call<void>(gr_rect_og, x, y, w, h, state);
}

void gr_rect_letterbox(int x1, int y1, int w, int h, int* state)
{
	if (TallHUDFix)
	{
		// 16:10, the game's top bar is in the 720 high frame, run it from the top of the screen
		const int extra = (int)std::ceil(get_safe_frame_y());
		DrawBorderRect(x1, 0, w, y1 + h + extra + 2, state);
		return;
	}
	if (*currentAR > widescreenvalue && bSmartCutsceneBorder && r_is_widescreen)
	{
		SmartCutsceneFrame f = GetSmartCutsceneFrame();

		constexpr int bleed = 2;

		// Top
		DrawBorderRect(
			0,
			0,
			f.screenW + bleed,
			f.top + bleed,
			state
		);

		// Bottom
		DrawBorderRect(
			0,
			f.screenH - f.bottom,
			f.screenW + bleed,
			f.bottom + bleed,
			state
		);

		// Left
		DrawBorderRect(
			0,
			0,
			f.left + bleed,
			f.screenH + bleed,
			state
		);

		// Right
		DrawBorderRect(
			f.screenW - f.right,
			0,
			f.right + bleed,
			f.screenH + bleed,
			state
		);

		return;
	}

	cdecl_call<void>(gr_rect_og, x1, y1, w, h, state);
}



void gr_rect_letterbox_below(int x1, int y1, int w, int h, int* state)
{
	if (TallHUDFix)
	{
		// bottom bar, moved into the centred frame and run to the bottom of the screen
		const int extra = (int)std::ceil(get_safe_frame_y());
		DrawBorderRect(x1, y1 + extra, w, h + extra + 2, state);
		return;
	}


	if (*currentAR > widescreenvalue && bSmartCutsceneBorder && r_is_widescreen)
	{
		return;
	}

	cdecl_call<void>(gr_rect_og, x1, y1, w, h, state);
}

char SR2Ultrawide_HUDScale() {
	Logger::TypedLog(CHN_DEBUG, "SR2Ultrawide Refreshing HUD {}\n", 1);
	float currentX = (float)(*(unsigned int*)0x022f63f8);
	float currentY = (float)(*(unsigned int*)0x022f63fc);
	char result;

	float aspectRatio = currentX / currentY;
	// Cutscene black bars
	//SafeWrite32((0x00755C49 + 1), 1280);
	Render3D::AspectRatioFix(true);
	SafeWrite32((0x75F1F6 + 4), processtextwidth(387));
	if (aspectRatio >= 1.77) {
		//SafeWrite32((0x00755C49 + 1), (uint32_t)(aspectRatio * 720));
		// Fix reflections being broken at ultrawide.
		*(float*)(0x0E86388) = aspectRatio;
#if !JLITE
	if (GameConfig::GetValue("Graphics", "IVRadarScaling", 0, "Makes the minimap scaling more in line with GTA IV (Clippy95)")) {
			IVRadarScaling = true;
			RadarScaling();
		}
	//RadarScaling();
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
	// 16:10 and friends: same scale on both axes so the HUD isn't stretched, safe_frame gets centred
	// vertically and cvint.dat T/B pushes elements to the top/bottom (like ultrawide does sideways)
	static const bool tall_hud_enabled = GameConfig::GetValue("Graphics", "FixTallHUD", 1,
		"Stops the HUD stretching on screens between 4:3 and 16:9 like 16:10, uses the T/B flags in cvint.dat (Clippy95)") != 0;
	const bool was_tall = TallHUDFix;
	if (tall_hud_enabled && aspectRatio > 1.59f && aspectRatio < 1.77f) {
		UltrawideFix = false;
		TallHUDFix = true;
		TallHUDHeight = 1280.f / aspectRatio;
		*(uint8_t*)0x0213c383 = 1; // is_screen_aspect_ratio_16_9
		*(uint8_t*)0x025272dd = 1; // r_is_widescreen
		*(float*)0x022fdcc0 = currentX / 1280.0f;
		*(float*)0x022fdcbc = currentX / 1280.0f;
		Logger::TypedLog(CHN_MOD, "SR2Ultrawide tall HUD, scale {:f} height {:f}\n", currentX / 1280.0f, TallHUDHeight);
		if (!was_tall) {
			RefreshHUD_thread = std::thread(RefreshHUD_loop);
			RefreshHUD_thread.detach();
		}
		return 1;
	}
	TallHUDFix = false;
	TallHUDHeight = 720.f;
	if (was_tall && aspectRatio < 1.77f) {
		// put safe_frame back where it was
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
	std::string displayText = SnapshotLooseLoadingText();
	// Own the snapshot through the native print call. Streaming can resize
	// or clear its buffer without invalidating the renderer's characters.
	if (!displayText.empty()) {
		displayText += "[JUICED] These are loose files loaded during THIS loading screen.";
		ChangeTextColor(238, 130, 238, 255);
		InGamePrintScale(6, displayText.c_str(), processtextwidth(0), 0, 0.7f);
	}
}
int* GR_FILTER = (int*)0xEC2740;

SAFETYHOOK_NOINLINE bool IsThisVintDoc(void* element,uint32_t crc)
{
	if (!element)
		return false;
	auto document = *(uintptr_t*)((uintptr_t)element + 0x30);
	if (!document)
		return false;
	return *(uint32_t*)(document + 0x48) == crc;
}

static uint32_t GetVintDocCRC(const void* element)
{
	if (!element)
		return 0;

	auto document = *(uintptr_t*)((uintptr_t)element + 0x30);
	if (!document)
		return 0;

	return *(uint32_t*)(document + 0x48);
}


// element's box in vint screen space (x0, y0, x1, y1), what "screen_nw"/"screen_se" read
static bool GetElementScreenCorners(void* element, float out[4])
{
	if (!element)
		return false;
	using get_corners_t = void(__thiscall*)(void*, float*, int);
	const get_corners_t get_corners = (*(get_corners_t**)element)[11]; // vtable + 0x2C
	get_corners(element, out, 1);
	return true;
}

void DrawUltraWideLeftRightBars(float alpha, void* element = nullptr)

	{
	if (*currentAR > widescreenvalue) {
		ChangeTextColor(0, 0, 0, (int)std::clamp((alpha * 255.f), 0.f, 255.f));
		float x1 = *currentAR * 720;
		auto bar_width = (float)((x1 - 1280) / 2);
		gr_rect(0, 0, bar_width, 720, GR_FILTER);
		gr_rect(x1 - bar_width, 0, bar_width, 720, GR_FILTER);
	}
	else if (TallHUDFix) {
		// 16:10, bars above and below the 720 high frame
		ChangeTextColor(0, 0, 0, (int)std::clamp((alpha * 255.f), 0.f, 255.f));
		float top_end = get_safe_frame_y();
		float bottom_start = TallHUDHeight - top_end;
		// start where the element ends, overlapping alpha shows as a dark line
		// (the sniper scope's bottom piece goes 2 units past the 720 frame)
		float c[4];
		if (GetElementScreenCorners(element, c)) {
			if (c[1] < top_end)
				top_end = c[1];
			if (c[3] > bottom_start)
				bottom_start = c[3];
		}
		const int height = (int)std::ceil(TallHUDHeight);
		const int top = (int)std::floor(top_end);
		const int bottom = (int)std::ceil(bottom_start);
		if (top > 0)
			gr_rect(0, 0, 1280, top, GR_FILTER);
		if (bottom < height)
			gr_rect(0, bottom, 1280, height - bottom + 1, GR_FILTER);
	}
	}
SafetyHookInline vint_element_base_renderD;

struct __declspec(align(8)) vint_render_params
{
	int unk;
	vector3 color;
	float alpha;
	bool mask;
	float rotation;
	char pad[0x50];
};

struct vint_object_base
{
	void* vft;
	char pad[0x24];
	uint32_t handle;
	uint32_t crc;
	void* document;
	char name[32];
};

struct vint_element_base : vint_object_base
{
	int unk1;
	bool mask;
	bool visible;
	vector2 v_anchor;
	vector2 v_offset;
	int enum1;
	vector2 v_unk;
	float unk3;
	vector3 color;
	float alpha;
	int color_render_type;
	vector2 scale;
};

struct Alignment {
	uint32_t h_left : 1;
	uint32_t h_right : 1;
	uint32_t h_center : 1;
	uint32_t v_top : 1;
	uint32_t v_bottom : 1;
	uint32_t v_center : 1;

	bool will_it_be_anchored()
	{
		return h_left == 1 || h_right == 1 || h_center == 1 || v_top == 1 || v_bottom == 1 || v_center == 1;
	}

	uint32_t black_bars : 1;
	uint32_t IVRadar : 1;
	// docs without a safe_frame (mission replay details...) are laid out for 1280x720 from the
	// top left, SF moves the element with safe_frame so it ends up centred like the rest
	uint32_t safe_frame : 1;
};

struct vint_cint_custom
{
	uint32_t document_crc = 0;
	bool any_document = false;
	Alignment align;
};

ankerl::unordered_dense::map<uint32_t, std::vector<vint_cint_custom>> g_custom_vint_cint_data;

static std::string TrimCopy(const std::string& value)
{
	const size_t start = value.find_first_not_of(" \t\r\n");
	if (start == std::string::npos)
		return {};

	const size_t end = value.find_last_not_of(" \t\r\n");
	return value.substr(start, end - start + 1);
}

static std::string ToLowerCopy(std::string value)
{
	std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
		return static_cast<char>(std::tolower(c));
		});
	return value;
}

static std::string ToUpperCopy(std::string value)
{
	std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
		return static_cast<char>(std::toupper(c));
		});
	return value;
}

static void ApplyVintCINTFlag(Alignment& align, const std::string& raw_flag)
{
	const std::string flag = ToUpperCopy(TrimCopy(raw_flag));
	if (flag.empty())
		return;

	if (flag == "L") {
		align.h_left = 1;
	}
	else if (flag == "R") {
		align.h_right = 1;
	}
	else if (flag == "CB") {
		align.h_center = 1;
		align.v_bottom = 1;
	}
	else if (flag == "CT") {
		align.h_center = 1;
		align.v_top = 1;
	}
	else if (flag == "LT") {
		align.h_left = 1;
		align.v_top = 1;
	}
	else if (flag == "RT") {
		align.h_right = 1;
		align.v_top = 1;
	}
	else if (flag == "LB") {
		align.h_left = 1;
		align.v_bottom = 1;
	}
	else if (flag == "RB") {
		align.h_right = 1;
		align.v_bottom = 1;
	}
	else if (flag == "T") {
		align.v_top = 1;
	}
	else if (flag == "B") {
		align.v_bottom = 1;
	}
	else if (flag == "C") {
		align.h_center = 1;
		align.v_center = 1;
	}
	else if (flag == "BLACKBARS") {
		align.black_bars = 1;
	}
	else if (flag == "IV") {
		align.IVRadar = 1;
	}
	else if (flag == "SF") {
		align.safe_frame = 1;
	}
}

static void MergeAlignment(Alignment& dst, const Alignment& src)
{
	dst.h_left |= src.h_left;
	dst.h_right |= src.h_right;
	dst.h_center |= src.h_center;
	dst.v_top |= src.v_top;
	dst.v_bottom |= src.v_bottom;
	dst.v_center |= src.v_center;
	dst.black_bars |= src.black_bars;
	dst.IVRadar |= src.IVRadar;
	dst.safe_frame |= src.safe_frame;
}

static const vint_cint_custom* FindVintCINT(const vint_element_base* element)
{
	if (!element)
		return nullptr;

	const auto it = g_custom_vint_cint_data.find(element->crc);
	if (it == g_custom_vint_cint_data.end())
		return nullptr;

	const uint32_t document_crc = GetVintDocCRC(element);
	const vint_cint_custom* any_document_rule = nullptr;
	for (const auto& rule : it->second)
	{
		if (rule.any_document)
		{
			if (!any_document_rule)
				any_document_rule = &rule;
			continue;
		}

		if (rule.document_crc == document_crc)
			return &rule;
	}

	return any_document_rule;
}

float ultrawide_get_x_difference()
{
	if (*currentAR > widescreenvalue)
	{
		return ((*currentAR * 720.f) - 1280) / 2;
	}
	return 0.f;
}

float ultrawide_safearea_x = 0.f;

SAFETYHOOK_NOINLINE bool modify_vint_anchor(const vint_cint_custom* cint, vint_element_base* element)
{
	auto align = cint->align;

	if (align.safe_frame)
	{
		element->v_anchor.x += get_safe_frame_x();
		element->v_anchor.y += get_safe_frame_y();
		return true;
	}

	if (align.will_it_be_anchored())
	{
		float x = ultrawide_get_x_difference();
		if (*currentAR > widescreenvalue)
		{
			x *= std::clamp(ultrawide_safearea_x,0.f,1.f);
			if (align.h_left)
				element->v_anchor.x -= x;
			else if (align.h_right)
				element->v_anchor.x += x;
			return true;
		}
		if (TallHUDFix)
		{
			// 16:10, T/B go to the top / bottom of the screen
			const float y = get_safe_frame_y() * std::clamp(tall_safearea_y, 0.f, 1.f);
			if (align.v_top)
				element->v_anchor.y -= y;
			else if (align.v_bottom)
				element->v_anchor.y += y;
			return true;
		}
	}
	return false;
}

// shift cvint.dat gives an element while it's drawn (modify_vint_anchor), the element itself
// doesn't move so things hit testing vint positions need it (MouseUI weapon wheel)
void GetCvintAnchorOffset(const char* document, const char* element, float& out_x, float& out_y)
{
	out_x = 0.f;
	out_y = 0.f;
	if (!r_is_widescreen || !document || !element)
		return;
	const bool wide = *currentAR > widescreenvalue;
	if (!wide && !TallHUDFix)
		return;
	const auto it = g_custom_vint_cint_data.find(Game::utils::str_to_hash(element));
	if (it == g_custom_vint_cint_data.end())
		return;

	// same pick as FindVintCINT, this document's rule over [ANY]
	const uint32_t document_crc = Game::utils::str_to_hash(document);
	const vint_cint_custom* rule = nullptr;
	for (const auto& r : it->second)
	{
		if (r.any_document)
		{
			if (!rule)
				rule = &r;
			continue;
		}
		if (r.document_crc == document_crc)
		{
			rule = &r;
			break;
		}
	}
	if (!rule)
		return;

	auto align = rule->align;
	if (align.safe_frame) {
		out_x = get_safe_frame_x();
		out_y = get_safe_frame_y();
		return;
	}
	if (!align.will_it_be_anchored())
		return;
	if (wide) {
		const float x = ultrawide_get_x_difference() * std::clamp(ultrawide_safearea_x, 0.f, 1.f);
		if (align.h_left)
			out_x = -x;
		else if (align.h_right)
			out_x = x;
	}
	else {
		const float y = get_safe_frame_y() * std::clamp(tall_safearea_y, 0.f, 1.f);
		if (align.v_top)
			out_y = -y;
		else if (align.v_bottom)
			out_y = y;
	}
}

bool ApplyIVRadarScaling(vint_element_base* element)
{
	return false;
	if (r_is_widescreen && IVRadarScalingDyn)
	{
		element->scale *= RadarScale;
		element->v_anchor.x *= -0.40650406504f;
		element->v_anchor.x /= -0.93802816901f;
		return true;
	}
	return false;
}

void __fastcall vint_element_base_render(
	vint_element_base* this_element,void* unused,
	vint_render_params* Cvint_render_params,
	uintptr_t Base,
	size_t a4)
{
	static bool parsed = false;
	if (!parsed)
	{
		ParseVintCINT();
		parsed = true;
	}
	bool visible = this_element->visible && Cvint_render_params->alpha > 0.00000011920929;
	bool modified_anchor = false;
	bool modified_scale = false;
	vector2 old_anchor;
	vector2 old_scale;
	if (r_is_widescreen && visible)
	{
		uint32_t crc = this_element->crc;
		float& alpha = this_element->alpha;
		bool drew_black_bars = false;
		//if (crc == bg_sniper_s_crc || crc == background_crc || (crc == rim_sw_crc && IsThisVintDoc(this_element, cte_sniper_rifle_crc)))
		//{
		//	DrawUltraWideLeftRightBars(alpha);
		//	drew_black_bars = true;
		//}
		
		
		if (auto* custom = FindVintCINT(this_element))
		{
			if (custom->align.black_bars && !drew_black_bars)
				DrawUltraWideLeftRightBars(alpha, this_element);
			old_anchor = this_element->v_anchor;
			modified_anchor = modify_vint_anchor(custom, this_element);
			if (custom->align.IVRadar) {
				modified_scale = ApplyIVRadarScaling(this_element);
				if (modified_scale) 
				{
					old_scale = this_element->scale;
					old_anchor = this_element->v_anchor;
					modified_anchor = true;
				}
			}
		}

	}
	vint_element_base_renderD.unsafe_thiscall<void>(this_element, Cvint_render_params, Base, a4);
	if (modified_anchor) {
		this_element->v_anchor = old_anchor;
	}
	if (modified_scale)
	{
		this_element->scale = old_scale;
	}
}
// load or reload, should empty g_custom_vint_cint_data
void ParseVintCINT()
{
	g_custom_vint_cint_data.clear();

	for (auto& entry : DirCache)
	{
		const std::string& filename = entry.first; // lowercase hopefully
		const std::string& filepath = entry.second.FilePath;
		if (filename != "cvint.dat")
			continue;

		std::ifstream file(filepath.c_str());
		if (!file.is_open())
		{
			Logger::TypedLog("VINT", "Failed to open {}\n", filepath);
			return;
		}

		std::string current_document_name;
		uint32_t current_document_crc = 0;
		bool current_document_any = false;
		size_t parsed_rules = 0;

		std::string line;
		int line_number = 0;
		while (std::getline(file, line))
		{
			++line_number;

			const size_t comment_pos = line.find("//");
			if (comment_pos != std::string::npos)
				line.erase(comment_pos);

			line = TrimCopy(line);
			if (line.empty())
				continue;

			if (line.front() == '[' && line.back() == ']')
			{
				current_document_name = TrimCopy(line.substr(1, line.size() - 2));
				current_document_any = (_stricmp(current_document_name.c_str(), "any") == 0);
				current_document_crc = current_document_any ? 0u : Game::utils::str_to_hash(current_document_name.c_str());
				continue;
			}

			if (current_document_name.empty() && !current_document_any)
			{
				Logger::TypedLog("VINT", "cvint.dat:{} missing [document] section before '{}'\n", line_number, line);
				continue;
			}

			std::stringstream stream(line);
			std::string token;
			std::vector<std::string> tokens;
			while (std::getline(stream, token, ','))
			{
				token = TrimCopy(token);
				if (!token.empty())
					tokens.push_back(token);
			}

			if (tokens.empty())
				continue;

			const std::string element_name = tokens.front();
			if (element_name.empty())
				continue;

			vint_cint_custom parsed_rule{};
			parsed_rule.document_crc = current_document_crc;
			parsed_rule.any_document = current_document_any;

			for (size_t i = 1; i < tokens.size(); ++i)
				ApplyVintCINTFlag(parsed_rule.align, tokens[i]);

			auto& rules = g_custom_vint_cint_data[Game::utils::str_to_hash(element_name.c_str())];
			auto existing = std::find_if(rules.begin(), rules.end(), [&](const vint_cint_custom& rule) {
				return rule.any_document == parsed_rule.any_document && rule.document_crc == parsed_rule.document_crc;
				});

			if (existing != rules.end())
			{
				MergeAlignment(existing->align, parsed_rule.align);
			}
			else
			{
				rules.push_back(parsed_rule);
			}

			++parsed_rules;
		}

		Logger::TypedLog("VINT", "Parsed {} cvint rules from {}\n", parsed_rules, filepath);
		return;
	}
}
uintptr_t diversion_image_sizeup_addr;
float saved_diversion_horz = 0.f;
float* diversion_horz = (float*)0xE8D668_g;
// the y of each diversion row, the game works them out once from 0xE8D66C on init
float* diversion_rows = (float*)0x1F8B0B0_g;
float saved_diversion_rows[6]{};
bool diversion_rows_saved = false;
void diversion_image_sizeup()
{
	cdecl_call<void>(diversion_image_sizeup_addr);
	if (r_is_widescreen)
	{
		saved_diversion_horz = *diversion_horz;
	}
	for (int i = 0; i < 6; i++)
		saved_diversion_rows[i] = diversion_rows[i];
	diversion_rows_saved = true;
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
	if (GameConfig::GetValue("Graphics", "X360GammaUI", 1, "Applies XBOX 360 gamma on HUD elements. (Clippy95)"))
			final_2d_render.enable();
	SetupWindowMode();
	Logger::TypedLog(CHN_DEBUG, "Window mode: {}\n", g_borderless ? "borderless" : "windowed");

	if (GameConfig::GetValue("Gameplay", "BetterChat", 1, "Increases character limit for Text Chat in Multiplayer (Creds to Tervel)")) // changes char limit from 64 to 128 and formats the input after the 64th character
		{
			BetterChatTest = 1;
			patchBytesM((BYTE*)0x0075C91E, (BYTE*)"\xC7\x05\x1C\x69\xF7\x01\x80\x00\00\x00", 10); // change chat char limit from 64 to 128
			patchBytesM((BYTE*)0x0075CCF7, (BYTE*)"\x6A\x82", 2);  // change chat print limit from 64 to 130 (extra 2 characters to account for formatted input with - and newline)
			patchBytesM((BYTE*)0x0075CDEA, (BYTE*)"\x68\xFF\x92\x20\x02", 5); // new chat read address for entered message
			Logger::TypedLog(CHN_DEBUG, "Enabling better chat...\n");
		}
	vint_element_base_renderD = safetyhook::create_inline(0xB95B10, vint_element_base_render);
	// tweens on safe_frame's anchor (the completion screen shake) use 1280x720 positions,
	// they'd pull safe_frame back to the left/top so add our offset to what they write
	static auto safe_frame_tween = safetyhook::create_mid(0x00B9F277_g, [](SafetyHookContext& ctx) {
		if (!UltrawideFix && !TallHUDFix)
			return;
		static const uint32_t safe_frame_hash = fastcall_call<uint32_t>(0x00BDC9B0_g, "safe_frame");
		static const uint32_t anchor_hash = fastcall_call<uint32_t>(0x00BDC9B0_g, "anchor");
		const uintptr_t tween = ctx.ebp;
		const uintptr_t target = *(uintptr_t*)(ctx.esp + 0x1C);
		uint32_t* value = (uint32_t*)(ctx.esp + 0x38); // vint_variant, 7 = vector2
		if (!target || *(uint32_t*)(tween + 0x90) != anchor_hash || *(uint32_t*)(target + 0x2C) != safe_frame_hash || value[0] != 7)
			return;
		((float*)value)[1] += get_safe_frame_x();
		((float*)value)[2] += get_safe_frame_y();
		});
	InterceptCall(0x79B064, diversion_image_sizeup_addr, diversion_image_sizeup);
	InterceptCall(0xB8A751, diversion_image_sizeup_addr, diversion_image_sizeup);

	ultrawide_safearea_x = std::clamp((float)GameConfig::GetDoubleValue("Graphics", "SafeArea_X_UW", 1.f, "Safearea in ultrawide for elements defined in cvint.dat (clippy95)"),0.f,1.f);
	tall_safearea_y = std::clamp((float)GameConfig::GetDoubleValue("Graphics", "SafeArea_Y_Tall", 0.f, "Safearea on 16:10 like screens for elements with T/B in cvint.dat, 0 = keep the HUD in the 16:9 area, 1 = push it to the screen edges (clippy95)"), 0.f, 1.f);
	InterceptCall(0x755C56, gr_rect_og,gr_rect_letterbox);
	InterceptCall(0x755C81, gr_rect_og, gr_rect_letterbox_below);

	bSmartCutsceneBorder = GameConfig::GetValue("Graphics", "SmartCutsceneBorders", 1,"Proper letterboxing for different aspect ratios above widescreen while in cutscenes (clippy95)");

	Juiced::onInputPoll() += []() 
		{
			
			if (r_is_widescreen && saved_diversion_horz != 0.f)
			{
				*diversion_horz = saved_diversion_horz + (ultrawide_get_x_difference() * ultrawide_safearea_x);
			}
			// 16:10, diversions sit in the middle of the screen so move them down with safe_frame
			if (diversion_rows_saved)
			{
				const float y = TallHUDFix ? get_safe_frame_y() : 0.f;
				for (int i = 0; i < 6; i++)
					diversion_rows[i] = saved_diversion_rows[i] + y;
			}
		};
	}
}
