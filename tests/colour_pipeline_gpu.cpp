#define NOMINMAX
#include <windows.h>
#include <d3d9.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <DirectXPackedVector.h>
#include "../Juiced Patch/Render/ColorShaders.h"
#include "../Juiced Patch/Render/TemporalShaders.h"
#include "../Juiced Patch/Render/ColorDXVK.h"
#include "../Juiced Patch/Render/DistortionShader.h"
#include <vector>
#include <set>
#include <cmath>
#include <fstream>
#include <iostream>
#include <algorithm>
using Microsoft::WRL::ComPtr;
using namespace DirectX::PackedVector;
constexpr UINT W=1024,H=8;
void Check(HRESULT hr,const char* what) {if (FAILED(hr)) {std::cerr<<what<<" 0x"<<std::hex<<hr<<"\n";exit(2);}}
struct Texture {ComPtr<IDirect3DTexture9> tex;ComPtr<IDirect3DSurface9> surface;};
Texture RT(IDirect3DDevice9* d,D3DFORMAT format) {Texture t;Check(d->CreateTexture(W,H,1,D3DUSAGE_RENDERTARGET,format,D3DPOOL_DEFAULT,t.tex.GetAddressOf(),nullptr),"RT");Check(t.tex->GetSurfaceLevel(0,t.surface.GetAddressOf()),"surface");return t;}
Texture Input(IDirect3DDevice9* d,float lo,float hi) {
 Texture t;Check(d->CreateTexture(W,H,1,D3DUSAGE_DYNAMIC,D3DFMT_A16B16G16R16F,D3DPOOL_DEFAULT,t.tex.GetAddressOf(),nullptr),"input");
 D3DLOCKED_RECT lock{};Check(t.tex->LockRect(0,&lock,nullptr,D3DLOCK_DISCARD),"lock");
 for(UINT y=0;y<H;++y) {auto* p=reinterpret_cast<uint16_t*>(static_cast<char*>(lock.pBits)+y*lock.Pitch);for(UINT x=0;x<W;++x) {float v=lo+(hi-lo)*x/(W-1);for(int c=0;c<3;++c)p[x*4+c]=XMConvertFloatToHalf(v);p[x*4+3]=XMConvertFloatToHalf(.35f);}}
 t.tex->UnlockRect(0);return t;
}
std::vector<float> Read(IDirect3DDevice9* d,IDirect3DSurface9* s) {
 D3DSURFACE_DESC desc{};s->GetDesc(&desc);ComPtr<IDirect3DSurface9> stage;Check(d->CreateOffscreenPlainSurface(W,H,desc.Format,D3DPOOL_SYSTEMMEM,stage.GetAddressOf(),nullptr),"stage");
 Check(d->GetRenderTargetData(s,stage.Get()),"readback");D3DLOCKED_RECT lock{};Check(stage->LockRect(&lock,nullptr,D3DLOCK_READONLY),"stage lock");
 lock.pBits=static_cast<char*>(lock.pBits)+(H/2)*lock.Pitch;
 std::vector<float> v(W*4);for(UINT x=0;x<W;++x)for(int c=0;c<4;++c) v[x*4+c]=desc.Format==D3DFMT_A16B16G16R16F ? XMConvertHalfToFloat(static_cast<uint16_t*>(lock.pBits)[x*4+c]) : float(reinterpret_cast<unsigned char*>(lock.pBits)[x*4+(c==0 ? 2 : c==2 ? 0 : c)])/255;
 stage->UnlockRect();return v;
}
struct Vertex {float x,y,z,w,u,v,u1,v1,u2,v2;};
void Draw(IDirect3DDevice9* d,IDirect3DSurface9* surface,IDirect3DTexture9* source,IDirect3DPixelShader9* shader) {
 Check(d->SetRenderTarget(0,surface),"set target");d->SetDepthStencilSurface(nullptr);d->SetTexture(0,source);d->SetPixelShader(shader);d->SetVertexShader(nullptr);
 d->SetFVF(D3DFVF_XYZRHW|D3DFVF_TEX2|D3DFVF_TEXCOORDSIZE4(1));
 D3DVIEWPORT9 vp{0,0,W,H,0,1};d->SetViewport(&vp);
 for(auto s:{D3DRS_ZENABLE,D3DRS_ZWRITEENABLE,D3DRS_ALPHABLENDENABLE,D3DRS_ALPHATESTENABLE,D3DRS_SRGBWRITEENABLE,D3DRS_SCISSORTESTENABLE,D3DRS_FOGENABLE,D3DRS_STENCILENABLE})d->SetRenderState(s,FALSE);
 d->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE);d->SetRenderState(D3DRS_COLORWRITEENABLE,15);
 for(UINT i=0;i<3;++i){d->SetSamplerState(i,D3DSAMP_ADDRESSU,D3DTADDRESS_CLAMP);d->SetSamplerState(i,D3DSAMP_ADDRESSV,D3DTADDRESS_CLAMP);d->SetSamplerState(i,D3DSAMP_MINFILTER,D3DTEXF_POINT);d->SetSamplerState(i,D3DSAMP_MAGFILTER,D3DTEXF_POINT);d->SetSamplerState(i,D3DSAMP_SRGBTEXTURE,FALSE);}
 const Vertex v[]{{-.5f,-.5f,0,1,0,0,0,0,0,0},{W-.5f,-.5f,0,1,1,0,1,0,1,0},{-.5f,H-.5f,0,1,0,1,0,1,0,1},{W-.5f,H-.5f,0,1,1,1,1,1,1,1}};
 Check(d->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP,2,v,sizeof(Vertex)),"draw");d->SetTexture(0,nullptr);
}
ComPtr<IDirect3DPixelShader9> Shader(IDirect3DDevice9* d,const uint32_t* code) {ComPtr<IDirect3DPixelShader9> s;Check(d->CreatePixelShader(reinterpret_cast<const DWORD*>(code),s.GetAddressOf()),"shader");return s;}
std::vector<uint32_t> Pixel(const std::vector<unsigned char>& bytes) {
 for(size_t p=0;p+4<=bytes.size();++p){uint32_t w;memcpy(&w,bytes.data()+p,4);if(w==0xffff0300){std::vector<uint32_t> code;size_t at=p;
  while(at+4<=bytes.size()){memcpy(&w,bytes.data()+at,4);unsigned op=w&65535;size_t n=op==65535 ? 1 : op==65534 ? 1+((w>>16)&32767) : 1+((w>>24)&15);if(at+n*4>bytes.size())break;size_t old=code.size();code.resize(old+n);memcpy(code.data()+old,bytes.data()+at,n*4);at+=n*4;if(op==65535)return code;}
 }}throw 1;
}
std::vector<unsigned char> File(const char* path){std::ifstream f(path,std::ios::binary);return {(std::istreambuf_iterator<char>(f)),{}};}
#include "colour_material_gpu.h"
float Expected(float v,float exposure,float peak,float start){float l=std::pow(std::max(v,0.f),2.2f)*std::exp2(exposure);if(l>start)l=start+(peak-start)*(1-std::exp(-(l-start)/(peak-start)));return std::pow(l,1/2.2f);}
int main(int argc,char** argv) {
 WNDCLASSA wc{};wc.lpfnWndProc=DefWindowProcA;wc.hInstance=GetModuleHandle(nullptr);wc.lpszClassName="JuicedColourTest";RegisterClassA(&wc);HWND window=CreateWindowA(wc.lpszClassName,"",WS_OVERLAPPEDWINDOW,0,0,W,H,nullptr,nullptr,wc.hInstance,nullptr);
 HMODULE lib=LoadLibraryA(argc>1 ? argv[1] : "d3d9.dll");if(!lib)return 1;
 auto create=reinterpret_cast<IDirect3D9* (WINAPI*)(UINT)>(GetProcAddress(lib,"Direct3DCreate9"));ComPtr<IDirect3D9> api;api.Attach(create(D3D_SDK_VERSION));
 ComPtr<ColorPipeline::DXVK::Interface> extension;bool dxvk=SUCCEEDED(api.As(&extension));if(dxvk)extension->UnlockAdditionalFormats();
 D3DPRESENT_PARAMETERS pp{};pp.BackBufferWidth=W;pp.BackBufferHeight=H;pp.BackBufferFormat=dxvk ? D3DFMT_A16B16G16R16F : D3DFMT_A8R8G8B8;pp.Windowed=TRUE;pp.SwapEffect=D3DSWAPEFFECT_DISCARD;pp.hDeviceWindow=window;
 ComPtr<IDirect3DDevice9> d;Check(api->CreateDevice(0,D3DDEVTYPE_HAL,window,D3DCREATE_HARDWARE_VERTEXPROCESSING,&pp,d.GetAddressOf()),"device");
 ComPtr<IDirect3DSwapChain9> swap;ComPtr<ColorPipeline::DXVK::Swapchain> hdr;Check(d->GetSwapChain(0,swap.GetAddressOf()),"swap");
 if(dxvk){Check(swap.As(&hdr),"HDR extension");Check(hdr->SetColorSpace(ColorPipeline::DXVK::SDR),"SDR space");}
 auto input=Input(d.Get(),.4f,.42f);auto fp=RT(d.Get(),D3DFMT_A16B16G16R16F);auto low=RT(d.Get(),D3DFMT_A8R8G8B8);auto out=RT(d.Get(),D3DFMT_A16B16G16R16F);
 auto tone=Shader(d.Get(),ColorPipeline::Bytecode::PS_Tonemap);auto output=Shader(d.Get(),ColorPipeline::Bytecode::PS_Output);
 float params[]{0,200,1000,0};float controls[]{.75f,.8f,0,0};d->SetPixelShaderConstantF(0,params,1);d->SetPixelShaderConstantF(1,controls,1);
 Check(d->BeginScene(),"begin");Draw(d.Get(),fp.surface.Get(),input.tex.Get(),tone.Get());Draw(d.Get(),low.surface.Get(),input.tex.Get(),tone.Get());Check(d->EndScene(),"end");
 auto high=Read(d.Get(),fp.surface.Get()),eight=Read(d.Get(),low.surface.Get());std::set<float> uniqueHigh,uniqueLow;
 for(unsigned x=0;x<W;++x){uniqueHigh.insert(high[x*4]);uniqueLow.insert(eight[x*4]);if(std::abs(high[x*4+3]-.35f)>.001)return 3;}
 std::cout<<"Gradient precision: FP16="<<uniqueHigh.size()<<" RGBA8="<<uniqueLow.size()<<" alpha preserved\n";if(uniqueHigh.size()<60 || uniqueHigh.size()<uniqueLow.size()*8)return 4;
 auto bright=Input(d.Get(),0,16);double maxError=0;
 for(int mode=0;mode<2;++mode){params[3]=float(mode);d->SetPixelShaderConstantF(0,params,1);d->SetPixelShaderConstantF(1,controls,1);Check(d->BeginScene(),"begin tone");Draw(d.Get(),fp.surface.Get(),bright.tex.Get(),tone.Get());Check(d->EndScene(),"end tone");auto mapped=Read(d.Get(),fp.surface.Get());
  for(unsigned x=0;x<W;++x){float source=XMConvertHalfToFloat(XMConvertFloatToHalf(16.f*x/(W-1)));float expect=Expected(source,0,mode ? 5.f : 1.f,mode ? 1.f : .75f);maxError=std::max(maxError,double(std::abs(mapped[x*4]-expect)));if(!std::isfinite(mapped[x*4]) || mapped[x*4]<0 || (x && mapped[x*4]+.002<mapped[(x-1)*4]))return 5;}
  if(mode){Check(d->BeginScene(),"begin UI");D3DRECT rect{0,0,8,H};Check(d->Clear(1,&rect,D3DCLEAR_TARGET,0xffffffff,1,0),"UI white");Draw(d.Get(),out.surface.Get(),fp.tex.Get(),output.Get());Check(d->EndScene(),"end output");auto rgb=Read(d.Get(),out.surface.Get());if(std::abs(rgb[0]-2.5f)>.002 || rgb[(W-1)*4]>12.51f)return 6;std::cout<<"HDR mapping: UI="<<rgb[0]<<" scRGB, brightest="<<rgb[(W-1)*4]<<" (1000 nits / 80)\n";}
 }
 std::cout<<"Tone-map reference maximum error="<<maxError<<"\n";if(maxError>.004)return 7;
 // Native FP16 MSAA colour must survive resolve above 1.0.
 if(SUCCEEDED(api->CheckDeviceMultiSampleType(0,D3DDEVTYPE_HAL,D3DFMT_A16B16G16R16F,TRUE,D3DMULTISAMPLE_2_SAMPLES,nullptr))){ComPtr<IDirect3DSurface9> msaa;Check(d->CreateRenderTarget(W,H,D3DFMT_A16B16G16R16F,D3DMULTISAMPLE_2_SAMPLES,0,FALSE,msaa.GetAddressOf(),nullptr),"MSAA");params[3]=1;d->SetPixelShaderConstantF(0,params,1);Check(d->BeginScene(),"begin MSAA");Draw(d.Get(),msaa.Get(),bright.tex.Get(),tone.Get());Check(d->EndScene(),"end MSAA");Check(d->StretchRect(msaa.Get(),nullptr,fp.surface.Get(),nullptr,D3DTEXF_NONE),"MSAA resolve");auto m=Read(d.Get(),fp.surface.Get());if(m[(W-1)*4]<2)return 8;std::cout<<"FP16 2x MSAA resolve: brightest="<<m[(W-1)*4]<<"\n";}
 if(argc>2){auto original=Pixel(File(argv[2]));std::vector<unsigned char> patched(std::begin(X360GammaShader),std::end(X360GammaShader));auto newCode=Pixel(patched);auto oldPS=Shader(d.Get(),original.data()),newPS=Shader(d.Get(),newCode.data());
  float tint[]{1,1,1,1},bcs[]{0,1,1,0},zero[]{0,0,0,0},resolution[]{float(W),float(H),0,0},alpha[]{1,0,0,0};
  d->SetPixelShaderConstantF(4,tint,1);d->SetPixelShaderConstantF(7,bcs,1);d->SetPixelShaderConstantF(24,zero,1);d->SetPixelShaderConstantF(25,zero,1);d->SetPixelShaderConstantF(26,zero,1);d->SetPixelShaderConstantF(187,zero,1);d->SetPixelShaderConstantF(188,resolution,1);d->SetPixelShaderConstantF(189,alpha,1);
  auto ramp=Input(d.Get(),0,1),neutral=Input(d.Get(),.5f,.5f);double gammaError=0;
  for(int blur=0;blur<2;++blur){alpha[0]=blur ? 0.f : 1.f;alpha[1]=float(blur);d->SetPixelShaderConstantF(189,alpha,1);d->SetTexture(1,neutral.tex.Get());d->SetTexture(2,neutral.tex.Get());Check(d->BeginScene(),"begin gamma");Draw(d.Get(),fp.surface.Get(),ramp.tex.Get(),oldPS.Get());Draw(d.Get(),out.surface.Get(),ramp.tex.Get(),newPS.Get());Check(d->EndScene(),"end gamma");auto a=Read(d.Get(),fp.surface.Get()),b=Read(d.Get(),out.surface.Get());for(unsigned x=0;x<W;++x)for(unsigned c=0;c<4;++c)gammaError=std::max(gammaError,double(std::abs(a[x*4+c]-b[x*4+c])));}
  alpha[0]=1;alpha[1]=0;d->SetPixelShaderConstantF(189,alpha,1);Check(d->BeginScene(),"begin highlight gamma");Draw(d.Get(),out.surface.Get(),bright.tex.Get(),newPS.Get());Check(d->EndScene(),"end highlight gamma");auto highlight=Read(d.Get(),out.surface.Get());if(!std::isfinite(highlight[(W-1)*4]) || highlight[(W-1)*4]<16 || gammaError>.002)return 9;
  std::cout<<"Juiced gamma SDR error="<<gammaError<<", FP16 highlight="<<highlight[(W-1)*4]<<"\n";
 }

 if(argc>3){std::string root=argv[3];float sun[]{3,2,1,2},fog[]{1,1,1,1},flag[]{0,0,0,0};d->SetPixelShaderConstantF(190,sun,1);d->SetPixelShaderConstantF(5,fog,1);
  auto oldCode=Pixel(File((root+"/shaders/original/bb-sunglow1_s.fxo_pc").c_str()));auto newCode=Pixel(File((root+"/game_root/mods/BlingGFX/bb-sunglow1_s.fxo_pc").c_str()));auto oldPS=Shader(d.Get(),oldCode.data()),newPS=Shader(d.Get(),newCode.data());d->SetPixelShaderConstantF(223,flag,1);Check(d->BeginScene(),"sun begin");Draw(d.Get(),fp.surface.Get(),nullptr,oldPS.Get());Draw(d.Get(),out.surface.Get(),nullptr,newPS.Get());Check(d->EndScene(),"sun end");auto a=Read(d.Get(),fp.surface.Get()),v=Read(d.Get(),out.surface.Get());double error=0;for(unsigned i=0;i<W*4;++i)error=std::max(error,double(std::abs(a[i]-v[i])));if(error>.002)return 10;
  flag[0]=1;d->SetPixelShaderConstantF(223,flag,1);Check(d->BeginScene(),"HDR sun begin");Draw(d.Get(),out.surface.Get(),nullptr,newPS.Get());Check(d->EndScene(),"HDR sun end");auto h=Read(d.Get(),out.surface.Get());float oldMax=*std::max_element(a.begin(),a.end()),newMax=*std::max_element(h.begin(),h.end());if(newMax<oldMax*2)return 11;std::cout<<"Bling sun legacy error="<<error<<"; old peak="<<oldMax<<" FP16 peak="<<newMax<<"\n";
  auto clouds=Pixel(File((root+"/game_root/mods/BlingGFX/sr2-skyboxclouds1_s.fxo_pc").c_str()));auto cloudPS=Shader(d.Get(),clouds.data());auto legacyCloudCode=Pixel(File((root+"/shaders/original/sr2-skyboxclouds1_s.fxo_pc").c_str()));auto legacyCloud=Shader(d.Get(),legacyCloudCode.data());auto cloudTex=Input(d.Get(),4,4);float p0[]{.3f,.3f,.3f,.1f},p1[]{1,0,1,0},sky[]{1,1,1,0},haze[]{.1f,.1f,.1f,0},white[]{1,1,1,1};d->SetPixelShaderConstantF(4,white,1);d->SetPixelShaderConstantF(24,p0,1);d->SetPixelShaderConstantF(25,p1,1);d->SetPixelShaderConstantF(26,sky,1);d->SetPixelShaderConstantF(27,sky,1);d->SetPixelShaderConstantF(201,haze,1);flag[0]=0;d->SetPixelShaderConstantF(223,flag,1);d->SetTexture(1,cloudTex.tex.Get());d->SetTexture(2,cloudTex.tex.Get());Check(d->BeginScene(),"cloud begin");Draw(d.Get(),fp.surface.Get(),cloudTex.tex.Get(),legacyCloud.Get());Draw(d.Get(),out.surface.Get(),cloudTex.tex.Get(),cloudPS.Get());Check(d->EndScene(),"cloud end");auto oldCloud=Read(d.Get(),fp.surface.Get()),newCloud=Read(d.Get(),out.surface.Get());double cloudError=0;for(unsigned i=0;i<W*4;++i){if(!std::isfinite(oldCloud[i]) || !std::isfinite(newCloud[i]))return 14;cloudError=std::max(cloudError,double(std::abs(oldCloud[i]-newCloud[i])));}if(cloudError>.002)return 15;flag[0]=1;d->SetPixelShaderConstantF(223,flag,1);Check(d->BeginScene(),"HDR cloud begin");Draw(d.Get(),out.surface.Get(),cloudTex.tex.Get(),cloudPS.Get());Check(d->EndScene(),"HDR cloud end");auto hdrCloud=Read(d.Get(),out.surface.Get());double cloudChange=0;for(unsigned i=0;i<W;++i){if(std::abs(hdrCloud[i*4+3]-oldCloud[i*4+3])>.001)return 16;cloudChange+=std::abs(hdrCloud[i*4]-oldCloud[i*4]);}if(cloudChange<1)return 17;std::cout<<"Bling cloud legacy error="<<cloudError<<"; FP16 colour changed="<<cloudChange<<"; alpha unchanged\n";
 }

 if(argc>3)CheckMaterialDirectory(d.Get(),std::filesystem::path(argv[3])/"shaders/original",std::filesystem::path(argv[3])/"game_root/mods/BlingGFX");
 if(argc>4)CheckMaterialDirectory(d.Get(),std::filesystem::path(argv[4])/"shaders/colour/original",std::filesystem::path(argv[4])/"Mods/JUICED - Shaders");
 // FP16 scene -> production TAA history -> production native-scene copy.
 {auto constant=Input(d.Get(),4,4);auto resolve=Shader(d.Get(),TemporalAA::Bytecode::PS_TemporalResolve),copy=Shader(d.Get(),TemporalAA::Bytecode::PS_CopyColor);float texel[]{1.f/W,1.f/H,float(W),float(H)},taa[]{0,0,0,1.25f},blend[]{.08f,.2f,.1f,1},reactive[]{0,0,0,0},depthReject[]{1,1,.04f,0};d->SetPixelShaderConstantF(0,texel,1);d->SetPixelShaderConstantF(1,taa,1);d->SetPixelShaderConstantF(2,blend,1);d->SetPixelShaderConstantF(3,reactive,1);d->SetPixelShaderConstantF(4,depthReject,1);for(UINT i=1;i<6;++i)d->SetTexture(i,constant.tex.Get());Check(d->BeginScene(),"FP16 TAA begin");Draw(d.Get(),fp.surface.Get(),constant.tex.Get(),resolve.Get());float alpha[]{1,0,0,0};d->SetPixelShaderConstantF(0,alpha,1);d->SetTexture(1,constant.tex.Get());Draw(d.Get(),out.surface.Get(),fp.tex.Get(),copy.Get());Check(d->EndScene(),"FP16 TAA end");auto result=Read(d.Get(),out.surface.Get());if(std::abs(result[0]-4)>.01 || std::abs(result[3]-.35f)>.001)return 13;for(UINT i=1;i<6;++i)d->SetTexture(i,nullptr);std::cout<<"Production TAA + scene copy retained FP16 RGB="<<result[0]<<" alpha="<<result[3]<<"\n";}
 // Real DXVK presentation, including switching SDR/HDR when supported.
 if(dxvk){ComPtr<IDirect3DSurface9> bb;Check(d->GetBackBuffer(0,0,D3DBACKBUFFER_TYPE_MONO,bb.GetAddressOf()),"backbuffer");Check(d->StretchRect(out.surface.Get(),nullptr,bb.Get(),nullptr,D3DTEXF_NONE),"output copy");Check(d->Present(nullptr,nullptr,nullptr,nullptr),"SDR present");bool supported=hdr->CheckColorSpaceSupport(ColorPipeline::DXVK::ScRGB);std::cout<<"DXVK float backbuffer and SDR presentation passed; HDR display available="<<supported<<"\n";if(supported){Check(hdr->SetColorSpace(ColorPipeline::DXVK::ScRGB),"HDR space");ColorPipeline::DXVK::Metadata meta;Check(hdr->SetHDRMetaData(&meta),"HDR metadata");Check(d->Present(nullptr,nullptr,nullptr,nullptr),"HDR present");Check(hdr->SetColorSpace(ColorPipeline::DXVK::SDR),"restore SDR");}}

 if(dxvk){input={};fp={};low={};out={};bright={};tone.Reset();output.Reset();d->SetPixelShader(nullptr);for(UINT i=0;i<16;++i)d->SetTexture(i,nullptr);ComPtr<IDirect3DSurface9> bb;Check(d->GetBackBuffer(0,0,D3DBACKBUFFER_TYPE_MONO,bb.GetAddressOf()),"reset backbuffer");d->SetRenderTarget(0,bb.Get());bb.Reset();hdr.Reset();swap.Reset();pp.BackBufferWidth=768;pp.BackBufferHeight=16;Check(d->Reset(&pp),"float resize reset");Check(d->GetBackBuffer(0,0,D3DBACKBUFFER_TYPE_MONO,bb.GetAddressOf()),"resized buffer");D3DSURFACE_DESC desc{};bb->GetDesc(&desc);if(desc.Width!=768 || desc.Height!=16 || desc.Format!=D3DFMT_A16B16G16R16F)return 12;std::cout<<"DXVK FP16 resize/reset passed\n";}
 std::cout<<"PASS\n";return 0;
}
