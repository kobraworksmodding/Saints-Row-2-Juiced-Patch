#pragma once
#include <string>
#include <vector>
#include <algorithm>
#ifdef __cplusplus


extern "C" {
#endif

	typedef bool (*VintLuaHookString)(const char* filename, std::string& content, size_t size);

	// Returns: true if successfully registered, false otherwise
	__declspec(dllexport) bool RegisterVintLuaHookString(VintLuaHookString callback);

	// Unregister a callback function
	__declspec(dllexport) bool UnregisterVintLuaHookString(VintLuaHookString callback);

#ifdef __cplusplus
}
extern std::vector<VintLuaHookString> g_VintluaHooksAPI;
#endif

