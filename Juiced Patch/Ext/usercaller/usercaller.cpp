#include "usercaller.hpp"

#include <Windows.h>

#include <xbyak.h>

#include <algorithm>
#include <cstring>
#include <memory>
#include <mutex>
#include <string>

namespace uc::detail
{
    namespace
    {
        constexpr std::size_t CodePageBytes = 64 * 1024;
        constexpr std::size_t CodeAlignBytes = 16;

        struct executable_page
        {
            executable_page()
                : base(static_cast<std::uint8_t*>(
                    VirtualAlloc(nullptr, CodePageBytes, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE)
                ))
            {
                if (!base)
                    throw std::runtime_error("UserCaller: VirtualAlloc failed for executable code page.");
            }

            ~executable_page()
            {
                if (base)
                    VirtualFree(base, 0, MEM_RELEASE);
            }

            executable_page(const executable_page&) = delete;
            executable_page& operator=(const executable_page&) = delete;

            bool make_writable()
            {
                DWORD old_protect{};
                return VirtualProtect(base, CodePageBytes, PAGE_READWRITE, &old_protect) != 0;
            }

            bool make_executable()
            {
                DWORD old_protect{};
                return VirtualProtect(base, CodePageBytes, PAGE_EXECUTE_READ, &old_protect) != 0;
            }

            std::uint8_t* reserve(std::size_t bytes)
            {
                const std::size_t aligned_offset =
                    (used + CodeAlignBytes - 1) & ~(CodeAlignBytes - 1);

                if (aligned_offset + bytes > CodePageBytes)
                    return nullptr;

                std::uint8_t* out = base + aligned_offset;
                used = aligned_offset + bytes;
                return out;
            }

            std::uint8_t* base{};
            std::size_t used{};
        };

        std::mutex& runtime_mutex()
        {
            static std::mutex mtx;
            return mtx;
        }

        std::vector<std::unique_ptr<executable_page>>& code_pages()
        {
            static std::vector<std::unique_ptr<executable_page>> pages;
            return pages;
        }

        struct code_allocation
        {
            executable_page* page{};
            void* ptr{};
        };

        code_allocation reserve_code(std::size_t bytes)
        {
            std::lock_guard<std::mutex> lock(runtime_mutex());

            for (const auto& page : code_pages())
            {
                if (void* p = page->reserve(bytes))
                    return { page.get(), p };
            }

            auto page = std::make_unique<executable_page>();
            void* p = page->reserve(bytes);

            if (!p)
                throw std::runtime_error("UserCaller: generated code is larger than one executable code page.");

            executable_page* page_ptr = page.get();
            code_pages().push_back(std::move(page));
            return { page_ptr, p };
        }

        Xbyak::Reg32 gp_from_loc(loc where)
        {
            using namespace Xbyak::util;

            switch (where)
            {
            case loc::eax: return eax;
            case loc::ebx: return ebx;
            case loc::ecx: return ecx;
            case loc::edx: return edx;
            case loc::esi: return esi;
            case loc::edi: return edi;
            default:
                throw std::runtime_error("UserCaller: invalid register location.");
            }
        }

        bool is_xmm_loc(loc where)
        {
            return
                where == loc::xmm0 ||
                where == loc::xmm1 ||
                where == loc::xmm2 ||
                where == loc::xmm3 ||
                where == loc::xmm4 ||
                where == loc::xmm5 ||
                where == loc::xmm6 ||
                where == loc::xmm7;
        }

        Xbyak::Xmm xmm_from_loc(loc where)
        {
            using namespace Xbyak::util;

            switch (where)
            {
            case loc::xmm0: return xmm0;
            case loc::xmm1: return xmm1;
            case loc::xmm2: return xmm2;
            case loc::xmm3: return xmm3;
            case loc::xmm4: return xmm4;
            case loc::xmm5: return xmm5;
            case loc::xmm6: return xmm6;
            case loc::xmm7: return xmm7;
            default:
                throw std::runtime_error("UserCaller: invalid xmm register location.");
            }
        }

        bool uses_register(const abi_desc& desc, loc reg)
        {
            for (const auto& arg : desc.args)
            {
                if (arg.where == reg)
                    return true;
            }

            return false;
        }

