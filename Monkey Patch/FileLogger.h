#pragma once

#include <stdint.h>
#include <cstring>
#include <float.h>
#include <string>
#include <windows.h>
#include <iostream>

#define CHN_METRICS                     "Metrics"
#define CHN_MEMORY                      "Mem"
#define CHN_DLL							"DLL"
#define CHN_SAVE						"Save"
#define CHN_MOD							"Mod"
#define CHN_RPC							"RPC"
#define CHN_LOG							"Log"
#define CHN_DEBUG						"Debug"
#define CHN_INI							"INI"
#define CHN_SDL							"SDL"
#define CHN_IMAGE                       "Image"
#define CHN_AUDIO                       "Audio"
#define CHN_NET                         "Net"
#define CHN_LUA                         "LUA"

namespace Logger
{
	void Log(const char* Format, ...);

	void Warn(const char* Format, ...);

	void TypedLog(const char* category, const char* Format, ...);
	void Error(const char* Format, ...);

	void Initialize();
	void PatchCFuncs();
	void PatchLogger();

}
