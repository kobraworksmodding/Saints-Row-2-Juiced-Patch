// SR2 FXO compiler. MIT; compiler DLL payload is Microsoft's Windows SDK redistributable.
#include <windows.h>
#include <d3dcompiler.h>
#include <bcrypt.h>
#include <wrl/client.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <map>
#include <set>
#include <vector>
#include <algorithm>
#include <stdexcept>
#include <cstring>
namespace fs=std::filesystem;
using Bytes=std::vector<unsigned char>;
using Microsoft::WRL::ComPtr;
Bytes Read(const fs::path& p) {std::ifstream f(p,std::ios::binary);if(!f)throw std::runtime_error("Cannot read "+p.string());return {(std::istreambuf_iterator<char>(f)),{}};}
void Write(const fs::path& p,const Bytes& data) {if(p.has_parent_path())fs::create_directories(p.parent_path());std::ofstream f(p,std::ios::binary);f.write(reinterpret_cast<const char*>(data.data()),data.size());if(!f)throw std::runtime_error("Cannot write "+p.string());}
void WriteText(const fs::path& p,const std::string& s) {Write(p,Bytes(s.begin(),s.end()));}
uint32_t Word(const Bytes& data,size_t at) {uint32_t w;memcpy(&w,data.data()+at,4);return w;}
struct Program {size_t start,end;uint32_t version;};
std::vector<Program> Programs(const Bytes& data) {
    std::vector<Program> list;
    for(size_t p=0;p+8<=data.size();++p) {
        auto version=Word(data,p);if(version!=0xffff0300u && version!=0xfffe0300u)continue;
        const size_t start=p;size_t at=p+4;bool ended=false;
        for(unsigned i=0;i<4096 && at+4<=data.size();++i) {
            const auto w=Word(data,at),op=w&65535;size_t n=0;
            if(op==65535){at+=4;ended=true;break;}
            if(op==65534)n=1+((w>>16)&32767);else if(op<=96)n=1+((w>>24)&15);else break;
            if(at+n*4>data.size())break;at+=n*4;
        }
        if(ended){list.push_back({start,at,version});p=at-1;}
    }
    return list;
}
Bytes StripComments(const Bytes& raw) {
    const auto ps=Programs(raw);if(ps.size()!=1 || ps[0].start!=0 || ps[0].end!=raw.size())throw std::runtime_error("Malformed compiled shader");
    Bytes out(raw.begin(),raw.begin()+4);
    for(size_t p=4;p<raw.size();) {
        const auto w=Word(raw,p),op=w&65535;const size_t n=op==65535 ? 1 : op==65534 ? 1+((w>>16)&32767) : 1+((w>>24)&15);
        if(op!=65534)out.insert(out.end(),raw.begin()+p,raw.begin()+p+n*4);p+=n*4;
    }
    return out;
}
std::set<uint32_t> ExternalConstants(const Bytes& data) {
    std::set<uint32_t> used,literals;
    for(size_t p=4;p+4<=data.size();) {
        auto w=Word(data,p),op=w&65535;if(op==65535)break;
        const size_t n=op==65534 ? 1+((w>>16)&32767) : 1+((w>>24)&15);
        if(!n || p+n*4>data.size())throw std::runtime_error("Malformed shader operands");
        if(op==81 && n>=2)literals.insert(Word(data,p+4)&2047);
        // DCL has an extra semantic token. DEF operands after its destination are raw floats.
        if(op!=65534 && op!=81 && op!=47 && op!=48)for(size_t i=(op==31 ? 2 : 1);i<n;++i) {
            auto t=Word(data,p+i*4);auto type=((t>>28)&7)|((t>>8)&24);
            if((t&0x80000000u) && type==2)used.insert(t&2047);
        }
        p+=n*4;
    }
    for(auto c:literals)used.erase(c);return used;
}
std::set<uint32_t> Declarations(const Bytes& data,bool samplers) {
    std::set<uint32_t> declarations;
    for(size_t p=4;p+4<=data.size();) {
        const auto w=Word(data,p),op=w&65535;if(op==65535)break;
        const size_t n=op==65534 ? 1+((w>>16)&32767) : 1+((w>>24)&15);
        if(!n || p+n*4>data.size())throw std::runtime_error("Malformed shader declarations");
        if(op==31 && n==3) {
            const auto token=Word(data,p+8);const auto type=((token>>28)&7)|((token>>8)&24);
            if(samplers && type==10)declarations.insert(((Word(data,p+4)>>27)&15)*32+(token&2047));
            if(!samplers && type==1)declarations.insert(Word(data,p+4)&0xF000Fu);
        }
        p+=n*4;
    }
    return declarations;
}
std::string Hash(const Bytes& data) {
    BCRYPT_ALG_HANDLE alg=nullptr;BCRYPT_HASH_HANDLE h=nullptr;DWORD n=0,size=0;
    if(BCryptOpenAlgorithmProvider(&alg,BCRYPT_SHA256_ALGORITHM,nullptr,0)<0)throw std::runtime_error("SHA256 unavailable");
    BCryptGetProperty(alg,BCRYPT_OBJECT_LENGTH,reinterpret_cast<PUCHAR>(&size),sizeof(size),&n,0);Bytes object(size),digest(32);
    auto ok=BCryptCreateHash(alg,&h,object.data(),size,nullptr,0,0);
    if(ok>=0)ok=BCryptHashData(h,const_cast<PUCHAR>(data.data()),static_cast<ULONG>(data.size()),0);
    if(ok>=0)ok=BCryptFinishHash(h,digest.data(),static_cast<ULONG>(digest.size()),0);
    if(h)BCryptDestroyHash(h);BCryptCloseAlgorithmProvider(alg,0);
    if(ok<0)throw std::runtime_error("SHA256 failed");std::ostringstream s;for(auto c:digest)s<<std::hex<<std::setw(2)<<std::setfill('0')<<unsigned(c);return s.str();
}
struct Compiler {
    HMODULE library=nullptr;fs::path payload;
    using Fn=HRESULT(WINAPI*)(LPCVOID,SIZE_T,LPCSTR,const D3D_SHADER_MACRO*,ID3DInclude*,LPCSTR,LPCSTR,UINT,UINT,ID3DBlob**,ID3DBlob**);
    Fn compile=nullptr;
    Compiler() {
        auto module=GetModuleHandleW(nullptr);auto resource=FindResourceW(module,MAKEINTRESOURCEW(101),RT_RCDATA);
        if(!resource)throw std::runtime_error("Embedded D3DCompiler_47 payload is missing");
        auto loaded=LoadResource(module,resource);auto bytes=static_cast<const unsigned char*>(LockResource(loaded));auto size=SizeofResource(module,resource);
        wchar_t directory[MAX_PATH]{},file[MAX_PATH]{};if(!GetTempPathW(MAX_PATH,directory) || !GetTempFileNameW(directory,L"FXO",0,file))throw std::runtime_error("Cannot create compiler payload file");
        payload=file;Write(payload,Bytes(bytes,bytes+size));library=LoadLibraryExW(payload.c_str(),nullptr,LOAD_LIBRARY_SEARCH_SYSTEM32);
        if(!library) {fs::remove(payload);throw std::runtime_error("Cannot load embedded D3DCompiler_47");}
        compile=reinterpret_cast<Fn>(GetProcAddress(library,"D3DCompile"));if(!compile)throw std::runtime_error("D3DCompile export unavailable");
    }
    ~Compiler(){if(library)FreeLibrary(library);std::error_code ec;fs::remove(payload,ec);}
    Bytes Run(const fs::path& source,const std::string& entry,const std::string& profile) {
        std::cerr<<"Compiling "<<source.string()<<" ["<<entry<<"]\n";auto data=Read(source);ComPtr<ID3DBlob> code,errors;
        auto hr=compile(data.data(),data.size(),source.string().c_str(),nullptr,D3D_COMPILE_STANDARD_FILE_INCLUDE,entry.c_str(),profile.c_str(),D3DCOMPILE_OPTIMIZATION_LEVEL3,0,code.GetAddressOf(),errors.GetAddressOf());
        if(errors)std::cerr.write(static_cast<const char*>(errors->GetBufferPointer()),errors->GetBufferSize());
        if(FAILED(hr))throw std::runtime_error(source.string()+": "+entry+" failed");
        auto p=static_cast<unsigned char*>(code->GetBufferPointer());return StripComments(Bytes(p,p+code->GetBufferSize()));
    }
};
std::string Trim(std::string s) {auto a=s.find_first_not_of(" \t\r\n"),b=s.find_last_not_of(" \t\r\n");return a==std::string::npos ? "" : s.substr(a,b-a+1);}
std::vector<std::string> Split(const std::string& s,char delimiter) {std::vector<std::string> v;std::istringstream in(s);std::string t;while(std::getline(in,t,delimiter))v.push_back(Trim(t));return v;}
struct Job {std::string type;std::map<std::string,std::string> values;std::vector<std::pair<std::string,std::string>> programs;};
std::string Required(const Job& j,const char* key) {auto p=j.values.find(key);if(p==j.values.end() || p->second.empty())throw std::runtime_error(std::string("Missing ")+key);return p->second;}
std::string Header(const Bytes& raw,const std::string& symbol,bool words) {
    std::ostringstream s;s<<"inline "<<(words ? "constexpr uint32_t " : "unsigned char ")<<symbol<<"[] = {\n";size_t stride=words ? 4 : 1,perLine=words ? 8 : 12;
    for(size_t p=0;p<raw.size();p+=stride){if((p/stride)%perLine==0)s<<"    ";s<<"0x"<<std::hex<<std::uppercase<<std::setw(words ? 8 : 2)<<std::setfill('0')<<(words ? Word(raw,p) : unsigned(raw[p]))<<(words ? "u," : ",");if((p/stride)%perLine==perLine-1 || p+stride==raw.size())s<<"\n";else s<<" ";}
    s<<"};\n";return s.str();
}
void Build(Compiler& compiler,const fs::path& base,const Job& job) {
    if(job.type=="fxo") {
        auto file=base/Required(job,"template");auto original=Read(file);auto result=original;auto slots=Programs(original);
        if(slots.empty())throw std::runtime_error("Template contains no valid SM3 shader programs");
        const auto expected=job.values.find("sha256");if(expected==job.values.end() || Hash(original)!=expected->second)throw std::runtime_error("Template hash mismatch: "+file.string());
        std::set<size_t> modified;
        for(const auto& [stage,text]:job.programs) {
            auto spec=Split(text,'|');if(spec.size()!=3)throw std::runtime_error("ps/vs expects index|HLSL source|entry point");
            const size_t index=std::stoul(spec[0]);std::vector<Program> select;for(auto& p:slots)if(p.version==(stage=="ps" ? 0xffff0300u : 0xfffe0300u))select.push_back(p);
            if(index>=select.size())throw std::runtime_error("Shader index outside template");auto slot=select[index];
            if(!modified.insert(slot.start).second)throw std::runtime_error("Duplicate shader slot");
            auto code=compiler.Run(base/spec[1],spec[2],stage+"_3_0");
            if(Word(code,0)!=slot.version || code.size()>slot.end-slot.start)throw std::runtime_error("Compiled shader does not fit matching FXO slot: "+file.string()+" #"+spec[0]);
            Bytes native(original.begin()+slot.start,original.begin()+slot.end);auto allowed=ExternalConstants(native);if(stage=="ps")allowed.insert(223);
            for(auto c:ExternalConstants(code))if(!allowed.count(c))throw std::runtime_error("Unbound new external shader constant c"+std::to_string(c));
            for(bool samplers:{false,true}) {
                const auto bindings=Declarations(native,samplers);
                for(auto binding:Declarations(code,samplers))if(!bindings.count(binding))throw std::runtime_error(samplers ? "New incompatible sampler binding" : "New incompatible input semantic");
            }
            std::fill(result.begin()+slot.start,result.begin()+slot.end,static_cast<unsigned char>(0));std::copy(code.begin(),code.end(),result.begin()+slot.start);
            std::cout<<file.filename().string()<<" "<<stage<<"#"<<index<<": "<<code.size()<<" / "<<slot.end-slot.start<<" bytes\n";
        }
        if(job.programs.empty())throw std::runtime_error("No replacement shaders specified");
        // Writes happen only after every entry passes compilation and ABI validation.
        if(auto p=job.values.find("output");p!=job.values.end())Write(base/p->second,result);
        if(auto p=job.values.find("header");p!=job.values.end())WriteText(base/p->second,"// Generated by FXOCompiler; edit the HLSL sources.\n#pragma once\n"+Header(result,Required(job,"symbol"),false));
        std::cout<<"FXO SHA256 "<<Hash(result)<<"\n";
    } else if(job.type=="bytecode-header") {
        const auto source=base/Required(job,"source");const auto entries=Split(Required(job,"entries"),',');
        std::string s="// Generated by FXOCompiler; edit the HLSL source.\n#pragma once\n#include <cstdint>\nnamespace "+Required(job,"namespace")+" {\n";
        for(auto& entry:entries){auto code=compiler.Run(source,entry,"ps_3_0");s+=Header(code,entry,true);std::cout<<entry<<": "<<code.size()/4<<" DWORDs\n";}
        s+="}\n";WriteText(base/Required(job,"output"),s);
    } else throw std::runtime_error("Unknown manifest section "+job.type);
}
int wmain(int argc,wchar_t** argv) {
    try {
        if((argc!=3 && argc!=4) || std::wstring(argv[1])!=L"build") {std::cout<<"FXOCompiler build <manifest.ini> [--fxo-only]\nHLSL -> matching SR2 FXO containers / embedded bytecode headers.\nD3DCompiler_47 is embedded; no DirectX installation or Python required.\n";return argc==1 ? 0 : 1;}
        const fs::path manifest=fs::absolute(argv[2]);auto raw=Read(manifest);std::istringstream input(std::string(raw.begin(),raw.end()));std::string line;std::vector<Job> jobs;Job job;
        while(std::getline(input,line)) {
            line=Trim(line);if(line.empty() || line[0]==';' || line[0]=='#')continue;
            if(line.front()=='[' && line.back()==']') {if(!job.type.empty())jobs.push_back(job);job={};job.type=line.substr(1,line.size()-2);continue;}
            auto at=line.find('=');if(at==std::string::npos || job.type.empty())throw std::runtime_error("Malformed manifest line: "+line);
            auto key=Trim(line.substr(0,at)),value=Trim(line.substr(at+1));if(key=="ps" || key=="vs")job.programs.emplace_back(key,value);else if(!job.values.emplace(key,value).second)throw std::runtime_error("Duplicate manifest key "+key);
        }
        if(!job.type.empty())jobs.push_back(job);if(jobs.empty())throw std::runtime_error("Empty shader manifest");
        std::cout.setf(std::ios::unitbuf);if(argc==4 && std::wstring(argv[3])!=L"--fxo-only")throw std::runtime_error("Unknown build option");
        Compiler compiler;size_t built=0;for(auto& j:jobs)if(argc==3 || j.type=="fxo"){Build(compiler,manifest.parent_path(),j);++built;}std::cout<<"Built "<<built<<" shader assets.\n";return 0;
    } catch(const std::exception& e){std::cerr<<"FXOCompiler: "<<e.what()<<"\n";return 1;}
}
