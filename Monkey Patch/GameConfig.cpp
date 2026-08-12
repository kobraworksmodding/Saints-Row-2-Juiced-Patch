#include "GameConfig.h"

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <windows.h>

#include "FileLogger.h"
#include "Patcher/patch.h"

char ini_name[] = "thaRow.ini";

namespace GameConfig
{
	static char inipath[MAX_PATH];

	namespace
	{
		struct ParsedSection
		{
			std::string name;
			size_t endIndex = 0;
			bool foundKey = false;
		};

		std::vector<std::string> SplitLines(const std::string& text)
		{
			std::vector<std::string> lines;
			if (text.empty())
				return lines;

			size_t start = 0;

			while (start < text.size())
			{
				size_t end = text.find('\n', start);
				if (end == std::string::npos)
					end = text.size();

				std::string line = text.substr(start, end - start);
				if (!line.empty() && line.back() == '\r')
					line.pop_back();

				lines.push_back(line);

				if (end == text.size())
					break;

				start = end + 1;
			}

			return lines;
		}

		std::string JoinLines(const std::vector<std::string>& lines, const std::string& newline, bool endWithNewline)
		{
			std::string text;
			for (size_t i = 0; i < lines.size(); ++i)
			{
				text += lines[i];
				if (i + 1 < lines.size() || endWithNewline)
					text += newline;
			}
			return text;
		}

		std::string Trim(const std::string& value)
		{
			size_t start = 0;
			while (start < value.size() && isspace(static_cast<unsigned char>(value[start])) != 0)
				++start;

			size_t end = value.size();
			while (end > start && isspace(static_cast<unsigned char>(value[end - 1])) != 0)
				--end;

			return value.substr(start, end - start);
		}

		bool IsSectionHeader(const std::string& line, std::string* sectionName = nullptr)
		{
			std::string trimmed = Trim(line);
			if (trimmed.size() >= 3 && trimmed.front() == '[' && trimmed.back() == ']')
			{
				if (sectionName)
					*sectionName = trimmed.substr(1, trimmed.size() - 2);
				return true;
			}

			return false;
		}

		bool TryParseKey(const std::string& line, std::string* key)
		{
			std::string trimmed = Trim(line);
			if (trimmed.empty() || trimmed.front() == ';')
				return false;

			size_t equals = trimmed.find('=');
			if (equals == std::string::npos)
				return false;

			*key = Trim(trimmed.substr(0, equals));
			return !key->empty();
		}

		bool ReadWholeFile(std::string* text)
		{
			std::ifstream file(inipath, std::ios::binary);
			if (!file.is_open())
				return false;

			text->assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
			return true;
		}

		bool WriteWholeFile(const std::string& text)
		{
			std::ofstream file(inipath, std::ios::binary | std::ios::trunc);
			if (!file.is_open())
				return false;

			file.write(text.data(), static_cast<std::streamsize>(text.size()));
			return file.good();
		}

		std::string DetectNewline(const std::string& text)
		{
			size_t crlf = text.find("\r\n");
			if (crlf != std::string::npos)
				return "\r\n";
			if (text.find('\n') != std::string::npos)
				return "\n";
			return "\r\n";
		}

		std::vector<ParsedSection> ParseSections(const std::vector<std::string>& lines, const char* keyName)
		{
			std::vector<ParsedSection> sections;
			ParsedSection* current = nullptr;

			for (size_t i = 0; i < lines.size(); ++i)
			{
				std::string sectionName;
				if (IsSectionHeader(lines[i], &sectionName))
				{
					if (current)
						current->endIndex = i;

					sections.push_back({ sectionName, lines.size(), false });
					current = &sections.back();
					continue;
				}

				if (!current)
					continue;

				std::string key;
				if (TryParseKey(lines[i], &key) && key == keyName)
					current->foundKey = true;
			}

			if (current)
				current->endIndex = lines.size();

			return sections;
		}

		size_t FindSectionInsertIndex(const std::vector<std::string>& lines, const ParsedSection& section)
		{
			size_t insertIndex = section.endIndex;
			while (insertIndex > 0 && insertIndex <= lines.size() && Trim(lines[insertIndex - 1]).empty())
				--insertIndex;
			return insertIndex;
		}

		std::vector<std::string> BuildCommentBlock(const char* comment)
		{
			std::vector<std::string> lines;
			if (!comment || comment[0] == '\0')
				return lines;

			for (const std::string& line : SplitLines(comment))
			{
				if (line.empty())
				{
					lines.emplace_back(";");
				}
				else if (!line.empty() && line.front() == ';')
				{
					lines.push_back(line);
				}
				else
				{
					lines.push_back("; " + line);
				}
			}

			return lines;
		}

		void EnsureValueExists(const char* appName, const char* keyName, const char* defaultValue, const char* comment)
		{
			if (!comment || comment[0] == '\0')
			{
				WritePrivateProfileStringA(appName, keyName, defaultValue, inipath);
				return;
			}

			std::string text;
			if (!ReadWholeFile(&text))
				text = "";

			const std::string newline = DetectNewline(text);
			const bool hadTrailingNewline = !text.empty() && (text.back() == '\n' || text.back() == '\r');
			std::vector<std::string> lines = SplitLines(text);
			std::vector<ParsedSection> sections = ParseSections(lines, keyName);
			for (const ParsedSection& section : sections)
			{
				if (section.name == appName && section.foundKey)
					return;
			}

			std::vector<std::string> blockLines = BuildCommentBlock(comment);
			blockLines.push_back(std::string(keyName) + "=" + defaultValue);

			for (const ParsedSection& section : sections)
			{
				if (section.name != appName)
					continue;

				const size_t insertIndex = FindSectionInsertIndex(lines, section);
				lines.insert(lines.begin() + insertIndex, blockLines.begin(), blockLines.end());
				WriteWholeFile(JoinLines(lines, newline, hadTrailingNewline));
				return;
			}

			if (!lines.empty() && !Trim(lines.back()).empty())
				lines.emplace_back();

			lines.push_back("[" + std::string(appName) + "]");
			lines.insert(lines.end(), blockLines.begin(), blockLines.end());
			WriteWholeFile(JoinLines(lines, newline, true));
		}

