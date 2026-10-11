// MouseUI.cpp (Clippy95)
// --------------------
// Created: 11/10/2026
//
// Mouse support for vint menus. The screen logic is in mouse_ui.lua (JUICED - Lua Libraries),
// this side handles the cursor, mouse buttons, the Lua natives and injecting vint input events.

#include <windows.h>
#include <cstdint>
#include <cstring>
#include <deque>
#include <string>
#include <unordered_map>

#include "MouseUI.h"
#include "Input.h"
#include "../FileLogger.h"
#include "../GameConfig.h"
#include "../SafeWrite.h"
#include "../LuaHandler.h"
#include "../Hooker.h"
#include <safetyhook.hpp>
#include <usercaller.hpp>

namespace MouseUI
{
	// same order as Input_event_names (0x00E8C550)
	static const char* const kEventNames[] = {
		"select", "alt_select", "exit", "back", "nav_up", "nav_down", "nav_left", "nav_right",
		"joy_up", "joy_down", "joy_left", "joy_right", "key_up", "key_down", "key_left", "key_right",
		"white", "black", "scroll_up", "scroll_down", "scroll_left", "scroll_right",
		"dpad_up", "dpad_down", "dpad_left", "dpad_right", "all_unassigned", "map", "pause", "pad_back",
		"playlist_up", "playlist_down", "chat_all", "chat_team",
	};
	constexpr int kNumEvents = (int)(sizeof(kEventNames) / sizeof(kEventNames[0]));
	constexpr int kAllUnassigned = 26;

	// scripts that get mouse_ui_attach() appended
	static const char* const kAttachScripts[] = {
		"menu_base", "main_menu", "dialog_box", "gmb_blackjack", "gmb_poker", "cellphone",
	};

	constexpr DWORD kRequestTimeoutMs = 120;  // cursor stays this long after the last mouse_ui_request()
	constexpr DWORD kRestoreCursorMs  = 2000; // reopening a menu within this puts the cursor back where it was
	constexpr DWORD kInjectExpireMs   = 750;
	constexpr size_t kMaxQueued       = 16;

	static bool g_enabled   = false;
	static bool g_luaLoaded = false;

	static DWORD g_lastRequestTick = 0;
	static bool  g_requestedOnce   = false;
	static bool  g_hoverAccum      = false;
	static bool  g_hoverPrev       = false;
	static bool  g_uiActive        = false;
	static bool  g_padHidden       = false;
	static bool  g_cursorShown     = false;
	static bool  g_dragPassthrough = false;

	static POINT g_prevScreen{};
	static bool  g_prevValid        = false;
	static POINT g_lastUiScreen{};
	static bool  g_haveLastUiScreen = false;
	static DWORD g_leaveTick        = 0;
	static int   g_wheelAccum       = 0;

	// counted in the window message callback
	static LONG g_lPressCount = 0, g_lReleaseCount = 0, g_rPressCount = 0, g_mPressCount = 0;
	static bool g_lDownMsg = false;
	static LONG g_lPressSeen = 0, g_lReleaseSeen = 0, g_rPressSeen = 0, g_mPressSeen = 0;

	struct State
	{
		float x = -10000.f;
		float y = -10000.f;
		bool moved = false;
		bool lPressed = false;
		bool lReleased = false;
		bool lDown = false;
		bool rPressed = false;
		bool mPressed = false;
		int wheel = 0;
		double frame = 0.0;
	};
	static State g_state;

	struct Injection
	{
		int event;
		int delay; // vint input passes to wait
		DWORD tick;
	};
	static std::deque<Injection> g_queue;
	static int  g_armedEvent  = -1;
	static int  g_armedPasses = 0;
	static bool g_idlePass    = false;

	static SafetyHookInline g_pollHook{};
	static SafetyHookInline g_captureHook{};
	static SafetyHookInline g_msgHook{};
	static SafetyHookMid g_passHook{};
	static SafetyHookMid g_eventHook{};

	bool IsActive()
	{
		return g_uiActive;
	}

	static int EventIndex(const char* name)
	{
		if (!name)
			return -1;
		for (int i = 0; i < kNumEvents; ++i) {
			if (_stricmp(name, kEventNames[i]) == 0)
				return i;
		}
		return -1;
	}

	static void ClearInjections()
	{
		g_queue.clear();
		g_armedEvent = -1;
		g_armedPasses = 0;
		g_idlePass = false;
	}

	// --- vint input event injection -------------------------------------------------------------

