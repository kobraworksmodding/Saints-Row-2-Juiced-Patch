#define NOMINMAX
#include "LuaHandler.h"
#include "FileLogger.h"
#include <tchar.h>
#include <algorithm>

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

				Logger::TypedLog(CHN_LUA, "%s", endres);

			}
			else
			{
				TCHAR* msgW = NULL;
				DWORD msgLen = MultiByteToWideChar(CP_ACP, 0, msg, -1, NULL, 0);
				msgW = (TCHAR*)malloc(msgLen * sizeof(TCHAR));
				MultiByteToWideChar(CP_ACP, 0, msg, -1, (LPWSTR)msgW, msgLen);

				std::string result = TCHARToString((TCHAR*)msgW);
				const char* endres = result.c_str();

				Logger::TypedLog(CHN_LUA, "%s", endres);

				free(msgW);
			}

		g_debugLines.push_back(_strdup(msg));
	}

	return 0;
}