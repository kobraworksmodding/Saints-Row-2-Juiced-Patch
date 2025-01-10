// InternalPrint.cpp (uzis, Tervel)
// --------------------
// Created: 11/01/2025

#include <stdio.h>
#include "FileLogger.h"
#include <intrin.h>
#include "Patcher/patch.h"
#include "GameConfig.h"

namespace InternalPrint
{
	void* retAddr;

	int vsprintfHook(char* const Buffer, const char* const Format, va_list ArgList) {
		int a = vsprintf(Buffer, Format, ArgList);
		if (retAddr == (void*)0X004813BC) {
			Logger::TypedLog(CHN_XACT, "- 0X%p] Loaded Soundbank: %s\n", retAddr, Buffer);
			return a;
		}
		if (retAddr == (void*)0X00481379) {
			Logger::TypedLog(CHN_XACT, "- 0X%p] Loaded Wavebank: %s\n", retAddr, Buffer);
			return a;
		}
		if (retAddr == (void*)0X00481313) {
			Logger::TypedLog(CHN_XACT, "- 0X%p] Loaded Music Wavebank: %s\n", retAddr, Buffer);
			return a;
		}
		if (retAddr == (void*)0X0047986E) {
			Logger::TypedLog(CHN_XACT, "- 0X%p] Loaded Audioevent: %s\n", retAddr, Buffer);
			return a;
		}
		if (retAddr == (void*)0X0069C708) {
			Logger::TypedLog(CHN_CHUNK, "- 0X%p] Loaded City Chunk: %s\n", retAddr, Buffer);
			return a;
		}
		if (retAddr == (void*)0X00541D3E) {
			Logger::TypedLog(CHN_CHUNK, "- 0X%p] Loaded Chunk Swap: %s\n", retAddr, Buffer);
			return a;
		}
		if (retAddr == (void*)0X00A842B0) {
			Logger::TypedLog(CHN_ANIM, "- 0X%p] Loaded Non-Permanent Animation: %s\n", retAddr, Buffer);
			return a;
		}
		if (retAddr == (void*)0X00C0AD2B) {
			Logger::TypedLog(CHN_ASSET, "- 0X%p] Accessing Asset: %s\n", retAddr, Buffer);
			return a;
		}
		Logger::TypedLog(CHN_INTPR, "- 0X%p] %s\n", retAddr, Buffer);

		return a;
	}

	void sprintfHook(char* const Buffer, const char* const Format, ...) {

		va_list vl;
		va_start(vl, Format);
		retAddr = _ReturnAddress();
		vsprintfHook(Buffer, Format, vl);

		va_end(vl);
	}

	void Init() {
		if (GameConfig::GetValue("Debug", "AssetLoadPrint", 0))
		{
			Logger::TypedLog(CHN_INTPR, "Printing Asset Loads...\n");
			patchNop((BYTE*)0x00C0ADEF, 5); // asset load
			patchCall((void*)0x00C0ADEF, (void*)sprintfHook);
			patchNop((BYTE*)0x00C0AD26, 5); // asset load
			patchCall((void*)0x00C0AD26, (void*)sprintfHook);
		}

		patchNop((BYTE*)0x00CF4C42, 5); // is not a shader file
		patchCall((void*)0x00CF4C42, (void*)sprintfHook);
		patchNop((BYTE*)0x00CF4C70, 5); // is out of date
		patchCall((void*)0x00CF4C70, (void*)sprintfHook);
		patchNop((BYTE*)0x00D19162, 5); // shader loaded
		patchCall((void*)0x00D19162, (void*)sprintfHook);

		patchNop((BYTE*)0x00541D39, 5); // chunk swapped?
		patchCall((void*)0x00541D39, (void*)sprintfHook);
		patchNop((BYTE*)0x00A241EE, 5); // chunk swapped?
		patchCall((void*)0x00A241EE, (void*)sprintfHook);

		patchNop((BYTE*)0x00CE5600, 5); // anim not paged in
		patchCall((void*)0x00CE5600, (void*)sprintfHook);
		patchNop((BYTE*)0x00A842AB, 5); // new anim loaded
		patchCall((void*)0x00A842AB, (void*)sprintfHook);
		patchNop((BYTE*)0x00A253CA, 5); // loaded map lua
		patchCall((void*)0x00A253CA, (void*)sprintfHook);
		patchNop((BYTE*)0x00520FAE, 5); // Savegame location
		patchCall((void*)0x00520FAE, (void*)sprintfHook);
		patchNop((BYTE*)0x0069C703, 5); // list of city chunks loaded
		patchCall((void*)0x0069C703, (void*)sprintfHook);
		patchNop((BYTE*)0x004A5513, 5); // Character error warning
		patchCall((void*)0x004A5513, (void*)sprintfHook);
		patchNop((BYTE*)0x004A4929, 5); // Character tables Loaded
		patchCall((void*)0x004A4929, (void*)sprintfHook);

		patchNop((BYTE*)0x0048130E, 5); // Music Wavebanks Loaded
		patchCall((void*)0x0048130E, (void*)sprintfHook);
		patchNop((BYTE*)0x00481374, 5); // Sound Wavebanks Loaded
		patchCall((void*)0x00481374, (void*)sprintfHook);
		patchNop((BYTE*)0x004813B7, 5); // Soundbanks Loaded
		patchCall((void*)0x004813B7, (void*)sprintfHook);

		patchNop((BYTE*)0x00479869, 5); // Audio Loaded
		patchCall((void*)0x00479869, (void*)sprintfHook);

		patchNop((BYTE*)0x008CBF79, 5); // GS Debugging
		patchCall((void*)0x008CBF79, (void*)vsprintfHook);

		patchNop((BYTE*)0x00BE1B1F, 5); // Something for gs i think
		patchCall((void*)0x00BE1B1F, (void*)vsprintfHook);

		patchNop((BYTE*)0x00BFACFF, 5); // Some Error Logging
		patchCall((void*)0x00BFACFF, (void*)vsprintfHook);
	}
}