	// start of vint_sr2_process_input, events_triggered still has last pass in it
	static void VintInputPassBegin(SafetyHookContext& ctx)
	{
		(void)ctx;
		const uint8_t* triggered = (const uint8_t*)0x022EF550_g;

		if (g_armedEvent >= 0) {
			const bool fired = triggered[g_armedEvent] == 1;
			// new subscriptions only become ready after one pass, give it a few tries
			if (!fired && ++g_armedPasses < 4)
				return;
			g_armedEvent = -1;
			g_idlePass = true; // skip a pass so the game resets the repeat delay
			if (!g_queue.empty())
				g_queue.pop_front();
		}

		if (g_idlePass) {
			g_idlePass = false;
			return;
		}

		const DWORD now = GetTickCount();
		while (!g_queue.empty() && now - g_queue.front().tick > kInjectExpireMs)
			g_queue.pop_front();

		if (g_queue.empty())
			return;

		Injection& next = g_queue.front();
		if (next.delay > 0) {
			--next.delay;
			return;
		}
		g_armedEvent = next.event;
		g_armedPasses = 0;
	}

	// process_event "nothing pressed" path, subscription is arg_0 at esp + 0x30.
	// Jumping to the "pressed" path makes the game dispatch the event like a key press.
	static void ProcessEventNotHit(SafetyHookContext& ctx)
	{
		if (g_armedEvent < 0)
			return;
		const uintptr_t sub = *(uintptr_t*)(ctx.esp + 0x30);
		if (!sub)
			return;
		if (*(int*)(sub + 0x2C) != g_armedEvent)
			return;
		if (*(uint8_t*)(sub + 0x30) != 1) // not ready yet
			return;
		ctx.eip = 0x00B99C02_g; // process_event "pressed" path
	}

	// --- Lua natives (only stack pushes, nothing that allocates in the game's lua heap) ---------

	// x, y, moved, l_pressed, l_released, l_down, r_pressed, wheel, frame, active, m_pressed
	static int __cdecl lua_mouse_ui_poll(lua_State* L)
	{
		const State& s = g_state;
		lua_pushnumber(L, s.x);
		lua_pushnumber(L, s.y);
		lua_pushboolean(L, s.moved);
		lua_pushboolean(L, s.lPressed);
		lua_pushboolean(L, s.lReleased);
		lua_pushboolean(L, s.lDown);
		lua_pushboolean(L, s.rPressed);
		lua_pushnumber(L, s.wheel);
		lua_pushnumber(L, s.frame);
		lua_pushboolean(L, g_uiActive);
		lua_pushboolean(L, s.mPressed);
		return 11;
	}

	// mouse_ui_request(hovering), called every frame while a screen wants the cursor
	static int __cdecl lua_mouse_ui_request(lua_State* L)
	{
		g_lastRequestTick = GetTickCount();
		g_requestedOnce = true;
		if (lua_gettop(L) >= 1 && lua_toboolean(L, 1))
			g_hoverAccum = true;
		return 0;
	}

	// mouse_ui_inject(event [, delay]) -> bool
	static int __cdecl lua_mouse_ui_inject(lua_State* L)
	{
		bool ok = false;
		if (lua_gettop(L) >= 1 && lua_type(L, 1) == LUA_TSTRING) {
			const int ev = EventIndex(lua_tostring(L, 1));
			int delay = 0;
			if (lua_gettop(L) >= 2 && lua_type(L, 2) == LUA_TNUMBER)
				delay = (int)lua_tonumber(L, 2);
			if (delay < 0)
				delay = 0;
			if (delay > 30)
				delay = 30;
			if (ev >= 0 && ev != kAllUnassigned && g_queue.size() < kMaxQueued) {
				g_queue.push_back({ ev, delay, GetTickCount() });
				ok = true;
			}
		}
		lua_pushboolean(L, ok);
		return 1;
	}

	static int __cdecl lua_mouse_ui_clear(lua_State* L)
	{
		(void)L;
		ClearInjections();
		return 0;
	}

	// mouse_ui_end(), last screen closed, give the mouse back now instead of waiting for the timeout
	static int __cdecl lua_mouse_ui_end(lua_State* L)
	{
		(void)L;
		g_requestedOnce = false;
		g_hoverAccum = false;
		return 0;
	}

	static int __cdecl lua_mouse_ui_enabled(lua_State* L)
	{
		lua_pushboolean(L, g_enabled);
		return 1;
	}

	// --- game lua functions -----------------------------------------------------------------------

