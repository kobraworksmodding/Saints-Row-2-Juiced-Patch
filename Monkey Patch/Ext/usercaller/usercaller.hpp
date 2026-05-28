#pragma once

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <vector>

#if !defined(_M_IX86) && !defined(__i386__)
#error UserCaller currently supports only 32-bit x86 builds.
#endif

#if defined(_MSC_VER)
#define UC_CDECL __cdecl
#else
#define UC_CDECL
#endif

namespace uc
{
    enum class loc : std::uint8_t
    {
        none,
        stack,
        eax,
        ebx,
        ecx,
        edx,
        esi,
        edi,
        xmm0,
        xmm1,
        xmm2,
        xmm3,
        xmm4,
        xmm5,
        xmm6,
        xmm7,

        st0,
    };

    enum class loc_pair : std::uint8_t
    {
        none,
        edx_eax,
    };

    enum class cleanup : std::uint8_t
    {
        caller,
        callee,
    };

    template <loc Where, typename T>
    struct arg
    {
        using type = T;
        static constexpr loc where = Where;
    };

    template <loc Where, typename T>
    struct ret
    {
        using type = T;
        static constexpr bool is_pair = false;
        static constexpr loc where = Where;
        static constexpr loc_pair pair_where = loc_pair::none;
    };

    template <loc_pair Where, typename T>
    struct ret64
    {
        using type = T;

        static constexpr bool is_pair = true;
        static constexpr loc where = loc::none;
        static constexpr loc_pair pair_where = Where;
    };

    template <typename T> using eax_ret = ret<loc::eax, T>;
    template <typename T> using xmm0_ret = ret<loc::xmm0, T>;
    template <typename T> using void_ret = ret<loc::none, T>;
    template <typename T> using st0_ret = ret<loc::st0, T>;
    template <typename T> using edx_eax_ret = ret64<loc_pair::edx_eax, T>;

    template <typename T> using eax_arg = arg<loc::eax, T>;
    template <typename T> using ebx_arg = arg<loc::ebx, T>;
    template <typename T> using ecx_arg = arg<loc::ecx, T>;
    template <typename T> using edx_arg = arg<loc::edx, T>;
    template <typename T> using esi_arg = arg<loc::esi, T>;
    template <typename T> using edi_arg = arg<loc::edi, T>;
    template <typename T> using xmm0_arg = arg<loc::xmm0, T>;
    template <typename T> using xmm1_arg = arg<loc::xmm1, T>;
    template <typename T> using xmm2_arg = arg<loc::xmm2, T>;
    template <typename T> using xmm3_arg = arg<loc::xmm3, T>;
    template <typename T> using xmm4_arg = arg<loc::xmm4, T>;
    template <typename T> using xmm5_arg = arg<loc::xmm5, T>;
    template <typename T> using xmm6_arg = arg<loc::xmm6, T>;
    template <typename T> using xmm7_arg = arg<loc::xmm7, T>;
    template <typename T> using stack_arg = arg<loc::stack, T>;

    namespace detail
    {
        struct arg_desc
        {
            loc where{};
            std::uint32_t bytes{};
        };

        struct abi_desc
        {
            cleanup cleanup_mode{};
            loc return_where{};
            std::uint32_t return_bytes{};
            bool runtime_target{};
            std::vector<arg_desc> args{};
        };

        struct code_block
        {
            explicit code_block(void* entry_) noexcept;
            ~code_block() noexcept;

            code_block(const code_block&) = delete;
            code_block& operator=(const code_block&) = delete;

            void* entry{};
        };

        std::shared_ptr<code_block> build_baked(const abi_desc& desc, std::uintptr_t target);
        std::shared_ptr<code_block> build_callback(const abi_desc& desc, std::uintptr_t callback);
        std::shared_ptr<code_block> build_invoker(const abi_desc& desc);

        template <typename T>
        consteval std::uint32_t abi_bytes()
        {
            if constexpr (std::is_void_v<T>)
            {
                return 0;
            }
            else
            {
                static_assert(sizeof(T) <= 8, "UserCaller only supports <= 8-byte args in this first version.");

                if constexpr (sizeof(T) <= 4)
                    return 4;
                else
                    return 8;
            }
        }

        template <typename ArgSpec>
        consteval bool valid_arg()
        {
            using T = typename ArgSpec::type;

            if constexpr (ArgSpec::where == loc::stack)
            {
                return sizeof(T) <= 8;
            }
            else
            {
                constexpr loc w = ArgSpec::where;

                constexpr bool is_gpr =
                    w == loc::eax ||
                    w == loc::ebx ||
                    w == loc::ecx ||
                    w == loc::edx ||
                    w == loc::esi ||
                    w == loc::edi;

                if constexpr (is_gpr)
                {
                    return sizeof(T) <= 4;
                }
                else
                {
                    constexpr bool is_xmm =
                        w == loc::xmm0 ||
                        w == loc::xmm1 ||
                        w == loc::xmm2 ||
                        w == loc::xmm3 ||
                        w == loc::xmm4 ||
                        w == loc::xmm5 ||
                        w == loc::xmm6 ||
                        w == loc::xmm7;

                    return is_xmm && (std::is_same_v<T, float> || std::is_same_v<T, double>);
                }
            }
        }

