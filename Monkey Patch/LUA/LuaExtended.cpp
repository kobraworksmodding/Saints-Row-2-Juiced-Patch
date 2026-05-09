// LuaExtended.cpp (clippy95)
// --------------------
// Created: May 8th 2026
import component;
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
#include "../loose files.h"
#include "../Hooker.h"
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <filesystem>
union SemInfo
{
    long double r;
    void* ts;
};


struct Token
{
    int token;
    SemInfo seminfo;
};

struct Zio
{
    unsigned int n;
    const char* p;
    const char* (__cdecl* reader)(lua_State*, void*, unsigned int*);
    void* data;
    const char* name;
};

struct Mbuffer
{
    char* buffer;
    unsigned int buffsize;
};


struct LexState
{
    int current;
    int linenumber;
    int lastline;
    Token t;
    Token lookahead;
    void* fs;
    lua_State* L;
    Zio* z;
    Mbuffer* buff;
    void* source;
    int nestlevel;
};

struct luaL_Reg
{
    const char* name;
    lua_CFunction func;
};


namespace LuaExtended
{

    int __cdecl luaZ_fill(Zio* z)
    {
        unsigned int size;
        const char* buff;

        buff = z->reader(0, z->data, &size);
        if (!buff || !size)
            return -1;
        z->n = size - 1;
        z->p = buff;
        return *(unsigned __int8*)z->p++;
    }

    static int lua_next_char(LexState* LS)
    {
        unsigned int n = LS->z->n;
        LS->z->n = n - 1;

        int c;
        if (n)
            c = *(unsigned char*)LS->z->p++;
        else
            c = luaZ_fill(LS->z);

        LS->current = c;
        return c;
    }

    static bool is_hex_digit_lua(int c)
    {
        return (c >= '0' && c <= '9') ||
            (c >= 'a' && c <= 'f') ||
            (c >= 'A' && c <= 'F');
    }

    static int hex_value_lua(int c)
    {
        if (c >= '0' && c <= '9')
            return c - '0';

        if (c >= 'a' && c <= 'f')
            return 10 + (c - 'a');

        if (c >= 'A' && c <= 'F')
            return 10 + (c - 'A');

        return -1;
    }

    static bool is_ident_char(int c)
    {
        return std::isalnum((unsigned char)c) || c == '_';
    }

    static bool try_read_hex_numeral(LexState* LS, int comma, SemInfo* seminfo)
    {
        /*
            Lua calls read_numeral(..., comma = 1) for numbers like .123.
            We only want normal 0x123 syntax, not weird .0x123 syntax.
        */
        if (comma)
            return false;

        if (LS->current != '0')
            return false;

        /*
            Peek after the 0.

            Important:
            luaZ_fill can mutate ZIO internals, so this simple save/restore peek
            is safest when the next character is already buffered.
        */
        unsigned int saved_n = LS->z->n;
        const char* saved_p = LS->z->p;
        int saved_current = LS->current;

        int after_zero = lua_next_char(LS);

        LS->z->n = saved_n;
        LS->z->p = saved_p;
        LS->current = saved_current;

        if (after_zero != 'x' && after_zero != 'X')
            return false;

        /*
            Confirmed hex literal.
            Now actually consume:
                0
                x/X
                hex digits
        */

        // consume '0'
        lua_next_char(LS);

        // consume 'x' or 'X'
        lua_next_char(LS);

        lua_Number value = 0.0;
        int digits = 0;

        while (is_hex_digit_lua(LS->current))
        {
            int hv = hex_value_lua(LS->current);

            value = (value * 16.0) + hv;
            digits++;

            lua_next_char(LS);
        }

        if (digits == 0)
        {
            __debugbreak();
            return true;
        }

        /*
            Reject things like:
                0x123g
                0x123_abc

            Without this, Lua could parse it as:
                number 0x123
                name g
            which is ugly and misleading.
        */
        if (is_ident_char(LS->current))
        {
            //luaX_lexerror(LS, "malformed number", TK_NUMBER_LUA50);
            __debugbreak();
            return true;
        }

        seminfo->r = value;
        return true;
    }
    SafetyHookInline o_read_numeral;
    void __cdecl hk_read_numeral(LexState* LS, int comma, SemInfo* seminfo)
    {
        if (try_read_hex_numeral(LS, comma, seminfo))
            return;

        o_read_numeral.unsafe_ccall<void>(LS, comma, seminfo);
    }

