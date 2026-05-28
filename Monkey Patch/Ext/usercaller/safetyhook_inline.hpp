#pragma once

#include "usercaller.hpp"

#if defined(UC_WITH_SAFETYHOOK)

#include <cassert>
#include <cstdint>
#include <type_traits>
#include <vector>

#include <safetyhook.hpp>

namespace uc
{
    template <typename Abi>
    class inline_hook
    {
    public:
        using ret_t = typename Abi::ret_t;

        inline_hook() = default;

        inline_hook(const inline_hook&) = delete;
        inline_hook& operator=(const inline_hook&) = delete;

        inline_hook(inline_hook&&) noexcept = default;
        inline_hook& operator=(inline_hook&&) noexcept = default;

        template <typename CallbackFn>
        bool create(void* target, CallbackFn callback)
        {
            reset();

            callback_ = Abi::make_callback(callback);
            invoker_ = Abi::make_invoker();
            hook_ = safetyhook::create_inline(target, callback_.raw());

            if (!hook_)
            {
                callback_ = typename Abi::callback_t{};
                invoker_ = {};
            }

            return static_cast<bool>(hook_);
        }

        template <typename CallbackFn>
        bool create(std::uintptr_t target, CallbackFn callback)
        {
            return create(reinterpret_cast<void*>(target), callback);
        }

        void reset()
        {
            hook_.reset();
            callback_ = typename Abi::callback_t{};
            invoker_ = {};
        }

        bool valid() const
        {
            return static_cast<bool>(hook_);
        }

        explicit operator bool() const
        {
            return valid();
        }

        bool enabled() const
        {
            return hook_.enabled();
        }

        bool enable()
        {
            if (!hook_)
                return false;

            return hook_.enable().has_value();
        }

        bool disable()
        {
            if (!hook_)
                return false;

            return hook_.disable().has_value();
        }

        std::uintptr_t target_address() const
        {
            return hook_ ? hook_.target_address() : 0;
        }

        std::uintptr_t destination_address() const
        {
            return hook_ ? hook_.destination_address() : 0;
        }

        std::uintptr_t trampoline_address() const
        {
            return hook_ ? hook_.trampoline().address() : 0;
        }

        const std::vector<std::uint8_t>& original_bytes() const
        {
            return hook_.original_bytes();
        }

        template <typename... Args>
        ret_t call_original(Args... args)
        {
            if (!hook_)
            {
#if defined(_DEBUG)
                assert(false && "UserCaller: inline_hook::call_original called on invalid hook.");
#endif

                if constexpr (!std::is_void_v<ret_t>)
                    return ret_t{};
                else
                    return;
            }

            const auto tramp = hook_.trampoline().address();

            if constexpr (std::is_void_v<ret_t>)
            {
                invoker_(tramp, args...);
            }
            else
            {
                return invoker_(tramp, args...);
            }
        }

        template <typename... Args>
        ret_t unsafe_call_original(Args... args)
        {
            if constexpr (std::is_void_v<ret_t>)
            {
                call_original(args...);
            }
            else
            {
                return call_original(args...);
            }
        }

    private:
        SafetyHookInline hook_{};
        typename Abi::callback_t callback_{};
        typename Abi::invoker_t invoker_{};
    };
}

#endif
