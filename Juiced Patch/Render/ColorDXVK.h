#pragma once
#include <d3d9.h>
// Public DXVK D3D9 COM ABI, from src/d3d9/d3d9_interfaces.h (zlib license).
// These declarations only use the HDR subset; no Vulkan loader or helper process.
namespace ColorPipeline::DXVK {
constexpr int SDR = 0;
constexpr int ScRGB = 1000104002;
struct XY { float x, y; };
struct Metadata {
    int type = 1000105000; const void* next = nullptr;
    XY red{.64f,.33f}, green{.30f,.60f}, blue{.15f,.06f}, white{.3127f,.3290f};
    float maxLuminance=1000, minLuminance=0, maxContentLightLevel=1000, maxFrameAverageLightLevel=200;
};
struct OutputMetadata { float red[2],green[2],blue[2],white[2],minimum,maximum,fullFrame; };
MIDL_INTERFACE("13776e93-4aa9-430a-a4ec-fe9e281181d5") Swapchain : IUnknown {
    virtual BOOL STDMETHODCALLTYPE CheckColorSpaceSupport(int) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetColorSpace(int) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetHDRMetaData(const Metadata*) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetCurrentOutputDesc(OutputMetadata*) = 0;
    virtual void STDMETHODCALLTYPE UnlockAdditionalFormats() = 0;
};
MIDL_INTERFACE("65b55086-e3e3-4c3e-b3a0-86815cce2c4c") Interface : IUnknown {
    virtual void STDMETHODCALLTYPE UnlockAdditionalFormats() = 0;
};
}
