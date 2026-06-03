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
    namespace detail
    {
        template <cleanup CleanupMode, typename Abi>
        struct inline_hook_traits;

        template <typename Abi>
        struct inline_hook_traits<cleanup::caller, Abi>
        {
            using callback_t = typename Abi::callback_t;

            template <typename CallbackFn>
            static callback_t make_callback(CallbackFn callback)
            {
                return Abi::make_callback(callback);
            }

            static typename Abi::invoker_t make_invoker()
            {
                return Abi::make_invoker();
            }
        };

        template <typename Abi>
        struct inline_hook_traits<cleanup::callee, Abi>
        {
            using callback_t = typename Abi::callback_callee_t;

            template <typename CallbackFn>
            static callback_t make_callback(CallbackFn callback)
            {
                return Abi::make_callback_callee(callback);
            }

            static typename Abi::invoker_t make_invoker()
            {
                return Abi::make_invoker_callee();
            }
        };
    }

    template <cleanup CleanupMode, typename Abi>
    class basic_inline_hook
    {
    public:
        using ret_t = typename Abi::ret_t;
        using traits_t = detail::inline_hook_traits<CleanupMode, Abi>;
        using callback_t = typename traits_t::callback_t;

        basic_inline_hook() = default;

        basic_inline_hook(const basic_inline_hook&) = delete;
        basic_inline_hook& operator=(const basic_inline_hook&) = delete;

        basic_inline_hook(basic_inline_hook&&) noexcept = default;
        basic_inline_hook& operator=(basic_inline_hook&&) noexcept = default;

        template <typename CallbackFn>
        bool create(void* target, CallbackFn callback)
        {
            reset();

            callback_ = traits_t::make_callback(callback);
            invoker_ = traits_t::make_invoker();
            hook_ = safetyhook::create_inline(target, callback_.raw());

            if (!hook_)
            {
                callback_ = callback_t{};
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
            callback_ = callback_t{};
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
                assert(false && "UserCaller: inline_hook call_original called on invalid hook.");
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
        callback_t callback_{};
        typename Abi::invoker_t invoker_{};
    };

    template <typename Abi>
    using inline_hook = basic_inline_hook<cleanup::caller, Abi>;

    template <typename Abi>
    using inline_hook_callee = basic_inline_hook<cleanup::callee, Abi>;
}

#endif