	using pushstring_t = uc::function<uc::cleanup::caller, uc::void_ret<void>,
		uc::esi_arg<lua_State*>, uc::stack_arg<const char*>>;
	using pushcclosure_t = uc::function<uc::cleanup::caller, uc::void_ret<void>,
		uc::esi_arg<lua_State*>, uc::stack_arg<lua_CFunction>, uc::stack_arg<int>>;
	using load_script_t = uc::function<uc::cleanup::caller, uc::eax_ret<uint8_t>,
		uc::ecx_arg<lua_State*>, uc::esi_arg<lua_State*>, uc::edi_arg<uintptr_t>, uc::stack_arg<const char*>>;
	using luaV_settable_t = void(__cdecl*)(lua_State* L, void* t, void* key, void* val);

	// Same as register_lua_vint_functions does it, so the name and closure live in the game's lua heap
	// (lua50.lib's pushstring would allocate with our CRT).
	static void RegisterGameLuaFunction(lua_State* L, const char* name, lua_CFunction fn)
	{
		static const pushstring_t pushstring = uc::make<uc::void_ret<void>,
			uc::esi_arg<lua_State*>, uc::stack_arg<const char*>>(0x00CDCAF0_g);
		static const pushcclosure_t pushcclosure = uc::make<uc::void_ret<void>,
			uc::esi_arg<lua_State*>, uc::stack_arg<lua_CFunction>, uc::stack_arg<int>>(0x00CDCD30_g);
		static const auto settable = (luaV_settable_t)0x00CD68D0_g;

		pushstring(L, name);
		pushcclosure(L, fn, 0);

		uint8_t*& top = *(uint8_t**)((uintptr_t)L + 0x08);   // L->top, TObject is 16 bytes
		void* globals = (void*)((uintptr_t)L + 0x40);         // L->_gt
		settable(L, globals, top - 0x20, top - 0x10);
		top -= 0x20;
	}

	// loads a lua file through the game's file system, same as vint_lib.lua
	static bool LoadVintLuaLib(lua_State* L, const char* name)
	{
		static const load_script_t load_script = uc::make<uc::eax_ret<uint8_t>,
			uc::ecx_arg<lua_State*>, uc::esi_arg<lua_State*>, uc::edi_arg<uintptr_t>, uc::stack_arg<const char*>>(0x00CDDF30_g);

		const uintptr_t pool = *(uintptr_t*)0x0252A250_g; // temp_mempool
		if (!L || !pool)
			return false;
		return load_script(L, L, pool, name) != 0;
	}

	// replaces "call register_lua_funcs" in vint_lua_init
	static void __cdecl RegisterLuaFuncsHook(lua_State* L)
	{
		cdecl_call<void>(0x007F35A0_g, L); // original register_lua_funcs
		if (!L)
			return;

		RegisterGameLuaFunction(L, "mouse_ui_poll", &lua_mouse_ui_poll);
		RegisterGameLuaFunction(L, "mouse_ui_request", &lua_mouse_ui_request);
		RegisterGameLuaFunction(L, "mouse_ui_inject", &lua_mouse_ui_inject);
		RegisterGameLuaFunction(L, "mouse_ui_clear", &lua_mouse_ui_clear);
		RegisterGameLuaFunction(L, "mouse_ui_end", &lua_mouse_ui_end);
		RegisterGameLuaFunction(L, "mouse_ui_enabled", &lua_mouse_ui_enabled);

		g_luaLoaded = LoadVintLuaLib(L, "mouse_ui.lua");
		Logger::TypedLog(CHN_LUA, "MouseUI: mouse_ui.lua {}\n", g_luaLoaded ? "loaded" : "not found");
	}

	// --- cursor ---------------------------------------------------------------------------------

	static void ClipToClient(HWND hwnd)
	{
		RECT rc;
		if (!GetClientRect(hwnd, &rc))
			return;
		POINT tl{ rc.left, rc.top };
		POINT br{ rc.right, rc.bottom };
		ClientToScreen(hwnd, &tl);
		ClientToScreen(hwnd, &br);
		RECT clip{ tl.x, tl.y, br.x, br.y };
		RECT current;
		if (!GetClipCursor(&current) || memcmp(&current, &clip, sizeof(RECT)) != 0)
			ClipCursor(&clip);
	}

	static void SyncButtonCounters()
	{
		g_lPressSeen = g_lPressCount;
		g_lReleaseSeen = g_lReleaseCount;
		g_rPressSeen = g_rPressCount;
		g_mPressSeen = g_mPressCount;
	}

	static void EnterUI(DWORD now)
	{
		g_dragPassthrough = false;
		g_wheelAccum = 0;
		g_prevValid = false;
		SyncButtonCounters(); // don't replay clicks from before the menu opened
		if (g_haveLastUiScreen && now - g_leaveTick < kRestoreCursorMs)
			SetCursorPos(g_lastUiScreen.x, g_lastUiScreen.y);
	}

