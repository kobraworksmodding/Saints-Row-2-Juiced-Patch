#include "patch.h"
#include <stdint.h>
#include <windows.h>
#include <Zydis.h>

void NopInstruction(void* address) {
    ZydisDecoder decoder;
    ZydisDecoderInit(&decoder, ZYDIS_MACHINE_MODE_LONG_64, ZYDIS_STACK_WIDTH_64);

    ZydisDecodedInstruction instruction;
    ZydisDecodedOperand operands[ZYDIS_MAX_OPERAND_COUNT];

    if (ZYAN_SUCCESS(ZydisDecoderDecodeFull(&decoder, address, 15,
        &instruction, operands))) {
        DWORD oldProtect;
        VirtualProtect(address, instruction.length, PAGE_EXECUTE_READWRITE, &oldProtect);
        memset(address, 0x90, instruction.length);
        VirtualProtect(address, instruction.length, oldProtect, &oldProtect);
    }
}

void patchNop(void* addr, size_t size) {

    if (size == 0) {
        NopInstruction(addr);
        return;
    }

    DWORD oldProtect{};

    VirtualProtect(addr, size, PAGE_EXECUTE_READWRITE, &oldProtect);
    memset(addr, NOP, size);
    VirtualProtect(addr, size, oldProtect, &oldProtect);
}

void patchInst(void* addr, enum instruction inst) {
    DWORD oldProtect;

    VirtualProtect(addr, 1, PAGE_EXECUTE_READWRITE, &oldProtect);
    *(uint8_t*)addr = inst;
    VirtualProtect(addr, 1, oldProtect, &oldProtect);
}

void patchByte(void* addr, uint8_t val) {
    DWORD oldProtect;

    VirtualProtect(addr, 1, PAGE_EXECUTE_READWRITE, &oldProtect);
    *(uint8_t*)addr = val;
    VirtualProtect(addr, 1, oldProtect, &oldProtect);
}

void patchBytesM(BYTE* addr, BYTE* val, unsigned int size)
{
    DWORD oldprotect;

    VirtualProtect(addr, size, PAGE_EXECUTE_READWRITE, &oldprotect);
    memcpy(addr, val, size);
    VirtualProtect(addr, size, oldprotect, &oldprotect);
}

void patchDWord(void* addr, uint32_t val) {
    DWORD oldProtect;

    VirtualProtect(addr, 1, PAGE_EXECUTE_READWRITE, &oldProtect);
    *(uint32_t*)addr = val;
    VirtualProtect(addr, 1, oldProtect, &oldProtect);
}

void patchuint8_t(void* addr, uint8_t val) {
    DWORD oldProtect;

    VirtualProtect(addr, 1, PAGE_EXECUTE_READWRITE, &oldProtect);
    *(uint8_t*)addr = val;
    VirtualProtect(addr, 1, oldProtect, &oldProtect);
}

void patchInt(void* addr, int val) {
    DWORD oldProtect;

    VirtualProtect(addr, 1, PAGE_EXECUTE_READWRITE, &oldProtect);
    *(int*)addr = val;
    VirtualProtect(addr, 1, oldProtect, &oldProtect);
}


void patchFloat(void* addr, float val) {
    DWORD oldProtect;

    VirtualProtect(addr, 1, PAGE_EXECUTE_READWRITE, &oldProtect);
    *(float*)addr = val;
    VirtualProtect(addr, 1, oldProtect, &oldProtect);
}

void patchDouble(void* addr, double val) {
    DWORD oldProtect;

    VirtualProtect(addr, 1, PAGE_EXECUTE_READWRITE, &oldProtect);
    *(double*)addr = val;
    VirtualProtect(addr, 1, oldProtect, &oldProtect);

}

void patchCall(void* addr, void* func) {
    DWORD oldProtect;

    VirtualProtect(addr, 5, PAGE_EXECUTE_READWRITE, &oldProtect);
    *(uint8_t*)addr = CALL;
    *(uint32_t*)((uint8_t*)addr + 1) = (uint32_t)func - (uint32_t)addr - 5;
    VirtualProtect(addr, 5, oldProtect, &oldProtect);
}

