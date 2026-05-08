#pragma once

#include "..\LuaHandler.h"

#include <cstdint>
#include <cstddef>
#include <optional>
#include <string>
#include <type_traits>

// note the second template arg
template<typename T, typename Enable = void>
struct lua_type_traits;

// ======================================================
// all integer types except bool
// int, uint32_t, uintptr_t, size_t, int64_t, etc.
// ======================================================

template<typename T>
struct lua_type_traits<
    T,
    std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<T, bool>>
>
{
    static T get(lua_State* L, int index)
    {
        return static_cast<T>(lua_tonumber(L, index));
    }

    static void push(lua_State* L, T value)
    {
        lua_pushnumber(L, static_cast<lua_Number>(value));
    }

    static bool is_valid_type(lua_State* L, int index)
    {
        return lua_isnumber(L, index);
    }
};

// ======================================================
// floating point
// ======================================================

template<typename T>
struct lua_type_traits<
    T,
    std::enable_if_t<std::is_floating_point_v<T>>
>
{
    static T get(lua_State* L, int index)
    {
        return static_cast<T>(lua_tonumber(L, index));
    }

    static void push(lua_State* L, T value)
    {
        lua_pushnumber(L, static_cast<lua_Number>(value));
    }

    static bool is_valid_type(lua_State* L, int index)
    {
        return lua_isnumber(L, index);
    }
};

// ======================================================
// bool
// ======================================================

template<>
struct lua_type_traits<bool>
{
    static bool get(lua_State* L, int index)
    {
        return lua_toboolean(L, index) != 0;
    }

    static void push(lua_State* L, bool value)
    {
        lua_pushboolean(L, value);
    }

    static bool is_valid_type(lua_State* L, int index)
    {
        return lua_isboolean(L, index);
    }
};

// ======================================================
// const char*
// ======================================================

template<>
struct lua_type_traits<const char*>
{
    static const char* get(lua_State* L, int index)
    {
        return lua_tostring(L, index);
    }

    static void push(lua_State* L, const char* value)
    {
        lua_pushstring(L, value ? value : "");
    }

    static bool is_valid_type(lua_State* L, int index)
    {
        return lua_isstring(L, index);
    }
};

// ======================================================
// char*
// ======================================================

template<>
struct lua_type_traits<char*>
{
    static char* get(lua_State* L, int index)
    {
        return const_cast<char*>(lua_tostring(L, index));
    }

    static void push(lua_State* L, const char* value)
    {
        lua_pushstring(L, value ? value : "");
    }

    static bool is_valid_type(lua_State* L, int index)
    {
        return lua_isstring(L, index);
    }
};

// ======================================================
// std::string
// ======================================================

template<>
struct lua_type_traits<std::string>
{
    static std::string get(lua_State* L, int index)
    {
        const char* str = lua_tostring(L, index);
        return str ? std::string(str) : std::string{};
    }

    static void push(lua_State* L, const std::string& value)
    {
        lua_pushstring(L, value.c_str());
    }

    static bool is_valid_type(lua_State* L, int index)
    {
        return lua_isstring(L, index);
    }
};

// ======================================================
// void*
// ======================================================

template<>
struct lua_type_traits<void*>
{
    static void* get(lua_State* L, int index)
    {
        return reinterpret_cast<void*>(
            static_cast<uintptr_t>(lua_tonumber(L, index))
            );
    }

    static void push(lua_State* L, void* value)
    {
        lua_pushnumber(
            L,
            static_cast<lua_Number>(reinterpret_cast<uintptr_t>(value))
        );
    }

    static bool is_valid_type(lua_State* L, int index)
    {
        return lua_isnumber(L, index);
    }
};

// ======================================================
// LuaArgs
// ======================================================

class LuaArgs {
    lua_State* L;
    int current_index;
    int total_args;

public:
    LuaArgs(lua_State* L)
        : L(L), current_index(1), total_args(lua_gettop(L)) {
    }

    template<typename T>
    T get() {
        if (current_index > total_args) {
            return T{};
        }

        T result = lua_type_traits<T>::get(L, current_index);
        current_index++;
        return result;
    }

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

    template<typename T>
    T get_or_unsafe(T default_value) {
        if (current_index > total_args) {
            return default_value;
        }

        T result = lua_type_traits<T>::get(L, current_index);
        current_index++;
        return result;
    }

    template<typename T>
    std::optional<T> get_optional() {
        if (current_index > total_args) {
            return std::nullopt;
        }

        if (!lua_type_traits<T>::is_valid_type(L, current_index)) {
            current_index++;
            return std::nullopt;
        }

        T result = lua_type_traits<T>::get(L, current_index);
        current_index++;
        return result;
    }

    bool has_more() const {
        return current_index <= total_args;
    }

    bool has_arg(int offset = 0) const {
        return (current_index + offset) <= total_args;
    }

    template<typename T>
    T peek(int offset = 0) {
        int index = current_index + offset;

        if (index > total_args) {
            return T{};
        }

        return lua_type_traits<T>::get(L, index);
    }

    int remaining() const {
        return total_args - current_index + 1;
    }

    int total() const {
        return total_args;
    }
};

// ======================================================
// LuaReturns
// ======================================================

class LuaReturns {
    lua_State* L;
    int return_count;

public:
    LuaReturns(lua_State* L)
        : L(L), return_count(0) {
    }

    template<typename T>
    LuaReturns& push(T value) {
        lua_type_traits<T>::push(L, value);
        return_count++;
        return *this;
    }

    template<typename T, typename... Args>
    LuaReturns& push_all(T first, Args... rest) {
        push(first);

        if constexpr (sizeof...(rest) > 0) {
            push_all(rest...);
        }

        return *this;
    }

    int count() const {
        return return_count;
    }
};

namespace GLua
{
    extern void Init();
}