	static void LeaveUI(HWND hwnd, bool focused, DWORD now)
	{
		POINT p;
		if (GetCursorPos(&p)) {
			g_lastUiScreen = p;
			g_haveLastUiScreen = true;
		}
		g_leaveTick = now;
		g_dragPassthrough = false;
		g_prevValid = false;
		ClearInjections();
		g_hoverAccum = false;
		g_hoverPrev = false;

		// game measures deltas from the window centre, recentre so the camera doesn't jump
		if (focused && hwnd) {
			RECT r;
			if (GetWindowRect(hwnd, &r))
				SetCursorPos(r.left + (r.right - r.left) / 2, r.top + (r.bottom - r.top) / 2);
		}
	}

	static void UpdateState(HWND hwnd, bool ui)
	{
		State& s = g_state;
		s.frame += 1.0;

		const LONG lp = g_lPressCount, lr = g_lReleaseCount, rp = g_rPressCount, mp = g_mPressCount;
		s.lPressed = lp != g_lPressSeen;
		s.lReleased = lr != g_lReleaseSeen;
		s.rPressed = rp != g_rPressSeen;
		s.mPressed = mp != g_mPressSeen;
		SyncButtonCounters();
		s.lDown = g_lDownMsg;

		g_hoverPrev = g_hoverAccum;
		g_hoverAccum = false;

		if (!ui || !hwnd) {
			s.moved = false;
			s.wheel = 0;
			s.lPressed = s.lReleased = s.rPressed = s.mPressed = false;
			g_prevValid = false;
			g_dragPassthrough = false;
			return;
		}

		POINT sp;
		if (!GetCursorPos(&sp))
			return;
		POINT cp = sp;
		ScreenToClient(hwnd, &cp);

		// client -> render resolution -> vint space (same space as "screen_nw"/"screen_se")
		float px = (float)cp.x;
		float py = (float)cp.y;
		RECT rc;
		const int resW = *(int*)0x022F63F8_g;
		const int resH = *(int*)0x022F63FC_g;
		if (GetClientRect(hwnd, &rc)) {
			const int cw = rc.right - rc.left;
			const int ch = rc.bottom - rc.top;
			if (cw > 0 && ch > 0 && resW > 0 && resH > 0) {
				px = px * (float)resW / (float)cw;
				py = py * (float)resH / (float)ch;
			}
		}
		float sx = *(float*)0x022FDCC0_g; // vint scale
		float sy = *(float*)0x022FDCBC_g;
		if (!(sx > 0.0001f))
			sx = 1.f;
		if (!(sy > 0.0001f))
			sy = 1.f;
		s.x = px / sx;
		s.y = py / sy;

		s.moved = g_prevValid && (sp.x != g_prevScreen.x || sp.y != g_prevScreen.y);

		// no camera movement over the UI, LMB drag on empty space passes it through (store player rotation)
		if (s.lPressed)
			g_dragPassthrough = !g_hoverPrev;
		if (!s.lDown && !s.lPressed)
			g_dragPassthrough = false;
		int dx = 0, dy = 0;
		if (g_dragPassthrough && g_prevValid) {
			dx = sp.x - g_prevScreen.x;
			dy = sp.y - g_prevScreen.y;
		}
		*(int*)0x0234F45C_g = dx; // Mouse_dx
		*(int*)0x0234F458_g = dy; // Mouse_dy

		// wheel goes to the UI only
		int& dwheel = *(int*)0x0234EA34_g; // Mouse_dwheel
		g_wheelAccum += dwheel;
		dwheel = 0;
		s.wheel = g_wheelAccum / WHEEL_DELTA;
		g_wheelAccum -= s.wheel * WHEEL_DELTA;

		if (s.moved || s.lPressed || s.rPressed || s.mPressed || s.wheel != 0) {
			g_padHidden = false;
			Input::g_lastInput = Input::MOUSE;
		}
		else if (Input::g_lastInput == Input::CONTROLLER) {
			g_padHidden = true;
		}

		g_prevScreen = sp;
		g_prevValid = true;
	}