		bool GetRawValue(const char* appName, const char* keyName, char* buffer, DWORD bufferSize)
		{
			buffer[0] = '\0';
			GetPrivateProfileStringA(appName, keyName, "", buffer, bufferSize, inipath);
			return buffer[0] != '\0';
		}

		std::string FormatUnsignedValue(uint32_t value)
		{
			char buffer[32];
			sprintf_s(buffer, "%u", value);
			return buffer;
		}

		std::string FormatSignedValue(int32_t value)
		{
			char buffer[32];
			sprintf_s(buffer, "%d", value);
			return buffer;
		}

		std::string FormatDoubleValue(double value)
		{
			char buffer[64];
			sprintf_s(buffer, "%f", value);
			return buffer;
		}

		uint32_t ParseUnsignedValue(const char* text, uint32_t def)
		{
			char* end = nullptr;
			errno = 0;
			unsigned long parsed = strtoul(text, &end, 0);
			if (errno != 0 || end == text)
				return def;
			return static_cast<uint32_t>(parsed);
		}

		int32_t ParseSignedValueInternal(const char* text, int32_t def)
		{
			char* end = nullptr;
			errno = 0;
			long parsed = strtol(text, &end, 0);
			if (errno != 0 || end == text)
				return def;
			return static_cast<int32_t>(parsed);
		}

		double ParseDoubleValueInternal(const char* text, double def)
		{
			char* end = nullptr;
			errno = 0;
			double parsed = strtod(text, &end);
			if (errno != 0 || end == text)
				return def;
			return parsed;
		}
	}

	void Initialize()
	{
		GetCurrentDirectoryA(MAX_PATH, inipath);
		strcat_s(inipath, MAX_PATH, "\\");
		strcat_s(inipath, MAX_PATH, ini_name);
	}

	char* GetINIPath() { return inipath; }

	uint32_t GetValue(const char* appName, const char* keyName, uint32_t def, const char* comment)
	{
		char returned[64];
		if (GetRawValue(appName, keyName, returned, static_cast<DWORD>(sizeof(returned))))
			return ParseUnsignedValue(returned, def);

		EnsureValueExists(appName, keyName, FormatUnsignedValue(def).c_str(), comment);
		return def;
	}

	void SetDoubleValue(const char* appName, const char* keyName, double new_value)
	{
		const std::string new_string = FormatDoubleValue(new_value);
		WritePrivateProfileStringA(appName, keyName, new_string.c_str(), inipath);
	}

	void SetValue(const char* appName, const char* keyName, uint32_t new_value)
	{
		const std::string new_string = FormatUnsignedValue(new_value);
		WritePrivateProfileStringA(appName, keyName, new_string.c_str(), inipath);
	}

	void GetStringValue(const char* appName, const char* keyName, const char* def, char* buffer, const char* comment)
	{
		if (GetRawValue(appName, keyName, buffer, MAX_PATH))
			return;

		EnsureValueExists(appName, keyName, def ? def : "", comment);
		strcpy_s(buffer, MAX_PATH, def ? def : "");
	}

	int32_t GetSignedValue(const char* appName, const char* keyName, int32_t def, const char* comment)
	{
		char returned[64];
		if (GetRawValue(appName, keyName, returned, static_cast<DWORD>(sizeof(returned))))
			return ParseSignedValueInternal(returned, def);

		EnsureValueExists(appName, keyName, FormatSignedValue(def).c_str(), comment);
		return def;
	}

	double GetDoubleValue(const char* appName, const char* keyName, double def, const char* comment)
	{
		char returned[64];
		if (GetRawValue(appName, keyName, returned, static_cast<DWORD>(sizeof(returned))))
			return ParseDoubleValueInternal(returned, def);

		EnsureValueExists(appName, keyName, FormatDoubleValue(def).c_str(), comment);
		return def;
	}

	char GetChar(const char* appName, const char* keyName, char def, const char* comment)
	{
		char buffer[2] = { 0 };
		if (GetRawValue(appName, keyName, buffer, static_cast<DWORD>(sizeof(buffer))))
			return buffer[0];

		char defaultValue[2] = { def, '\0' };
		EnsureValueExists(appName, keyName, defaultValue, comment);
		return def;
	}

	void SetStringValue(const char* appName, const char* keyName, const char* buffer)
	{
		WritePrivateProfileStringA(appName, keyName, buffer, inipath);
	}

	bool FeatureEnabled(const char* featureName, bool defaultValue)
	{
		char checkString[256];
		snprintf(checkString, sizeof(checkString), "Fix%s", featureName);

		Logger::TypedLog(CHN_DLL, "Performing {}...\n", featureName);

		if (GetValue("Debug", checkString, defaultValue))
			return true;

		return false;
	}

	bool FeatureEnabled(const char* featureName) { return FeatureEnabled(featureName, true); }

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
