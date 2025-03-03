#pragma once

#include <Windows.h>
#ifdef __cplusplus
#include <functional>
#endif
extern "C" {
#ifdef __cplusplus
    typedef void (*BlingMenuAddFuncStd_TYPE)(const char* path, const char* name,
        std::function<void()> func);
#endif
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

    typedef void (*BlingMenuAddFuncCustom_TYPE)(
        const char* path,
        const char* name,
        void* userData,
        const char* (*customFunc)(void* userData, int action),
        void (*triggerFunc)()
        );
    typedef void (*BlingMenuAddDouble_TYPE)(const char* path, const char* name, double* ptr,
        void (*triggerFunc)(), double step,
        double lowerBound, double upperBound);

    typedef void (*BlingMenuAddInt64_TYPE)(const char* path, const char* name, __int64* ptr,
        void (*triggerFunc)(), __int64 step,
        __int64 lowerBound, __int64 upperBound);

    struct BlingMenuAPI
    {
        bool isLoaded;
        HMODULE module;
#ifdef __cplusplus
        BlingMenuAddFuncStd_TYPE AddFuncStd;
#endif
        BlingMenuAddInt8_TYPE AddInt8;
        BlingMenuAddBool_TYPE AddBool;
        BlingMenuAddFloat_TYPE AddFloat;
        BlingMenuAddInt_TYPE AddInt;
        BlingMenuAddFunc_TYPE AddFunc;
        BlingMenuAddFuncRaw_TYPE AddFuncRaw;
        BlingMenuAddFuncCustom_TYPE AddFuncCustom;
        BlingMenuAddDouble_TYPE AddDouble;
        BlingMenuAddInt64_TYPE AddInt64;
    };


    inline BlingMenuAPI gBlingMenuAPI = { false };
#ifdef __cplusplus
    inline void BlingMenuAddFuncStd(const char* path, const char* name,
        std::function<void()> func)
    {
        if (gBlingMenuAPI.isLoaded && gBlingMenuAPI.AddFuncStd)
            gBlingMenuAPI.AddFuncStd(path, name, func);
    }
#endif
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

    inline void BlingMenuAddFuncCustom(
        const char* path,
        const char* name,
        void* userData,
        const char* (*customFunc)(void* userData, int action),
        void (*triggerFunc)()
    )
    {
        if (gBlingMenuAPI.isLoaded && gBlingMenuAPI.AddFuncCustom)
            gBlingMenuAPI.AddFuncCustom(path, name, userData, customFunc, triggerFunc);
    }
    inline void BlingMenuAddDouble(const char* path, const char* name, double* ptr,
        void (*triggerFunc)(), double step,
        double lowerBound, double upperBound)
    {
        if (gBlingMenuAPI.isLoaded && gBlingMenuAPI.AddDouble)
            gBlingMenuAPI.AddDouble(path, name, ptr, triggerFunc, step, lowerBound, upperBound);
    }

    inline void BlingMenuAddInt64(const char* path, const char* name, __int64* ptr,
        void (*triggerFunc)(), __int64 step,
        __int64 lowerBound, __int64 upperBound)
    {
        if (gBlingMenuAPI.isLoaded && gBlingMenuAPI.AddInt64)
            gBlingMenuAPI.AddInt64(path, name, ptr, triggerFunc, step, lowerBound, upperBound);
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
    gBlingMenuAPI.AddFuncCustom = (BlingMenuAddFuncCustom_TYPE)GetProcAddress(mod, "BlingMenuAddFuncCustom");
#ifdef __cplusplus
    gBlingMenuAPI.AddFuncStd = (BlingMenuAddFuncStd_TYPE)GetProcAddress(mod, "BlingMenuAddFuncStd");
#endif
    gBlingMenuAPI.AddDouble = (BlingMenuAddDouble_TYPE)GetProcAddress(mod, "BlingMenuAddDouble");
    gBlingMenuAPI.AddInt64 = (BlingMenuAddInt64_TYPE)GetProcAddress(mod, "BlingMenuAddInt64");
    if (!gBlingMenuAPI.AddInt8 || !gBlingMenuAPI.AddBool || !gBlingMenuAPI.AddFloat ||
        !gBlingMenuAPI.AddInt || !gBlingMenuAPI.AddFunc || !gBlingMenuAPI.AddFuncRaw) {
        FreeLibrary(mod);
        return false;
    }

    gBlingMenuAPI.isLoaded = true;
    gBlingMenuAPI.module = mod;
    return true;
}