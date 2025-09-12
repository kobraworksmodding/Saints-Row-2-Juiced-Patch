#include "../FileLogger.h"
#include "../Patcher/patch.h"
#include "../SafeWrite.h"
#include "../GameConfig.h"
#include "GLua.h"
#include "..\LuaHandler.h"

#include "../Render/Render3D.h"

#include <safetyhook.hpp>
#include "..\General\General.h"
#include "InGameConfig.h"
#include "../Player/Input.h"

struct luaL_Reg
{
    const char* name;
    lua_CFunction lcfunc;
};

template<typename T>
struct lua_type_traits;

template<>
struct lua_type_traits<int> {
    static int get(lua_State* L, int index) {
        return (int)lua_tonumber(L, index);
    }
    static void push(lua_State* L, int value) {
        lua_pushnumber(L, (lua_Number)value);
    }
    static bool is_valid_type(lua_State* L, int index) {
        return lua_isnumber(L, index);
    }
};

template<>
struct lua_type_traits<double> {
    static double get(lua_State* L, int index) {
        return lua_tonumber(L, index);
    }
    static void push(lua_State* L, double value) {
        lua_pushnumber(L, value);
    }
    static bool is_valid_type(lua_State* L, int index) {
        return lua_isnumber(L, index);
    }
};

template<>
struct lua_type_traits<float> {
    static float get(lua_State* L, int index) {
        return (float)lua_tonumber(L, index);
    }
    static void push(lua_State* L, float value) {
        lua_pushnumber(L, (lua_Number)value);
    }
    static bool is_valid_type(lua_State* L, int index) {
        return lua_isnumber(L, index);
    }
};

template<>
struct lua_type_traits<bool> {
    static bool get(lua_State* L, int index) {
        return lua_toboolean(L, index);
    }
    static void push(lua_State* L, bool value) {
        lua_pushboolean(L, value);
    }
    static bool is_valid_type(lua_State* L, int index) {
        return lua_isboolean(L, index);
    }
};

template<>
struct lua_type_traits<const char*> {
    static const char* get(lua_State* L, int index) {
        return lua_tostring(L, index);
    }
    static void push(lua_State* L, const char* value) {
        lua_pushstring(L, value);
    }
    static bool is_valid_type(lua_State* L, int index) {
        return lua_isstring(L, index);
    }
};

class LuaArgs {
    lua_State* L;
    int current_index;
    int total_args;

public:
    LuaArgs(lua_State* L) : L(L), current_index(1), total_args(lua_gettop(L)) {}

    // Get required argument
    template<typename T>
    T get() {
        if (current_index > total_args) {
            // Handle error - not enough arguments
            return T{};
        }
        T result = lua_type_traits<T>::get(L, current_index);
        current_index++;
        return result;
    }

    // Get optional argument with default value
    template<typename T>
    T get_or(T default_value) {
        if (current_index > total_args) {
            return default_value;
        }

        if (!lua_type_traits<T>::is_valid_type(L, current_index)) {
            current_index++;
            return default_value;
        }

        T result = lua_type_traits<T>::get(L, current_index);
        current_index++;
        return result;
    }

    // so this basically returns the arg even if the type is incorrect, if for some reason needed
    template<typename T>
    T get_or_unsafe(T default_value) {
        if (current_index > total_args) {
            return default_value;
        }
        T result = lua_type_traits<T>::get(L, current_index);
        current_index++;
        return result;
    }

    // Get optional argument as std::optional
    template<typename T>
    std::optional<T> get_optional() {
        if (current_index > total_args) {
            return std::nullopt;
        }
        T result = lua_type_traits<T>::get(L, current_index);
        current_index++;
        return result;
    }

    // Check if more arguments are available
    bool has_more() const {
        return current_index <= total_args;
    }

    // Check if argument exists at current position + offset
    bool has_arg(int offset = 0) const {
        return (current_index + offset) <= total_args;
    }

    // Peek at argument without consuming it
    template<typename T>
    T peek(int offset = 0) {
        if ((current_index + offset) > total_args) {
            return T{};
        }
        return lua_type_traits<T>::get(L, current_index + offset);
    }

    // Get remaining argument count
    int remaining() const {
        return total_args - current_index + 1;
    }

    // Get total argument count
    int total() const {
        return total_args;
    }
};


class LuaReturns {
    lua_State* L;
    int return_count;

public:
    LuaReturns(lua_State* L) : L(L), return_count(0) {}

