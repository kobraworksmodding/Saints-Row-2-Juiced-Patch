// Sampleable final depth for SR2's native MSAA surface.
// Prefer native RESZ depth resolve. Retain effective-shader replay as a
// diagnostic/unsupported-driver fallback; no material shader files are replaced.
#pragma once
#include <d3d9.h>
#include <wrl/client.h>
#include <array>
#include <algorithm>

namespace TemporalAA::MSAA
{
    struct CoverageState
    {
        bool available = false;
        D3DRENDERSTATETYPE state = D3DRS_ADAPTIVETESS_Y;
        DWORD enable = 0, disable = 0;
    };
    class DepthCompanion
    {
        template<class T> using Ptr = Microsoft::WRL::ComPtr<T>;
        Ptr<IDirect3DTexture9> depthTexture, colorTexture;
        Ptr<IDirect3DSurface9> depthSurface, colorSurface;
        bool hardwareResolve = false;
        // Only the bindings/constants/states changed by this pass are saved.
        // Creating a full state block for each material draw is unnecessarily costly.
        class Bindings
        {
            IDirect3DDevice9* device;
            std::array<Ptr<IDirect3DSurface9>,4> targets;
            Ptr<IDirect3DSurface9> depth;
            D3DVIEWPORT9 viewport{};
            DWORD coverage = 0;
            float alphaOptions[4]{};
            CoverageState alpha;
            UINT count = 1;
            bool valid = false, changed = false, alphaChanged = false;
        public:
            Bindings(IDirect3DDevice9* d, const CoverageState& a) : device(d), alpha(a)
            {
                D3DCAPS9 caps{};
                if (FAILED(d->GetDeviceCaps(&caps))) return;
                count = std::min<UINT>(4,caps.NumSimultaneousRTs);
                if (!count || FAILED(d->GetRenderTarget(0,targets[0].GetAddressOf())) ||
                    FAILED(d->GetDepthStencilSurface(depth.GetAddressOf())) ||
                    FAILED(d->GetViewport(&viewport))) return;
                for (UINT i=0;i<count;++i)
                {
                    if (i) d->GetRenderTarget(i,targets[i].GetAddressOf());
                }
                if (alpha.available && FAILED(d->GetRenderState(alpha.state,&coverage))) return;
                valid = true;
            }
            bool Bind(IDirect3DSurface9* color, IDirect3DSurface9* z, bool drawing)
            {
                if (!valid) return false;
                changed = true;
                bool ok = SUCCEEDED(device->SetDepthStencilSurface(nullptr));
                for (UINT i=1;i<count;++i) ok = SUCCEEDED(device->SetRenderTarget(i,nullptr)) && ok;
                ok = SUCCEEDED(device->SetRenderTarget(0,color)) && ok;
                ok = SUCCEEDED(device->SetDepthStencilSurface(z)) && ok;
                ok = SUCCEEDED(device->SetViewport(&viewport)) && ok;
                // The single-sample attachment determines pixel-center coverage;
                // native raster, blend and color-write states remain intact.
                if (drawing)
                {
                    // This private color target is scratch storage. Retain native
                    // color-write/blend state while deriving depth.
                    if (alpha.available && coverage==alpha.enable)
                    {
                        // MSAA color keeps its alpha-to-coverage branch. Only the
                        // single-sample depth draw uses the material's existing
                        // Bayer/texkill branch, retaining holes in hair/foliage.
                        if (FAILED(device->GetPixelShaderConstantF(189,alphaOptions,1))) return false;
                        alphaChanged = true;
                        float centerOptions[4]{0,0,alphaOptions[2],alphaOptions[3]};
                        ok = SUCCEEDED(device->SetPixelShaderConstantF(189,centerOptions,1)) && ok;
                        ok = SUCCEEDED(device->SetRenderState(alpha.state,alpha.disable)) && ok;
                    }
                }
                return ok;
            }
            ~Bindings()
            {
                if (!changed) return;
                device->SetDepthStencilSurface(nullptr);
                for (UINT i=1;i<count;++i) device->SetRenderTarget(i,nullptr);
                device->SetRenderTarget(0,targets[0].Get());
                for (UINT i=1;i<count;++i) device->SetRenderTarget(i,targets[i].Get());
                device->SetDepthStencilSurface(depth.Get());
                device->SetViewport(&viewport);

                if (alphaChanged)
                {
                    device->SetPixelShaderConstantF(189,alphaOptions,1);
                    device->SetRenderState(alpha.state,coverage);
                }
            }
        };
    public:
        void Release()
        {
            depthSurface.Reset(); colorSurface.Reset(); depthTexture.Reset(); colorTexture.Reset(); hardwareResolve=false;
        }
        bool Create(IDirect3DDevice9* d, UINT width, UINT height, bool forceReplay = false)
        {
            Release();
            Microsoft::WRL::ComPtr<IDirect3D9> api;
            D3DDEVICE_CREATION_PARAMETERS creation{}; D3DDISPLAYMODE display{};
            hardwareResolve=!forceReplay && SUCCEEDED(d->GetDirect3D(api.GetAddressOf())) &&
                SUCCEEDED(d->GetCreationParameters(&creation)) &&
                SUCCEEDED(api->GetAdapterDisplayMode(creation.AdapterOrdinal,&display)) &&
                SUCCEEDED(api->CheckDeviceFormat(creation.AdapterOrdinal,creation.DeviceType,display.Format,
                    D3DUSAGE_RENDERTARGET,D3DRTYPE_SURFACE,static_cast<D3DFORMAT>(MAKEFOURCC('R','E','S','Z'))));
            const auto intz = static_cast<D3DFORMAT>(MAKEFOURCC('I','N','T','Z'));
            if (FAILED(d->CreateTexture(width,height,1,D3DUSAGE_DEPTHSTENCIL,intz,
                    D3DPOOL_DEFAULT,depthTexture.GetAddressOf(),nullptr)) ||
                FAILED(depthTexture->GetSurfaceLevel(0,depthSurface.GetAddressOf())))
            { Release(); return false; }
            if (hardwareResolve) return true; // No scratch color or geometry replay.
            if (FAILED(d->CreateTexture(width,height,1,D3DUSAGE_RENDERTARGET,D3DFMT_A8R8G8B8,
                    D3DPOOL_DEFAULT,colorTexture.GetAddressOf(),nullptr)) ||
                FAILED(colorTexture->GetSurfaceLevel(0,colorSurface.GetAddressOf())))
            { Release(); return false; }
            // SR2's full-resolution native clear supplies the first contents.
            return true;
        }
        IDirect3DTexture9* Texture() const { return depthTexture.Get(); }
        bool UsesHardwareResolve() const { return hardwareResolve; }
        bool ResolveNative(IDirect3DDevice9* d)
        {
            if (!hardwareResolve || !depthTexture) return false;
            // AMD RESZ (also exposed by compatible D3D9 backends) resolves sample
            // zero of the currently bound DS into texture unit 0. A no-write
            // point establishes native GPU bindings before triggering the resolve.
            Ptr<IDirect3DStateBlock9> saved;
            if (FAILED(d->CreateStateBlock(D3DSBT_ALL,saved.GetAddressOf())) || FAILED(saved->Capture())) return false;
            bool ok=SUCCEEDED(d->SetVertexShader(nullptr)) && SUCCEEDED(d->SetPixelShader(nullptr)) &&
                SUCCEEDED(d->SetFVF(D3DFVF_XYZRHW)) && SUCCEEDED(d->SetTexture(0,depthTexture.Get()));
            for (auto state:{D3DRS_ZENABLE,D3DRS_ZWRITEENABLE,D3DRS_STENCILENABLE,D3DRS_ALPHATESTENABLE,
                D3DRS_ALPHABLENDENABLE,D3DRS_SCISSORTESTENABLE,D3DRS_COLORWRITEENABLE})
                ok=SUCCEEDED(d->SetRenderState(state,FALSE)) && ok;
            const CoverageState amd{true,D3DRS_POINTSIZE,MAKEFOURCC('A','2','M','1'),MAKEFOURCC('A','2','M','0')};
            DWORD pointSize=0; d->GetRenderState(D3DRS_POINTSIZE,&pointSize);
            // Disable both vendor ATOC forms for the no-write point; the state
            // block restores the engine's original enables after resolving.
            d->SetRenderState(amd.state,amd.disable); d->SetRenderState(D3DRS_ADAPTIVETESS_Y,0);
            const float point[4]{0,0,0,1};
            ok=ok && SUCCEEDED(d->DrawPrimitiveUP(D3DPT_POINTLIST,1,point,sizeof(point)));
            ok=ok && SUCCEEDED(d->SetRenderState(D3DRS_POINTSIZE,0x7FA05000u));
            d->SetRenderState(D3DRS_POINTSIZE,pointSize);
            ok=SUCCEEDED(saved->Apply()) && ok;
            return ok;
        }
        template<class Draw> bool Replay(IDirect3DDevice9* d, const CoverageState& alpha, Draw&& draw)
        {
            if (hardwareResolve) return true;
            Bindings saved(d,alpha);
            return depthSurface && saved.Bind(colorSurface.Get(),depthSurface.Get(),true) && SUCCEEDED(draw());
        }
        bool Clear(IDirect3DDevice9* d, DWORD count, const D3DRECT* rects, DWORD flags, float z, DWORD stencil)
        {
            if (hardwareResolve) return true;
            flags &= D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL;
            if (!flags) return true;
            Bindings saved(d,{});
            return depthSurface && saved.Bind(colorSurface.Get(),depthSurface.Get(),false) &&
                SUCCEEDED(d->Clear(count,rects,flags,0,z,stencil));
        }
    };
}