        template <typename RetSpec>
        consteval bool valid_ret()
        {
            using T = typename RetSpec::type;

            if constexpr (RetSpec::is_pair)
            {
                return
                    RetSpec::pair_where == loc_pair::edx_eax &&
                    sizeof(T) == 8 &&
                    (std::is_integral_v<T> || std::is_enum_v<T>);
            }
            else
            {
                if constexpr (std::is_void_v<T>)
                {
                    return RetSpec::where == loc::none;
                }
                else if constexpr (RetSpec::where == loc::eax)
                {
                    return sizeof(T) <= 4;
                }
                else if constexpr (RetSpec::where == loc::st0)
                {
                    return std::is_same_v<T, float> || std::is_same_v<T, double>;
                }
                else if constexpr (RetSpec::where == loc::xmm0)
                {
                    return std::is_same_v<T, float> || std::is_same_v<T, double>;
                }
                else
                {
                    return false;
                }
            }
        }

        template <loc Where, typename... ArgSpecs>
        consteval int count_arg_loc()
        {
            return ((ArgSpecs::where == Where ? 1 : 0) + ... + 0);
        }

        template <typename... ArgSpecs>
        consteval bool no_duplicate_register_args()
        {
            return
                count_arg_loc<loc::eax, ArgSpecs...>() <= 1 &&
                count_arg_loc<loc::ebx, ArgSpecs...>() <= 1 &&
                count_arg_loc<loc::ecx, ArgSpecs...>() <= 1 &&
                count_arg_loc<loc::edx, ArgSpecs...>() <= 1 &&
                count_arg_loc<loc::esi, ArgSpecs...>() <= 1 &&
                count_arg_loc<loc::edi, ArgSpecs...>() <= 1 &&
                count_arg_loc<loc::xmm0, ArgSpecs...>() <= 1 &&
                count_arg_loc<loc::xmm1, ArgSpecs...>() <= 1 &&
                count_arg_loc<loc::xmm2, ArgSpecs...>() <= 1 &&
                count_arg_loc<loc::xmm3, ArgSpecs...>() <= 1 &&
                count_arg_loc<loc::xmm4, ArgSpecs...>() <= 1 &&
                count_arg_loc<loc::xmm5, ArgSpecs...>() <= 1 &&
                count_arg_loc<loc::xmm6, ArgSpecs...>() <= 1 &&
                count_arg_loc<loc::xmm7, ArgSpecs...>() <= 1;
        }

        template <cleanup CleanupMode, bool RuntimeTarget, typename RetSpec, typename... ArgSpecs>
        abi_desc make_desc()
        {
            static_assert(valid_ret<RetSpec>(), "Only void return, <=4-byte eax return, st0/xmm0 float/double return, or edx:eax int64 return is supported for now.");
            static_assert((valid_arg<ArgSpecs>() && ...), "Invalid arg. GPR args must be <=4 bytes. XMM args must be float/double. Stack args may be <=8 bytes.");

            static_assert(
                no_duplicate_register_args<ArgSpecs...>(),
                "Duplicate register arguments are not allowed. Only stack arguments may repeat."
                );

            abi_desc desc{};
            desc.cleanup_mode = CleanupMode;
            desc.return_where = RetSpec::where;
            desc.return_bytes = abi_bytes<typename RetSpec::type>();
            desc.runtime_target = RuntimeTarget;

            desc.args = {
                arg_desc{
                    ArgSpecs::where,
                    abi_bytes<typename ArgSpecs::type>()
                }...
            };

            return desc;
        }

        template <cleanup CleanupMode, typename RetSpec, typename... ArgSpecs>
        struct invoker_cache
        {
            using ret_t = typename RetSpec::type;
            using fn_t = ret_t(UC_CDECL*)(std::uintptr_t target, typename ArgSpecs::type...);

            static fn_t get()
            {
                static std::shared_ptr<code_block> block =
                    build_invoker(make_desc<CleanupMode, true, RetSpec, ArgSpecs...>());

                return reinterpret_cast<fn_t>(block->entry);
            }
        };
    }

    template <cleanup CleanupMode, typename RetSpec, typename... ArgSpecs>
    class function
    {
    public:
        using ret_t = typename RetSpec::type;
        using fn_t = ret_t(UC_CDECL*)(typename ArgSpecs::type...);

        explicit function(std::uintptr_t target)
        {
            auto desc = detail::make_desc<CleanupMode, false, RetSpec, ArgSpecs...>();
            block_ = detail::build_baked(desc, target);
            fn_ = reinterpret_cast<fn_t>(block_->entry);
        }

