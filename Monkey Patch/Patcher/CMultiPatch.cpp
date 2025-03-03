#include "CMultiPatch.h"
#include <windows.h>
#include <cstring> // for std::memcpy

CMultiPatch::CMultiPatch()
    : m_isApplied(false)
{
}

// This constructor takes an initializer_list of lambdas. Each lambda receives
// "CMultiPatch&" so it can call AddSafeWriteXX, etc. We just run them all.
CMultiPatch::CMultiPatch(std::initializer_list<std::function<void(CMultiPatch&)>> init)
    : m_isApplied(false)
{
    for (auto& fn : init)
    {
        fn(*this); // Let each lambda define patches.
    }
}

CMultiPatch::~CMultiPatch()
{
    // If still applied, restore automatically.
    if (m_isApplied) {
        Restore();
    }
}

//------------------------------------
// Private AddPatch
//------------------------------------
void CMultiPatch::AddPatch(std::uintptr_t address, const std::vector<uint8_t>& patchBytes)
{
    PatchEntry entry;
    entry.address = address;
    entry.patchBytes = patchBytes;
    entry.originalBytes.resize(patchBytes.size());

    DWORD oldProtect = 0;
    VirtualProtect(reinterpret_cast<void*>(address), patchBytes.size(), PAGE_EXECUTE_READWRITE, &oldProtect);

    // store original
    std::memcpy(entry.originalBytes.data(), reinterpret_cast<void*>(address), patchBytes.size());

    VirtualProtect(reinterpret_cast<void*>(address), patchBytes.size(), oldProtect, &oldProtect);

    m_patches.push_back(std::move(entry));
}

//------------------------------------
// "Add..." methods replicate CPatch
//------------------------------------
void CMultiPatch::AddSafeWrite8(std::uintptr_t addr, uint8_t value)
{
    std::vector<uint8_t> bytes = { value };
    AddPatch(addr, bytes);
}

void CMultiPatch::AddSafeWrite16(std::uintptr_t addr, uint16_t value)
{
    std::vector<uint8_t> bytes;
    bytes.push_back(static_cast<uint8_t>(value & 0xFF));
    bytes.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
    AddPatch(addr, bytes);
}

void CMultiPatch::AddSafeWrite32(std::uintptr_t addr, uint32_t value)
{
    std::vector<uint8_t> bytes;
    bytes.push_back(static_cast<uint8_t>(value & 0xFF));
    bytes.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
    bytes.push_back(static_cast<uint8_t>((value >> 16) & 0xFF));
    bytes.push_back(static_cast<uint8_t>((value >> 24) & 0xFF));
    AddPatch(addr, bytes);
}

void CMultiPatch::AddSafeWriteBuf(std::uintptr_t addr, const void* data, size_t len)
{
    std::vector<uint8_t> bytes(len);
    std::memcpy(bytes.data(), data, len);
    AddPatch(addr, bytes);
}

void CMultiPatch::AddWriteRelJump(std::uintptr_t jumpSrc, std::uintptr_t jumpTgt)
{
    auto bytes = BuildRel32Patch(0xE9, jumpSrc, jumpTgt);
    AddPatch(jumpSrc, bytes);
}

void CMultiPatch::AddWriteRelCall(std::uintptr_t callSrc, std::uintptr_t callTgt)
{
    auto bytes = BuildRel32Patch(0xE8, callSrc, callTgt);
    AddPatch(callSrc, bytes);
}

void CMultiPatch::AddWriteRelJnz(std::uintptr_t jumpSrc, std::uintptr_t jumpTgt)
{
    auto bytes = BuildRel32Patch(0x0F, 0x85, jumpSrc, jumpTgt);
    AddPatch(jumpSrc, bytes);
}

void CMultiPatch::AddWriteRelJle(std::uintptr_t jumpSrc, std::uintptr_t jumpTgt)
{
    auto bytes = BuildRel32Patch(0x0F, 0x8E, jumpSrc, jumpTgt);
    AddPatch(jumpSrc, bytes);
}

void CMultiPatch::AddPatchNop(std::uintptr_t addr, size_t size)
{
    std::vector<uint8_t> bytes(size, 0x90);
    AddPatch(addr, bytes);
}

//------------------------------------
// Apply / Restore all
//------------------------------------
void CMultiPatch::Apply()
{
    if (m_isApplied) return;

    for (auto& patch : m_patches)
    {
        DWORD oldProtect = 0;
        VirtualProtect(reinterpret_cast<void*>(patch.address), patch.patchBytes.size(), PAGE_EXECUTE_READWRITE, &oldProtect);

        std::memcpy(reinterpret_cast<void*>(patch.address), patch.patchBytes.data(), patch.patchBytes.size());

        VirtualProtect(reinterpret_cast<void*>(patch.address), patch.patchBytes.size(), oldProtect, &oldProtect);
    }

    m_isApplied = true;
}

void CMultiPatch::Restore()
{
    if (!m_isApplied) return;

    for (auto& patch : m_patches)
    {
        DWORD oldProtect = 0;
        VirtualProtect(reinterpret_cast<void*>(patch.address), patch.originalBytes.size(), PAGE_EXECUTE_READWRITE, &oldProtect);

        std::memcpy(reinterpret_cast<void*>(patch.address), patch.originalBytes.data(), patch.originalBytes.size());

        VirtualProtect(reinterpret_cast<void*>(patch.address), patch.originalBytes.size(), oldProtect, &oldProtect);
    }

    m_isApplied = false;
}

//------------------------------------
// Helpers for rel32 instructions
//------------------------------------
std::vector<uint8_t> CMultiPatch::BuildRel32Patch(uint8_t opcode, std::uintptr_t src, std::uintptr_t tgt)
{
    // JMP or CALL: 5 bytes total
    std::int32_t disp = static_cast<std::int32_t>(tgt - (src + 5));
    std::vector<uint8_t> bytes;
    bytes.reserve(5);
    bytes.push_back(opcode);
    bytes.push_back(static_cast<uint8_t>(disp & 0xFF));
    bytes.push_back(static_cast<uint8_t>((disp >> 8) & 0xFF));
    bytes.push_back(static_cast<uint8_t>((disp >> 16) & 0xFF));
    bytes.push_back(static_cast<uint8_t>((disp >> 24) & 0xFF));
    return bytes;
}

std::vector<uint8_t> CMultiPatch::BuildRel32Patch(uint8_t opcode1, uint8_t opcode2, std::uintptr_t src, std::uintptr_t tgt)
{
    // JNZ / JLE: 6 bytes total
    std::int32_t disp = static_cast<std::int32_t>(tgt - (src + 6));
    std::vector<uint8_t> bytes;
    bytes.reserve(6);
    bytes.push_back(opcode1);
    bytes.push_back(opcode2);
    bytes.push_back(static_cast<uint8_t>(disp & 0xFF));
    bytes.push_back(static_cast<uint8_t>((disp >> 8) & 0xFF));
    bytes.push_back(static_cast<uint8_t>((disp >> 16) & 0xFF));
    bytes.push_back(static_cast<uint8_t>((disp >> 24) & 0xFF));
    return bytes;
}