void patchJmp(void* addr, void* func) {
    DWORD oldProtect;

    VirtualProtect(addr, 5, PAGE_EXECUTE_READWRITE, &oldProtect);
    *(uint8_t*)addr = JMP9;
    *(uint32_t*)((uint8_t*)addr + 1) = (uint32_t)func - (uint32_t)addr - 5;
    VirtualProtect(addr, 5, oldProtect, &oldProtect);
}

void patchThisToCdecl(void* addr, void* func) {
    // convenience function that takes 'this' and calls a c function with a pointer to it - results in 7 bytes written
    DWORD oldProtect;

    VirtualProtect(addr, 7, PAGE_EXECUTE_READWRITE, &oldProtect);

    // PUSH ECX (0x51)
    *((uint8_t*)addr) = 0x51;

    // CALL (func)
    *((uint8_t*)addr + 1) = CALL;
    *(uint32_t*)((uint8_t*)addr + 2) = (uint32_t)func - (uint32_t)((uint8_t*)addr + 1) - 5;

    // POP ECX (0x59)
    *((uint8_t*)addr + 6) = 0x59;

    VirtualProtect(addr, 7, oldProtect, &oldProtect);
}
void callFunc(void* addr) {
    // calls function with the assumption of it returning and accepting void
   // void (*fp)() = (*fp)(addr) addr;
   // fp();
}

void* copyFunc(uint32_t func_start, uint32_t func_end, void* new_func)
{
    uint32_t func_size = func_end - func_start;

    void* copied = (void*)calloc(func_size, 1);
    memcpy(copied, (void*)func_start, func_size);

    patchJmp((void*)func_start, new_func);
    return copied;
}

bool patch_lea_to_mov_ptr(uintptr_t address, uintptr_t static_ptr)
{
    ZydisDecoder            decoder;
    ZydisDecodedInstruction insn;
    ZydisDecodedOperand     ops[ZYDIS_MAX_OPERAND_COUNT];

    ZydisDecoderInit(&decoder, ZYDIS_MACHINE_MODE_LEGACY_32, ZYDIS_STACK_WIDTH_32);

    uint8_t* cursor = (uint8_t*)address;

    if (!ZYAN_SUCCESS(ZydisDecoderDecodeFull(&decoder, cursor, 15, &insn, ops)))
        return false;

    if (insn.mnemonic != ZYDIS_MNEMONIC_LEA
        || insn.operand_count < 2
        || ops[0].type != ZYDIS_OPERAND_TYPE_REGISTER
        || ops[1].type != ZYDIS_OPERAND_TYPE_MEMORY
        || ops[1].mem.base != ZYDIS_REGISTER_ESP
        || ops[1].mem.index != ZYDIS_REGISTER_NONE)
        return false;

    if (insn.length < 5)
        return false;

    DWORD old_prot;
    VirtualProtect(cursor, insn.length, PAGE_EXECUTE_READWRITE, &old_prot);

    memset(cursor, 0x90, insn.length);

    auto reg_to_rd = [](ZydisRegister reg) -> uint8_t {
        switch (reg) {
        case ZYDIS_REGISTER_EAX: return 0;
        case ZYDIS_REGISTER_ECX: return 1;
        case ZYDIS_REGISTER_EDX: return 2;
        case ZYDIS_REGISTER_EBX: return 3;
        case ZYDIS_REGISTER_ESP: return 4;
        case ZYDIS_REGISTER_EBP: return 5;
        case ZYDIS_REGISTER_ESI: return 6;
        case ZYDIS_REGISTER_EDI: return 7;
        default:                 return 0xFF;
        }
        };

    uint8_t rd = reg_to_rd(ops[0].reg.value);
    cursor[0] = 0xB8 | rd;
    memcpy(cursor + 1, &static_ptr, 4);

    VirtualProtect(cursor, insn.length, old_prot, &old_prot);

    return true;
}
