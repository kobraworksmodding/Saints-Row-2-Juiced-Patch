#pragma once

#include <cstdint>
#include <vector>

class CPatch
{
public:
    // Constructor: Takes an address and the new bytes to patch in. 
    // Automatically reads/stores the original bytes (same size as patchBytes).
    CPatch(std::uintptr_t address, const std::vector<uint8_t>& patchBytes);

    // Destructor: Optionally restore if still applied (comment out if undesired).
    ~CPatch();

    // Apply (write) the patch bytes to the target address
    void Apply();

    // Restore the original bytes to the target address
    void Restore();

    // Check if the patch is currently applied
    bool IsApplied() const { return m_isApplied; }


    // SafeWrite8
    static CPatch SafeWrite8(std::uintptr_t addr, uint8_t data);

    // SafeWrite16
    static CPatch SafeWrite16(std::uintptr_t addr, uint16_t data);

    // SafeWrite32
    static CPatch SafeWrite32(std::uintptr_t addr, uint32_t data);

    // SafeWriteBuf
    // Creates a patch object that will write 'len' bytes from data -> addr
    static CPatch SafeWriteBuf(std::uintptr_t addr, const void* data, size_t len);

    // WriteRelJump (jmp rel32)
    static CPatch WriteRelJump(std::uintptr_t jumpSrc, std::uintptr_t jumpTgt);

    // WriteRelCall (call rel32)
    static CPatch WriteRelCall(std::uintptr_t callSrc, std::uintptr_t callTgt);

    // WriteRelJnz (jnz rel32)
    static CPatch WriteRelJnz(std::uintptr_t jumpSrc, std::uintptr_t jumpTgt);

    // WriteRelJle (jle rel32)
    static CPatch WriteRelJle(std::uintptr_t jumpSrc, std::uintptr_t jumpTgt);

    static CPatch PatchNop(std::uintptr_t addr, size_t size);

private:
    std::uintptr_t         m_address;
    std::vector<uint8_t>   m_originalBytes;
    std::vector<uint8_t>   m_patchBytes;
    bool                   m_isApplied;

    // Helper: build rel32 bytes for an x86 JMP, CALL, or conditional jump
    static std::vector<uint8_t> BuildRel32Patch(uint8_t opcode, std::uintptr_t src, std::uintptr_t tgt);
    static std::vector<uint8_t> BuildRel32Patch(uint8_t opcode1, uint8_t opcode2, std::uintptr_t src, std::uintptr_t tgt);
};