        function(const function&) = default;
        function& operator=(const function&) = default;

        function(function&&) noexcept = default;
        function& operator=(function&&) noexcept = default;

        ret_t operator()(typename ArgSpecs::type... args) const
        {
            if constexpr (std::is_void_v<ret_t>)
            {
                fn_(args...);
            }
            else
            {
                return fn_(args...);
            }
        }

        fn_t raw() const noexcept
        {
            return fn_;
        }

    private:
        std::shared_ptr<detail::code_block> block_{};
        fn_t fn_{};
    };

    template <typename RetSpec, typename... ArgSpecs>
    auto make(std::uintptr_t target)
    {
        return function<cleanup::caller, RetSpec, ArgSpecs...>(target);
    }

    template <typename RetSpec, typename... ArgSpecs>
    auto make_callee(std::uintptr_t target)
    {
        return function<cleanup::callee, RetSpec, ArgSpecs...>(target);
    }

    template <cleanup CleanupMode, typename RetSpec, typename... ArgSpecs>
    class callback
    {
    public:
        using ret_t = typename RetSpec::type;
        using callback_fn_t = ret_t(UC_CDECL*)(typename ArgSpecs::type...);

        callback() = default;

        explicit callback(callback_fn_t callback_fn)
        {
            auto desc = detail::make_desc<CleanupMode, false, RetSpec, ArgSpecs...>();
            block_ = detail::build_callback(desc, reinterpret_cast<std::uintptr_t>(callback_fn));
        }

        callback(const callback&) = default;
        callback& operator=(const callback&) = default;

        callback(callback&&) noexcept = default;
        callback& operator=(callback&&) noexcept = default;

        void* raw() const noexcept
        {
            return block_ ? block_->entry : nullptr;
        }

        std::uintptr_t address() const noexcept
        {
            return reinterpret_cast<std::uintptr_t>(raw());
        }

    private:
        std::shared_ptr<detail::code_block> block_{};
    };

    template <typename RetSpec, typename... ArgSpecs>
    auto make_callback(
        typename callback<cleanup::caller, RetSpec, ArgSpecs...>::callback_fn_t callback_fn
    )
    {
        return callback<cleanup::caller, RetSpec, ArgSpecs...>(callback_fn);
    }

    template <typename RetSpec, typename... ArgSpecs>
    auto make_callback_callee(
        typename callback<cleanup::callee, RetSpec, ArgSpecs...>::callback_fn_t callback_fn
    )
    {
        return callback<cleanup::callee, RetSpec, ArgSpecs...>(callback_fn);
    }

    template <typename RetSpec, typename... ArgSpecs>
    struct abi
    {
        using ret_t = typename RetSpec::type;

        using invoker_t = ret_t(UC_CDECL*)(
            std::uintptr_t target,
            typename ArgSpecs::type...
        );

        using callback_fn_t = ret_t(UC_CDECL*)(
            typename ArgSpecs::type...
        );

        using function_t = function<
            cleanup::caller,
            RetSpec,
            ArgSpecs...
        >;

        using function_callee_t = function<
            cleanup::callee,
            RetSpec,
            ArgSpecs...
        >;

        using callback_t = callback<
            cleanup::caller,
            RetSpec,
            ArgSpecs...
        >;

        using callback_callee_t = callback<
            cleanup::callee,
            RetSpec,
            ArgSpecs...
        >;

        static function_t make(std::uintptr_t target)
        {
            return uc::make<RetSpec, ArgSpecs...>(target);
        }

        static function_callee_t make_callee(std::uintptr_t target)
        {
            return uc::make_callee<RetSpec, ArgSpecs...>(target);
        }

        static invoker_t make_invoker()
        {
            return detail::invoker_cache<cleanup::caller, RetSpec, ArgSpecs...>::get();
        }

        static invoker_t make_invoker_callee()
        {
            return detail::invoker_cache<cleanup::callee, RetSpec, ArgSpecs...>::get();
        }

        static callback_t make_callback(callback_fn_t fn)
        {
            return uc::make_callback<RetSpec, ArgSpecs...>(fn);
        }

        static callback_callee_t make_callback_callee(callback_fn_t fn)
        {
            return uc::make_callback_callee<RetSpec, ArgSpecs...>(fn);
        }
    };

    template <typename RetSpec, typename... ArgSpecs>
    auto make_invoker()
    {
        return detail::invoker_cache<cleanup::caller, RetSpec, ArgSpecs...>::get();
    }

    template <typename RetSpec, typename... ArgSpecs>
    auto make_invoker_callee()
    {
        return detail::invoker_cache<cleanup::callee, RetSpec, ArgSpecs...>::get();
    }

    void patch_call(void* call_site, void* new_target);

    inline void patch_call(std::uintptr_t call_site, void* new_target)
    {
        patch_call(reinterpret_cast<void*>(call_site), new_target);
    }
}
