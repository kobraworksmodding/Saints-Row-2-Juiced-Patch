// JuicedAPI.cpp (Clippy95)
// --------------------
// Created: 13/06/2025
// as of 13/06 this file is meant for other script/hooks mod to be able to hook into vint lua to modify it.
// DO NOT USE IN JUICED INTERNALLY!
#include "JuicedAPI.h"


std::vector<VintLuaHookString> g_VintluaHooksAPI;

extern "C" {
    __declspec(dllexport) bool RegisterVintLuaHookString(VintLuaHookString callback) {
        if (!callback) return false;

        // Check if already registered
        auto it = std::find(g_VintluaHooksAPI.begin(), g_VintluaHooksAPI.end(), callback);
        if (it != g_VintluaHooksAPI.end()) {
            return false;
        }

        g_VintluaHooksAPI.push_back(callback);
        return true;
    }

    __declspec(dllexport) bool UnregisterVintLuaHookString(VintLuaHookString callback) {
        return false;
    }
}