    // Push a return value
    template<typename T>
    LuaReturns& push(T value) {
        lua_type_traits<T>::push(L, value);
        return_count++;
        return *this;
    }

    // Push multiple values at once
    template<typename T, typename... Args>
    LuaReturns& push_all(T first, Args... rest) {
        push(first);
        if constexpr (sizeof...(rest) > 0) {
            push_all(rest...);
        }
        return *this;
    }

    // Get number of values that will be returned
    int count() const {
        return return_count;
    }
};

typedef void(__stdcall* NeverDieT)(int character,uint8_t status);
NeverDieT NeverDie = (NeverDieT)0x00966720;

namespace GLua
{
    int __cdecl lua_func_never_die(lua_State* L)
    {
        LuaArgs args(L);
        LuaReturns returns(L);

        int player = *(int*)0x21703D4;

        if (!player) {
            returns.push(false);
            return returns.count();
        }

       bool status = args.get_or(false);

       NeverDie(player, status ? 1 : 0);
       returns.push(true);
       return returns.count();
}
    void PatchSleepHack(int value) {
        if (value == 0) {
            if(Render3D::IsSleepHooked)
            Render3D::UnHookSleep();
            Render3D::CMPatches_PatchLowSleepHack.Restore();
            Render3D::CPatches_MediumSleepHack.Restore();
        }
        else if (value == 1) {
            if (Render3D::IsSleepHooked)
            Render3D::UnHookSleep();
            Render3D::CMPatches_PatchLowSleepHack.Apply();
            Render3D::CPatches_MediumSleepHack.Restore();
        }
        else if (value == 2) {
            if (Render3D::IsSleepHooked)
            Render3D::UnHookSleep();
            Render3D::CMPatches_PatchLowSleepHack.Apply();
            Render3D::CPatches_MediumSleepHack.Apply();
        }
        else if (value == 3) {
            //Render3D::HookSleep();
            Render3D::CMPatches_PatchLowSleepHack.Apply();
            Render3D::CPatches_MediumSleepHack.Apply();
        }

    }
    int __cdecl lua_func_vint_get_avg_processing_time(lua_State* L) {
        using namespace InGameConfig;
        if (L == NULL) {
            return 0;
        }
        const char* cmd = lua_tostring(L, 1);
        if (cmd == NULL) {
            lua_pushnil(L);
            return 0;
        }
        if (strcmp(cmd, "INPUT") == 0) {
            if (Input::LastInputUI() == Input::GAME_LAST_INPUT::CONTROLLER) {
                if(!Input::UsePS3Prompts())
                lua_pushstring(L, "XBOX360");
                else lua_pushstring(L, "PS3");
                return 1;
            }
                lua_pushstring(L, "PC");
            return 1;
        }
        if (strcmp(cmd, "JuicedCall") == 0) {
            int value = (int)lua_tonumber(L, 2);
            GLuaWrapper(cmd, &value, false);
            lua_pushboolean(L, 1);
            return 1;
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
            int value = 0;

            if (strcmp(varName, "DisableAimAssist") == 0) {
                value = Input::disable_aim_assist_noMatterInput;
                lua_pushnumber(L, value);
                return 1;
            }

            if (strcmp(varName, "SleepHack") == 0) {
                if (Render3D::IsSleepHooked)
                    value = 3;
                else if (Render3D::CPatches_MediumSleepHack.IsApplied() && Render3D::CMPatches_PatchLowSleepHack.IsApplied())
                    value = 2;
                else if (Render3D::CMPatches_PatchLowSleepHack.IsApplied() && !Render3D::CPatches_MediumSleepHack.IsApplied())
                    value = 1;
                lua_pushnumber(L, value);
                return 1;
            }

            PatchEntry* entry = FindPatchEntry(varName);
            if (entry) {
                if (entry->singlePatch)
                    value = entry->singlePatch->IsApplied();
                else if (entry->multiPatch)
                    value = entry->multiPatch->IsApplied();

                lua_pushnumber(L, value);
                return 1;
            }
            if(&value)
            InGameConfig::GLuaWrapper(varName, &value, false);

            lua_pushnumber(L, value);
            return 1;
        }
        else if (strcmp(cmd, "WriteJuiced") == 0) {
            const char* varName = lua_tostring(L, 2);
            int value = (int)lua_tonumber(L, 3);

            if (varName == NULL) {
                lua_pushboolean(L, 0); // Failure
                return 1;
            }

            if (strcmp(varName, "DisableAimAssist") == 0) {
                    Input::disable_aim_assist_noMatterInput = value;
                    GameConfig::SetValue("Gameplay", "DisableAimAssist", std::clamp(
                        (int)Input::disable_aim_assist_noMatterInput, 0, 2));
            }

            if (strcmp(varName, "SleepHack") == 0) {
                PatchSleepHack(value);
                GameConfig::SetValue("Debug", "SleepHack", value);
                lua_pushboolean(L, 1); // Success
                return 1;
            }
            PatchEntry* entry = FindPatchEntry(varName);
            if (entry) {
                if (entry->singlePatch)
                    value ? entry->singlePatch->Apply() : entry->singlePatch->Restore();
                else if (entry->multiPatch) {
                    value ? entry->multiPatch->Apply() : entry->multiPatch->Restore();
                    // Hard coded fix for X360 Gamma toggle, would implement a callback system into CMultiPatch but feels too much.
                    if (strcmp(entry->name, "VFXPlus") == 0)
                        Render3D::VFXBrightnesstoggle();
                }
                GameConfig::SetValue(entry->configApp, entry->configKey, value);
            lua_pushboolean(L, 1); // Success
            return 1;
            }
            if (&value)
                InGameConfig::GLuaWrapper(varName, &value, true);
            lua_pushboolean(L, 1);
            return 1;
        }

        lua_pushnil(L);
        return 1;
    }