        std::uint32_t pushed_stack_bytes(const abi_desc& desc)
        {
            std::uint32_t total = 0;

            for (const auto& arg : desc.args)
            {
                if (arg.where == loc::stack)
                    total += arg.bytes;
            }

            return total;
        }

        std::vector<std::uint32_t> wrapper_arg_offsets(const abi_desc& desc)
        {
            std::vector<std::uint32_t> offsets;
            offsets.reserve(desc.args.size());

            std::uint32_t offset = desc.runtime_target ? 12u : 8u;

            for (const auto& arg : desc.args)
            {
                offsets.push_back(offset);
                offset += arg.bytes;
            }

            return offsets;
        }

        std::vector<std::uint32_t> incoming_stack_arg_offsets(const abi_desc& desc)
        {
            std::vector<std::uint32_t> offsets(desc.args.size(), 0);
            std::uint32_t offset = 8;

            for (std::size_t i = 0; i < desc.args.size(); ++i)
            {
                const auto& arg = desc.args[i];

                if (arg.where == loc::stack)
                {
                    offsets[i] = offset;
                    offset += arg.bytes;
                }
            }

            return offsets;
        }

        std::uint32_t callback_arg_bytes(const abi_desc& desc)
        {
            std::uint32_t total = 0;

            for (const auto& arg : desc.args)
                total += arg.bytes;

            return total;
        }

        void emit_push_stack_arg(
            Xbyak::CodeGenerator& c,
            const arg_desc& arg,
            std::uint32_t wrapper_offset)
        {
            using namespace Xbyak::util;

            if (arg.bytes == 4)
            {
                c.push(dword[ebp + static_cast<int>(wrapper_offset)]);
                return;
            }

            if (arg.bytes == 8)
            {
                c.push(dword[ebp + static_cast<int>(wrapper_offset + 4)]);
                c.push(dword[ebp + static_cast<int>(wrapper_offset)]);
                return;
            }

            throw std::runtime_error("UserCaller: unsupported stack arg size.");
        }

        void emit_load_xmm_arg(
            Xbyak::CodeGenerator& c,
            loc where,
            std::uint32_t wrapper_offset,
            std::uint32_t bytes
        )
        {
            using namespace Xbyak::util;

            const auto xmm = xmm_from_loc(where);

            if (bytes == 4)
            {
                c.movss(xmm, dword[ebp + static_cast<int>(wrapper_offset)]);
                return;
            }

            if (bytes == 8)
            {
                c.movsd(xmm, qword[ebp + static_cast<int>(wrapper_offset)]);
                return;
            }

            throw std::runtime_error("UserCaller: unsupported xmm arg size.");
        }

        void emit_store_xmm_arg_on_stack(
            Xbyak::CodeGenerator& c,
            loc where,
            std::uint32_t bytes
        )
        {
            using namespace Xbyak::util;

            const auto xmm = xmm_from_loc(where);

            if (bytes == 4)
            {
                c.sub(esp, 4);
                c.movss(dword[esp], xmm);
                return;
            }

            if (bytes == 8)
            {
                c.sub(esp, 8);
                c.movsd(qword[esp], xmm);
                return;
            }

            throw std::runtime_error("UserCaller: unsupported xmm callback arg size.");
        }

        void emit_bridge_xmm0_to_st0(Xbyak::CodeGenerator& c, std::uint32_t bytes)
        {
            using namespace Xbyak::util;

            if (bytes == 4)
            {
                c.sub(esp, 4);
                c.movss(dword[esp], xmm0);
                c.fld(dword[esp]);
                c.add(esp, 4);
                return;
            }

            if (bytes == 8)
            {
                c.sub(esp, 8);
                c.movsd(qword[esp], xmm0);
                c.fld(qword[esp]);
                c.add(esp, 8);
                return;
            }

            throw std::runtime_error("UserCaller: unsupported xmm0 return size.");
        }

        void emit_bridge_st0_to_xmm0(Xbyak::CodeGenerator& c, std::uint32_t bytes)
        {
            using namespace Xbyak::util;

            if (bytes == 4)
            {
                c.sub(esp, 4);
                c.fstp(dword[esp]);
                c.movss(xmm0, dword[esp]);
                c.add(esp, 4);
                return;
            }

            if (bytes == 8)
            {
                c.sub(esp, 8);
                c.fstp(qword[esp]);
                c.movsd(xmm0, qword[esp]);
                c.add(esp, 8);
                return;
            }

            throw std::runtime_error("UserCaller: unsupported st0 return size for xmm0 bridge.");
        }

