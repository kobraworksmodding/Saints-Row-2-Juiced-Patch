// GPU comparison of native FXO programs against rebuilt HLSL equivalents.
#include <filesystem>
#include <array>
struct FixtureVS {ComPtr<IDirect3DVertexShader9> shader;ComPtr<IDirect3DCubeTexture9> cube;};
FixtureVS MaterialFixture(IDirect3DDevice9* d) {
    const char* source=R"(
struct O {float4 p:POSITION;float4 t0:TEXCOORD0;float4 t1:TEXCOORD1;float4 t2:TEXCOORD2;float4 t3:TEXCOORD3;float4 t4:TEXCOORD4;float4 t5:TEXCOORD5;float4 t6:TEXCOORD6;float4 t7:TEXCOORD7;float4 t8:TEXCOORD8;float4 t9:TEXCOORD9;};
O VS(float4 p:POSITION,float2 uv:TEXCOORD0) {O o;o.p=p;o.t0=float4(uv,uv);o.t1=float4(.3,.4,.5,.6);o.t2=float4(.4,.5,.6,.7);o.t3=o.t1;o.t4=o.t2;o.t5=o.t1;o.t6=o.t2;o.t7=o.t1;o.t8=o.t2;o.t9=o.t1;return o;}
)";
    ComPtr<ID3DBlob> code,error;Check(D3DCompile(source,strlen(source),nullptr,nullptr,nullptr,"VS","vs_3_0",D3DCOMPILE_OPTIMIZATION_LEVEL3,0,code.GetAddressOf(),error.GetAddressOf()),"material fixture VS compile");
    FixtureVS result;Check(d->CreateVertexShader(static_cast<DWORD*>(code->GetBufferPointer()),result.shader.GetAddressOf()),"material fixture VS");
    Check(d->CreateCubeTexture(4,1,0,D3DFMT_A16B16G16R16F,D3DPOOL_MANAGED,result.cube.GetAddressOf(),nullptr),"material cube");
    for(int face=0;face<6;++face){D3DLOCKED_RECT lock{};Check(result.cube->LockRect(static_cast<D3DCUBEMAP_FACES>(face),0,&lock,nullptr,0),"cube lock");for(int y=0;y<4;++y)for(int x=0;x<16;++x)reinterpret_cast<uint16_t*>(static_cast<char*>(lock.pBits)+y*lock.Pitch)[x]=XMConvertFloatToHalf(.35f);result.cube->UnlockRect(static_cast<D3DCUBEMAP_FACES>(face),0);}
    return result;
}
std::vector<std::vector<uint32_t>> AllPixels(const std::vector<unsigned char>& bytes) {
    std::vector<std::vector<uint32_t>> programs;
    for(size_t p=0;p+8<=bytes.size();++p){uint32_t w;memcpy(&w,bytes.data()+p,4);if(w!=0xffff0300u && w!=0xfffe0300u)continue;const auto version=w;size_t at=p+4;std::vector<uint32_t> code{w};bool ended=false;
        for(int i=0;i<4096 && at+4<=bytes.size();++i){memcpy(&w,bytes.data()+at,4);const unsigned op=w&65535;const size_t n=op==65535 ? 1 : op==65534 ? 1+((w>>16)&32767) : op<=96 ? 1+((w>>24)&15) : 0;if(!n || at+n*4>bytes.size())break;size_t before=code.size();code.resize(before+n);memcpy(code.data()+before,bytes.data()+at,n*4);at+=n*4;if(op==65535){ended=true;break;}}
        if(ended){if(version==0xffff0300u)programs.push_back(code);p=at-1;}
    }
    return programs;
}
std::array<bool,16> Cubes(const std::vector<uint32_t>& code) {
    std::array<bool,16> types{};for(size_t p=1;p<code.size();){const auto op=code[p]&65535;if(op==65535)break;const size_t n=op==65534 ? 1+((code[p]>>16)&32767) : 1+((code[p]>>24)&15);if(op==31 && n==3 && ((code[p+1]>>27)&15)==3)types[code[p+2]&15]=true;p+=n;}return types;
}
void DrawMaterial(IDirect3DDevice9* d,IDirect3DSurface9* target,IDirect3DPixelShader9* shader,FixtureVS& fixture,IDirect3DTexture9* input,const std::array<bool,16>& cubes) {
    d->SetRenderTarget(0,target);d->SetDepthStencilSurface(nullptr);d->SetVertexShader(fixture.shader.Get());d->SetPixelShader(shader);d->SetFVF(D3DFVF_XYZ|D3DFVF_TEX1);
    for(UINT i=0;i<16;++i){d->SetTexture(i,cubes[i] ? static_cast<IDirect3DBaseTexture9*>(fixture.cube.Get()) : input);d->SetSamplerState(i,D3DSAMP_MINFILTER,D3DTEXF_POINT);d->SetSamplerState(i,D3DSAMP_MAGFILTER,D3DTEXF_POINT);d->SetSamplerState(i,D3DSAMP_MIPFILTER,D3DTEXF_NONE);d->SetSamplerState(i,D3DSAMP_SRGBTEXTURE,FALSE);}
    struct V{float x,y,z,u,v;};const V vertices[]{{-1,1,0,0,0},{1,1,0,1,0},{-1,-1,0,0,1},{1,-1,0,1,1}};
    Check(d->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP,2,vertices,sizeof(V)),"material draw");
}
void CheckMaterialDirectory(IDirect3DDevice9* d,const std::filesystem::path& originals,const std::filesystem::path& rebuilt) {
    auto fixture=MaterialFixture(d);auto input=Input(d,.35f,.65f),oldTarget=RT(d,D3DFMT_A16B16G16R16F),newTarget=RT(d,D3DFMT_A16B16G16R16F);unsigned count=0;double maxRelative=0,totalChange=0;
    for(auto& file:std::filesystem::directory_iterator(originals)) {
        if(file.path().extension()!=".fxo_pc")continue;
        auto oldPrograms=AllPixels(File(file.path().string().c_str())),newPrograms=AllPixels(File((rebuilt/file.path().filename()).string().c_str()));if(oldPrograms.size()!=newPrograms.size())exit(31);
        for(size_t ps=0;ps<oldPrograms.size();++ps){if(oldPrograms[ps]==newPrograms[ps])continue;
            auto oldPS=Shader(d,oldPrograms[ps].data()),newPS=Shader(d,newPrograms[ps].data());auto cubes=Cubes(oldPrograms[ps]);
            for(int test=0;test<2;++test){float constants[224][4];for(int c=0;c<224;++c){constants[c][0]=.3f+test*.4f;constants[c][1]=.5f;constants[c][2]=.7f;constants[c][3]=.9f;}
                if(test){constants[24][0]=2;constants[24][1]=1.4f;constants[24][2]=2.8f;constants[26][0]=8;constants[26][1]=6;constants[26][2]=4;constants[26][3]=1;constants[29][0]=2;constants[29][2]=0;constants[27][0]=2;constants[27][2]=0;}
                constants[223][0]=0;d->SetPixelShaderConstantF(0,constants[0],224);Check(d->BeginScene(),"material begin");DrawMaterial(d,oldTarget.surface.Get(),oldPS.Get(),fixture,input.tex.Get(),cubes);DrawMaterial(d,newTarget.surface.Get(),newPS.Get(),fixture,input.tex.Get(),cubes);Check(d->EndScene(),"material end");auto a=Read(d,oldTarget.surface.Get()),b=Read(d,newTarget.surface.Get());
                for(size_t i=0;i<a.size();++i){if(!std::isfinite(a[i]) || !std::isfinite(b[i])){std::cerr<<file.path().filename()<<" #"<<ps<<" non-finite fixture\n";exit(32);}double relative=std::abs(a[i]-b[i])/std::max(1.f,std::abs(a[i]));maxRelative=std::max(maxRelative,relative);if(relative>.006){std::cerr<<file.path().filename()<<" #"<<ps<<" legacy mismatch "<<relative<<"\n";exit(33);}}
                constants[223][0]=1;d->SetPixelShaderConstantF(223,constants[223],1);Check(d->BeginScene(),"FP16 material begin");DrawMaterial(d,newTarget.surface.Get(),newPS.Get(),fixture,input.tex.Get(),cubes);Check(d->EndScene(),"FP16 material end");auto high=Read(d,newTarget.surface.Get());for(size_t i=0;i<a.size();++i){if(!std::isfinite(high[i]))exit(34);if(i%4==3 && std::abs(high[i]-a[i])>.003)exit(35);if(i%4<3)totalChange+=std::abs(high[i]-a[i]);}
            }
            ++count;
        }
    }
    for(UINT i=0;i<16;++i)d->SetTexture(i,nullptr);d->SetVertexShader(nullptr);
    std::cout<<"HLSL material equivalence: "<<originals.string()<<" / "<<count<<" programs; legacy relative error="<<maxRelative<<"; FP16 colour change="<<totalChange<<"; alpha unchanged\n";
    if(!count || totalChange<1)exit(36);
}
