#include "patch.h"
#include <stdint.h>
#include <windows.h>
#include "../Hooker.h"

void patchNop(void* addr, size_t size) {
    addr = DynAddress(addr);
    DWORD oldProtect;

    VirtualProtect(addr, size, PAGE_EXECUTE_READWRITE, &oldProtect);
    memset(addr, NOP, size);
    VirtualProtect(addr, size, oldProtect, &oldProtect);
}

void patchInst(void* addr, enum instruction inst) {
    addr = DynAddress(addr);
    DWORD oldProtect;

    VirtualProtect(addr, 1, PAGE_EXECUTE_READWRITE, &oldProtect);
    *(uint8_t*)addr = inst;
    VirtualProtect(addr, 1, oldProtect, &oldProtect);
}

void patchByte(void* addr, uint8_t val) {
    addr = DynAddress(addr);
    DWORD oldProtect;

    VirtualProtect(addr, 1, PAGE_EXECUTE_READWRITE, &oldProtect);
    *(uint8_t*)addr = val;
    VirtualProtect(addr, 1, oldProtect, &oldProtect);
}

void patchBytesM(BYTE* addr, BYTE* val, unsigned int size)
{
    addr = DynAddress(addr);
    DWORD oldprotect;

    VirtualProtect(addr, size, PAGE_EXECUTE_READWRITE, &oldprotect);
    memcpy(addr, val, size);
    VirtualProtect(addr, size, oldprotect, &oldprotect);
}

void patchDWord(void* addr, uint32_t val) {
    addr = DynAddress(addr);
    DWORD oldProtect;

    VirtualProtect(addr, 1, PAGE_EXECUTE_READWRITE, &oldProtect);
    *(uint32_t*)addr = val;
    VirtualProtect(addr, 1, oldProtect, &oldProtect);
}

void patchuint8_t(void* addr, uint8_t val) {
    addr = DynAddress(addr);
    DWORD oldProtect;

    VirtualProtect(addr, 1, PAGE_EXECUTE_READWRITE, &oldProtect);
    *(uint8_t*)addr = val;
    VirtualProtect(addr, 1, oldProtect, &oldProtect);
}

void patchInt(void* addr, int val) {
    addr = DynAddress(addr);
    DWORD oldProtect;

    VirtualProtect(addr, 1, PAGE_EXECUTE_READWRITE, &oldProtect);
    *(int*)addr = val;
    VirtualProtect(addr, 1, oldProtect, &oldProtect);
}


void patchFloat(void* addr, float val) {
    addr = DynAddress(addr);
    DWORD oldProtect;

    VirtualProtect(addr, 1, PAGE_EXECUTE_READWRITE, &oldProtect);
    *(float*)addr = val;
    VirtualProtect(addr, 1, oldProtect, &oldProtect);
}

void patchDouble(void* addr, double val) {
    addr = DynAddress(addr);
    DWORD oldProtect;

    VirtualProtect(addr, 1, PAGE_EXECUTE_READWRITE, &oldProtect);
    *(double*)addr = val;
    VirtualProtect(addr, 1, oldProtect, &oldProtect);

}

void patchCall(void* addr, void* func) {
    addr = DynAddress(addr);
    DWORD oldProtect;

    VirtualProtect(addr, 5, PAGE_EXECUTE_READWRITE, &oldProtect);
    *(uint8_t*)addr = CALL;
    *(uint32_t*)((uint8_t*)addr + 1) = (uint32_t)func - (uint32_t)addr - 5;
    VirtualProtect(addr, 5, oldProtect, &oldProtect);
}

void patchJmp(void* addr, void* func) {
    addr = DynAddress(addr);
    DWORD oldProtect;

    VirtualProtect(addr, 5, PAGE_EXECUTE_READWRITE, &oldProtect);
    *(uint8_t*)addr = JMP9;
    *(uint32_t*)((uint8_t*)addr + 1) = (uint32_t)func - (uint32_t)addr - 5;
    VirtualProtect(addr, 5, oldProtect, &oldProtect);
}

void patchThisToCdecl(void* addr, void* func) {
    addr = DynAddress(addr);
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
    addr = DynAddress(addr);
    // calls function with the assumption of it returning and accepting void
   // void (*fp)() = (*fp)(addr) addr;
   // fp();
}

void* copyFunc(uint32_t func_start, uint32_t func_end, void* new_func)
{
    func_start = DynAddress(func_start);
    func_end = DynAddress(func_end);
    uint32_t func_size = func_end - func_start;

    void* copied = (void*)calloc(func_size, 1);
    memcpy(copied, (void*)func_start, func_size);

    patchJmp((void*)func_start, new_func);
    return copied;
}