        void emit_push_callback_arg(
            Xbyak::CodeGenerator& c,
            const abi_desc& desc,
            const std::vector<std::uint32_t>& incoming_offsets,
            std::size_t index
        )
        {
            using namespace Xbyak::util;

            const auto& arg = desc.args[index];

            if (arg.where == loc::stack)
            {
                const auto offset = incoming_offsets[index];

                if (arg.bytes == 4)
                {
                    c.push(dword[ebp + static_cast<int>(offset)]);
                    return;
                }

                if (arg.bytes == 8)
                {
                    c.push(dword[ebp + static_cast<int>(offset + 4)]);
                    c.push(dword[ebp + static_cast<int>(offset)]);
                    return;
                }

                throw std::runtime_error("UserCaller: unsupported callback stack arg size.");
            }

            if (is_xmm_loc(arg.where))
            {
                emit_store_xmm_arg_on_stack(c, arg.where, arg.bytes);
                return;
            }

            if (arg.bytes != 4)
                throw std::runtime_error("UserCaller: callback GPR args must be 4 bytes in this version.");

            c.push(gp_from_loc(arg.where));
        }

        void emit_thunk_body(
            Xbyak::CodeGenerator& c,
            const abi_desc& desc,
            std::uintptr_t baked_target)
        {
            using namespace Xbyak::util;

            const auto offsets = wrapper_arg_offsets(desc);

            const bool preserve_ebx = uses_register(desc, loc::ebx);
            const bool preserve_esi = uses_register(desc, loc::esi);
            const bool preserve_edi = uses_register(desc, loc::edi);
            const bool baked = !desc.runtime_target;

            c.push(ebp);
            c.mov(ebp, esp);

            if (baked)
            {
                c.sub(esp, 4);
                c.mov(dword[ebp - 4], static_cast<std::uint32_t>(baked_target));
            }

            if (preserve_ebx)
                c.push(ebx);

            if (preserve_esi)
                c.push(esi);

            if (preserve_edi)
                c.push(edi);

            for (std::size_t i = desc.args.size(); i > 0; --i)
            {
                const std::size_t index = i - 1;
                const auto& arg = desc.args[index];

                if (arg.where == loc::stack)
                    emit_push_stack_arg(c, arg, offsets[index]);
            }

            for (std::size_t i = 0; i < desc.args.size(); ++i)
            {
                const auto& arg = desc.args[i];

                if (arg.where == loc::stack)
                    continue;

                if (is_xmm_loc(arg.where))
                {
                    emit_load_xmm_arg(c, arg.where, offsets[i], arg.bytes);
                    continue;
                }

                if (arg.bytes != 4)
                    throw std::runtime_error("UserCaller: GPR args must be 4 bytes in this version.");

                c.mov(gp_from_loc(arg.where), dword[ebp + static_cast<int>(offsets[i])]);
            }

            if (desc.runtime_target)
                c.call(dword[ebp + 8]);
            else
                c.call(dword[ebp - 4]);

            const std::uint32_t stack_bytes = pushed_stack_bytes(desc);

            if (desc.cleanup_mode == cleanup::caller && stack_bytes != 0)
                c.add(esp, stack_bytes);

            if (preserve_edi)
                c.pop(edi);

            if (preserve_esi)
                c.pop(esi);

            if (preserve_ebx)
                c.pop(ebx);

            if (desc.return_where == loc::xmm0)
                emit_bridge_xmm0_to_st0(c, desc.return_bytes);

            c.mov(esp, ebp);
            c.pop(ebp);
            c.ret();
        }

