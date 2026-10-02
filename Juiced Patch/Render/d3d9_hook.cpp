#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <d3d9.h>
#include <safetyhook.hpp>
#include "../FileLogger.h"
#include "d3d9_hook.h"

typedef IDirect3D9* (WINAPI* Direct3DCreate9_t)(UINT SDKVersion);
SafetyHookInline g_create_device_hook;

HRESULT STDMETHODCALLTYPE hooked_CreateDevice(
    IDirect3D9* pThis,
    UINT Adapter,
    D3DDEVTYPE DeviceType,
    HWND hFocusWindow,
    DWORD BehaviorFlags,
    D3DPRESENT_PARAMETERS* pPresentationParameters,
    IDirect3DDevice9** ppReturnedDeviceInterface
) {

    HRESULT result = g_create_device_hook.unsafe_stdcall<HRESULT>(
        pThis, Adapter, DeviceType, hFocusWindow,
        BehaviorFlags, pPresentationParameters, ppReturnedDeviceInterface
    );
    if (SUCCEEDED(result)) {
        Logger::TypedLog("D3D9", "CreateDevice succeeded - Device created\n");
    }
    if (!SUCCEEDED(result) || !ppReturnedDeviceInterface || !*ppReturnedDeviceInterface) {
        Logger::TypedLog("D3D9", "CreateDevice Failed, Force Windowed mode!\n");
        pPresentationParameters->Windowed = true;
        result = g_create_device_hook.unsafe_stdcall<HRESULT>(
            pThis, Adapter, DeviceType, hFocusWindow,
            BehaviorFlags, pPresentationParameters, ppReturnedDeviceInterface
        );
    }
    return result;
}

bool D3D9Hook::initialize() {
    HMODULE hD3D9 = GetModuleHandleW(L"d3d9.dll");
    if (!hD3D9) {
        hD3D9 = LoadLibraryW(L"d3d9.dll");
        if (!hD3D9) {
            return false;
        }
    }
    Direct3DCreate9_t pDirect3DCreate9 = reinterpret_cast<Direct3DCreate9_t>(
        GetProcAddress(hD3D9, "Direct3DCreate9")
        );
    if (!pDirect3DCreate9) {
        return false;
    }
    IDirect3D9* pD3D = pDirect3DCreate9(0x20);
    if (!pD3D) {
        return false;
    }
    void** vtable = *reinterpret_cast<void***>(pD3D);
    void* createDeviceAddr = vtable[16];
    pD3D->Release();
    g_create_device_hook = safetyhook::create_inline(createDeviceAddr, hooked_CreateDevice);
    return g_create_device_hook.enabled();
}
