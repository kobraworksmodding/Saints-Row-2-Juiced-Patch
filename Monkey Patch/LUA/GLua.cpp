#include "../FileLogger.h"
#include "../Patcher/patch.h"
#include "../SafeWrite.h"
#include "../GameConfig.h"
#include "GLua.h"
#include "..\LuaHandler.h"

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
    void Init() {
#if !RELOADED
        SafeWrite32(0x00A4EC84 + 4, (UInt32)&lua_func_never_die);
#endif
    }
}