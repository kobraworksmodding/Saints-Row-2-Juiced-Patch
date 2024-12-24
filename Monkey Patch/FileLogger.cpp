#include "FileLogger.h"

#include "GameConfig.h"
#include "Patcher/patch.h"

#include <windows.h>
#include <iostream>
#include <cstdio>
#include <cstdarg>
#include <stdio.h>


bool l_PrintScriptNames = false;
bool l_ExitOnAssert = true;
bool l_UseConsole, l_DebugOutput;
FILE* CON, * f_tracer, * f_logger;
HANDLE consoleHandle;

char dump_name[] = "dump";

HWND ConsoleWindow = nullptr;

namespace Logger
{
	bool ConsoleAllowed() { return l_UseConsole; }
	bool OutputAllowed() { return l_DebugOutput; }

	// Ported from WTDE, fix this in the future if desired
	// (For now, we'll just spit all logs to console)
	bool ChannelAllowed(const char* category) { return true; }

	//------------------------
	// Prepare the logger
	//------------------------

	void Initialize()
	{
		char logpath[MAX_PATH];
		char tracepath[MAX_PATH];

		if (GameConfig::GetValue("Logger", "Console", 0))
			l_UseConsole = true;
		if (GameConfig::GetValue("Logger", "PrintScriptNames", 0))
			l_PrintScriptNames = true;
		if (GameConfig::GetValue("Logger", "WriteFile", 1))
			l_DebugOutput = true;

		l_ExitOnAssert = GameConfig::GetValue("Logger", "ExitOnAssert", 1);

		// Create console?
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

		// Write to debug file?
		if (l_DebugOutput)
		{
			//Log("Logger file output initialized.\n");
			fopen_s(&f_logger, "debug.txt", "w");
			fopen_s(&f_tracer, "trace.txt", "w");

			if (!f_logger)
				Log("Failed to create debug.txt file.\n");
			if (!f_tracer)
				Log("Failed to create trace.txt file.\n");
		}
	}

	//------------------------
	// Prints message to console and log!
	//------------------------

	void CoreLog(const char* to_log, const char* category = CHN_LOG)
	{
		// Output to console
		if (ConsoleAllowed())
		{
			if (ChannelAllowed(category))
				printf("[%s] %s", category, to_log);
		}

		// Also write to .txt file!
		if (OutputAllowed() && f_logger)
		{
			fputs(to_log, f_logger);
			fflush(f_logger);
		}
	}

	//------------------------
	// Prints log message, with arguments!
	//------------------------

	void Log(const char* Format, ...)
	{
		char final_buffer[2000];

		va_list args;
		va_start(args, Format);
		vsnprintf(final_buffer, 2000, Format, args);
		va_end(args);

		CoreLog(final_buffer, CHN_LOG);
	}

	//------------------------
	// Typed log
	//------------------------

	void TypedLog(const char* category, const char* Format, ...)
	{
		char final_buffer[3000];

		va_list args;
		va_start(args, Format);
		vsnprintf(final_buffer, 3000, Format, args);
		va_end(args);

		CoreLog(final_buffer, category);
	}

	//------------------------
	// Warn
	//------------------------

	void Warn(const char* Format, ...)
	{
		char final_buffer[3000];
		memset(&final_buffer, 0, sizeof(final_buffer));

		va_list args;
		va_start(args, Format);
		vsnprintf(final_buffer, 3000, Format, args);
		va_end(args);

	}


	//------------------------
	// Print a nasty error!
	//------------------------

	void Error(const char* Format, ...)
	{
		//THAWPlus::DebugScriptCallstack();

		char final_buffer[2000];

		va_list args;
		va_start(args, Format);
		vsnprintf(final_buffer, 2000, Format, args);
		va_end(args);

		char finalString[2048];
		sprintf(finalString, "CRITICAL: %s", final_buffer);
		Log("%s\n\n", finalString);

		MessageBoxA(NULL, final_buffer, "Critical Error", MB_ICONERROR);

		ExitProcess(0);
	}

	//------------------------
	// Indented log
	//------------------------

	int struct_indent;

	void StructLog(const char* to_log)
	{
		char* final_log = new char[256];

		if (struct_indent > 0)
		{
			memset(final_log, 0x20, struct_indent);
			final_log[struct_indent] = '\0';
			TypedLog(CHN_DLL, "%s%s", final_log, to_log);
		}
		else
			TypedLog(CHN_DLL, to_log);

		if (final_log)
			delete[] final_log;
	}

}
