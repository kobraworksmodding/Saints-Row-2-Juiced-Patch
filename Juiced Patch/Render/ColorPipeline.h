#pragma once
#include <d3d9.h>
#ifndef JUICED_FP16
#define JUICED_FP16 1
#endif
namespace ColorPipeline {
#if JUICED_FP16
void Init();
void BeforeCreateDevice(IDirect3D9*, D3DPRESENT_PARAMETERS*);
void AfterCreateDevice(IDirect3DDevice9*, D3DPRESENT_PARAMETERS*);
void CreationFailed(D3DPRESENT_PARAMETERS*);
void ReleaseResources();
#else
inline void Init() {}
inline void BeforeCreateDevice(IDirect3D9*, D3DPRESENT_PARAMETERS*) {}
inline void AfterCreateDevice(IDirect3DDevice9*, D3DPRESENT_PARAMETERS*) {}
inline void CreationFailed(D3DPRESENT_PARAMETERS*) {}
inline void ReleaseResources() {}
#endif
}