    int __cdecl lua_testing(lua_State* L)
    {
        LuaArgs args(L);
        LuaReturns returns(L);

        double value = args.get<double>();
        auto name = args.get_or<const char*>("YOU GOT NONE");

        printf("value %f string %s \n", value, name);

        return returns.count();
    }

    void register_custom_vint_functions(lua_State* L) {
        luaL_Reg funcs_to_add[] = {
            {"VINT_NEVER_DIE", lua_func_never_die},
            {"CTEST", lua_testing}
        };

        for (const auto& func : funcs_to_add) {
            lua_pushstring(L, func.name);
            lua_pushcfunction(L, func.lcfunc);
            lua_settable(L, LUA_GLOBALSINDEX);
        }
    }
    lua_CFunction register_vint_lua_funcsT = (lua_CFunction)0x7F35A0;
    void __cdecl register_vint_lua_funcs(lua_State* L) {
        (void)register_vint_lua_funcsT(L);
        register_custom_vint_functions(L);
    }

    typedef char(__thiscall* cellphone_dial_numberT)(const char* number);
    cellphone_dial_numberT cellphone_dial_number = (cellphone_dial_numberT)0x788840;

    char __fastcall cellphone_dial_number_c_function(const char* number, void* arg) {
        const char* JuicedUnlockAllPhoneNumber1 = "#2008";
        const char* JuicedUnlockAllPhoneNumber2 = "588444222333";
        if (!strncmp(number, JuicedUnlockAllPhoneNumber1, sizeof(JuicedUnlockAllPhoneNumber1) - 1) || !strncmp(number, JuicedUnlockAllPhoneNumber2, sizeof(JuicedUnlockAllPhoneNumber2) - 1)) {
            ((void(*)())0x6849F0)();
            // -- Clippy, there is a proper way to have the game show up a proper cellphone message but for now we'll have to do AddMessage.
            General::AddMessage(L"Juiced", L"All cheats have been unlocked!");
        }
        return cellphone_dial_number(number);
    }
    void Init() {
//#if !RELOADED
        SafeWrite32(0x00A4EC84 + 4, (UInt32)&lua_func_never_die);
        Logger::TypedLog("CHN_DBG", "address of lua func 0x%X \n", &lua_func_vint_get_avg_processing_time);
        //static SafetyHookInline memoryutils = safetyhook::create_inline(0x00B907F0, &lua_func_vint_get_avg_processing_time);
        SafeWrite32(0x00B91212 + 7, (UInt32)&lua_func_vint_get_avg_processing_time);
        WriteRelCall(0x00789018, (uint32_t)&cellphone_dial_number_c_function);

        // THIS IS JUST FOR VINT HOOK 0xA4EE00 IF YOU WANT GAMEPLAY ones!
        //WriteRelCall(0xB91539, (uint32_t)&register_vint_lua_funcs);
//#endif
    }
}