        void emit_callback_body(
            Xbyak::CodeGenerator& c,
            const abi_desc& desc,
            std::uintptr_t callback_addr
        )
        {
            using namespace Xbyak::util;

            const auto incoming_offsets = incoming_stack_arg_offsets(desc);

            c.push(ebp);
            c.mov(ebp, esp);

            c.sub(esp, 4);
            c.mov(dword[ebp - 4], static_cast<std::uint32_t>(callback_addr));

            for (std::size_t i = desc.args.size(); i > 0; --i)
                emit_push_callback_arg(c, desc, incoming_offsets, i - 1);

            c.call(dword[ebp - 4]);

            const std::uint32_t cb_bytes = callback_arg_bytes(desc);

            if (cb_bytes != 0)
                c.add(esp, cb_bytes);

            if (desc.return_where == loc::xmm0)
                emit_bridge_st0_to_xmm0(c, desc.return_bytes);

            c.mov(esp, ebp);
            c.pop(ebp);

            const std::uint32_t incoming_stack_bytes = pushed_stack_bytes(desc);

            if (desc.cleanup_mode == cleanup::callee && incoming_stack_bytes != 0)
                c.ret(incoming_stack_bytes);
            else
                c.ret();
        }

        void* finalize_code(Xbyak::CodeGenerator& code)
        {
            try
            {
                code.ready();
            }
            catch (const std::exception& e)
            {
                throw std::runtime_error(std::string("UserCaller: Xbyak finalize failed: ") + e.what());
            }

            const std::size_t bytes = code.getSize();
            const auto allocation = reserve_code(std::max<std::size_t>(bytes, 1));

            if (!allocation.page->make_writable())
                throw std::runtime_error("UserCaller: VirtualProtect failed making code page writable.");

            std::memcpy(allocation.ptr, code.getCode(), bytes);

            if (!allocation.page->make_executable())
                throw std::runtime_error("UserCaller: VirtualProtect failed making code page executable.");

            FlushInstructionCache(GetCurrentProcess(), allocation.ptr, bytes);
            return allocation.ptr;
        }

        void* build_code(const abi_desc& desc, std::uintptr_t baked_target)
        {
            Xbyak::CodeGenerator code(4096, Xbyak::AutoGrow);
            emit_thunk_body(code, desc, baked_target);
            return finalize_code(code);
        }

        void* build_callback_code(const abi_desc& desc, std::uintptr_t callback_addr)
        {
            Xbyak::CodeGenerator code(4096, Xbyak::AutoGrow);
            emit_callback_body(code, desc, callback_addr);
            return finalize_code(code);
        }
    }

    code_block::code_block(void* entry_) noexcept
        : entry(entry_)
    {
    }

    code_block::~code_block() noexcept
    {
        if (!entry)
            return;

        entry = nullptr;
    }

    std::shared_ptr<code_block> build_baked(const abi_desc& desc, std::uintptr_t target)
    {
        if (desc.runtime_target)
            throw std::runtime_error("UserCaller: build_baked received runtime-target descriptor.");

        return std::make_shared<code_block>(build_code(desc, target));
    }

    std::shared_ptr<code_block> build_callback(const abi_desc& desc, std::uintptr_t callback)
    {
        if (desc.runtime_target)
            throw std::runtime_error("UserCaller: build_callback received runtime-target descriptor.");

        return std::make_shared<code_block>(build_callback_code(desc, callback));
    }

    std::shared_ptr<code_block> build_invoker(const abi_desc& desc)
    {
        if (!desc.runtime_target)
            throw std::runtime_error("UserCaller: build_invoker received baked-target descriptor.");

        return std::make_shared<code_block>(build_code(desc, 0));
    }
}

namespace uc
{
    void patch_call(void* call_site, void* new_target)
    {
        auto* p = static_cast<std::uint8_t*>(call_site);

        if (p[0] != 0xE8)
            throw std::runtime_error("UserCaller: patch_call expected E8 call instruction.");

        DWORD old_protect{};
        if (!VirtualProtect(p, 5, PAGE_EXECUTE_READWRITE, &old_protect))
            throw std::runtime_error("UserCaller: VirtualProtect failed in patch_call.");

        const auto rel =
            reinterpret_cast<std::intptr_t>(new_target) -
            reinterpret_cast<std::intptr_t>(p + 5);

        p[0] = 0xE8;
        *reinterpret_cast<std::int32_t*>(p + 1) = static_cast<std::int32_t>(rel);

        DWORD temp{};
        VirtualProtect(p, 5, old_protect, &temp);
        FlushInstructionCache(GetCurrentProcess(), p, 5);
    }
}
