#define NOMINMAX
#include "LuaHandler.h"
#include "FileLogger.h"
#include <tchar.h>
#include <algorithm>
#include <cstdint>
#include "Hooker.h"
#include <safetyhook.hpp>
#include <usercaller.hpp>

using namespace std;

vector<char*> g_debugLines;
fnLuaFile LuaFile = (fnLuaFile)0x00D74B40;
fnLuaRegister LuaRegister = (fnLuaRegister)0x00CD68D0;
fnLuaPushCFunction LuaPushCFunction = (fnLuaPushCFunction)0x00CDCD30;
static lua_State* hkg_luaState = NULL;

std::string TCHARToString(const TCHAR* tcharStr) {
	if (!tcharStr) return std::string(); // Handle null input

	 // Define the maximum path length
	const int MAX_PATH_LENGTH = MAX_PATH;

	// Determine the size needed for the ANSI string
	int size_needed = WideCharToMultiByte(CP_ACP, 0, tcharStr, -1, NULL, 0, NULL, NULL);
	if (size_needed == 0) return std::string(); // Conversion failed

	// Clamp the size_needed to MAX_PATH_LENGTH + 1 (to account for null terminator)
	size_needed = std::min(size_needed, MAX_PATH_LENGTH + 1);

	// Allocate a buffer and perform the conversion
	std::string ansiStr(size_needed - 1, 0); // size_needed includes null terminator
	WideCharToMultiByte(CP_ACP, 0, tcharStr, -1, &ansiStr[0], size_needed, NULL, NULL);

	return ansiStr;
}

INT WINAPIV HookedDebugPrint(lua_State* lua)
{

	if (hkg_luaState == NULL)
	{
		hkg_luaState = lua;
	}
	const char* msg = NULL;

	if (lua_isstring(lua, 1) == TRUE)
	{ 
		// determine if we need to ignore the "vint" debug_print argument
		msg = lua_tostring(lua, 1);

		if (_strcmpi(msg, "vint") == 0)
		{
			if (lua_isstring(lua, 2) == TRUE)
				msg = lua_tostring(lua, 2);
		}

		if (g_debugLines.size() == 10)
		{
			free(g_debugLines.front());
			g_debugLines.erase(g_debugLines.begin());
		}

			if (sizeof(TCHAR) == sizeof(char))
			{
				std::string result = TCHARToString((TCHAR*)msg);
				const char* endres = result.c_str();
				std::string endlinecheck = "\n";

				if (result.find(endlinecheck) != std::string::npos) {
					Logger::TypedLog(CHN_LUA, "{}", endres);
				}
				else {
					Logger::TypedLog(CHN_LUA, "{}\n", endres);
				}

			}
			else
			{
				TCHAR* msgW = NULL;
				DWORD msgLen = MultiByteToWideChar(CP_ACP, 0, msg, -1, NULL, 0);
				msgW = (TCHAR*)malloc(msgLen * sizeof(TCHAR));
				MultiByteToWideChar(CP_ACP, 0, msg, -1, (LPWSTR)msgW, msgLen);

				std::string result = TCHARToString((TCHAR*)msgW);
				const char* endres = result.c_str();
				std::string endlinecheck = "\n";

				if (result.find(endlinecheck) != std::string::npos) {
					Logger::TypedLog(CHN_LUA, "{}", endres);
				}
				else {
					Logger::TypedLog(CHN_LUA, "{}\n", endres);
				}

				free(msgW);
			}

		g_debugLines.push_back(_strdup(msg));
	}

	return 0;
}
#if !RELOADED
bool GOTR() {
	return isGOTR;

}
#endif

namespace GameLua
{
	using pushstring_t = uc::function<uc::cleanup::caller, uc::void_ret<void>,
		uc::esi_arg<lua_State*>, uc::stack_arg<const char*>>;
	using pushcclosure_t = uc::function<uc::cleanup::caller, uc::void_ret<void>,
		uc::esi_arg<lua_State*>, uc::stack_arg<lua_CFunction>, uc::stack_arg<int>>;
	using load_script_t = uc::function<uc::cleanup::caller, uc::eax_ret<uint8_t>,
		uc::ecx_arg<lua_State*>, uc::esi_arg<lua_State*>, uc::edi_arg<uintptr_t>, uc::stack_arg<const char*>>;
	using luaV_settable_t = void(__cdecl*)(lua_State* L, void* t, void* key, void* val);

	// Same as register_lua_vint_functions does it, so the name and closure live in the game's lua heap
	// (lua50.lib's pushstring would allocate with our CRT).
	void RegisterFunction(lua_State* L, const char* name, lua_CFunction fn)
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

	bool LoadScript(lua_State* L, const char* name)
	{
		static const load_script_t load_script = uc::make<uc::eax_ret<uint8_t>,
			uc::ecx_arg<lua_State*>, uc::esi_arg<lua_State*>, uc::edi_arg<uintptr_t>, uc::stack_arg<const char*>>(0x00CDDF30_g);

		const uintptr_t pool = *(uintptr_t*)0x0252A250_g; // temp_mempool
		if (!L || !pool)
			return false;
		return load_script(L, L, pool, name) != 0;
	}
}
