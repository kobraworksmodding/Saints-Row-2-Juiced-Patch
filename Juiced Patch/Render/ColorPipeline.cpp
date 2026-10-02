#define NOMINMAX
#include "ColorPipeline.h"
#if JUICED_FP16
#include "ColorDXVK.h"
#include "ColorShaders.h"
#include "../Hooker.h"
#include "../GameConfig.h"
#include "../BlingMenu_public.h"
#include "../FileLogger.h"
#include <windows.h>
#include <safetyhook.hpp>
#include <wrl/client.h>
#include <array>
#include <atomic>
#include <algorithm>
#include <cstring>
#include <mutex>
#include <vector>
namespace ColorPipeline {
namespace {
using Microsoft::WRL::ComPtr;
struct Settings { bool precision=false,hdr=false,dither=true,debug=false; float exposure=0,paper=200,peak=1000,shoulder=.75f,rolloff=.8f; };
Settings UI,Published,Frame;
std::mutex SettingsMutex;
bool Loaded=false,HooksReady=false,Enabled=false,Unlocked=false,OutputHDR=false;
IDirect3DDevice9* MainDevice=nullptr;
D3DFORMAT OriginalFormat=D3DFMT_UNKNOWN;
std::atomic<int> ActivePrecision{0},ActiveHDR{0},ScenePasses{0},Failures{0};
std::vector<SafetyHookMid> Hooks;
ComPtr<IDirect3DTexture9> Copy;
ComPtr<IDirect3DSurface9> CopySurface;
ComPtr<IDirect3DPixelShader9> TonemapPS,OutputPS;
UINT Width=0,Height=0;
int AppliedHDR=-1;float AppliedPaper=-1,AppliedPeak=-1;
constexpr uint32_t Marker=0x4A434F4C; // JCOL, outside native command opcodes.
struct Packet { uint32_t stage; Settings settings; };
struct Buffer { char* data; int size; char* executeEnd; char* write; char* writeStart; char* read; };
auto& Commands() { return *reinterpret_cast<Buffer*>(0x033D62EC_g); }
// Slots are verified against the retail and symbol databases. The table excludes
// the two cube targets; slot 6/7 are player-image readbacks, 10..15 are water/
// luminance data, 19 packs depth/stencil, and 24..26 are AO data. Preserve those.
constexpr unsigned ColorTargets[]{0,1,2,3,4,5,8,9,16,17,18,20,21,22,23};
std::array<DWORD,27> OriginalTargets{};
std::array<DWORD,2> OriginalCubes{};
std::array<DWORD,7> OriginalScene{};
bool TablesSaved=false,SceneMapped=false,FormatCaptured=false;
void SetTables(bool fp16) {
    auto* targets=reinterpret_cast<DWORD*>(0x00DC8FB8_g);
    auto* cubes=reinterpret_cast<DWORD*>(0x00DC8FB0_g);
    auto* scene=reinterpret_cast<DWORD*>(0x00E98B2C_g);
    if (!TablesSaved) {
        std::copy(targets,targets+27,OriginalTargets.begin());
        std::copy(cubes,cubes+2,OriginalCubes.begin());
        for (unsigned i=0;i<7;++i) OriginalScene[i]=scene[i*5];
        TablesSaved=true;
    }
    DWORD protect=0;
    if (VirtualProtect(cubes,2*sizeof(DWORD),PAGE_READWRITE,&protect)) {
        for (unsigned i=0;i<2;++i) cubes[i]=fp16 ? D3DFMT_A16B16G16R16F : OriginalCubes[i];
        VirtualProtect(cubes,2*sizeof(DWORD),protect,&protect);
    }
    if (VirtualProtect(targets,27*sizeof(DWORD),PAGE_READWRITE,&protect)) {
        for (auto i:ColorTargets) targets[i]=fp16 ? D3DFMT_A16B16G16R16F : OriginalTargets[i];
        VirtualProtect(targets,27*sizeof(DWORD),protect,&protect);
    }
    if (VirtualProtect(scene,7*20,PAGE_READWRITE,&protect)) {
        // Main colour and emissive glow. Distortion/depth/stencil are data.
        for (auto i:{0u,1u}) scene[i*5]=fp16 ? D3DFMT_A16B16G16R16F : OriginalScene[i];
        VirtualProtect(scene,7*20,protect,&protect);
    }
}
void Save() {
    std::lock_guard lock(SettingsMutex);
    Published=UI;
    GameConfig::SetValue("ColorPipeline","FP16",UI.precision);
    GameConfig::SetValue("ColorPipeline","HDR",UI.hdr);
    GameConfig::SetValue("ColorPipeline","OutputDither",UI.dither);
    GameConfig::SetDoubleValue("ColorPipeline","ExposureEV",UI.exposure);
    GameConfig::SetDoubleValue("ColorPipeline","PaperWhiteNits",UI.paper);
    GameConfig::SetDoubleValue("ColorPipeline","PeakBrightnessNits",UI.peak);
    GameConfig::SetDoubleValue("ColorPipeline","SDRShoulder",UI.shoulder);
    GameConfig::SetDoubleValue("ColorPipeline","HDRRollOff",UI.rolloff);
}
void DebugChanged() { std::lock_guard lock(SettingsMutex); Published=UI; }
void Load() {
    if (Loaded) return;
    Loaded=true;
    UI.precision=GameConfig::GetValue("ColorPipeline","FP16",0,"Experimental FP16 colour pipeline. Requires compatible DXVK; restart after changing.")!=0;
    UI.hdr=GameConfig::GetValue("ColorPipeline","HDR",0,"HDR output with Windows HDR enabled. FP16 must be enabled.")!=0;
    UI.dither=GameConfig::GetValue("ColorPipeline","OutputDither",1,"Sub-LSB SDR output dither. Does not blur the image.")!=0;
    auto read=[](const char* key,double def,float lo,float hi){return std::clamp(static_cast<float>(GameConfig::GetDoubleValue("ColorPipeline",key,def)),lo,hi);};
    UI.exposure=read("ExposureEV",0,-4,4); UI.paper=read("PaperWhiteNits",200,80,500);
    UI.peak=read("PeakBrightnessNits",1000,100,4000); UI.shoulder=read("SDRShoulder",.75,.25,.95);
    UI.rolloff=read("HDRRollOff",.8,.1,.95); Published=UI;
}
bool Queue(uint32_t stage) {
    Packet packet{stage}; {std::lock_guard lock(SettingsMutex); packet.settings=Published;}
    auto& b=Commands(); constexpr size_t bytes=sizeof(Marker)+sizeof(Packet);
    if (!b.data || !b.write || b.write<b.data || b.write>b.data+b.size || bytes>size_t(b.data+b.size-b.write)) return false;
    memcpy(b.write,&Marker,sizeof(Marker)); memcpy(b.write+sizeof(Marker),&packet,sizeof(packet)); b.write+=bytes; return true;
}
void BeginBuilder(SafetyHookContext&) { if (Enabled && !Queue(0)) ++Failures; }
void EndBuilder(SafetyHookContext&) { if (Enabled && !Queue(1)) ++Failures; }
void UpdateOutput(IDirect3DDevice9* d) {
    ComPtr<IDirect3DSwapChain9> swap; ComPtr<DXVK::Swapchain> ext;
    bool hdr=false;
    if (SUCCEEDED(d->GetSwapChain(0,swap.GetAddressOf())) && SUCCEEDED(swap.As(&ext))) {
        hdr=Frame.hdr && ext->CheckColorSpaceSupport(DXVK::ScRGB);
        if (AppliedHDR!=int(hdr)) {
            if (FAILED(ext->SetColorSpace(hdr ? DXVK::ScRGB : DXVK::SDR))) {ext->SetColorSpace(DXVK::SDR);hdr=false;}
            AppliedHDR=int(hdr);
        }
        if (hdr && (AppliedPaper!=Frame.paper || AppliedPeak!=Frame.peak)) {
            DXVK::Metadata metadata;
            metadata.maxLuminance=metadata.maxContentLightLevel=std::max(Frame.peak,Frame.paper);
            metadata.maxFrameAverageLightLevel=Frame.paper;
            ext->SetHDRMetaData(&metadata);AppliedPaper=Frame.paper;AppliedPeak=Frame.peak;
        }
    }
    OutputHDR=hdr; ActiveHDR=hdr;
}
bool Resources(IDirect3DDevice9* d,const D3DSURFACE_DESC& desc) {
    if (Width!=desc.Width || Height!=desc.Height) {Copy.Reset();CopySurface.Reset();Width=desc.Width;Height=desc.Height;}
    if (!TonemapPS && FAILED(d->CreatePixelShader(reinterpret_cast<const DWORD*>(Bytecode::PS_Tonemap),TonemapPS.GetAddressOf()))) return false;
    if (!OutputPS && FAILED(d->CreatePixelShader(reinterpret_cast<const DWORD*>(Bytecode::PS_Output),OutputPS.GetAddressOf()))) return false;
    if (!Copy && (FAILED(d->CreateTexture(Width,Height,1,D3DUSAGE_RENDERTARGET,D3DFMT_A16B16G16R16F,D3DPOOL_DEFAULT,Copy.GetAddressOf(),nullptr)) ||
        FAILED(Copy->GetSurfaceLevel(0,CopySurface.GetAddressOf())))) return false;
    return true;
}
bool Pass(IDirect3DDevice9* d,bool output) {
    if (!Enabled || d!=MainDevice) return false;
    ComPtr<IDirect3DSurface9> target,back;
    D3DSURFACE_DESC desc{};
    if (FAILED(d->GetRenderTarget(0,target.GetAddressOf())) || FAILED(d->GetBackBuffer(0,0,D3DBACKBUFFER_TYPE_MONO,back.GetAddressOf())) ||
        target.Get()!=back.Get() || FAILED(target->GetDesc(&desc)) || desc.Format!=D3DFMT_A16B16G16R16F ||
        desc.MultiSampleType!=D3DMULTISAMPLE_NONE || !Resources(d,desc)) return false;
    ComPtr<IDirect3DStateBlock9> state;
    ComPtr<IDirect3DSurface9> depth;
    std::array<ComPtr<IDirect3DSurface9>,4> targets;
    D3DCAPS9 caps{}; d->GetDeviceCaps(&caps);
    for (unsigned i=0;i<std::min<DWORD>(caps.NumSimultaneousRTs,4);++i) d->GetRenderTarget(i,targets[i].GetAddressOf());
    d->GetDepthStencilSurface(depth.GetAddressOf());
    if (FAILED(d->CreateStateBlock(D3DSBT_ALL,state.GetAddressOf())) || FAILED(state->Capture())) return false;
    HRESULT hr=d->StretchRect(target.Get(),nullptr,CopySurface.Get(),nullptr,D3DTEXF_NONE);
    d->SetDepthStencilSurface(nullptr);
    for (unsigned i=1;i<std::min<DWORD>(caps.NumSimultaneousRTs,4);++i) d->SetRenderTarget(i,nullptr);
    for (unsigned i=0;i<16;++i) d->SetTexture(i,nullptr);
    for (auto s:{D3DRS_ZENABLE,D3DRS_ZWRITEENABLE,D3DRS_ALPHABLENDENABLE,D3DRS_ALPHATESTENABLE,D3DRS_STENCILENABLE,
        D3DRS_SCISSORTESTENABLE,D3DRS_SRGBWRITEENABLE,D3DRS_FOGENABLE,D3DRS_SEPARATEALPHABLENDENABLE}) d->SetRenderState(s,FALSE);
    d->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE); d->SetRenderState(D3DRS_COLORWRITEENABLE,15);
    for (auto s:{D3DSAMP_ADDRESSU,D3DSAMP_ADDRESSV}) d->SetSamplerState(0,s,D3DTADDRESS_CLAMP);
    d->SetSamplerState(0,D3DSAMP_MINFILTER,D3DTEXF_POINT); d->SetSamplerState(0,D3DSAMP_MAGFILTER,D3DTEXF_POINT);
    d->SetSamplerState(0,D3DSAMP_MIPFILTER,D3DTEXF_NONE); d->SetSamplerState(0,D3DSAMP_SRGBTEXTURE,FALSE);
    const float settings[]{Frame.exposure,Frame.paper,std::max(Frame.peak,Frame.paper),OutputHDR ? 1.f : 0.f};
    const float controls[]{Frame.shoulder,Frame.rolloff,Frame.dither ? 1.f : 0.f,Frame.debug ? 1.f : 0.f};
    d->SetPixelShaderConstantF(0,settings,1);d->SetPixelShaderConstantF(1,controls,1);
    d->SetTexture(0,Copy.Get()); d->SetPixelShader(output ? OutputPS.Get() : TonemapPS.Get());
    d->SetVertexShader(nullptr); d->SetFVF(D3DFVF_XYZRHW|D3DFVF_TEX1); d->SetStreamSourceFreq(0,1);
    D3DVIEWPORT9 viewport{0,0,Width,Height,0,1};d->SetViewport(&viewport);
    struct Vertex {float x,y,z,w,u,v;};
    const Vertex v[]{{-.5f,-.5f,0,1,0,0},{Width-.5f,-.5f,0,1,1,0},{-.5f,Height-.5f,0,1,0,1},{Width-.5f,Height-.5f,0,1,1,1}};
    if (SUCCEEDED(hr)) hr=d->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP,2,v,sizeof(Vertex));
    d->SetTexture(0,nullptr);
    for (unsigned i=0;i<std::min<DWORD>(caps.NumSimultaneousRTs,4);++i) d->SetRenderTarget(i,targets[i].Get());
    d->SetDepthStencilSurface(depth.Get()); state->Apply();
    if (FAILED(hr)) {Logger::TypedLog(CHN_DEBUG,"FP16: {} pass failed: 0x{:08X}\n",output ? "output" : "scene",uint32_t(hr));}
    return SUCCEEDED(hr);
}
void Execute(SafetyHookContext& ctx) {
    if (ctx.eax!=Marker) return;
    auto& b=Commands(); Packet packet{};memcpy(&packet,b.read,sizeof(packet));b.read+=sizeof(packet);
    if (Enabled && MainDevice) {
        if (packet.stage==0) {Frame=packet.settings;SceneMapped=false;UpdateOutput(MainDevice);
            const float flag[]{1,0,0,0};MainDevice->SetPixelShaderConstantF(223,flag,1);}
        else {if (Pass(MainDevice,false)) {SceneMapped=true;++ScenePasses;} else ++Failures;}
    }
    *reinterpret_cast<HRESULT*>(ctx.ebp-8)=S_OK;ctx.eax=S_OK;ctx.eip=static_cast<uint32_t>(0x00D2113E_g);
}
using ResetFn=HRESULT (STDMETHODCALLTYPE*)(IDirect3DDevice9*,D3DPRESENT_PARAMETERS*);
using EndFn=HRESULT (STDMETHODCALLTYPE*)(IDirect3DDevice9*);
using TargetFn=HRESULT (STDMETHODCALLTYPE*)(IDirect3DDevice9*,UINT,UINT,D3DFORMAT,D3DMULTISAMPLE_TYPE,DWORD,BOOL,IDirect3DSurface9**,HANDLE*);
using SampleFn=HRESULT (STDMETHODCALLTYPE*)(IDirect3D9*,UINT,D3DDEVTYPE,D3DFORMAT,BOOL,D3DMULTISAMPLE_TYPE,DWORD*);
SampleFn RealSample=nullptr;
HRESULT STDMETHODCALLTYPE CheckSamples(IDirect3D9* api,UINT adapter,D3DDEVTYPE type,D3DFORMAT format,BOOL windowed,D3DMULTISAMPLE_TYPE samples,DWORD* quality) {
    // SR2 validates native MSAA against a hard-coded RGBA8 format. Validate the
    // actual promoted colour format, including when the menu changes sample count.
    if (Enabled && format==D3DFMT_A8R8G8B8) format=D3DFMT_A16B16G16R16F;
    return RealSample(api,adapter,type,format,windowed,samples,quality);
}
ResetFn RealReset=nullptr;EndFn RealEnd=nullptr;TargetFn RealTarget=nullptr;
HRESULT STDMETHODCALLTYPE Reset(IDirect3DDevice9* d,D3DPRESENT_PARAMETERS* pp) {
    if (d!=MainDevice) return RealReset(d,pp);
    ReleaseResources(); pp->BackBufferFormat=Enabled ? D3DFMT_A16B16G16R16F : OriginalFormat;
    HRESULT hr=RealReset(d,pp);
    if (FAILED(hr) && Enabled) {
        Enabled=false;ActivePrecision=0;OutputHDR=false;ActiveHDR=0;SetTables(false);
        pp->BackBufferFormat=OriginalFormat;hr=RealReset(d,pp);
        Logger::TypedLog(CHN_DEBUG,"FP16: reset refused floating-point output; restored SDR formats.\n");
    }
    return hr;
}
HRESULT STDMETHODCALLTYPE End(IDirect3DDevice9* d) {
    if (d==MainDevice && Enabled) {
        // Menu-only frames need current output settings too.
        if (!SceneMapped) {std::lock_guard lock(SettingsMutex); Frame=Published; UpdateOutput(d);}
        if (!Pass(d,true)) ++Failures; SceneMapped=false;
    }
    return RealEnd(d);
}
HRESULT STDMETHODCALLTYPE Target(IDirect3DDevice9* d,UINT w,UINT h,D3DFORMAT format,D3DMULTISAMPLE_TYPE samples,DWORD quality,BOOL lockable,IDirect3DSurface9** out,HANDLE* shared) {
    if (d==MainDevice && Enabled && out==reinterpret_cast<IDirect3DSurface9**>(0x0252A2E4_g)) format=D3DFMT_A16B16G16R16F;
    return RealTarget(d,w,h,format,samples,quality,lockable,out,shared);
}
template<class T> void Replace(void** table,unsigned slot,T function,T& original) {
    if (table[slot]==reinterpret_cast<void*>(function)) return;
    original=reinterpret_cast<T>(table[slot]);DWORD protect=0;
    if (VirtualProtect(table+slot,sizeof(void*),PAGE_READWRITE,&protect)) {
        table[slot]=reinterpret_cast<void*>(function);VirtualProtect(table+slot,sizeof(void*),protect,&protect);
    }
}
}
void ReleaseResources() {Copy.Reset();CopySurface.Reset();TonemapPS.Reset();OutputPS.Reset();Width=Height=0;SceneMapped=false;AppliedHDR=-1;AppliedPaper=AppliedPeak=-1;}
void BeforeCreateDevice(IDirect3D9* api,D3DPRESENT_PARAMETERS* pp) {
    Load(); if (!UI.precision || !HooksReady || !api || !pp) return;
    if (pp->BackBufferFormat!=D3DFMT_A16B16G16R16F) {OriginalFormat=pp->BackBufferFormat;FormatCaptured=true;}
    ComPtr<DXVK::Interface> ext;
    if (FAILED(api->QueryInterface(__uuidof(DXVK::Interface),reinterpret_cast<void**>(ext.GetAddressOf())))) {
        Logger::TypedLog(CHN_DEBUG,"FP16: compatible DXVK D3D9 HDR interface unavailable; using native SDR.\n");return;
    }
    ext->UnlockAdditionalFormats();Unlocked=true;
    if (FAILED(api->CheckDeviceFormat(D3DADAPTER_DEFAULT,D3DDEVTYPE_HAL,D3DFMT_X8R8G8B8,
        D3DUSAGE_RENDERTARGET|D3DUSAGE_QUERY_POSTPIXELSHADER_BLENDING,D3DRTYPE_TEXTURE,D3DFMT_A16B16G16R16F)) ||
        FAILED(api->CheckDeviceFormat(D3DADAPTER_DEFAULT,D3DDEVTYPE_HAL,D3DFMT_X8R8G8B8,
        D3DUSAGE_RENDERTARGET,D3DRTYPE_CUBETEXTURE,D3DFMT_A16B16G16R16F))) return;
    pp->BackBufferFormat=D3DFMT_A16B16G16R16F;
}
void CreationFailed(D3DPRESENT_PARAMETERS* pp) {
    if (pp && pp->BackBufferFormat==D3DFMT_A16B16G16R16F && FormatCaptured) pp->BackBufferFormat=OriginalFormat;
    Unlocked=false;
}
void AfterCreateDevice(IDirect3DDevice9* d,D3DPRESENT_PARAMETERS* pp) {
    if (!d || !pp || !Unlocked || pp->BackBufferFormat!=D3DFMT_A16B16G16R16F) return;
    MainDevice=d;Enabled=true;ActivePrecision=1;SetTables(true);Frame=Published;
    const float flag[]{1,0,0,0};d->SetPixelShaderConstantF(223,flag,1);
    auto** table=*reinterpret_cast<void***>(d);
    Replace(table,16,&Reset,RealReset);Replace(table,42,&End,RealEnd);Replace(table,28,&Target,RealTarget);
    ComPtr<IDirect3D9> api;if (SUCCEEDED(d->GetDirect3D(api.GetAddressOf()))) Replace(*reinterpret_cast<void***>(api.Get()),11,&CheckSamples,RealSample);
    UpdateOutput(d);
    Logger::TypedLog(CHN_DEBUG,"FP16: RGBA16F scene/glow/postFX/reflections/backbuffer enabled; colour mapping precedes HUD.\n");
}
void Init() {
    Load();
    if (memcmp(reinterpret_cast<void*>(0x00D203F6_g),"\x0F\x87\x41\x0D\x00\x00",6) ||
        memcmp(reinterpret_cast<void*>(0x0052ABE6_g),"\xE8\x05\x66\xFF\xFF",5)) {
        Logger::TypedLog(CHN_DEBUG,"FP16: executable signatures mismatch; disabled.\n");return;
    }
    auto consumer=safetyhook::create_mid(0x00D203F6_g,Execute);
    auto begin=safetyhook::create_mid(0x0052A710_g,BeginBuilder);
    auto end=safetyhook::create_mid(0x0052ABE6_g,EndBuilder);
    if (!consumer || !begin|| !end) return;
    Hooks.emplace_back(std::move(consumer));Hooks.emplace_back(std::move(begin));Hooks.emplace_back(std::move(end));HooksReady=true;
    const char* path="Juiced Colour / HDR";
    BlingMenuAddBool(path,"FP16 colour (restart)",&UI.precision,Save);
    BlingMenuAddBool(path,"HDR output (Windows HDR)",&UI.hdr,Save);
    BlingMenuAddBool(path,"SDR output dither",&UI.dither,Save);
    BlingMenuAddFloat(path,"Exposure EV",&UI.exposure,Save,.1f,-4,4);
    BlingMenuAddFloat(path,"SDR highlight shoulder",&UI.shoulder,Save,.05f,.25f,.95f);
    BlingMenuAddFloat(path,"Paper white / UI nits",&UI.paper,Save,10,80,500);
    BlingMenuAddFloat(path,"Peak brightness nits",&UI.peak,Save,50,100,4000);
    BlingMenuAddFloat(path,"HDR highlight rolloff",&UI.rolloff,Save,.05f,.1f,.95f);
    BlingMenuAddBool(path,"Show highlights above SDR white",&UI.debug,DebugChanged);
    // Diagnostic snapshots are read-only from the renderer's point of view.
    BlingMenuAddInt(path,"Active FP16",reinterpret_cast<int*>(&ActivePrecision),nullptr,0,0,1);
    BlingMenuAddInt(path,"Active HDR",reinterpret_cast<int*>(&ActiveHDR),nullptr,0,0,1);
    BlingMenuAddInt(path,"Scene tone-map passes",reinterpret_cast<int*>(&ScenePasses),nullptr,0,0,0x7fffffff);
    BlingMenuAddInt(path,"Failed colour passes",reinterpret_cast<int*>(&Failures),nullptr,0,0,0x7fffffff);
}
}
#endif
