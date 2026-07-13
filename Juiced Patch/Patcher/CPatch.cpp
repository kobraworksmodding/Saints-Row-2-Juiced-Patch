#include "CPatch.h"
#include <windows.h>
#include <cstring>

CPatch::CPatch(std::uintptr_t address, const std::vector<uint8_t>& patchBytes)
    : m_address(address),
    m_patchBytes(patchBytes),
    m_isApplied(false)
{
    m_originalBytes.resize(m_patchBytes.size());


    DWORD oldProtect = 0;
    VirtualProtect(reinterpret_cast<void*>(m_address), m_patchBytes.size(), PAGE_EXECUTE_READWRITE, &oldProtect);


    std::memcpy(m_originalBytes.data(), reinterpret_cast<void*>(m_address), m_patchBytes.size());

    VirtualProtect(reinterpret_cast<void*>(m_address), m_patchBytes.size(), oldProtect, &oldProtect);
}

CPatch::~CPatch()
{
    if (m_isApplied) {
        Restore();
    }
}

void CPatch::Apply()
{
    if (m_isApplied) return;

    DWORD oldProtect = 0;
    VirtualProtect(reinterpret_cast<void*>(m_address), m_patchBytes.size(), PAGE_EXECUTE_READWRITE, &oldProtect);

    std::memcpy(reinterpret_cast<void*>(m_address), m_patchBytes.data(), m_patchBytes.size());

    VirtualProtect(reinterpret_cast<void*>(m_address), m_patchBytes.size(), oldProtect, &oldProtect);
    m_isApplied = true;
}

void CPatch::Restore()
{
    if (!m_isApplied) return;

    DWORD oldProtect = 0;
    VirtualProtect(reinterpret_cast<void*>(m_address), m_originalBytes.size(), PAGE_EXECUTE_READWRITE, &oldProtect);

    // Restore original bytes
    std::memcpy(reinterpret_cast<void*>(m_address), m_originalBytes.data(), m_originalBytes.size());

    VirtualProtect(reinterpret_cast<void*>(m_address), m_originalBytes.size(), oldProtect, &oldProtect);
    m_isApplied = false;
}



CPatch CPatch::SafeWrite8(std::uintptr_t addr, uint8_t data)
{
    std::vector<uint8_t> patchBytes = { data };
    return CPatch(addr, patchBytes);
}

CPatch CPatch::SafeWrite16(std::uintptr_t addr, uint16_t data)
{

    std::vector<uint8_t> patchBytes = {
        static_cast<uint8_t>(data & 0xFF),
        static_cast<uint8_t>((data >> 8) & 0xFF)
    };
    return CPatch(addr, patchBytes);
}

CPatch CPatch::SafeWrite32(std::uintptr_t addr, uint32_t data)
{
    std::vector<uint8_t> patchBytes = {
        static_cast<uint8_t>(data & 0xFF),
        static_cast<uint8_t>((data >> 8) & 0xFF),
        static_cast<uint8_t>((data >> 16) & 0xFF),
        static_cast<uint8_t>((data >> 24) & 0xFF)
    };
    return CPatch(addr, patchBytes);
}

CPatch CPatch::SafeWriteBuf(std::uintptr_t addr, const void* data, size_t len)
{
    std::vector<uint8_t> patchBytes(len);
    std::memcpy(patchBytes.data(), data, len);
    return CPatch(addr, patchBytes);
}

CPatch CPatch::WriteRelJump(std::uintptr_t jumpSrc, std::uintptr_t jumpTgt)
{
    // For a JMP rel32, opcode = 0xE9
    //  E9 [disp32]
    // disp32 = jumpTgt - (jumpSrc + 5)
    return CPatch(jumpSrc, BuildRel32Patch(0xE9, jumpSrc, jumpTgt));
}

CPatch CPatch::WriteRelCall(std::uintptr_t callSrc, std::uintptr_t callTgt)
{
    // For a CALL rel32, opcode = 0xE8
    //  E8 [disp32]
    return CPatch(callSrc, BuildRel32Patch(0xE8, callSrc, callTgt));
}

CPatch CPatch::WriteRelJnz(std::uintptr_t jumpSrc, std::uintptr_t jumpTgt)
{
    // jnz rel32 has opcodes 0x0F, 0x85, then disp32
    //   0F 85 [disp32]
    return CPatch(jumpSrc, BuildRel32Patch(0x0F, 0x85, jumpSrc, jumpTgt));
}

CPatch CPatch::WriteRelJle(std::uintptr_t jumpSrc, std::uintptr_t jumpTgt)
{
    // jle rel32 has opcodes 0x0F, 0x8E, then disp32
    //   0F 8E [disp32]
    return CPatch(jumpSrc, BuildRel32Patch(0x0F, 0x8E, jumpSrc, jumpTgt));
}

//--------------------------------------------------------------------------------
// Helpers
//--------------------------------------------------------------------------------

// Single-byte opcode version (JMP or CALL)
std::vector<uint8_t> CPatch::BuildRel32Patch(uint8_t opcode, std::uintptr_t src, std::uintptr_t tgt)
{
    // For JMP or CALL, we have a 1-byte opcode followed by a 4-byte disp
    // disp = target - (src + 5)
    std::int32_t disp = static_cast<std::int32_t>(tgt - (src + 5));

    std::vector<uint8_t> patch;
    patch.reserve(5);
    patch.push_back(opcode);

    patch.push_back(static_cast<uint8_t>(disp & 0xFF));
    patch.push_back(static_cast<uint8_t>((disp >> 8) & 0xFF));
    patch.push_back(static_cast<uint8_t>((disp >> 16) & 0xFF));
    patch.push_back(static_cast<uint8_t>((disp >> 24) & 0xFF));

    return patch;
}

// Two-byte opcode version (e.g. 0x0F, 0x85 for jnz, 0x0F, 0x8E for jle)
std::vector<uint8_t> CPatch::BuildRel32Patch(uint8_t opcode1, uint8_t opcode2, std::uintptr_t src, std::uintptr_t tgt)
{
    // For jnz/jle, we have 2 bytes of opcode, then 4 bytes disp
    // length is 6 in this case
    std::int32_t disp = static_cast<std::int32_t>(tgt - (src + 6));

    std::vector<uint8_t> patch;
    patch.reserve(6);
    patch.push_back(opcode1);
    patch.push_back(opcode2);
    patch.push_back(static_cast<uint8_t>(disp & 0xFF));
    patch.push_back(static_cast<uint8_t>((disp >> 8) & 0xFF));
    patch.push_back(static_cast<uint8_t>((disp >> 16) & 0xFF));
    patch.push_back(static_cast<uint8_t>((disp >> 24) & 0xFF));

    return patch;
}

CPatch CPatch::PatchNop(std::uintptr_t addr, size_t size)
{

    std::vector<uint8_t> patchBytes(size, 0x90);

    return CPatch(addr, patchBytes);
}
