#include "GameConfig.h"

#include <cstdio>
#include <cstring>
#include <stdio.h>
#include <windows.h>
#include "FileLogger.h"
#include <ctime>

#include "Patcher/patch.h"

#if !RELOADED
char ini_name[] = "juiced.ini";
#else
char ini_name[] = "reloaded.ini";
#endif

namespace GameConfig
{
	static char inipath[MAX_PATH];

	//---------------------------------
	// Set up INI path
	//---------------------------------

	void Initialize()
	{
		// Create a buffer with the INI path
		GetCurrentDirectoryA(MAX_PATH, inipath);
		strcat_s(inipath, MAX_PATH, "\\");
		strcat_s(inipath, MAX_PATH, ini_name);
	}

	//---------------------------------
	// Get INI path
	//---------------------------------

	char* GetINIPath() { return (char*)inipath; }


	//---------------------------------
	// Get a value from the INI
	//---------------------------------

	uint32_t GetValue(const char* appName, const char* keyName, uint32_t def)
	{
		//Logger::TypedLog(CHN_DLL, "trying to get value\n");
		//Logger::TypedLog(CHN_DLL, "%s\n", appName);
		//Logger::TypedLog(CHN_DLL, "%s", keyName);
		//Logger::TypedLog(CHN_DLL, "%s", def);
		//Logger::TypedLog(CHN_DLL, "%s", inipath);
		return GetPrivateProfileIntA(appName, keyName, def, inipath);
	}

	void SetDoubleValue(const char* appName, const char* keyName, double new_value)
	{
		char new_string[12];

		sprintf_s(new_string, "%f", new_value);
		WritePrivateProfileStringA(appName, keyName, new_string, inipath);
	}
	//---------------------------------
	// Set a value from the INI
	//---------------------------------

	void SetValue(const char* appName, const char* keyName, uint32_t new_value)
	{
		char new_string[12];

		sprintf_s(new_string, "%d", new_value);
		WritePrivateProfileStringA(appName, keyName, new_string, inipath);
	}

	//---------------------------------
	// Get a string value from the INI
	//---------------------------------

	void GetStringValue(const char* appName, const char* keyName, const char* def, char* buffer)
	{
		GetPrivateProfileStringA(appName, keyName, def, buffer, MAX_PATH, inipath);
	}

	//---------------------------------
	// Get a signed value from the INI
	//---------------------------------

	int32_t GetSignedValue(const char* appName, const char* keyName, int32_t def)
	{
		char returned[32];

		GetStringValue(appName, keyName, "", returned);

		if (strlen(returned))
		{
			int32_t final_value;
			int32_t string_value = atoi(returned);
			return string_value;
		}

		return def;
	}

	double GetDoubleValue(const char* appName, const char* keyName, double def)
	{
		char returned[32];

		GetStringValue(appName, keyName, "", returned);

		if (strlen(returned))
		{
			double final_value;
			double string_value = atof(returned);
			return string_value;
		}

		return def;
	}

	//---------------------------------
	// Set a string value from the INI
	//---------------------------------

	void SetStringValue(const char* appName, const char* keyName, char* buffer)
	{
		WritePrivateProfileStringA( appName,  keyName,  buffer,  inipath);
	}

	//---------------------------------
	// Check to see if a feature is
	// enabled. Users can disable this
	// at any time from the [Debug]
	// section by prepending the feature
	// name with "Fix"
	//---------------------------------

	bool FeatureEnabled(const char* featureName, bool defaultValue)
	{
		char checkString[256];

		snprintf(checkString, sizeof(checkString), "Fix%s", featureName);

		Logger::TypedLog(CHN_DLL, "Performing %s...\n", featureName);

		if (GetValue("Debug", checkString, defaultValue))
			return true;

		return false;
	}

	bool FeatureEnabled(const char* featureName) { return FeatureEnabled(featureName, true); }


	//---------------------------------
	// Force Crash the Game
	//---------------------------------

	bool CFunc_ForceCrash()
	{
		throw std::invalid_argument("received negative value");
		return true;
	}

	//---------------------------------
	// Get screen width and height.
	//---------------------------------

	void GetScreenResolution(float* width, float* height)
	{
		int defWidth = GetSystemMetrics(SM_CXSCREEN);
		int defHeight = GetSystemMetrics(SM_CYSCREEN);

		*width = (float)GameConfig::GetValue("Graphics", "ResolutionX", (int)defWidth);
		*height = (float)GameConfig::GetValue("Graphics", "ResolutionY", (int)defHeight);
	}

}