	static void __cdecl MousePollHook()
	{
		HWND hwnd = *(HWND*)0x0252A410_g;
		const bool focused = *(uint8_t*)0x0252A406_g != 0;
		const DWORD now = GetTickCount();
		const bool requested = g_requestedOnce && (now - g_lastRequestTick) < kRequestTimeoutMs;
		const bool ui = g_enabled && g_luaLoaded && hwnd && focused && requested;

		if (ui && !g_uiActive)
			EnterUI(now);
		else if (!ui && g_uiActive)
			LeaveUI(hwnd, focused, now);
		g_uiActive = ui;

		uint8_t& centering = *(uint8_t*)0x0252A5D5_g; // mouse_pc_centering
		const uint8_t savedCentering = centering;
		if (ui)
			centering = 0;
		g_pollHook.unsafe_ccall<void>();
		if (ui)
			centering = savedCentering;

		UpdateState(hwnd, ui);
	}

	static void __cdecl MouseCaptureUpdateHook()
	{
		HWND hwnd = *(HWND*)0x0252A410_g;
		const bool focused = *(uint8_t*)0x0252A406_g != 0;

		if (g_uiActive && focused && hwnd) {
			*(int*)0x0252A620_g = 0; // request_capture_change (1 = hide + clip, 2 = release)
			if (!g_padHidden) {
				if (!g_cursorShown) {
					while (ShowCursor(TRUE) < 0) {}
					g_cursorShown = true;
				}
				static HCURSOR arrow = LoadCursor(nullptr, IDC_ARROW);
				if (arrow && GetCursor() != arrow)
					SetCursor(arrow);
			}
			else if (g_cursorShown) {
				while (ShowCursor(FALSE) >= 0) {}
				g_cursorShown = false;
			}
			ClipToClient(hwnd);
			return;
		}

		if (g_cursorShown) {
			while (ShowCursor(FALSE) >= 0) {}
			g_cursorShown = false;
			if (focused)
				*(int*)0x0252A620_g = 1; // let the game hide + clip it again
		}
		g_captureHook.unsafe_ccall<void>();
	}

	static void __cdecl MouseMsgHook(int msg, int wparam)
	{
		bool swallow = false;
		switch (msg) {
		case WM_LBUTTONDOWN:
		case WM_LBUTTONDBLCLK:
			++g_lPressCount;
			g_lDownMsg = true;
			swallow = g_uiActive;
			break;
		case WM_LBUTTONUP:
			if (g_lDownMsg)
				++g_lReleaseCount;
			g_lDownMsg = false;
			break; // always pass releases so nothing gets stuck
		case WM_RBUTTONDOWN:
		case WM_RBUTTONDBLCLK:
			++g_rPressCount;
			swallow = g_uiActive;
			break;
		case WM_MBUTTONDOWN:
		case WM_MBUTTONDBLCLK:
			++g_mPressCount;
			swallow = g_uiActive;
			break;
		default:
			break;
		}
		if (swallow)
			return;
		g_msgHook.unsafe_ccall<void>(msg, wparam);
	}

	// --- script attach --------------------------------------------------------------------------

	static const char* MatchScript(const char* filename)
	{
		if (!filename)
			return nullptr;
		for (const char* script : kAttachScripts) {
			const size_t n = strlen(script);
			if (_strnicmp(filename, script, n) == 0 && _stricmp(filename + n, ".lua") == 0)
				return script;
		}
		return nullptr;
	}

	bool WantsScript(const char* filename)
	{
		return g_enabled && g_luaLoaded && MatchScript(filename) != nullptr;
	}

	bool PatchVintScript(const char* filename, std::string& content)
	{
		if (!g_enabled || !g_luaLoaded)
			return false;
		const char* script = MatchScript(filename);
		if (!script)
			return false;
		content += "\nif mouse_ui_attach ~= nil then mouse_ui_attach(\"";
		content += script;
		content += "\") end\n";
		return true;
	}

	void Init()
	{
		g_enabled = GameConfig::GetValue("Input", "MouseUI", 1,
			"Mouse support in menus, dialogs, gambling and the phone. (Clippy95)") != 0;
		if (!g_enabled)
			return;

		WriteRelCall(0x00B91539_g, (UInt32)&RegisterLuaFuncsHook); // vint lua init -> register_lua_funcs
		g_pollHook = safetyhook::create_inline(0x00C1F320_g, &MousePollHook); // mouse_pc_poll
		g_captureHook = safetyhook::create_inline(0x00C1F210_g, &MouseCaptureUpdateHook); // mouse capture update
		g_msgHook = safetyhook::create_inline(0x00C1EFF0_g, &MouseMsgHook); // mouse window message callback
		g_passHook = safetyhook::create_mid(0x00B99550_g, &VintInputPassBegin); // vint_sr2_process_input
		g_eventHook = safetyhook::create_mid(0x00B99D72_g, &ProcessEventNotHit); // process_event, nothing pressed

		Logger::TypedLog(CHN_MOD, "MouseUI: hooks installed.\n");
	}
}
