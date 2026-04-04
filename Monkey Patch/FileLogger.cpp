#include "FileLogger.h"

#include "GameConfig.h"
#include "Patcher/patch.h"

#include <spdlog/async.h>
#include <spdlog/async_logger.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <filesystem>
#include <iostream>
#include <memory>
#include <stdio.h>
#include <string>
#include <vector>
#include <windows.h>

bool l_PrintScriptNames = false;
bool l_ExitOnAssert = true;
bool l_UseConsole, l_DebugOutput;
FILE* CON, * f_tracer, * f_logger;
HANDLE consoleHandle;

char dump_name[] = "dump";

HWND ConsoleWindow = nullptr;

namespace Logger
{
	namespace
	{
		constexpr size_t kLogQueueSize = 8192;

		std::shared_ptr<spdlog::async_logger> g_Logger;
		bool g_ThreadPoolReady = false;

		void EnsureThreadPool()
		{
			if (!g_ThreadPoolReady)
			{
				spdlog::init_thread_pool(kLogQueueSize, 1);
				g_ThreadPoolReady = true;
			}
		}

		std::string TrimTrailingLineEndings(std::string_view message)
		{
			std::string trimmed(message);
			while (!trimmed.empty() && (trimmed.back() == '\n' || trimmed.back() == '\r'))
				trimmed.pop_back();
			return trimmed;
		}

		std::shared_ptr<spdlog::async_logger> CreateLogger()
		{
			std::vector<spdlog::sink_ptr> sinks;

			if (l_UseConsole)
			{
				auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
				consoleSink->set_pattern("%v");
				sinks.push_back(consoleSink);
			}

			if (l_DebugOutput)
			{
				auto fileSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("debug.txt", true);
				fileSink->set_pattern("%v");
				sinks.push_back(fileSink);
			}

			if (sinks.empty())
				return {};

			EnsureThreadPool();

			auto logger = std::make_shared<spdlog::async_logger>(
				"Log",
				sinks.begin(),
				sinks.end(),
				spdlog::thread_pool(),
				spdlog::async_overflow_policy::block);

			logger->set_level(spdlog::level::trace);
			logger->flush_on(spdlog::level::info);
			return logger;
		}

		void ResetLogger()
		{
			if (g_Logger)
			{
				g_Logger->flush();
				g_Logger.reset();
			}
		}
	}

	bool ConsoleAllowed() { return l_UseConsole; }
	bool OutputAllowed() { return l_DebugOutput; }


	bool ChannelAllowed(std::string_view category) { return true; }

	void Initialize()
	{
		if (GameConfig::GetValue("Logger", "Console", 1, "Debug Console"))
			l_UseConsole = true;
		if (GameConfig::GetValue("Logger", "PrintScriptNames", 0))
			l_PrintScriptNames = true;
		if (GameConfig::GetValue("Logger", "WriteFile", 1, "Prints to debug.txt file in game directory."))
			l_DebugOutput = true;

		l_ExitOnAssert = GameConfig::GetValue("Logger", "ExitOnAssert", 1);

		if (l_UseConsole)
		{
			AllocConsole();
			freopen_s(&CON, "CONIN$", "r", stdin);
			freopen_s(&CON, "CONOUT$", "w", stdout);
			freopen_s(&CON, "CONOUT$", "w", stderr);
			ConsoleWindow = GetConsoleWindow();
			std::cout.clear();
			std::cerr.clear();

			consoleHandle = GetStdHandle(STD_INPUT_HANDLE);
		}

		if (l_DebugOutput)
		{
			std::error_code ec;
			std::filesystem::remove("debug.txt", ec);
		}

		f_logger = nullptr;
		f_tracer = nullptr;
		g_Logger = CreateLogger();

		if (l_DebugOutput && !g_Logger)
			Log("Failed to initialize debug logger.");
	}

	bool SaveDebugLogCopy(const wchar_t* destPath)
	{
		if (!OutputAllowed())
			return false;

		ResetLogger();
		const bool success = CopyFileW(L"debug.txt", destPath, FALSE) != FALSE;
		g_Logger = CreateLogger();
		return success;
	}

	namespace Detail
	{
		void WriteMessage(const char* category, std::string_view message)
		{
			if (!g_Logger || !ChannelAllowed(category))
				return;

			g_Logger->info("[{}] {}", category, TrimTrailingLineEndings(message));
		}

		[[noreturn]] void RaiseError(std::string_view message)
		{
			Log("CRITICAL: {}", message);
			ResetLogger();

			const std::string errorText(message);
			MessageBoxA(NULL, errorText.c_str(), "Critical Error", MB_ICONERROR);
			ExitProcess(0);
		}
	}

	int struct_indent;

	void StructLog(const char* to_log)
	{
		char* final_log = new char[256];

		if (struct_indent > 0)
		{
			memset(final_log, 0x20, struct_indent);
			final_log[struct_indent] = '\0';
			TypedLog(CHN_DLL, "{}{}", final_log, to_log);
		}
		else
		{
			TypedLog(CHN_DLL, to_log);
		}

		delete[] final_log;
	}

}
