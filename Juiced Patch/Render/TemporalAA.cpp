#include "TemporalAA.h"
#if JUICED_TAA

// Native x86 D3D9 temporal AA for the supported SR2_PC executable.
// The temporal resolve is derived from FusionFix; see TemporalSR2.hlsl and
// third_party/FusionFix-TAA. Geometry stays in SR2's renderer.
#define NOMINMAX
#include <windows.h>
#include "TemporalShader.h"
#include "TemporalProjection.h"
#if JUICED_TAA_MSAA
#include "TemporalMSAA.h"
#endif
#include <cstdio>
#include "TemporalShaders.h"
#include "../Hooker.h"
#include "../GameConfig.h"
#include "../BlingMenu_public.h"
#include "../FileLogger.h"
#include <d3d9.h>
#include <wrl/client.h>
#include <safetyhook.hpp>
#include <array>
#include <atomic>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace TemporalAA
{
namespace
{
    using Microsoft::WRL::ComPtr;
    using Projection::Matrix;
    using Projection::Multiply;
    constexpr uint32_t Marker = 0x4A544141; // "JTAA"; outside the native opcode range.
    constexpr size_t MaxDraws = 4096;
    constexpr UINT BoneWidth = 192;
    struct RenderBuffer
    {
        char* data;
        int size;
        char* executeEnd;
        char* write;
        char* writeStart;
        char* read;
    };
    auto& Commands() { return *reinterpret_cast<RenderBuffer*>(0x033D62EC_g); }
    auto* Device() { return *reinterpret_cast<IDirect3DDevice9**>(0x0252A2D0_g); }
    // Requested samples (252A2DC) change before the native surfaces are rebuilt.
    // Always use the allocated sample count for render commands and history.
    int Samples() { return *reinterpret_cast<int*>(0x0252A2E0_g); }
#if JUICED_TAA_MSAA
    auto* MultisampleDepth() { return *reinterpret_cast<IDirect3DSurface9**>(0x0252A2E8_g); }
    auto* MultisampleColor() { return *reinterpret_cast<IDirect3DSurface9**>(0x0252A2E4_g); }
    MSAA::CoverageState NativeCoverage()
    {
        return {*reinterpret_cast<bool*>(0x0252A2EC_g),
            static_cast<D3DRENDERSTATETYPE>(*reinterpret_cast<DWORD*>(0x022FD8E0_g)),
            *reinterpret_cast<DWORD*>(0x022FD8DC_g),*reinterpret_cast<DWORD*>(0x022F6434_g)};
    }
#endif
    auto* SceneTexture() { return *reinterpret_cast<IDirect3DTexture9**>(0x00FA236C_g); }
    auto* GlowTexture() { return *reinterpret_cast<IDirect3DTexture9**>(0x00FA2370_g); }
    auto* NativeDepth() { return *reinterpret_cast<IDirect3DTexture9**>(0x0252A2D8_g); }
    // UI owns UiSettings. Render commands carry an immutable settings snapshot.
    struct Settings
    {
        bool enabled = false, jitter = true, objectMotion = true, reactive = true, accumulation = true, temporalDither = true, depthRejection = false, temporalBloom = true, temporalGlow = true;
#if JUICED_TAA_MSAA
        bool combineMSAA = true, forceDepthReplay = false;
#endif
        float gamma = 1.25f, minWeight = 0.08f, maxWeight = 0.20f, motionScale = 0.10f;
        float lumaWeight = 1.0f, reactiveScale = 2.0f, reactiveMax = 0.75f;
        float debugMotionScale = 8.0f;
        uint32_t debugMode = 0, revision = 1;
    };
    bool AllowsSampleMode(const Settings& settings)
    {
#if JUICED_TAA_MSAA
        return Samples()==0 || settings.combineMSAA;
#else
        return Samples()==0;
#endif
    }
    Settings UiSettings, PublishedSettings;
    bool ShowMotion = false, ShowDepth = false, ShowReactive = false, ShowHistory = false, ShowValidity = false, ShowBloom = false, ShowRejection = false, ShowAlpha = false;
    bool HookReady = false;
    std::mutex SettingsMutex;
    std::atomic<bool> Wanted = false, GPUReady = false;
    std::atomic<unsigned> ActiveSamples = 0;
#if JUICED_TAA_MSAA
    std::atomic<bool> AllowMSAA = true;
    std::atomic<unsigned> DepthReplayDraws = 0, DepthSourceMode = 0;
#endif
    std::atomic<unsigned> HistoryFrames = 0, CapturedDraws = 0, DitherDraws = 0, UnsupportedShaders = 0, MatchedDraws = 0, FallbackDraws = 0, GlowFrames = 0, MainDraws = 0, IdentifiedDraws = 0, SupportedDraws = 0;
    enum class Status { Disabled, Waiting, MSAA, Unsupported, Active, Fault, HooksUnavailable };
    std::atomic<Status> RenderStatus = Status::Disabled;

    void SaveSettings()
    {
        UiSettings.minWeight = std::clamp(UiSettings.minWeight, 0.0f, 1.0f);
        UiSettings.maxWeight = std::clamp(UiSettings.maxWeight, UiSettings.minWeight, 1.0f);
        GameConfig::SetValue("Graphics", "TemporalAA", UiSettings.enabled);
#if JUICED_TAA_MSAA
        GameConfig::SetValue("TemporalAA", "CombineMSAA", UiSettings.combineMSAA);
        GameConfig::SetValue("TemporalAA", "ForceDepthReplay", UiSettings.forceDepthReplay);
        AllowMSAA = UiSettings.combineMSAA;
#endif
        GameConfig::SetValue("TemporalAA", "Jitter", UiSettings.jitter);
        GameConfig::SetValue("TemporalAA", "ObjectMotion", UiSettings.objectMotion);
        GameConfig::SetValue("TemporalAA", "ReactiveMask", UiSettings.reactive);
        GameConfig::SetValue("TemporalAA", "HistoryAccumulation", UiSettings.accumulation);
        GameConfig::SetValue("TemporalAA", "TemporalDither", UiSettings.temporalDither);
        GameConfig::SetValue("TemporalAA", "DepthRejection", UiSettings.depthRejection);
        GameConfig::SetValue("TemporalAA", "TemporalBloomAlpha", UiSettings.temporalBloom);
        GameConfig::SetValue("TemporalAA", "TemporalGlow", UiSettings.temporalGlow);
        GameConfig::SetDoubleValue("TemporalAA", "VarianceGamma", UiSettings.gamma);
        GameConfig::SetDoubleValue("TemporalAA", "MinCurrentWeight", UiSettings.minWeight);
        GameConfig::SetDoubleValue("TemporalAA", "MaxCurrentWeight", UiSettings.maxWeight);
        GameConfig::SetDoubleValue("TemporalAA", "MotionWeightScale", UiSettings.motionScale);
        GameConfig::SetDoubleValue("TemporalAA", "LumaWeight", UiSettings.lumaWeight);
        GameConfig::SetDoubleValue("TemporalAA", "ReactiveScale", UiSettings.reactiveScale);
        GameConfig::SetDoubleValue("TemporalAA", "ReactiveMaxWeight", UiSettings.reactiveMax);
        ++UiSettings.revision;
        std::lock_guard lock(SettingsMutex);
        PublishedSettings = UiSettings;
        Wanted = UiSettings.enabled && HookReady;
    }
    void PublishDebug()
    {
        // Debug views are session-only; do not change temporal history or the INI.
        std::lock_guard lock(SettingsMutex);
        PublishedSettings.debugMode = UiSettings.debugMode;
        PublishedSettings.debugMotionScale = UiSettings.debugMotionScale;
    }
    void SelectDebug(uint32_t mode, bool enabled)
    {
        UiSettings.debugMode = enabled ? mode : 0;
        ShowMotion = UiSettings.debugMode == 1; ShowDepth = UiSettings.debugMode == 2;
        ShowReactive = UiSettings.debugMode == 3; ShowHistory = UiSettings.debugMode == 4;
        ShowValidity = UiSettings.debugMode == 5; ShowBloom = UiSettings.debugMode == 6;
        ShowRejection = UiSettings.debugMode == 7; ShowAlpha = UiSettings.debugMode == 8;
        PublishDebug();
    }
    void ResetHistory()
    {
        ++UiSettings.revision;
        std::lock_guard lock(SettingsMutex);
        PublishedSettings.revision = UiSettings.revision;
    }
    void RegisterMenu()
    {
        const char* path = "Juiced TAA";
        BlingMenuAddCategory(path);
        BlingMenuAddFuncCustomStd(path, "Status", [](int) -> const char* {
            if (!HookReady) return "HOOKS UNAVAILABLE";
            switch (RenderStatus.load()) {
            case Status::Disabled: return "OFF";
            case Status::Waiting: return "WAITING FOR SCENE";
#if JUICED_TAA_MSAA
            case Status::MSAA: return "ENABLE COMBINE WITH MSAA";
#else
            case Status::MSAA: return "DISABLE MSAA FOR TAA";
#endif
            case Status::Unsupported: return "DEPTH / GPU UNSUPPORTED";
#if JUICED_TAA_MSAA
            case Status::Active: return ActiveSamples.load() ? "ACTIVE: MSAA + TAA" : "ACTIVE: TAA";
#else
            case Status::Active: return "ACTIVE: TAA";
#endif
            case Status::Fault: return "GPU PASS FAILED - RESET";
            default: return "HOOKS UNAVAILABLE";
            }
        }, nullptr);
        BlingMenuAddBool(path, "Enabled", &UiSettings.enabled, SaveSettings);
#if JUICED_TAA_MSAA
        BlingMenuAddBool(path, "Combine with MSAA", &UiSettings.combineMSAA, SaveSettings);
#endif
        BlingMenuAddBool(path, "Projection jitter", &UiSettings.jitter, SaveSettings);
        BlingMenuAddBool(path, "Object / skin motion", &UiSettings.objectMotion, SaveSettings);
        BlingMenuAddBool(path, "Reactive transparency mask", &UiSettings.reactive, SaveSettings);
        BlingMenuAddBool(path, "History accumulation", &UiSettings.accumulation, SaveSettings);
        BlingMenuAddBool(path, "Temporal hair / foliage dither", &UiSettings.temporalDither, SaveSettings);
        BlingMenuAddBool(path, "Reject history at depth changes", &UiSettings.depthRejection, SaveSettings);
        BlingMenuAddBool(path, "Filter scene alpha / bloom weight", &UiSettings.temporalBloom, SaveSettings);
        BlingMenuAddBool(path, "Temporal emissive glow", &UiSettings.temporalGlow, SaveSettings);
        BlingMenuAddFloat(path, "Variance clip gamma", &UiSettings.gamma, SaveSettings, 0.05f, 0.25f, 3.0f);
        BlingMenuAddFloat(path, "Minimum current weight", &UiSettings.minWeight, SaveSettings, 0.01f, 0.0f, 1.0f);
        BlingMenuAddFloat(path, "Maximum current weight", &UiSettings.maxWeight, SaveSettings, 0.01f, 0.0f, 1.0f);
        BlingMenuAddFloat(path, "Motion weight scale", &UiSettings.motionScale, SaveSettings, 0.01f, 0.0f, 1.0f);
        BlingMenuAddFloat(path, "Luminance weight", &UiSettings.lumaWeight, SaveSettings, 0.05f, 0.0f, 2.0f);
        BlingMenuAddFloat(path, "Reactive scale", &UiSettings.reactiveScale, SaveSettings, 0.1f, 0.0f, 10.0f);
        BlingMenuAddFloat(path, "Maximum reactive weight", &UiSettings.reactiveMax, SaveSettings, 0.05f, 0.0f, 1.0f);
        BlingMenuAddFunc(path, "Reset history / retry GPU passes", ResetHistory);
        BlingMenuAddFunc(path, "Restore FusionFix defaults", []() {
            const bool enabled = UiSettings.enabled;
            const uint32_t revision = UiSettings.revision, debug = UiSettings.debugMode;
            const float scale = UiSettings.debugMotionScale;
            UiSettings = {}; UiSettings.enabled = enabled; UiSettings.revision = revision;
            UiSettings.debugMode = debug; UiSettings.debugMotionScale = scale;
            SaveSettings();
        });
        const char* debug = "Juiced TAA Debug";
        BlingMenuAddCategory(debug);
        auto counter = [debug](const char* label, std::atomic<unsigned>* value) {
            BlingMenuAddFuncCustomStd(debug, label, [value](int) -> const char* {
                thread_local char text[32]; std::snprintf(text, sizeof(text), "%u", value->load());
                return text;
            }, nullptr);
        };
#if JUICED_TAA_MSAA
        BlingMenuAddFuncCustomStd(debug,"MSAA depth source",[](int) -> const char* {
            switch (DepthSourceMode.load()) {
            case 1: return "SHADER REPLAY (SLOW)";
            case 2: return "GPU RESOLVE (FAST)";
            default: return "NATIVE / INACTIVE";
            }
        },nullptr);
        BlingMenuAddBool(debug,"Force MSAA depth replay (slow)",&UiSettings.forceDepthReplay,SaveSettings);
        counter("Active MSAA samples (0 = off)", &ActiveSamples);
        counter("MSAA companion depth draws", &DepthReplayDraws);
#endif
        counter("Consecutive accumulated frames", &HistoryFrames);
        counter("Captured scene draws", &CapturedDraws);
        counter("Main indexed draws", &MainDraws);
        counter("Draws with entity identity", &IdentifiedDraws);
        counter("Draws with supported motion shader", &SupportedDraws);
        counter("Consecutive glow history frames", &GlowFrames);
        counter("Matched object motion draws", &MatchedDraws);
        counter("Unmatched draws using camera motion", &FallbackDraws);
        counter("Temporal dither draws", &DitherDraws);
        counter("Unsupported motion shaders", &UnsupportedShaders);
        BlingMenuAddBool(debug, "Show motion vectors", &ShowMotion, []() { SelectDebug(1, ShowMotion); });
        BlingMenuAddBool(debug, "Show depth", &ShowDepth, []() { SelectDebug(2, ShowDepth); });
        BlingMenuAddBool(debug, "Show reactive mask", &ShowReactive, []() { SelectDebug(3, ShowReactive); });
        BlingMenuAddBool(debug, "Show resolved scene (before postFX)", &ShowHistory, []() { SelectDebug(4, ShowHistory); });
        BlingMenuAddBool(debug, "Show motion history validity", &ShowValidity, []() { SelectDebug(5, ShowValidity); });
        BlingMenuAddBool(debug, "Show resolved emissive glow", &ShowBloom, []() { SelectDebug(6, ShowBloom); });
        BlingMenuAddBool(debug, "Show rejected history (red)", &ShowRejection, []() { SelectDebug(7, ShowRejection); });
        BlingMenuAddBool(debug, "Show resolved scene alpha", &ShowAlpha, []() { SelectDebug(8, ShowAlpha); });
        BlingMenuAddFloat(debug, "Motion display scale (pixels)", &UiSettings.debugMotionScale, PublishDebug, 1.0f, 1.0f, 128.0f);
        BlingMenuAddFunc(debug, "Hide debug views", []() { SelectDebug(0, false); });
    }
    void LoadSettings()
    {
        UiSettings.enabled = GameConfig::GetValue("Graphics", "TemporalAA", 0,
#if JUICED_TAA_MSAA
            "Experimental FusionFix TAA, optionally combined with native MSAA. BlingGFX/Juiced shader priority is retained.") != 0;
#else
            "Experimental FusionFix TAA. Requires MSAA off. BlingGFX/Juiced shader priority is retained.") != 0;
#endif
#if JUICED_TAA_MSAA
        UiSettings.combineMSAA = GameConfig::GetValue("TemporalAA", "CombineMSAA", 1) != 0;
        UiSettings.forceDepthReplay = GameConfig::GetValue("TemporalAA", "ForceDepthReplay", 0) != 0;
        AllowMSAA = UiSettings.combineMSAA;
#endif
        UiSettings.jitter = GameConfig::GetValue("TemporalAA", "Jitter", 1) != 0;
        UiSettings.objectMotion = GameConfig::GetValue("TemporalAA", "ObjectMotion", 1) != 0;
        UiSettings.reactive = GameConfig::GetValue("TemporalAA", "ReactiveMask", 1) != 0;
        UiSettings.accumulation = GameConfig::GetValue("TemporalAA", "HistoryAccumulation", 1) != 0;
        UiSettings.temporalDither = GameConfig::GetValue("TemporalAA", "TemporalDither", 1) != 0;
        UiSettings.depthRejection = GameConfig::GetValue("TemporalAA", "DepthRejection", 0) != 0;
        UiSettings.temporalBloom = GameConfig::GetValue("TemporalAA", "TemporalBloomAlpha", 1) != 0;
        UiSettings.temporalGlow = GameConfig::GetValue("TemporalAA", "TemporalGlow", 1) != 0;
        auto read = [](const char* key, float def, float low, float high) {
            const double v = GameConfig::GetDoubleValue("TemporalAA", key, def);
            return std::isfinite(v) ? std::clamp(static_cast<float>(v), low, high) : def;
        };
        UiSettings.gamma = read("VarianceGamma", 1.25f, 0.25f, 3.0f);
        UiSettings.minWeight = read("MinCurrentWeight", 0.08f, 0, 1);
        UiSettings.maxWeight = read("MaxCurrentWeight", 0.20f, UiSettings.minWeight, 1);
        UiSettings.motionScale = read("MotionWeightScale", 0.10f, 0, 1);
        UiSettings.lumaWeight = read("LumaWeight", 1, 0, 2);
        UiSettings.reactiveScale = read("ReactiveScale", 2, 0, 10);
        UiSettings.reactiveMax = read("ReactiveMaxWeight", 0.75f, 0, 1);
        PublishedSettings = UiSettings;
    }

    enum class Stage : uint32_t { Begin, Main, Opaque, FinalizeScene, Resolve, Debug, Entity };
    struct Packet
    {
        Stage stage;
        uint64_t entity = 0;
        Matrix viewProjection{};
        float jitter[2]{};
        unsigned phase = 0, glowAvailable = 0;
        float depthProjection[2]{1.0001f,-0.10001f};
        Settings settings{};
    };
    static_assert(sizeof(Packet) % 4 == 0);
    bool DebugFrame = false;
    bool Building = false; // Game command-builder thread only.
    unsigned Phase = 0;
    thread_local uint64_t CharacterKey = 0;
    thread_local uint64_t PartKey = 0;
    std::mutex InstanceMutex;
    std::unordered_map<uintptr_t, uint64_t> InstanceKeys;
    std::vector<SafetyHookMid> Hooks;
    SafetyHookInline CharacterHook, HumanHook, VehicleHook, ProjectionHook;
    // Only the main view's command-building thread mutates its projection.
    // Shadow/reflection contexts and their worker threads stay independent.
    thread_local bool SceneScope = false, ProjectionShifted = false;
    thread_local uintptr_t MainWorld = 0;
    thread_local float SceneNDC[2]{};
    uintptr_t World() { return reinterpret_cast<uintptr_t(__cdecl*)()>(0x00D22790_g)(); }
    Matrix& WorldMatrix(uintptr_t world, size_t offset) { return *reinterpret_cast<Matrix*>(world+offset); }
    void RestoreProjection()
    {
        if (SceneScope && MainWorld && ProjectionShifted)
        {
            Projection::Shift(WorldMatrix(MainWorld,0x84),-SceneNDC[0],-SceneNDC[1]);
            Projection::Shift(WorldMatrix(MainWorld,0x1558),-SceneNDC[0],-SceneNDC[1]);
            WorldMatrix(MainWorld,0x1598)=Multiply(WorldMatrix(MainWorld,0x1558),WorldMatrix(MainWorld,0xC4));
        }
        SceneScope = ProjectionShifted = false;
        MainWorld = 0;
    }
    uintptr_t __cdecl BuildProjection(char ortho, char infiniteFar, char tileFlip)
    {
        const auto result=ProjectionHook.ccall<uintptr_t>(ortho,infiniteFar,tileFlip);
        if (SceneScope && World()==MainWorld)
        {
            ProjectionShifted = !ortho;
            if (ProjectionShifted) Projection::Shift(WorldMatrix(MainWorld,0x84),SceneNDC[0],SceneNDC[1]);
        }
        return result;
    }
    void CachedInstanceBuilder(SafetyHookContext& ctx)
    {
        if (!SceneScope || !ProjectionShifted || ctx.eax != MainWorld) return;
        const int depth=*reinterpret_cast<int*>(MainWorld+0x1554);
        if (depth < 1 || depth > 16) return;
        const uintptr_t instance=MainWorld+0x114+(depth-1)*0x144;
        // Cached instances predate jitter. Rebuild the copied clip x/y before
        // native gr_start_cached_instance transposes/uploads c4-c7.
        Projection::RefreshCachedClip(WorldMatrix(instance,0x40),WorldMatrix(instance,0x80),WorldMatrix(MainWorld,0x1558));
    }

    uint64_t Combine(uint64_t a, uint64_t b)
    {
        return a ^ (b + 0x9E3779B97F4A7C15ull + (a << 6) + (a >> 2));
    }
    float Halton(unsigned index, unsigned base)
    {
        float result = 0, weight = 1;
        while (index) { weight /= base; result += (index % base) * weight; index /= base; }
        return result;
    }
    bool Queue(const Packet& packet)
    {
        auto& cmd = Commands();
        const size_t bytes = sizeof(Marker) + sizeof(packet);
        if (!cmd.data || !cmd.write || cmd.write < cmd.data ||
            cmd.write > cmd.data + cmd.size || bytes > static_cast<size_t>(cmd.data + cmd.size - cmd.write))
            return false;
        memcpy(cmd.write, &Marker, sizeof(Marker));
        memcpy(cmd.write + sizeof(Marker), &packet, sizeof(packet));
        cmd.write += bytes;
        return true;
    }
    bool Inverse(const Matrix& matrix, Matrix& result)
    {
        double a[4][8]{};
        for (unsigned r = 0; r < 4; ++r)
        {
            for (unsigned c = 0; c < 4; ++c) a[r][c] = matrix[r * 4 + c];
            a[r][r + 4] = 1;
        }
        for (unsigned c = 0; c < 4; ++c)
        {
            unsigned pivot = c;
            for (unsigned r = c + 1; r < 4; ++r)
                if (std::abs(a[r][c]) > std::abs(a[pivot][c])) pivot = r;
            if (!std::isfinite(a[pivot][c]) || std::abs(a[pivot][c]) < 1e-10) return false;
            if (pivot != c) for (unsigned k = 0; k < 8; ++k) std::swap(a[pivot][k], a[c][k]);
            const double divisor = a[c][c];
            for (auto& value : a[c]) value /= divisor;
            for (unsigned r = 0; r < 4; ++r)
                if (r != c)
                {
                    const double factor = a[r][c];
                    for (unsigned k = 0; k < 8; ++k) a[r][k] -= a[c][k] * factor;
                }
        }
        for (unsigned r = 0; r < 4; ++r)
            for (unsigned c = 0; c < 4; ++c)
            {
                result[r * 4 + c] = static_cast<float>(a[r][c + 4]);
                if (!std::isfinite(result[r * 4 + c])) return false;
            }
        return true;
    }
    struct Target
    {
        ComPtr<IDirect3DTexture9> texture;
        ComPtr<IDirect3DSurface9> surface;
        bool Create(IDirect3DDevice9* device, UINT w, UINT h, D3DFORMAT format)
        {
            return SUCCEEDED(device->CreateTexture(w, h, 1, D3DUSAGE_RENDERTARGET, format,
                D3DPOOL_DEFAULT, texture.ReleaseAndGetAddressOf(), nullptr)) &&
                SUCCEEDED(texture->GetSurfaceLevel(0, surface.ReleaseAndGetAddressOf()));
        }
    };
    struct ShaderEntry
    {
        ComPtr<IDirect3DVertexShader9> native;
        ComPtr<IDirect3DVertexShader9> velocity;
        bool skinned = false;
    };
    struct DitherShaderEntry
    {
        ComPtr<IDirect3DPixelShader9> native, temporal;
    };
    struct Stream
    {
        ComPtr<IDirect3DVertexBuffer9> buffer;
        UINT offset = 0, stride = 0, frequency = 1;
    };
    struct Draw
    {
        uint64_t key = 0;
        ShaderEntry* shader = nullptr;
        ComPtr<IDirect3DVertexDeclaration9> declaration;
        ComPtr<IDirect3DIndexBuffer9> indices;
        std::array<Stream, 16> streams;
        std::array<float, 248 * 4> constants{};
        Matrix previousMVP{};
        std::array<float, BoneWidth * 4> previousBones{};
        D3DPRIMITIVETYPE type{};
        INT base = 0;
        UINT minimum = 0, vertices = 0, start = 0, primitives = 0;
        DWORD cull = D3DCULL_CCW;
        unsigned boneRow = 0;
        bool valid = false;
    };
    struct PreviousDraw
    {
        uint64_t frame = 0;
        Matrix mvp{};
        std::array<float, BoneWidth * 4> bones{};
    };
    struct State
    {
        ComPtr<IDirect3DStateBlock9> block;
        std::array<ComPtr<IDirect3DSurface9>, 4> targets;
        ComPtr<IDirect3DSurface9> depth;
        D3DVIEWPORT9 viewport{};
        IDirect3DDevice9* device;
        UINT count = 1;
        bool valid = false;
        explicit State(IDirect3DDevice9* d) : device(d)
        {
            D3DCAPS9 caps{};
            if (SUCCEEDED(d->GetDeviceCaps(&caps))) count = std::min<UINT>(4, caps.NumSimultaneousRTs);
            valid = SUCCEEDED(d->CreateStateBlock(D3DSBT_ALL, block.GetAddressOf())) &&
                SUCCEEDED(block->Capture()) && SUCCEEDED(d->GetRenderTarget(0, targets[0].GetAddressOf())) &&
                SUCCEEDED(d->GetViewport(&viewport));
            for (UINT i = 1; i < count; ++i) d->GetRenderTarget(i, targets[i].GetAddressOf());
            d->GetDepthStencilSurface(depth.GetAddressOf());
        }
        ~State()
        {
            if (!valid) return;
            // Apply first; restoring RT0 can reset the viewport.
            block->Apply();
            for (UINT i = 0; i < count; ++i) device->SetRenderTarget(i, targets[i].Get());
            device->SetDepthStencilSurface(depth.Get());
            device->SetViewport(&viewport);
        }
    };
    struct Renderer
    {
        IDirect3DDevice9* device = nullptr; // Owned by the game.
        UINT width = 0, height = 0, boneRows = 0;
        D3DFORMAT sceneFormat{};
        unsigned samples = 0;
        IDirect3DTexture9* sourceIdentity = nullptr;
#if JUICED_TAA_MSAA
        unsigned depthReplayDraws = 0;
        IDirect3DSurface9* multisampleIdentity = nullptr;
        MSAA::DepthCompanion msaaDepth;
        bool depthCleared = false, depthReplayOK = true, allocatedForceReplay = false;
#endif
        Target scene, depth, motion, opaque, history[2], historyDepth[2];
        Target glowCurrent, glowHistory[2];
        UINT glowWidth = 0, glowHeight = 0;
        D3DFORMAT glowFormat{};
        unsigned glowIndex = 0;
        bool glowValid = false, glowResolved = false, sceneReady = false;
        ComPtr<IDirect3DTexture9> bones;
        ComPtr<IDirect3DPixelShader9> cameraPS, depthPS, velocityPS, lumaPS, copyPS, resolvePS, debugPS, storeDepthPS;
        std::unordered_map<IDirect3DVertexShader9*, ShaderEntry> shaders;
        std::unordered_map<IDirect3DPixelShader9*, DitherShaderEntry> ditherShaders;
        std::unordered_map<uint64_t, PreviousDraw> previousDraws;
        std::unordered_map<uint64_t, size_t> drawIndices;
        std::vector<Draw> draws;
        Settings options{};
        Matrix currentVP{}, previousVP{}, inverseVP{};
        float jitter[2]{}, depthProjection[2]{1.0001f,-0.10001f};
        uint64_t entity = 0, frame = 0, lastTick = 0;
        unsigned historyIndex = 0, skinRows = 0, ditherDraws = 0, phase = 0;
        bool active = false, jittering = false, capturing = false, opaqueReady = false;
        bool historyValid = false, resolvedPrevious = false, overflow = false, usedHistory = false;
        bool failureLogged = false, readyLogged = false, faultUntilReset = false;
        unsigned unsupported = 0, mainDraws = 0, identifiedDraws = 0, supportedDraws = 0;

        void Release()
        {
            active = jittering = capturing = opaqueReady = historyValid = resolvedPrevious = false;
            faultUntilReset = usedHistory = false;
            GPUReady = false; ActiveSamples = 0; HistoryFrames = CapturedDraws = DitherDraws = UnsupportedShaders = MatchedDraws = FallbackDraws = 0;
#if JUICED_TAA_MSAA
            DepthReplayDraws = DepthSourceMode = 0;
#endif
            draws.clear();
            drawIndices.clear();
            previousDraws.clear();
            shaders.clear(); ditherShaders.clear(); unsupported = 0;
            scene = {}; depth = {}; motion = {}; opaque = {}; history[0] = {}; history[1] = {}; historyDepth[0] = {}; historyDepth[1] = {};
            samples = 0; sourceIdentity = nullptr;
#if JUICED_TAA_MSAA
            msaaDepth.Release(); depthReplayDraws = 0; multisampleIdentity = nullptr;
            depthCleared = false; depthReplayOK = true;
#endif
            bones.Reset();
            cameraPS.Reset(); depthPS.Reset(); velocityPS.Reset(); lumaPS.Reset(); copyPS.Reset(); resolvePS.Reset(); debugPS.Reset(); storeDepthPS.Reset();
            width = height = boneRows = 0;
            device = nullptr;
            glowCurrent = {}; glowHistory[0] = {}; glowHistory[1] = {};
            glowWidth = glowHeight = 0; glowValid = glowResolved = sceneReady = false;
            GlowFrames = MainDraws = IdentifiedDraws = SupportedDraws = 0;
        }
        bool Ensure()
        {
            auto* d = Device();
            auto* source = SceneTexture();
            auto* sourceDepth = NativeDepth();
            D3DSURFACE_DESC desc{}, depthDesc{};
            const unsigned allocatedSamples = Samples();
            if (!d || !source || FAILED(source->GetLevelDesc(0,&desc))) return false;
#if JUICED_TAA_MSAA
            if (allocatedSamples)
            {
                D3DSURFACE_DESC colorDesc{};
                if (!options.combineMSAA || !MultisampleColor() || !MultisampleDepth() ||
                    FAILED(MultisampleColor()->GetDesc(&colorDesc)) || FAILED(MultisampleDepth()->GetDesc(&depthDesc)) ||
                    colorDesc.MultiSampleType!=allocatedSamples || depthDesc.MultiSampleType!=allocatedSamples ||
                    colorDesc.Width!=desc.Width || colorDesc.Height!=desc.Height) return false;
            }
            else if (!sourceDepth || FAILED(sourceDepth->GetLevelDesc(0,&depthDesc))) return false;
#else
            if (allocatedSamples || !sourceDepth || FAILED(sourceDepth->GetLevelDesc(0,&depthDesc))) return false;
#endif
            // RAWZ packs depth into RGB; do not interpret it as standard perspective depth.
            const DWORD format = *reinterpret_cast<DWORD*>(0x0252A2F0_g);
            if (!allocatedSamples && format != MAKEFOURCC('I','N','T','Z') && format != MAKEFOURCC('D','F','2','4') &&
                format != MAKEFOURCC('D','F','1','6')) return false;
            if (!desc.Width || !desc.Height || desc.Width != depthDesc.Width || desc.Height != depthDesc.Height) return false;
            if (device == d && width == desc.Width && height == desc.Height && sceneFormat == desc.Format &&
                samples==allocatedSamples && sourceIdentity==source &&
#if JUICED_TAA_MSAA
                multisampleIdentity==MultisampleDepth() && allocatedForceReplay==options.forceDepthReplay &&
#endif
                resolvePS) return true;
            Release();
            device = d; width = desc.Width; height = desc.Height; sceneFormat = desc.Format;
            samples=allocatedSamples; sourceIdentity=source;
#if JUICED_TAA_MSAA
            allocatedForceReplay=options.forceDepthReplay; multisampleIdentity=MultisampleDepth();
#endif
            D3DCAPS9 caps{};
            if (FAILED(d->GetDeviceCaps(&caps)) || caps.VertexShaderVersion < D3DVS_VERSION(3,0) ||
                caps.PixelShaderVersion < D3DPS_VERSION(3,0) || caps.MaxVertexShaderConst < 256) return false;
            boneRows = std::min<UINT>(1024, caps.MaxTextureHeight);
            bool ok =
#if JUICED_TAA_MSAA
                (!samples || msaaDepth.Create(d,width,height,options.forceDepthReplay)) &&
#endif
                scene.Create(d, width, height, desc.Format) &&
                depth.Create(d, width, height, D3DFMT_R32F) &&
                motion.Create(d, width, height, D3DFMT_A16B16G16R16F) &&
                opaque.Create(d, width, height, D3DFMT_R16F) &&
                history[0].Create(d, width, height, D3DFMT_A16B16G16R16F) &&
                history[1].Create(d, width, height, D3DFMT_A16B16G16R16F) &&
                historyDepth[0].Create(d, width, height, D3DFMT_R16F) &&
                historyDepth[1].Create(d, width, height, D3DFMT_R16F) &&
                SUCCEEDED(d->CreateTexture(BoneWidth, boneRows, 1, D3DUSAGE_DYNAMIC,
                    D3DFMT_A32B32G32R32F, D3DPOOL_DEFAULT, bones.GetAddressOf(), nullptr)) &&
                SUCCEEDED(d->CreatePixelShader(reinterpret_cast<const DWORD*>(Bytecode::PS_CameraMotion), cameraPS.GetAddressOf())) &&
                SUCCEEDED(d->CreatePixelShader(reinterpret_cast<const DWORD*>(Bytecode::PS_CopyDepth), depthPS.GetAddressOf())) &&
                SUCCEEDED(d->CreatePixelShader(reinterpret_cast<const DWORD*>(Bytecode::PS_Velocity), velocityPS.GetAddressOf())) &&
                SUCCEEDED(d->CreatePixelShader(reinterpret_cast<const DWORD*>(Bytecode::PS_OpaqueLuma), lumaPS.GetAddressOf())) &&
                SUCCEEDED(d->CreatePixelShader(reinterpret_cast<const DWORD*>(Bytecode::PS_CopyColor), copyPS.GetAddressOf())) &&
                SUCCEEDED(d->CreatePixelShader(reinterpret_cast<const DWORD*>(Bytecode::PS_TemporalResolve), resolvePS.GetAddressOf())) &&
                SUCCEEDED(d->CreatePixelShader(reinterpret_cast<const DWORD*>(Bytecode::PS_Debug), debugPS.GetAddressOf())) &&
                SUCCEEDED(d->CreatePixelShader(reinterpret_cast<const DWORD*>(Bytecode::PS_StoreHistoryDepth), storeDepthPS.GetAddressOf()));
            // A successful texture allocation alone does not establish vertex texture support.
            ComPtr<IDirect3D9> api;
            D3DDEVICE_CREATION_PARAMETERS creation{};
            D3DDISPLAYMODE display{};
            ok = ok && SUCCEEDED(d->GetDirect3D(api.GetAddressOf())) &&
                SUCCEEDED(d->GetCreationParameters(&creation)) &&
                SUCCEEDED(api->GetAdapterDisplayMode(creation.AdapterOrdinal, &display)) &&
                SUCCEEDED(api->CheckDeviceFormat(creation.AdapterOrdinal, creation.DeviceType,
                    display.Format, D3DUSAGE_QUERY_VERTEXTEXTURE, D3DRTYPE_TEXTURE, D3DFMT_A32B32G32R32F));
            if (!ok)
            {
                Release();
                if (!failureLogged) Logger::TypedLog(CHN_DEBUG, "TAA: required SM3/depth/float render targets/vertex textures unavailable; using native AA.\n");
                failureLogged = true;
                faultUntilReset = true;
                return false;
            }
            if (!readyLogged) Logger::TypedLog(CHN_DEBUG, "TAA: native D3D9 ready at {}x{} ({} native MSAA samples); effective loose-file shaders retained.\n", width, height, samples);
            readyLogged = true;
            return true;
        }
        void Begin(const Packet& packet)
        {
            const bool changed = options.revision != packet.settings.revision;
            options = packet.settings;
            if (changed) { historyValid = glowValid = false; previousDraws.clear(); faultUntilReset = false; }
            const bool prior = resolvedPrevious;
            active = jittering = capturing = opaqueReady = false;
            resolvedPrevious = usedHistory = sceneReady = glowResolved = false;
            MatchedDraws = FallbackDraws = 0;
            mainDraws = identifiedDraws = supportedDraws = 0;
            draws.clear(); drawIndices.clear(); skinRows = ditherDraws = 0; entity = 0; overflow = false;
#if JUICED_TAA_MSAA
            depthCleared=false; depthReplayOK=true; depthReplayDraws=0; DepthReplayDraws=0;
#endif
            if (!options.enabled || !AllowsSampleMode(options))
            {
                historyValid = glowValid = false; HistoryFrames = GlowFrames = ActiveSamples = 0; GPUReady = false;
#if JUICED_TAA_MSAA
                DepthSourceMode = 0;
#endif
                RenderStatus = options.enabled ? Status::MSAA : Status::Disabled;
                return;
            }
            if (device!=Device() || samples!=static_cast<unsigned>(Samples()) || sourceIdentity!=SceneTexture()
#if JUICED_TAA_MSAA
                || multisampleIdentity!=MultisampleDepth()
#endif
                ) faultUntilReset=false;
            if (faultUntilReset || !Ensure())
            {
                historyValid = false; HistoryFrames = 0; GPUReady = false;
                RenderStatus = faultUntilReset ? Status::Fault : Status::Unsupported;
                return;
            }
            ActiveSamples=samples;
#if JUICED_TAA_MSAA
            DepthSourceMode=samples ? (msaaDepth.UsesHardwareResolve() ? 2 : 1) : 0;
#endif
            RenderStatus = Status::Active;
            currentVP = packet.viewProjection;
            if (!Inverse(currentVP, inverseVP)) { historyValid = false; GPUReady = false; HistoryFrames = 0; RenderStatus = Status::Unsupported; return; }
            const uint64_t now = GetTickCount64();
            historyValid = historyValid && prior && now - lastTick < 250;
            Matrix previousInverse{};
            if (historyValid && Inverse(previousVP, previousInverse))
            {
                float distance = 0;
                for (unsigned c = 0; c < 3; ++c)
                {
                    const float delta = inverseVP[12 + c] / inverseVP[15] - previousInverse[12 + c] / previousInverse[15];
                    distance += delta * delta;
                }
                if (!std::isfinite(distance) || distance > 25 * 25) historyValid = false;
                // A large rotation/FOV change also invalidates the temporal camera.
                float change = 0, scale = 0;
                for (unsigned i = 0; i < 12; ++i)
                {
                    change += std::abs(currentVP[i] - previousVP[i]);
                    scale += std::abs(previousVP[i]);
                }
                if (change > std::max(0.5f, scale * 0.4f)) historyValid = false;
            }
            glowValid = glowValid && historyValid;
            lastTick = now;
            GPUReady = true;
            phase = packet.phase;
            depthProjection[0]=packet.depthProjection[0]; depthProjection[1]=packet.depthProjection[1];
            jitter[0] = packet.jitter[0]; jitter[1] = packet.jitter[1];
            ++frame;
            std::erase_if(previousDraws, [this](const auto& item) { return item.second.frame + 1 < frame; });
            active = jittering = true;
            capturing = false; // The earlier low-resolution prepass is not an object-motion source.
        }
        void Sampler(UINT index, D3DTEXTUREFILTERTYPE filter)
        {
            device->SetSamplerState(index, D3DSAMP_MINFILTER, filter);
            device->SetSamplerState(index, D3DSAMP_MAGFILTER, filter);
            device->SetSamplerState(index, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
            device->SetSamplerState(index, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
            device->SetSamplerState(index, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
            device->SetSamplerState(index, D3DSAMP_SRGBTEXTURE, FALSE);
        }
        void Setup()
        {
            D3DCAPS9 caps{};
            device->GetDeviceCaps(&caps);
            for (UINT i = 1; i < std::min<UINT>(4, caps.NumSimultaneousRTs); ++i) device->SetRenderTarget(i, nullptr);
            device->SetDepthStencilSurface(nullptr);
            for (UINT i = 0; i < 16; ++i) device->SetTexture(i, nullptr);
            for (UINT i = 0; i < 4; ++i) device->SetTexture(D3DVERTEXTEXTURESAMPLER0 + i, nullptr);
            device->SetRenderState(D3DRS_ZENABLE, FALSE);
            device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
            device->SetRenderState(D3DRS_STENCILENABLE, FALSE);
            device->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
            device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
            device->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, FALSE);
            device->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
            device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
            device->SetRenderState(D3DRS_COLORWRITEENABLE, 15);
            device->SetRenderState(D3DRS_SRGBWRITEENABLE, FALSE);
            device->SetRenderState(D3DRS_FOGENABLE, FALSE);
            // Single-sample targets already determine the pass's sample count.
            // Preserve native MULTISAMPLEANTIALIAS for MSAA color passes that resume.
            device->SetRenderState(D3DRS_CLIPPLANEENABLE, 0);
        }
        HRESULT Fullscreen(IDirect3DSurface9* target, IDirect3DPixelShader9* shader, UINT outputWidth = 0, UINT outputHeight = 0)
        {
            if (!outputWidth) outputWidth = width;
            if (!outputHeight) outputHeight = height;
            struct Vertex { float x,y,z,rhw,u,v; };
            const Vertex vertices[] = {{-0.5f,-0.5f,0,1,0,0}, {outputWidth-0.5f,-0.5f,0,1,1,0},
                {-0.5f,outputHeight-0.5f,0,1,0,1}, {outputWidth-0.5f,outputHeight-0.5f,0,1,1,1}};
            HRESULT hr = device->SetRenderTarget(0, target);
            if (FAILED(hr)) return hr;
            D3DVIEWPORT9 viewport{0,0,outputWidth,outputHeight,0,1};
            device->SetViewport(&viewport);
            device->SetVertexShader(nullptr);
            device->SetFVF(D3DFVF_XYZRHW | D3DFVF_TEX1);
            device->SetStreamSourceFreq(0, 1);
            device->SetPixelShader(shader);
            return device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, vertices, sizeof(Vertex));
        }
        ShaderEntry* FindShader()
        {
            ComPtr<IDirect3DVertexShader9> shader;
            if (FAILED(device->GetVertexShader(shader.GetAddressOf())) || !shader) return nullptr;
            auto [it, inserted] = shaders.try_emplace(shader.Get());
            auto& entry = it->second;
            if (inserted)
            {
                entry.native = shader;
                UINT bytes = 0;
                if (SUCCEEDED(shader->GetFunction(nullptr, &bytes)) && bytes && bytes % 4 == 0 && bytes < 256 * 1024)
                {
                    std::vector<uint32_t> code(bytes / 4);
                    if (SUCCEEDED(shader->GetFunction(code.data(), &bytes)))
                    {
                        auto program = Shader::MakeVelocity(code);
                        if (!program.tokens.empty() && SUCCEEDED(device->CreateVertexShader(reinterpret_cast<const DWORD*>(program.tokens.data()), entry.velocity.GetAddressOf())))
                            entry.skinned = program.skinned;
                    }
                }
                if (!entry.velocity)
                {
                    ++unsupported;
                    UINT bytes=0; DWORD version=0;
                    if (SUCCEEDED(shader->GetFunction(nullptr,&bytes)) && bytes>=4)
                    {
                        std::vector<DWORD> code(bytes/4);
                        if (SUCCEEDED(shader->GetFunction(code.data(),&bytes))) version=code[0];
                    }
                    if (unsupported<=64) Logger::TypedLog(CHN_DEBUG, "TAA: unsupported native motion VS {} (version {:#x}, bytes {}).\n", unsupported, version, bytes);
                }
            }
            return entry.velocity ? &entry : nullptr;
        }
        bool MainViewport()
        {
            D3DVIEWPORT9 viewport{};
            ComPtr<IDirect3DSurface9> target;
            D3DSURFACE_DESC desc{};
            return SUCCEEDED(device->GetViewport(&viewport)) && viewport.X == 0 && viewport.Y == 0 &&
                viewport.Width == width && viewport.Height == height &&
                SUCCEEDED(device->GetRenderTarget(0, target.GetAddressOf())) &&
                SUCCEEDED(target->GetDesc(&desc)) && desc.Width == width && desc.Height == height &&
                (desc.MultiSampleType == D3DMULTISAMPLE_NONE
#if JUICED_TAA_MSAA
                || (samples && desc.MultiSampleType==samples)
#endif
                );
        }
        void Unjitter(Matrix& mvp)
        {
            Projection::ShiftConstants(mvp,-2*jitter[0]/width,2*jitter[1]/height);
        }
        DitherShaderEntry* FindDitherShader()
        {
            // Native MSAA hair/foliage keeps alpha-to-coverage. Its fallback Bayer
            // variant is phased only when native coverage is unavailable.
            if (!active || !jittering || !options.temporalDither) return nullptr;
#if JUICED_TAA_MSAA
            if (samples && NativeCoverage().available) return nullptr;
#endif
            ComPtr<IDirect3DPixelShader9> shader;
            if (FAILED(device->GetPixelShader(shader.GetAddressOf())) || !shader) return nullptr;
            auto [it, inserted]=ditherShaders.try_emplace(shader.Get());
            auto& entry=it->second;
            if (inserted)
            {
                entry.native=shader;
                UINT bytes=0;
                if (SUCCEEDED(shader->GetFunction(nullptr,&bytes)) && bytes && bytes%4==0 && bytes<256*1024)
                {
                    std::vector<uint32_t> code(bytes/4);
                    if (SUCCEEDED(shader->GetFunction(code.data(),&bytes)))
                    {
                        const auto temporal=Shader::MakeTemporalDither(code);
                        if (!temporal.empty())
                            device->CreatePixelShader(reinterpret_cast<const DWORD*>(temporal.data()),entry.temporal.GetAddressOf());
                    }
                }
            }
            return entry.temporal ? &entry : nullptr;
        }
        // Save/restore the actual device state around the draw so SR2's cached
        // shader/constant state continues to describe the native program.
        struct DitherDraw
        {
            Renderer& renderer;
            DitherShaderEntry* shader = nullptr;
            float original[4]{};
            explicit DitherDraw(Renderer& r) : renderer(r)
            {
                auto* candidate=r.FindDitherShader();
                if (!candidate || FAILED(r.device->GetPixelShaderConstantF(190,original,1))) return;
                const float offset[4]{float(r.phase&1),float((r.phase>>1)&1),0,0};
                if (SUCCEEDED(r.device->SetPixelShaderConstantF(190,offset,1)) &&
                    SUCCEEDED(r.device->SetPixelShader(candidate->temporal.Get())))
                {
                    shader=candidate; ++r.ditherDraws;
                }
                else r.device->SetPixelShaderConstantF(190,original,1);
            }
            ~DitherDraw()
            {
                if (!shader) return;
                renderer.device->SetPixelShader(shader->native.Get());
                renderer.device->SetPixelShaderConstantF(190,original,1);
            }
        };
        void Capture(ShaderEntry* shader, const uint32_t* args)
        {
            if (!options.objectMotion || !capturing || !entity || !shader || overflow) return;
            Draw draw;
            draw.shader = shader;
            draw.type = static_cast<D3DPRIMITIVETYPE>(args[0]); draw.base = static_cast<INT>(args[1]);
            draw.minimum = args[2]; draw.vertices = args[3]; draw.start = args[4]; draw.primitives = args[5];
            if (FAILED(device->GetVertexDeclaration(draw.declaration.GetAddressOf())) || !draw.declaration ||
                FAILED(device->GetIndices(draw.indices.GetAddressOf())) || !draw.indices ||
                FAILED(device->GetVertexShaderConstantF(0, draw.constants.data(), 248))) return;
            device->GetRenderState(D3DRS_CULLMODE, &draw.cull);
            for (UINT i = 0; i < draw.streams.size(); ++i)
            {
                auto& stream = draw.streams[i];
                device->GetStreamSource(i, stream.buffer.GetAddressOf(), &stream.offset, &stream.stride);
                device->GetStreamSourceFreq(i, &stream.frequency);
            }
            // Exclude dynamic VB offsets/base vertices; those are reallocated every frame.
            draw.key = Combine(entity, reinterpret_cast<uintptr_t>(draw.indices.Get()));
            draw.key = Combine(draw.key, reinterpret_cast<uintptr_t>(shader->native.Get()));
            D3DVERTEXELEMENT9 elements[MAXD3DDECLLENGTH+1]{};
            UINT elementCount = MAXD3DDECLLENGTH+1;
            if (FAILED(draw.declaration->GetDeclaration(elements,&elementCount))) return;
            for (UINT i=0; i<elementCount; ++i)
            {
                const auto& e=elements[i];
                const uint64_t identity=uint64_t(e.Stream) | (uint64_t(e.Offset)<<16) |
                    (uint64_t(e.Type)<<32) | (uint64_t(e.Method)<<40) |
                    (uint64_t(e.Usage)<<48) | (uint64_t(e.UsageIndex)<<56);
                draw.key=Combine(draw.key,identity);
            }
            for (const auto& stream : draw.streams)
                draw.key=Combine(draw.key,(uint64_t(stream.stride)<<32)|stream.frequency);
            draw.key = Combine(draw.key, draw.type);
            draw.key = Combine(draw.key, (uint64_t(draw.start) << 32) | draw.primitives);
            draw.key = Combine(draw.key, draw.vertices);
            const auto old = previousDraws.find(draw.key);
            draw.valid = historyValid && old != previousDraws.end() && old->second.frame + 1 == frame;
            memcpy(draw.previousMVP.data(), draw.constants.data() + 16, sizeof(Matrix));
            Unjitter(draw.previousMVP);
            memcpy(draw.previousBones.data(), draw.constants.data() + 52 * 4, sizeof(draw.previousBones));
            if (draw.valid)
            {
                draw.previousMVP = old->second.mvp;
                draw.previousBones = old->second.bones;
            }
            auto existing = drawIndices.find(draw.key);
            if (existing != drawIndices.end())
            {
                draw.boneRow = draws[existing->second].boneRow;
                draws[existing->second] = std::move(draw);
            }
            else
            {
                if (draws.size() >= MaxDraws || (shader->skinned && skinRows >= boneRows)) { overflow = true; return; }
                if (shader->skinned) draw.boneRow = skinRows++;
                drawIndices.emplace(draw.key, draws.size());
                draws.emplace_back(std::move(draw));
            }
        }
#if JUICED_TAA_MSAA
        bool CompanionTarget()
        {
            if (!samples || msaaDepth.UsesHardwareResolve() || !capturing || !MainViewport()) return false;
            ComPtr<IDirect3DSurface9> bound;
            return SUCCEEDED(device->GetDepthStencilSurface(bound.GetAddressOf())) && bound.Get()==multisampleIdentity;
        }
        template<class DrawCall> void ReplayDepth(DrawCall&& draw)
        {
            if (!CompanionTarget()) return;
            DWORD z=0,write=0,stencil=0;
            device->GetRenderState(D3DRS_ZENABLE,&z);
            device->GetRenderState(D3DRS_ZWRITEENABLE,&write);
            device->GetRenderState(D3DRS_STENCILENABLE,&stencil);
            if (!(z && write) && !stencil) return;
            depthReplayOK = msaaDepth.Replay(device,NativeCoverage(),draw) && depthReplayOK;
            ++depthReplayDraws;
        }
        HRESULT Clear(const uint32_t* args)
        {
            const DWORD count=args[0];
            const auto* rects=reinterpret_cast<const D3DRECT*>(args+1);
            const auto* values=args+1+count*4;
            float z=0; memcpy(&z,values+2,sizeof(z));
            const auto hr=device->Clear(count,count ? rects : nullptr,values[0],values[1],z,values[3]);
            if (SUCCEEDED(hr) && CompanionTarget())
            {
                depthReplayOK = msaaDepth.Clear(device,count,count ? rects : nullptr,values[0],z,values[3]) && depthReplayOK;
                if ((values[0]&D3DCLEAR_ZBUFFER) && (!count || (count==1 && rects[0].x1<=0 && rects[0].y1<=0 &&
                    rects[0].x2>=static_cast<LONG>(width) && rects[0].y2>=static_cast<LONG>(height)))) depthCleared=true;
            }
            return hr;
        }
#endif
        HRESULT DrawIndexed(const uint32_t* args)
        {
            if (active && capturing && MainViewport())
            {
                Matrix mvp{};
                if (SUCCEEDED(device->GetVertexShaderConstantF(4,mvp.data(),4)) &&
                    std::abs(mvp[12])+std::abs(mvp[13])+std::abs(mvp[14])>1e-6f)
                {
                    ++mainDraws;
                    if (entity) ++identifiedDraws;
                    auto* shader=FindShader();
                    if (shader) ++supportedDraws;
                    Capture(shader,args);
                }
            }
            DitherDraw dither(*this);
            const auto draw=[&]() { return device->DrawIndexedPrimitive(static_cast<D3DPRIMITIVETYPE>(args[0]),
                static_cast<INT>(args[1]),args[2],args[3],args[4],args[5]); };
            const auto hr=draw();
#if JUICED_TAA_MSAA
            // Native first: switching to a single-sample attachment before the
            // native target's first draw loses MSAA coverage on the installed DXVK.
            if (SUCCEEDED(hr)) ReplayDepth(draw);
#endif
            if (FAILED(hr)) historyValid=false;
            return hr;
        }
        HRESULT DrawPrimitive(const uint32_t* args)
        {
            DitherDraw dither(*this);
            const auto draw=[&]() { return device->DrawPrimitive(static_cast<D3DPRIMITIVETYPE>(args[0]),args[1],args[2]); };
            const auto hr=draw();
#if JUICED_TAA_MSAA
            // Native first: switching to a single-sample attachment before the
            // native target's first draw loses MSAA coverage on the installed DXVK.
            if (SUCCEEDED(hr)) ReplayDepth(draw);
#endif
            if (FAILED(hr)) historyValid=false;
            return hr;
        }
#if JUICED_TAA_MSAA
        static UINT PrimitiveVertices(D3DPRIMITIVETYPE type, UINT primitives)
        {
            switch (type)
            {
            case D3DPT_POINTLIST: return primitives;
            case D3DPT_LINELIST: return primitives*2;
            case D3DPT_LINESTRIP: case D3DPT_TRIANGLEFAN: return primitives+1; // retail CF5900 stream cursor
            case D3DPT_TRIANGLELIST: return primitives*3;
            case D3DPT_TRIANGLESTRIP: return primitives+2;
            default: return 0;
            }
        }
        HRESULT DrawPrimitiveUP(const uint32_t* args)
        {
            DitherDraw dither(*this);
            const auto draw=[&]() { return device->DrawPrimitiveUP(static_cast<D3DPRIMITIVETYPE>(args[0]),args[1],args+3,args[2]); };
            const auto hr=draw();
            // Native first: switching to a single-sample attachment before the
            // native target's first draw loses MSAA coverage on the installed DXVK.
            if (SUCCEEDED(hr)) ReplayDepth(draw);
            if (FAILED(hr)) historyValid=false;
            return hr;
        }
#endif
        void Opaque()
        {
            if (!active || !MainViewport()) return;
            State saved(device);
            if (!saved.valid) return;
            Setup();
            bool ok = SUCCEEDED(device->StretchRect(saved.targets[0].Get(), nullptr, scene.surface.Get(), nullptr, D3DTEXF_NONE));
            Sampler(0,D3DTEXF_POINT);
            device->SetTexture(0,scene.texture.Get());
            opaqueReady = ok && SUCCEEDED(Fullscreen(opaque.surface.Get(),lumaPS.Get()));
        }
        void FinalizeScene()
        {
            // Alpha, isolated player and glare have finished. Capture their depth
            // before create_supp_texture clears it for distortion/emissive glow.
            capturing = false;
            if (!active || !MainViewport()) return;
            State saved(device);
            if (!saved.valid) { historyValid=false; return; }
            bool depthOK=true;
            auto* sourceDepth=NativeDepth();
#if JUICED_TAA_MSAA
            depthOK=!samples || (msaaDepth.UsesHardwareResolve() ? msaaDepth.ResolveNative(device) : depthCleared && depthReplayOK);
            if (samples) sourceDepth=msaaDepth.Texture();
            DepthReplayDraws=depthReplayDraws;
#endif
            Setup();
            Sampler(0,D3DTEXF_POINT);
            device->SetTexture(0,sourceDepth);
            bool ok=depthOK && SUCCEEDED(Fullscreen(depth.surface.Get(),depthPS.Get()));
            Sampler(0, D3DTEXF_POINT);
            device->SetTexture(0, depth.texture.Get());
            float params[4]{2*jitter[0]/width,-2*jitter[1]/height,1.0f/width,-1.0f/height};
            device->SetPixelShaderConstantF(0, params, 1);
            Matrix reprojection{};
            ok = ok && Projection::Reproject(currentVP, historyValid ? previousVP : currentVP, reprojection);
            device->SetPixelShaderConstantF(1, reprojection.data(), 4);
            const float cameraDepth[4]{depthProjection[0],depthProjection[1],0,0};
            device->SetPixelShaderConstantF(5,cameraDepth,1);
            ok = ok && SUCCEEDED(Fullscreen(motion.surface.Get(), cameraPS.Get()));
            D3DLOCKED_RECT locked{};
            if (skinRows)
            {
                if (SUCCEEDED(bones->LockRect(0, &locked, nullptr, D3DLOCK_DISCARD)))
                {
                    for (const auto& draw : draws)
                        if (draw.shader->skinned)
                            memcpy(static_cast<char*>(locked.pBits) + draw.boneRow * locked.Pitch,
                                draw.previousBones.data(), sizeof(draw.previousBones));
                    ok = ok && SUCCEEDED(bones->UnlockRect(0));
                }
                else ok = false;
            }
            device->SetPixelShader(velocityPS.Get());
            device->SetTexture(D3DVERTEXTEXTURESAMPLER0, bones.Get());
            Sampler(D3DVERTEXTEXTURESAMPLER0, D3DTEXF_POINT);
            const float offsets[4]{0.5f/BoneWidth,1.5f/BoneWidth,2.5f/BoneWidth,0};
            const float undoJitter[4]{-2*jitter[0]/width,2*jitter[1]/height,0,0};
            device->SetVertexShaderConstantF(253, offsets, 1);
            device->SetVertexShaderConstantF(254, undoJitter, 1);
            unsigned matched=0, fallback=0;
            for (const auto& draw : draws)
            {
                PreviousDraw previous;
                previous.frame = frame;
                memcpy(previous.mvp.data(), draw.constants.data()+16, sizeof(Matrix));
                Unjitter(previous.mvp);
                memcpy(previous.bones.data(), draw.constants.data()+52*4, sizeof(previous.bones));
                previousDraws[draw.key] = std::move(previous);
                // A new/rotating mesh has no object transform history. Keep the
                // camera reprojection already written for it instead of replacing
                // it with an invalid zero vector that forces current-only resolve.
                if (!draw.valid) { ++fallback; continue; }
                ++matched;
                device->SetVertexShader(draw.shader->velocity.Get());
                device->SetVertexDeclaration(draw.declaration.Get());
                device->SetIndices(draw.indices.Get());
                for (UINT i = 0; i < draw.streams.size(); ++i)
                {
                    const auto& stream = draw.streams[i];
                    device->SetStreamSource(i, stream.buffer.Get(), stream.offset, stream.stride);
                    device->SetStreamSourceFreq(i, stream.frequency);
                }
                device->SetRenderState(D3DRS_CULLMODE, draw.cull);
                device->SetVertexShaderConstantF(0, draw.constants.data(), 248);
                // The native shared projection already jittered c4-c7.
                device->SetVertexShaderConstantF(248, draw.previousMVP.data(), 4);
                const float boneParams[4]{1.0f/BoneWidth,(draw.boneRow+0.5f)/boneRows,0,0};
                device->SetVertexShaderConstantF(252, boneParams, 1);
                float depthTolerance=0.00001f;
#if JUICED_TAA_MSAA
                if (samples && msaaDepth.UsesHardwareResolve()) depthTolerance=-depthTolerance;
#endif
                const float velocityParams[4]{1.0f/width,1.0f/height,draw.valid ? 1.0f : 0.0f,depthTolerance};
                device->SetPixelShaderConstantF(0, velocityParams, 1);
                ok = ok && SUCCEEDED(device->DrawIndexedPrimitive(draw.type, draw.base, draw.minimum, draw.vertices, draw.start, draw.primitives));
            }
            MatchedDraws = matched; FallbackDraws = fallback;
            CapturedDraws = static_cast<unsigned>(draws.size()); UnsupportedShaders = unsupported;
            sceneReady = ok;
            MainDraws = mainDraws; IdentifiedDraws = identifiedDraws; SupportedDraws = supportedDraws;
            if (!ok || overflow) historyValid = false;
            if (!ok)
            {
                faultUntilReset = true; GPUReady = false; RenderStatus = Status::Fault;
                Logger::TypedLog(CHN_DEBUG, "TAA: final scene depth/motion pass failed; disabling until render targets reset.\n");
            }
            if (frame == 1) Logger::TypedLog(CHN_DEBUG, "TAA: captured {} scene draws ({} skin rows), {} unsupported shader variants.\n", draws.size(), skinRows, unsupported);
            draws.clear(); drawIndices.clear();
        }
        bool ResolveGlow(bool available)
        {
            glowResolved = false;
            if (!available || !options.temporalGlow || !sceneReady) { glowValid=false; GlowFrames=0; return true; }
            auto* source=GlowTexture();
            D3DSURFACE_DESC desc{};
            ComPtr<IDirect3DSurface9> surface;
            if (!source || FAILED(source->GetLevelDesc(0,&desc)) || !desc.Width || !desc.Height ||
                FAILED(source->GetSurfaceLevel(0,surface.GetAddressOf()))) return false;
            if (!glowCurrent.texture || glowWidth!=desc.Width || glowHeight!=desc.Height || glowFormat!=desc.Format)
            {
                glowValid=false; GlowFrames=0;
                glowCurrent={}; glowHistory[0]={}; glowHistory[1]={};
                glowWidth=desc.Width; glowHeight=desc.Height; glowFormat=desc.Format;
                if (!glowCurrent.Create(device,glowWidth,glowHeight,desc.Format) ||
                    !glowHistory[0].Create(device,glowWidth,glowHeight,D3DFMT_A16B16G16R16F) ||
                    !glowHistory[1].Create(device,glowWidth,glowHeight,D3DFMT_A16B16G16R16F)) return false;
            }
            Setup();
            bool ok=SUCCEEDED(device->StretchRect(surface.Get(),nullptr,glowCurrent.surface.Get(),nullptr,D3DTEXF_NONE));
            const unsigned next=glowIndex^1;
            const bool accumulated=glowValid && historyValid && options.accumulation && !overflow;
            device->SetTexture(0,glowCurrent.texture.Get());
            device->SetTexture(1,glowValid ? glowHistory[glowIndex].texture.Get() : glowCurrent.texture.Get());
            device->SetTexture(2,motion.texture.Get()); device->SetTexture(3,depth.texture.Get());
            device->SetTexture(4,opaque.texture.Get()); device->SetTexture(5,depth.texture.Get());
            for (UINT i=0; i<6; ++i) Sampler(i,i==1 ? D3DTEXF_LINEAR : D3DTEXF_POINT);
            // The shared NDC projection is identical; convert its displacement to
            // this target's pixels if the game's glow resolution differs.
            const float texel[4]{1.f/glowWidth,1.f/glowHeight,float(glowWidth),float(glowHeight)};
            const float taa[4]{jitter[0]*glowWidth/width,jitter[1]*glowHeight/height,accumulated ? 1.f : 0.f,options.gamma};
            const float blend[4]{options.minWeight,options.maxWeight,options.motionScale,options.lumaWeight};
            const float reactive[4]{};
            // Glow is its own material pass; scene luma/depth rejection does not
            // represent its coverage. Color variance clipping still applies.
            const float reject[4]{depthProjection[0],depthProjection[1],.04f,0};
            device->SetPixelShaderConstantF(0,texel,1); device->SetPixelShaderConstantF(1,taa,1);
            device->SetPixelShaderConstantF(2,blend,1); device->SetPixelShaderConstantF(3,reactive,1);
            device->SetPixelShaderConstantF(4,reject,1);
            ok=ok && SUCCEEDED(Fullscreen(glowHistory[next].surface.Get(),resolvePS.Get(),glowWidth,glowHeight));
            for (UINT i=0; i<6; ++i) device->SetTexture(i,nullptr);
            device->SetTexture(0,glowHistory[next].texture.Get());
            Sampler(0,D3DTEXF_POINT);
            const float copy[4]{1,0,0,0}; device->SetPixelShaderConstantF(0,copy,1);
            ok=ok && SUCCEEDED(Fullscreen(surface.Get(),copyPS.Get(),glowWidth,glowHeight));
            if (ok)
            {
                GlowFrames=accumulated ? GlowFrames.load()+1 : 0;
                glowIndex=next; glowValid=glowResolved=true;
            }
            else glowValid=false;
            return ok;
        }
        void Resolve(bool glowAvailable)
        {
            jittering = capturing = false;
            // msaa_surfaces_end may already allocate next frame's sample setting.
            // Resolve this frame using the allocation captured in Begin.
            if (!active) { historyValid = resolvedPrevious = active = false; return; }
            auto* source = SceneTexture();
            ComPtr<IDirect3DSurface9> surface;
            if (!source || FAILED(source->GetSurfaceLevel(0, surface.GetAddressOf()))) { historyValid = active = false; return; }
            State saved(device);
            if (!saved.valid) { historyValid = active = false; return; }
            Setup();
            bool ok = SUCCEEDED(device->StretchRect(surface.Get(), nullptr, scene.surface.Get(), nullptr, D3DTEXF_NONE));
            const unsigned next = historyIndex ^ 1;
            // Never bind an uninitialized history texture. gTaa.z still gates accumulation.
            device->SetTexture(0, scene.texture.Get());
            device->SetTexture(1, historyValid ? history[historyIndex].texture.Get() : scene.texture.Get());
            // A failed opaque pass still receives a current-only Gaussian resolve to
            // remove this frame's jitter; future frames bypass TAA until reset.
            if (!sceneReady) historyValid = false;
            device->SetTexture(2, sceneReady ? motion.texture.Get() : scene.texture.Get());
            device->SetTexture(3, sceneReady ? depth.texture.Get() : scene.texture.Get());
            device->SetTexture(4, opaqueReady ? opaque.texture.Get() : scene.texture.Get());
            device->SetTexture(5, historyValid && options.depthRejection ? historyDepth[historyIndex].texture.Get() : depth.texture.Get());
            for (UINT i = 0; i < 6; ++i) Sampler(i, i == 1 ? D3DTEXF_LINEAR : D3DTEXF_POINT);
            const float texel[4]{1.0f/width,1.0f/height,float(width),float(height)};
            const bool accumulated=historyValid && !overflow && options.accumulation;
            const float taa[4]{jitter[0],jitter[1],accumulated ? 1.0f : 0.0f,options.gamma};
            const float blend[4]{options.minWeight,options.maxWeight,options.motionScale,options.lumaWeight};
            const float reactive[4]{options.reactiveScale,options.reactiveMax,options.reactive && opaqueReady ? 1.0f : 0.0f,0};
            device->SetPixelShaderConstantF(0, texel, 1);
            device->SetPixelShaderConstantF(1, taa, 1);
            device->SetPixelShaderConstantF(2, blend, 1);
            device->SetPixelShaderConstantF(3, reactive, 1);
            const float depthReject[4]{depthProjection[0],depthProjection[1],0.04f,options.depthRejection ? 1.0f : 0.0f};
            device->SetPixelShaderConstantF(4,depthReject,1);
            ok = ok && SUCCEEDED(Fullscreen(history[next].surface.Get(), resolvePS.Get()));
            for (UINT i=0; i<6; ++i) device->SetTexture(i,nullptr);
            // Independent metadata leaves scene alpha available for native bloom.
            if (options.depthRejection && sceneReady)
            {
                device->SetTexture(0,depth.texture.Get());
                device->SetPixelShaderConstantF(0,depthReject,1);
                device->SetPixelShaderConstantF(1,texel,1);
                ok = ok && SUCCEEDED(Fullscreen(historyDepth[next].surface.Get(),storeDepthPS.Get()));
            }
            device->SetTexture(0, history[next].texture.Get());
            device->SetTexture(1, scene.texture.Get());
            for (UINT i = 2; i < 6; ++i) device->SetTexture(i, nullptr);
            Sampler(0, D3DTEXF_POINT); Sampler(1, D3DTEXF_POINT);
            const float copyAlpha[4]{options.temporalBloom ? 1.0f : 0.0f,0,0,0};
            device->SetPixelShaderConstantF(0,copyAlpha,1);
            // Preserve/filter native alpha for HDR bloom weighting, independently of emissive glow.
            ok = ok && SUCCEEDED(Fullscreen(surface.Get(), copyPS.Get()));
            // Native HDR blur/composite also samples the original sharp glow texture.
            ok = ok && ResolveGlow(glowAvailable);
            if (ok)
            {
                HistoryFrames = accumulated ? HistoryFrames.load()+1 : 0;
                usedHistory = accumulated;
                historyIndex = next;
                previousVP = currentVP;
                historyValid = resolvedPrevious = true;
            }
            else
            {
                historyValid = resolvedPrevious = usedHistory = false;
                faultUntilReset = true; GPUReady = false; RenderStatus = Status::Fault;
                Logger::TypedLog(CHN_DEBUG, "TAA: scene resolve failed; disabling until render targets reset.\n");
            }
            DitherDraws = ditherDraws;
            active = false;
        }
        void Debug()
        {
            if (!options.debugMode || !resolvedPrevious || !sceneReady) return;
            State saved(device);
            D3DSURFACE_DESC desc{};
            if (!saved.valid || FAILED(saved.targets[0]->GetDesc(&desc))) return;
            Setup();
            device->SetTexture(0, motion.texture.Get()); device->SetTexture(1, depth.texture.Get());
            device->SetTexture(2, scene.texture.Get()); device->SetTexture(3, opaque.texture.Get());
            device->SetTexture(4, options.debugMode==6 ? (glowResolved ? glowHistory[glowIndex].texture.Get() : nullptr) : history[historyIndex].texture.Get());
            device->SetTexture(5,usedHistory && options.depthRejection ? historyDepth[historyIndex^1].texture.Get() : depth.texture.Get());
            for (UINT i = 0; i < 6; ++i) Sampler(i, D3DTEXF_POINT);
            const float params[4]{float(options.debugMode),options.debugMotionScale,float(width),float(height)};
            const float reactive[4]{options.reactiveScale,options.reactiveMax,options.reactive && opaqueReady ? 1.0f : 0.0f,0};
            device->SetPixelShaderConstantF(0, params, 1);
            device->SetPixelShaderConstantF(1, reactive, 1);
            const float depthReject[4]{depthProjection[0],depthProjection[1],0.04f,options.depthRejection ? 1.0f : 0.0f};
            const float used[4]{usedHistory ? 1.0f : 0.0f,0,0,0};
            device->SetPixelShaderConstantF(2,depthReject,1);
            device->SetPixelShaderConstantF(3,used,1);
            Fullscreen(saved.targets[0].Get(), debugPS.Get(), desc.Width, desc.Height);
        }
        void Execute(const Packet& packet)
        {
            switch (packet.stage)
            {
            case Stage::Begin: Begin(packet); break;
            case Stage::Main: capturing = active; break;
            case Stage::Opaque: Opaque(); break;
            case Stage::FinalizeScene: FinalizeScene(); break;
            case Stage::Resolve: Resolve(packet.glowAvailable!=0); break;
            case Stage::Debug: Debug(); break;
            case Stage::Entity: entity = packet.entity; break;
            }
        }
    } GPU;

    void BeginBuilder(SafetyHookContext&)
    {
        RestoreProjection();
        Building = DebugFrame = false;
        Packet packet{Stage::Begin};
        {
            std::lock_guard lock(SettingsMutex);
            packet.settings = PublishedSettings;
        }
        memcpy(packet.viewProjection.data(), reinterpret_cast<void*>(0x0277D010_g), sizeof(Matrix));
        packet.phase = Phase++ % 8;
        const auto world=World();
        packet.depthProjection[0]=WorldMatrix(world,0x84)[10];
        packet.depthProjection[1]=WorldMatrix(world,0x84)[14];
        D3DSURFACE_DESC desc{};
        // One unjittered warm-up frame establishes GPU support before changing
        // the CPU projection. Every later pass consumes this same projection.
        Matrix inverse{};
        const bool canJitter=packet.settings.enabled && packet.settings.jitter && GPUReady &&
            Inverse(packet.viewProjection,inverse) && WorldMatrix(world,0x84)[11]!=0 &&
            AllowsSampleMode(packet.settings) && ActiveSamples.load()==static_cast<unsigned>(Samples()) && SceneTexture() && SUCCEEDED(SceneTexture()->GetLevelDesc(0,&desc)) && desc.Width && desc.Height;
        if (canJitter)
        {
            packet.jitter[0] = Halton(packet.phase+1,2)-0.5f;
            packet.jitter[1] = Halton(packet.phase+1,3)-0.5f;
        }
        Building = Queue(packet) && packet.settings.enabled && AllowsSampleMode(packet.settings);
        DebugFrame = Building && packet.settings.debugMode!=0;
        if (Building && canJitter)
        {
            MainWorld=World();
            SceneNDC[0]=2*packet.jitter[0]/desc.Width;
            SceneNDC[1]=-2*packet.jitter[1]/desc.Height;
            SceneScope=ProjectionShifted=true;
            Projection::Shift(WorldMatrix(MainWorld,0x84),SceneNDC[0],SceneNDC[1]);
            Projection::Shift(WorldMatrix(MainWorld,0x1558),SceneNDC[0],SceneNDC[1]);
            WorldMatrix(MainWorld,0x1598)=Multiply(WorldMatrix(MainWorld,0x1558),WorldMatrix(MainWorld,0xC4));
        }
    }
    void MainBuilder(SafetyHookContext&)
    {
        if (Building) Queue(Packet{Stage::Main});
    }
    void OpaqueBuilder(SafetyHookContext&)
    {
        if (Building) Queue(Packet{Stage::Opaque});
    }
    void FinalizeBuilder(SafetyHookContext&)
    {
        if (Building) Queue(Packet{Stage::FinalizeScene});
    }
    void ResolveBuilder(SafetyHookContext& ctx)
    {
        // Keep the same projection through alpha/player/glare and supplemental
        // glow; restore it before native fullscreen postFX starts.
        RestoreProjection();
        if (Building)
        {
            Packet packet{Stage::Resolve};
            const auto* params=reinterpret_cast<const uint8_t*>(ctx.esi);
            packet.glowAvailable=params && params[4] && *reinterpret_cast<const uint8_t*>(0x00E9787D_g);
            Queue(packet);
        }
        Building = false;
    }
    void DebugBuilder(SafetyHookContext&)
    {
        if (DebugFrame) Queue(Packet{Stage::Debug});
        DebugFrame = false;
    }
    void EntityBuilder(SafetyHookContext& ctx)
    {
        if (!Building) return;
        Packet packet{Stage::Entity};
        {
            std::lock_guard lock(InstanceMutex);
            auto it = InstanceKeys.find(ctx.edi);
            if (it != InstanceKeys.end()) packet.entity = it->second;
        }
        Queue(packet);
    }
    void OpaqueEntityBuilder(SafetyHookContext& ctx)
    {
        if (!Building) return;
        Packet packet{Stage::Entity};
        {
            std::lock_guard lock(InstanceMutex);
            auto it=InstanceKeys.find(ctx.ecx); // native renderable this, before its virtual getter/render
            if (it!=InstanceKeys.end()) packet.entity=it->second;
        }
        Queue(packet);
    }
    void EndEntityBuilder(SafetyHookContext&)
    {
        if (Building) Queue(Packet{Stage::Entity});
    }
    void InstanceBuilder(SafetyHookContext& ctx)
    {
        if (!Wanted) return;
        auto* args = reinterpret_cast<uintptr_t*>(ctx.esp);
        const auto instance = args[1], position = args[2];
        const auto* tib = reinterpret_cast<NT_TIB*>(NtCurrentTeb());
        // Stack-local static positions cannot identify an entity across frames.
        const bool onStack = position >= reinterpret_cast<uintptr_t>(tib->StackLimit) &&
            position < reinterpret_cast<uintptr_t>(tib->StackBase);
        const uint64_t key = PartKey ? PartKey : CharacterKey ? CharacterKey : (onStack ? 0 : Combine(position, 0x53544154u));
        std::lock_guard lock(InstanceMutex);
        if (InstanceKeys.size() > 65536) InstanceKeys.clear();
        InstanceKeys[instance] = key;
    }
    void __cdecl AddHuman(void* human)
    {
        const auto old = CharacterKey, oldPart = PartKey;
        PartKey = 0;
        CharacterKey = Combine(reinterpret_cast<uintptr_t>(human), 0x48554D41u);
        HumanHook.ccall<void>(human);
        CharacterKey = old; PartKey = oldPart;
    }
    void __cdecl AddVehicle(void* vehicle, int a2, int a3, int a4, int a5)
    {
        const auto old = CharacterKey, oldPart = PartKey;
        PartKey = 0;
        CharacterKey = Combine(reinterpret_cast<uintptr_t>(vehicle), 0x56454849u);
        VehicleHook.ccall<void>(vehicle, a2, a3, a4, a5);
        CharacterKey = old; PartKey = oldPart;
    }
    void VehiclePartBuilder(SafetyHookContext& ctx)
    {
        if (CharacterKey) PartKey = Combine(CharacterKey, ctx.eax);
    }
    bool __cdecl AddCharacter(int batch, void* mesh, void* human, void* params)
    {
        const auto old = CharacterKey, oldPart = PartKey;
        PartKey = 0;
        CharacterKey = Combine(reinterpret_cast<uintptr_t>(human), reinterpret_cast<uintptr_t>(mesh));
        const bool result = CharacterHook.ccall<bool>(batch, mesh, human, params);
        CharacterKey = old; PartKey = oldPart;
        return result;
    }
    void ExecuteCommand(SafetyHookContext& ctx)
    {
        auto& cmd = Commands();
        if (ctx.eax == Marker)
        {
            Packet packet;
            memcpy(&packet, cmd.read, sizeof(packet));
            cmd.read += sizeof(packet);
            GPU.Execute(packet);
            ctx.eip = static_cast<uint32_t>(0x00D2113E_g); // Skip the native switch, including its debugbreak.
        }
#if JUICED_TAA_MSAA
        else if (ctx.eax==2 && GPU.active && GPU.samples && !GPU.msaaDepth.UsesHardwareResolve())
        {
            const auto* args=reinterpret_cast<const uint32_t*>(cmd.read);
            const HRESULT hr=GPU.Clear(args);
            cmd.read += 20+args[0]*sizeof(D3DRECT);
            *reinterpret_cast<HRESULT*>(ctx.ebp-8)=hr;
            ctx.eax=static_cast<uint32_t>(hr);
            ctx.eip=static_cast<uint32_t>(0x00D2113E_g);
        }
        else if (ctx.eax==13 && GPU.active && GPU.samples && !GPU.msaaDepth.UsesHardwareResolve())
        {
            const auto* args=reinterpret_cast<const uint32_t*>(cmd.read);
            const HRESULT hr=GPU.DrawPrimitiveUP(args);
            cmd.read += 12+args[2]*Renderer::PrimitiveVertices(static_cast<D3DPRIMITIVETYPE>(args[0]),args[1]);
            *reinterpret_cast<HRESULT*>(ctx.ebp-8)=hr;
            ctx.eax=static_cast<uint32_t>(hr);
            ctx.eip=static_cast<uint32_t>(0x00D2113E_g);
        }
#endif
        else if ((ctx.eax == 11 || ctx.eax == 12) && GPU.active)
        {
            const auto* args = reinterpret_cast<const uint32_t*>(cmd.read);
            const HRESULT hr = ctx.eax == 12 ? GPU.DrawIndexed(args) : GPU.DrawPrimitive(args);
            cmd.read += ctx.eax == 12 ? 24 : 12;
            *reinterpret_cast<HRESULT*>(ctx.ebp - 8) = hr;
            ctx.eax = static_cast<uint32_t>(hr);
            ctx.eip = static_cast<uint32_t>(0x00D2113E_g);
        }
    }
    bool Check(uintptr_t address, std::initializer_list<uint8_t> bytes)
    {
        return memcmp(reinterpret_cast<void*>(DynAddress(address)), bytes.begin(), bytes.size()) == 0;
    }
}
bool UsesTemporalFiltering()
{
    return Wanted && GPUReady &&
#if JUICED_TAA_MSAA
        (Samples()==0 || AllowMSAA) &&
#else
        Samples()==0 &&
#endif
        ActiveSamples.load()==static_cast<unsigned>(Samples());
}
void ReleaseResources()
{
    GPU.Release();
}
void Init()
{
    LoadSettings();
    RegisterMenu();
    RenderStatus = Status::HooksUnavailable;
    // Retail symbols/database were used to verify these instruction boundaries.
    // Install no hooks on a mismatched executable.
    if (!Check(0x0052A969, {0xC6,0x05,0x38,0xA3,0x52,0x02,0x01}) ||
        !Check(0x0052AA32, {0xC6,0x05,0x38,0xA3,0x52,0x02,0x01}) ||
        !Check(0x00D23C40, {0x55,0x8B,0xEC,0x83,0xE4,0xF0,0x83,0xEC,0x2C}) ||
        !Check(0x00D23690, {0x5F,0x5E,0xE9,0x19,0xF3,0xFF,0xFF}) || !Check(0x0052AABA,{0xE8,0x61,0xC5,0xFF,0xFF}) ||
        !Check(0x0052ABDC,{0x8D,0x44,0x24,0x18,0x50}) ||
        !Check(0x0052ABA0,{0xE8,0x0B,0xD0,0xFE,0xFF}) || !Check(0x0052ABAD,{0xE8,0xEE,0xE0,0xFE,0xFF}) ||
        !Check(0x00D203E9,{0x89,0x85,0xF8,0xF7,0xFF,0xFF}) ||
        !Check(0x00524659,{0x8B,0x17,0x8B,0x42,0x1C}) ||
        !Check(0x00524678,{0x5E,0x5D,0x84,0xDB,0x5B}) ||
        !Check(0x00526606,{0x8B,0x01,0x8B,0x50,0x1C}) ||
        !Check(0x00526620,{0xF6,0x45,0x00,0x20,0x0F,0x84,0xB4,0x00,0x00,0x00}) ||
        !Check(0x00526257,{0x8B,0x01,0x8B,0x50,0x1C}) ||
        !Check(0x00526276,{0x84,0xC0,0x74,0x04,0x01,0x7C,0x24,0x18}) ||
        !Check(0x00526902,{0x8B,0x11,0x8B,0x42,0x1C}) ||
        !Check(0x0052691E,{0xF6,0x07,0x20,0x0F,0x84,0xD2,0x00,0x00,0x00}) ||
        !Check(0x00CF62A0,{0x80,0x7C,0x24,0x18,0x00}) ||
        !Check(0x0052EDA0,{0xA1,0x48,0x18,0x79,0x02}) ||
        !Check(0x009ADF30,{0x55,0x8B,0xEC,0x83,0xE4,0xF8}) ||
        !Check(0x00AE0350,{0x55,0x8B,0xEC,0x83,0xE4,0xF8}) ||
        !Check(0x00ADF430,{0x81,0xEC,0xE4,0x02,0x00,0x00}) ||
        !Check(0x00D2113E,{0x8B,0x55,0xF4,0x89,0x55,0xFC}))
    {
        Wanted = false;
        Logger::TypedLog(CHN_DEBUG, "TAA: executable hook signatures do not match; feature disabled.\n");
        return;
    }
    // Consumer first: no queued JTAA command may reach the native default case.
    auto consumer = safetyhook::create_mid(0x00D203E9_g, ExecuteCommand);
    if (!consumer) { return; }
    Hooks.emplace_back(std::move(consumer));
    CharacterHook = safetyhook::create_inline(0x0052EDA0_g, AddCharacter);
    HumanHook = safetyhook::create_inline(0x009ADF30_g, AddHuman);
    VehicleHook = safetyhook::create_inline(0x00AE0350_g, AddVehicle);
    ProjectionHook = safetyhook::create_inline(0x00D23C40_g, BuildProjection);
    if (!CharacterHook || !HumanHook || !VehicleHook || !ProjectionHook)
    {
        Wanted = false; Hooks.clear(); CharacterHook.reset(); HumanHook.reset(); VehicleHook.reset(); ProjectionHook.reset();
        return;
    }
    const std::pair<uintptr_t, void(*)(SafetyHookContext&)> sites[] = {
        {0x00ADF430,VehiclePartBuilder}, {0x00CF62A0,InstanceBuilder}, {0x00524659,EntityBuilder}, {0x00524678,EndEntityBuilder},
        {0x00526606,OpaqueEntityBuilder}, {0x00526620,EndEntityBuilder},
        {0x00526257,OpaqueEntityBuilder}, {0x00526276,EndEntityBuilder},
        {0x00526902,OpaqueEntityBuilder}, {0x0052691E,EndEntityBuilder},
        {0x0052AABA,OpaqueBuilder}, {0x0052ABA0,FinalizeBuilder}, {0x0052ABAD,ResolveBuilder}, {0x0052ABDC,DebugBuilder},
        {0x00D23690,CachedInstanceBuilder}, {0x0052AA32,MainBuilder}, {0x0052A969,BeginBuilder}
    };
    for (const auto& [address, callback] : sites)
    {
        auto hook = safetyhook::create_mid(DynAddress(address), callback);
        if (!hook)
        {
            Wanted = false;
            Hooks.clear(); CharacterHook.reset(); HumanHook.reset(); VehicleHook.reset(); ProjectionHook.reset();
            Logger::TypedLog(CHN_DEBUG, "TAA: failed to install render hooks; feature disabled.\n");
            return;
        }
        Hooks.emplace_back(std::move(hook));
    }
    HookReady = true;
    Wanted = UiSettings.enabled;
    RenderStatus = UiSettings.enabled ? Status::Waiting : Status::Disabled;
#if JUICED_TAA_MSAA
    Logger::TypedLog(CHN_DEBUG, "TAA: native D3D9 hooks ready; live controls in Juiced TAA / Juiced TAA Debug. Native MSAA combination supported.\n");
#else
    Logger::TypedLog(CHN_DEBUG, "TAA: native D3D9 hooks ready; live controls in Juiced TAA / Juiced TAA Debug. MSAA must be off.\n");
#endif
}
}

#endif // JUICED_TAA