    // like a _lib so globals do savey!!
    uintptr_t luaL_loadbuffer_gameaddr = 0xCD9FD0_g;
    int luaL_loadbuffer_game(void* buffer, size_t size_buffer, lua_State* ls, const char* name)
    {
        int result;
        __asm {
            pushad
            pushfd
            mov eax, buffer
            mov ecx, size_buffer
            mov esi, ls
            push name
            call luaL_loadbuffer_gameaddr
            mov result, eax
            add esp, 4
            popfd
            popad
        }
    }

    struct lua_thread
    {
        uint16_t handle;
        lua_State* ls;
        lua_State* rs;
        bool un1;
        char* function_name;
        uint32_t context_handle;
        uint32_t flags;
        int num_pushed_args;
    };

    uintptr_t lua_execute_thread_immediate_game = 0xCE0740_g;
    int lua_execute_thread_immediate(lua_thread** t)
    {
        __asm 
        {
            pushad
            mov eax, t
            call lua_execute_thread_immediate_game
            popad
        }
        return 0;
    }

    uintptr_t new_thread_internal_game = 0xCE01E0_g;
    lua_thread* new_thread_internal(lua_State* ls,const char* function_name,lua_State* sm,int main, int init)
    {
        lua_thread* l_thread;
        __asm
        {
            pushad
            mov eax, ls
            push init
            push main
            push sm
            push function_name
            call new_thread_internal_game
            mov l_thread,eax
            add esp, 0x10
            popad
        }
        return l_thread;
    }

    uintptr_t lua_does_function_exist_game = 0xCE0120_g;
    bool lua_does_function_exist(lua_State* ls, const char* function_name)
    {
        bool result;
        __asm
        {
            pushad
            mov eax, ls
            push function_name
            call lua_does_function_exist_game
            add esp, 0x4
            mov byte ptr[result], al
            popad
        }
        return result;
    }
    lua_State** Vint_lua_state = (lua_State**)0x0252A1B8_g;
    static bool read_file_binary(const std::string& path, std::vector<char>& out)
    {
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file.is_open())
            return false;

        std::streamsize size = file.tellg();
        if (size <= 0)
            return false;

        file.seekg(0, std::ios::beg);

        out.resize((size_t)size);

        if (!file.read(out.data(), size))
            return false;

