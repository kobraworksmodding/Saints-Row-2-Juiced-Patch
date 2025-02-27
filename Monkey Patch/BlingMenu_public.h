#pragma once

#include <Windows.h>

extern "C" {

    typedef void (*BlingMenuAddInt8_TYPE)(const char* path, const char* name, signed char* ptr,
        void (*triggerFunc)(), signed char step,
        signed char lowerBound, signed char upperBound);

    typedef void (*BlingMenuAddBool_TYPE)(const char* path, const char* name, bool* ptr,
        void (*triggerFunc)());

    typedef void (*BlingMenuAddFloat_TYPE)(const char* path, const char* name, float* ptr,
        void (*triggerFunc)(), float step,
        float lowerBound, float upperBound);

    typedef void (*BlingMenuAddInt_TYPE)(const char* path, const char* name, int* ptr,
        void (*triggerFunc)(), int step,
        int lowerBound, int upperBound);

    typedef void (*BlingMenuAddFunc_TYPE)(const char* path, const char* name,
        void (*func)());

    typedef void (*BlingMenuAddFuncRaw_TYPE)(const char* path, const char* name,
        void* rawAddress);


    struct BlingMenuAPI
    {
        bool isLoaded;
        HMODULE module;
        BlingMenuAddInt8_TYPE AddInt8;
        BlingMenuAddBool_TYPE AddBool;
        BlingMenuAddFloat_TYPE AddFloat;
        BlingMenuAddInt_TYPE AddInt;
        BlingMenuAddFunc_TYPE AddFunc;
        BlingMenuAddFuncRaw_TYPE AddFuncRaw;
    };


    inline BlingMenuAPI gBlingMenuAPI = { false };


    inline void BlingMenuAddInt8(const char* path, const char* name, signed char* ptr,
        void (*triggerFunc)(), signed char step,
        signed char lowerBound, signed char upperBound)
    {
        if (gBlingMenuAPI.isLoaded && gBlingMenuAPI.AddInt8)
            gBlingMenuAPI.AddInt8(path, name, ptr, triggerFunc, step, lowerBound, upperBound);
    }

    inline void BlingMenuAddBool(const char* path, const char* name, bool* ptr,
        void (*triggerFunc)())
    {
        if (gBlingMenuAPI.isLoaded && gBlingMenuAPI.AddBool)
            gBlingMenuAPI.AddBool(path, name, ptr, triggerFunc);
    }

    inline void BlingMenuAddFloat(const char* path, const char* name, float* ptr,
        void (*triggerFunc)(), float step,
        float lowerBound, float upperBound)
    {
        if (gBlingMenuAPI.isLoaded && gBlingMenuAPI.AddFloat)
            gBlingMenuAPI.AddFloat(path, name, ptr, triggerFunc, step, lowerBound, upperBound);
    }

    inline void BlingMenuAddInt(const char* path, const char* name, int* ptr,
        void (*triggerFunc)(), int step,
        int lowerBound, int upperBound)
    {
        if (gBlingMenuAPI.isLoaded && gBlingMenuAPI.AddInt)
            gBlingMenuAPI.AddInt(path, name, ptr, triggerFunc, step, lowerBound, upperBound);
    }

    inline void BlingMenuAddFunc(const char* path, const char* name,
        void (*func)())
    {
        if (gBlingMenuAPI.isLoaded && gBlingMenuAPI.AddFunc)
            gBlingMenuAPI.AddFunc(path, name, func);
    }

    inline void BlingMenuAddFuncRaw(const char* path, const char* name,
        void* rawAddress)
    {
        if (gBlingMenuAPI.isLoaded && gBlingMenuAPI.AddFuncRaw)
            gBlingMenuAPI.AddFuncRaw(path, name, rawAddress);
    }
}


inline bool BlingMenuLoad(void)
{
    if (gBlingMenuAPI.isLoaded)
        return true;

    HMODULE mod = LoadLibraryA("BlingMenu.dll");
    if (!mod) {
        char modulePath[MAX_PATH];
        HMODULE dllModule;


        LPVOID funcAddr = (LPVOID)(&BlingMenuLoad);
        GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            (LPCSTR)funcAddr, &dllModule);

        GetModuleFileNameA(dllModule, modulePath, MAX_PATH);
        char* p = strrchr(modulePath, '\\');
        if (p) p[1] = '\0';
        strcat_s(modulePath, MAX_PATH, "BlingMenu.dll");
        mod = LoadLibraryA(modulePath);
    }

    if (!mod)
        return false;


    gBlingMenuAPI.AddInt8 = (BlingMenuAddInt8_TYPE)GetProcAddress(mod, "BlingMenuAddInt8");
    gBlingMenuAPI.AddBool = (BlingMenuAddBool_TYPE)GetProcAddress(mod, "BlingMenuAddBool");
    gBlingMenuAPI.AddFloat = (BlingMenuAddFloat_TYPE)GetProcAddress(mod, "BlingMenuAddFloat");
    gBlingMenuAPI.AddInt = (BlingMenuAddInt_TYPE)GetProcAddress(mod, "BlingMenuAddInt");
    gBlingMenuAPI.AddFunc = (BlingMenuAddFunc_TYPE)GetProcAddress(mod, "BlingMenuAddFunc");
    gBlingMenuAPI.AddFuncRaw = (BlingMenuAddFuncRaw_TYPE)GetProcAddress(mod, "BlingMenuAddFuncRaw");


    if (!gBlingMenuAPI.AddInt8 || !gBlingMenuAPI.AddBool || !gBlingMenuAPI.AddFloat ||
        !gBlingMenuAPI.AddInt || !gBlingMenuAPI.AddFunc || !gBlingMenuAPI.AddFuncRaw) {
        FreeLibrary(mod);
        return false;
    }

    gBlingMenuAPI.isLoaded = true;
    gBlingMenuAPI.module = mod;
    return true;
}