#pragma once
#include <stdint.h>
#include <windows.h>

enum instruction {
    NOP = 0x90,
    MOVIMM8 = 0xB0,
    JMP8 = 0xEB, // jump with 8 bit relative address
    JMP9 = 0xE9,
    CALL = 0xE8 // NOTE: relative
};

void patchNop(void* addr, size_t size);
void patchInst(void* addr, enum instruction inst);
void patchByte(void* addr, uint8_t val);
void patchBytesM(BYTE* dst, BYTE* src, unsigned int size);
void patchInt(void* addr, int val);
void patchDouble(void* addr, double val);
void patchDWord(void* addr, uint32_t val);
void patchFloat(void* addr, float val);
void patchCall(void* addr, void* func);
void patchJmp(void* addr, void* func);
void patchThisToCdecl(void* addr, void* func);
void callFunc(void* addr);

void* copyFunc(uint32_t func_start, uint32_t func_end, void* new_func);

void* copyFunc(uint32_t func_start, uint32_t func_end, void* new_func);
