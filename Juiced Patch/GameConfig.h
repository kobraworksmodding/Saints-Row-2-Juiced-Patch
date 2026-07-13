#pragma once

#include <cstdint>

namespace GameConfig
{
	void Initialize();
	void PatchCFuncs();
	char* GetINIPath();

	bool IsSuper();

	uint32_t GetKeyMask(const char* appName, const char* keyName, uint32_t def);

	uint32_t GetValue(const char* appName, const char* keyName, uint32_t def, const char* comment = nullptr);
	void SetValue(const char* appName, const char* keyName, uint32_t new_value);
	void SetDoubleValue(const char* appName, const char* keyName, double new_value);

	int32_t GetSignedValue(const char* appName, const char* keyName, int32_t def, const char* comment = nullptr);
	double GetDoubleValue(const char* appName, const char* keyName, double def, const char* comment = nullptr);
	char GetChar(const char* appName, const char* keyName, char def, const char* comment = nullptr);
	void GetStringValue(const char* appName, const char* keyName, const char* def, char* buffer, const char* comment = nullptr);
	void SetStringValue(const char* appName, const char* keyName, const char* buffer);

	bool FeatureEnabled(const char* featureName, bool defaultValue);
	bool FeatureEnabled(const char* featureName);

	void GetScreenResolution(float* width, float* height);
	
void SetDefaultGameSettings();
}
