#pragma once
#include <safetyhook.hpp>
#include <libloaderapi.h>
#include <bit>


//template<typename AT>
//inline AT DynAddress(AT address)
//{
//	static_assert(sizeof(AT) == sizeof(uintptr_t), "AT must be pointer sized");
//#ifdef _WIN64
//	return (ptrdiff_t)GetModuleHandle(nullptr) - 0x140000000 + address;
//#else
//	return (ptrdiff_t)GetModuleHandle(nullptr) - 0x400000 + address;
//#endif
//}
static auto HandleDynAddress = GetModuleHandle(nullptr);
template<typename AT>
__declspec(noinline) AT DynAddress(AT address)
{
    printf("HandleDynAddress 0x%p\n", HandleDynAddress);
    static_assert(sizeof(AT) == sizeof(uintptr_t), "AT must be pointer sized");

    uintptr_t inputAddr = std::bit_cast<uintptr_t>(address);

    // This is SR2 exe range, this function should only be really used for the mass conversion of older functions, and from now on we have to rely on MemoryMgr.h from ModUtils -- Clippy95
    if (inputAddr >= 0x00400000ULL && inputAddr <= 0x03559000ULL) {
        uintptr_t baseAddr = std::bit_cast<uintptr_t>(HandleDynAddress);

#ifdef _WIN64
        uintptr_t result = baseAddr - 0x140000000ULL + inputAddr;
#else
        uintptr_t result = baseAddr - 0x400000UL + inputAddr;
#endif
        printf("return 0x%p\n", std::bit_cast<AT>(result));
        return std::bit_cast<AT>(result);
    }

    // Return the original address if it's outside the range
    return address;
}

template <typename T, typename U>
[[nodiscard]] inline safetyhook::InlineHook create_inlinehook(T target, U destination, safetyhook::InlineHook::Flags flags = safetyhook::InlineHook::Default) {

    return safetyhook::create_inline(DynAddress(reinterpret_cast<void*>(target)), reinterpret_cast<void*>(destination), flags);
}

template <typename T>
[[nodiscard]] inline safetyhook::MidHook create_midhook(T target, safetyhook::MidHookFn destination, safetyhook::MidHook::Flags flags = safetyhook::MidHook::Default) {
    return safetyhook::create_mid(DynAddress(reinterpret_cast<void*>(target)), destination, flags);
}