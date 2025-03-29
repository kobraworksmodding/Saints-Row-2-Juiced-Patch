#include "../FileLogger.h"
#include "../Patcher/patch.h"
#include "../SafeWrite.h"
#include "../GameConfig.h"
#include "GLua.h"
#include "..\LuaHandler.h"

#include "../Render/Render3D.h"

#include <safetyhook.hpp>

typedef void(__stdcall* NeverDieT)(int character,uint8_t status);
NeverDieT NeverDie = (NeverDieT)0x00966720;

namespace GLua
{
    int __cdecl lua_func_never_die(lua_State* L)
    {
        if (lua_gettop(L) >= 1)
        {
            int player = *(int*)0x21703D4;
            if (!player) {
                lua_error(L);
                return 0;
            }
            bool status = lua_toboolean(L, 1);
            NeverDie(player, status ? 1 : 0);
            return 0;
        }
        lua_pushstring(L, "Error: Expected a boolean argument.");
        lua_error(L); 

        return 0; 
    }
    int __cdecl lua_func_vint_get_avg_processing_time(lua_State* L) {
        if (L == NULL) {
            return 0;
        }
        const char* cmd = lua_tostring(L, 1);
        if (cmd == NULL) {
            lua_pushnil(L);
            return 0;
        }
        if (strcmp(cmd, "ReadInt") == 0) {
            unsigned int address = (unsigned int)lua_tonumber(L, 2);
            int value = *(int*)address;
            lua_pushnumber(L, value);
            Logger::TypedLog(CHN_LUA, "game called %s it read 0x%X and got %d", "ReadInt", address, value);
            return 1;
        }
        else if (strcmp(cmd, "WriteInt") == 0) {
            unsigned int address = (unsigned int)lua_tonumber(L, 2);
            int value = (int)lua_tonumber(L, 3);
            *(int*)address = value;
            lua_pushboolean(L, 1); // Success
            return 1;
        }
        else if (strcmp(cmd, "ReadJuiced") == 0) {
            const char* varName = lua_tostring(L, 2);
            if (varName == NULL) {
                lua_pushnil(L);
                return 1;
            }  
            if (strcmp(varName, "VFXPlus") == 0) {
                int value = Render3D::CMPatches_VFXPlus.IsApplied();
                lua_pushnumber(L, value);
            }
            return 1;
        }
        else if (strcmp(cmd, "WriteJuiced") == 0) {
            const char* varName = lua_tostring(L, 2);
            int value = (int)lua_tonumber(L, 3);

            if (varName == NULL) {
                lua_pushboolean(L, 0); // Failure
                return 1;
            }

            if (strcmp(varName, "VFXPlus") == 0) {
                value ? Render3D::CMPatches_VFXPlus.Apply() : Render3D::CMPatches_VFXPlus.Restore();
            }

            lua_pushboolean(L, 1); // Success
            return 1;
        }

        lua_pushnil(L);
        return 1;
    }
    void Init() {
#if !RELOADED
        SafeWrite32(0x00A4EC84 + 4, (UInt32)&lua_func_never_die);
        Logger::TypedLog("CHN_DBG", "address of lua func 0x%X \n", &lua_func_vint_get_avg_processing_time);
        //static SafetyHookInline memoryutils = safetyhook::create_inline(0x00B907F0, &lua_func_vint_get_avg_processing_time);
        SafeWrite32(0x00B91212 + 7, (UInt32)&lua_func_vint_get_avg_processing_time);
#endif
    }
}