        return true;
    }

    static std::string make_func_name(std::string_view filename, std::string_view prefix)
    {
        // "main_menu_ui.lua" -> "main_menu_ui_init"

        std::string name(filename);

        if (name.ends_with(".lua"))
            name.resize(name.size() - 4);

        name += prefix;
        return name;
    }

    void LoadExtendedLuaFiles(lua_State* ls,const char* prefix)
    {


        if (ls == nullptr)
        {
            Logger::TypedLog(CHN_DEBUG, "LuaExtended: Vint lua state is null");
            return;
        }

        for (auto& entry : DirCache)
        {
            const std::string& filename = entry.first; // should already be lowercase
            const std::string& filepath = entry.second.FilePath;

            // Only load loose VINT UI lua files.
            if (!std::string_view(filename).ends_with(prefix))
                continue;

            std::vector<char> buffer;

            if (!read_file_binary(filepath, buffer))
            {
                Logger::TypedLog(CHN_DEBUG, "LuaExtended: failed to read {}", filepath);
                continue;
            }
            //General::generalluaLoadBuff_disabled = true;
            int result = luaL_loadbuffer_game(
                buffer.data(),
                buffer.size(),
                ls,
                filename.c_str()
            );
            General::generalluaLoadBuff_disabled = false;
            Logger::TypedLog(
                CHN_DEBUG,
                "LuaExtended: loaded {} result={}",
                filename,
                result
            );
            
            auto init = make_func_name(filename,"_init");
            auto main = make_func_name(filename, "_main");
                auto thread = new_thread_internal(ls, init.c_str(), nullptr, 0, 1);
                if (thread) {
                    lua_execute_thread_immediate(&thread);
                }
            
                auto thread_main = new_thread_internal(ls, main.c_str(), nullptr, 1, 0);

        }
        
    }

    void LoadVintExtended()
    {
        if (Vint_lua_state != nullptr)
        {
            LoadExtendedLuaFiles(*Vint_lua_state, "_ui.lua");
        }
    }

    uintptr_t luaL_openlib_retail = 0xCD9A30_g;
    void* luaL_openlib(lua_State* L, luaL_Reg* reg, const char* eh)
    {
        __asm
        {
            mov ecx, L
            mov eax, reg
            push eh
            call luaL_openlib_retail
            add esp, 4
        }
        return NULL;
    }
    SafetyHookInline register_vint_lua_funcsD;

        //    static luaL_Reg lua_patching_functions[] = {
        //{"vint_subscribe_to_mouse_input", VintSubscribeToMouseInput},
        //{"vint_unsubscribe_to_mouse_input", VintUnsubscribeToMouseInput},
        //{"vint_get_current_clickable_element", VintGetCurrentClickableElement},
        //{"vint_get_object_bbox", VintGetObjectBBox},
        //{"vint_force_mouse_move_event", VintForceMouseMoveEvent},
        //{NULL, NULL}
        //};

    static std::string GetCurrentLuaSource(lua_State* L)
    {
        lua_Debug ar{};

        // level 0 = this C function
        // level 1 = Lua function that called this C function
        if (!lua_getstack(L, 1, &ar))
            return {};

        // "S" = source info
        if (!lua_getinfo(L, "S", &ar))
            return {};

        if (ar.source)
            return ar.source;

        if (ar.short_src)
            return ar.short_src;

        return {};
    }

    template <typename T>
    static int PatchValue(lua_State* L)
    {
        LuaArgs args(L);

        auto address = args.get<uintptr_t>();
        auto value = args.get<T>();
        auto source = GetCurrentLuaSource(L);
        Logger::TypedLog(
            CHN_DEBUG,
            "PatchValue called from Lua source: {}",
            source
        );

        Patch<T>(address, value);

        return 0;
    }

    int Patch_bool(lua_State* L)
    {
        return PatchValue<bool>(L);
    }

    int Patch_int8_t(lua_State* L)
    {
        return PatchValue<int8_t>(L);
    }

    int Patch_uint8_t(lua_State* L)
    {
        return PatchValue<uint8_t>(L);
    }

    int Patch_int16_t(lua_State* L)
    {
        return PatchValue<int16_t>(L);
    }

    int Patch_uint16_t(lua_State* L)
    {
        return PatchValue<uint16_t>(L);
    }

    int Patch_int32_t(lua_State* L)
    {
        return PatchValue<int32_t>(L);
    }

    int Patch_uint32_t(lua_State* L)
    {
        return PatchValue<uint32_t>(L);
    }

    int Patch_int64_t(lua_State* L)
    {
        return PatchValue<int64_t>(L);
    }

    int Patch_uint64_t(lua_State* L)
    {
        return PatchValue<uint64_t>(L);
    }

    int Patch_float(lua_State* L)
    {
        return PatchValue<float>(L);
    }

    int Patch_double(lua_State* L)
    {
        return PatchValue<double>(L);
    }

    int Patch_uintptr_t(lua_State* L)
    {
        return PatchValue<uintptr_t>(L);
    }

    int Patch_intptr_t(lua_State* L)
    {
        return PatchValue<intptr_t>(L);
    }

    int Patch_size_t(lua_State* L)
    {
        return PatchValue<size_t>(L);
    }

    static luaL_Reg lua_patching_functions[] =
    {
        { "PatchBool",   Patch_bool },

        { "PatchI8",     Patch_int8_t },
        { "PatchU8",     Patch_uint8_t },

        { "PatchI16",    Patch_int16_t },
        { "PatchU16",    Patch_uint16_t },

        { "PatchI32",    Patch_int32_t },
        { "PatchU32",    Patch_uint32_t },

        { "PatchI64",    Patch_int64_t },
        { "PatchU64",    Patch_uint64_t },

        { "PatchFloat",  Patch_float },
        { "PatchDouble", Patch_double },

        { "PatchPtr",    Patch_uintptr_t },
        { "PatchSize",   Patch_size_t },

        { NULL, NULL }
    };

    void register_vint_lua_funcs(lua_State* L)
    {
        register_vint_lua_funcsD.unsafe_ccall<void>();
        luaL_openlib(L, lua_patching_functions, "_G");

    }

    void Init()
    {
        register_vint_lua_funcsD = safetyhook::create_inline(0x7F35A0, register_vint_lua_funcs);
        static auto vint_lib_hook = safetyhook::create_mid(0xB9155D, [](SafetyHookContext& ctx) {
            LoadVintExtended();
            });
        static auto system_lib = safetyhook::create_mid(0xA23E72, [](SafetyHookContext& ctx) {
            if (ctx.esi)
            {
                LoadExtendedLuaFiles((lua_State*)ctx.esi, "_gs.lua");
            }
            });

        static auto register_main = safetyhook::create_mid(0x89DA60, [](SafetyHookContext& ctx) {
            luaL_openlib((lua_State*)ctx.eax, lua_patching_functions, "_G");
                

            });

    }

    class LUAX {
    public:
        LUAX() {
            Juiced::onAttach() += []() {
                o_read_numeral = safetyhook::create_inline(0xD70450, hk_read_numeral);
                LuaExtended::Init();
                };
        }
    }LUAX;

}