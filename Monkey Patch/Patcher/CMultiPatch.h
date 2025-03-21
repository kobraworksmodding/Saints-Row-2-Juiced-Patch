#pragma once

#include <cstdint>
#include <vector>
#include <cstddef>           // for size_t
#include <functional>        // for std::function
#include <initializer_list>  // for std::initializer_list
#include <cstring>           // for memcpy

class CMultiPatch
{
public:
    // Default constructor (empty multi-patch)
    CMultiPatch();

    // This constructor takes an initializer list of functions (lambdas).
    // Each lambda receives a reference to this CMultiPatch and can call
    // the "Add..." methods to define a patch.
    CMultiPatch(std::initializer_list<std::function<void(CMultiPatch&)>> init);

    ~CMultiPatch();

    //--------------------------------------------------------------------------
    // The "Add..." methods are the multi-patch equivalents of CPatch::SafeWrite8,
    // CPatch::WriteRelJump, etc. Each call records a new patch (address + new bytes).
    //--------------------------------------------------------------------------
    void AddSafeWrite8(std::uintptr_t addr, uint8_t value);
    void AddSafeWrite16(std::uintptr_t addr, uint16_t value);
    void AddSafeWrite32(std::uintptr_t addr, uint32_t value);
    void AddSafeWriteBuf(std::uintptr_t addr, const void* data, size_t len);

    void AddWriteRelJump(std::uintptr_t jumpSrc, std::uintptr_t jumpTgt);
    void AddWriteRelCall(std::uintptr_t callSrc, std::uintptr_t callTgt);
    void AddWriteRelJnz(std::uintptr_t jumpSrc, std::uintptr_t jumpTgt);
    void AddWriteRelJle(std::uintptr_t jumpSrc, std::uintptr_t jumpTgt);

    // A multi-patch "NOP" for size bytes.
    void AddPatchNop(std::uintptr_t addr, size_t size);


    template <typename T>
    void AddSafeWrite(std::uintptr_t addr, const T& value);
    //--------------------------------------------------------------------------
    // Apply all recorded patches (writes the new bytes into memory).
    //--------------------------------------------------------------------------
    void Apply();

    //--------------------------------------------------------------------------
    // Restore all recorded patches (writes back the original bytes).
    //--------------------------------------------------------------------------
    void Restore();

    bool IsApplied() const { return m_isApplied; }

private:
    // Each patch we store has:
    //  - address
    //  - originalBytes
    //  - patchBytes
    struct PatchEntry
    {
        std::uintptr_t address;
        std::vector<uint8_t> originalBytes;
        std::vector<uint8_t> patchBytes;
    };

    std::vector<PatchEntry> m_patches;
    bool m_isApplied;

    // Low-level method that actually:
    //  1) reads original bytes
    //  2) saves them
    //  3) saves patchBytes
    void AddPatch(std::uintptr_t address, const std::vector<uint8_t>& patchBytes);

    // The same rel32-building helpers from CPatch
    static std::vector<uint8_t> BuildRel32Patch(uint8_t opcode, std::uintptr_t src, std::uintptr_t tgt);
    static std::vector<uint8_t> BuildRel32Patch(uint8_t opcode1, uint8_t opcode2, std::uintptr_t src, std::uintptr_t tgt);
};


template <typename T>
void CMultiPatch::AddSafeWrite(std::uintptr_t addr, const T& value)
{
    std::vector<uint8_t> bytes(sizeof(T));
    std::memcpy(bytes.data(), &value, sizeof(T));
    AddPatch(addr, bytes);
}
