#pragma once

#include <stdint.h>
#include <cstring>
#include <float.h>
#include <string>
#include <string_view>
#include <utility>
#include <windows.h>
#include <iostream>

#include <spdlog/fmt/fmt.h>

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
#define CHN_LAA                         "LAA"
#define CHN_RL                         "RELOADED"
#define CHN_INTPR                      "INTERNAL"
#define CHN_SHADER                     "SHADER"
#define CHN_STARTUP                     "STARTUP"
#define CHN_XACT                      "XACT"
#define CHN_CHUNK                     "CHUNK"
#define CHN_ANIM                    "ANIM"
#define CHN_ASSET                    "ASSET"
#define CHN_XTBL					"XTBL"
 
extern HWND ConsoleWindow;
// Legacy exports retained so existing code can still link without call-site changes.
extern FILE* f_logger;
extern FILE* f_tracer;
namespace Logger
{
	namespace Detail
	{
		void WriteMessage(const char* category, std::string_view message);
		[[noreturn]] void RaiseError(std::string_view message);
	}

	inline void Log(std::string_view message)
	{
		Detail::WriteMessage(CHN_LOG, message);
	}

	template <typename... Args>
	inline void Log(fmt::format_string<Args...> format, Args&&... args)
	{
		Detail::WriteMessage(CHN_LOG, fmt::format(format, std::forward<Args>(args)...));
	}

	inline void Warn(std::string_view message)
	{
		Detail::WriteMessage(CHN_DEBUG, message);
	}

	template <typename... Args>
	inline void Warn(fmt::format_string<Args...> format, Args&&... args)
	{
		Detail::WriteMessage(CHN_DEBUG, fmt::format(format, std::forward<Args>(args)...));
	}

	inline void TypedLog(const char* category, std::string_view message)
	{
		Detail::WriteMessage(category, message);
	}

	template <typename... Args>
	inline void TypedLog(const char* category, fmt::format_string<Args...> format, Args&&... args)
	{
		Detail::WriteMessage(category, fmt::format(format, std::forward<Args>(args)...));
	}

	inline void Error(std::string_view message)
	{
		Detail::RaiseError(message);
	}

	template <typename... Args>
	inline void Error(fmt::format_string<Args...> format, Args&&... args)
	{
		Detail::RaiseError(fmt::format(format, std::forward<Args>(args)...));
	}

	void Initialize();
	void PatchCFuncs();
	void PatchLogger();

	bool SaveDebugLogCopy(const wchar_t* destPath);


}
