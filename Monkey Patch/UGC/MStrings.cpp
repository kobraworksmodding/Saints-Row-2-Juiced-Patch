#include "CModLoader.h"
#include "../GameConfig.h"
#include "../SafeWrite.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <regex>
#include "../loose files.h"
#include "../FileLogger.h"
#include "../Game/GFile.h"
#include <ankerl/unordered_dense.h>
#include "../Hooker.h"
#include "../Render/bitmap.h"
#include "../Game/Game.h"



const char* GetCurrentLanguage()
{
	return cdecl_call<const char*>(0x7F4CF0_g);
}

void* __fastcall static_mempool_align_alloc(void* mempool, size_t size, uint32_t align)
{
	return thiscall_call<void*>(0xC00DB0_g, mempool, size, align);
}

void* __fastcall mempool_localization_align_alloc(size_t size, uint32_t align)
{
	return static_mempool_align_alloc((void*)0x02A15730_g, size, align);
}

ankerl::unordered_dense::map<uint32_t, wchar_t*> g_CustomStrings;
ankerl::unordered_dense::map<uint32_t, std::wstring> g_CustomVoiceStrings;

namespace MStrings
{
	struct ParsedString
	{
		uint32_t Hash;
		std::wstring Text;
	};

	static std::vector<ParsedString> g_ParsedStrings;

	static bool EndsWithInsensitive(const std::string& str, const std::string& suffix)
	{
		if (suffix.size() > str.size())
			return false;

		return std::equal(
			suffix.rbegin(),
			suffix.rend(),
			str.rbegin(),
			[](char a, char b)
			{
				return std::tolower((unsigned char)a) == std::tolower((unsigned char)b);
			}
		);
	}

	static std::string Trim(const std::string& s)
	{
		size_t start = 0;
		while (start < s.size() && std::isspace((unsigned char)s[start]))
			++start;

		size_t end = s.size();
		while (end > start && std::isspace((unsigned char)s[end - 1]))
			--end;

		return s.substr(start, end - start);
	}

	static void StripUtf8Bom(std::string& s)
	{
		if (s.size() >= 3 &&
			(unsigned char)s[0] == 0xEF &&
			(unsigned char)s[1] == 0xBB &&
			(unsigned char)s[2] == 0xBF)
		{
			s.erase(0, 3);
		}
	}

	static std::wstring Utf8ToWide(const std::string& input)
	{
		if (input.empty())
			return L"";

		int needed = MultiByteToWideChar(
			CP_UTF8,
			0,
			input.c_str(),
			(int)input.size(),
			nullptr,
			0
		);

		if (needed <= 0)
		{
			// fallback: simple ANSI-ish widening
			std::wstring out;
			out.reserve(input.size());

			for (unsigned char c : input)
				out.push_back((wchar_t)c);

			return out;
		}

		std::wstring out;
		out.resize(needed);

		MultiByteToWideChar(
			CP_UTF8,
			0,
			input.c_str(),
			(int)input.size(),
			out.data(),
			needed
		);

		return out;
	}

	static wchar_t* AllocLocalizationString(uint32_t hash, const std::wstring& text)
	{
		size_t char_count = text.size() + 1;

		size_t bytes =
			sizeof(uint32_t) +
			sizeof(wchar_t) * char_count;

		auto* raw = reinterpret_cast<uint8_t*>(
			mempool_localization_align_alloc(bytes, 4)
			);

		if (!raw)
			return nullptr;

		// this is important !!! the function that reads this shit is inlined D: 
		// text_ptr - 4 should be the localization hash
		*reinterpret_cast<uint32_t*>(raw) = hash;

		wchar_t* text_ptr = reinterpret_cast<wchar_t*>(raw + sizeof(uint32_t));

		memcpy(
			text_ptr,
			text.c_str(),
			sizeof(wchar_t) * char_count
		);

		return text_ptr;
	}

	static bool ParseCxtFile(const std::string& filepath, std::vector<ParsedString>& out)
	{
		std::ifstream file(filepath, std::ios::binary);
		if (!file)
			return false;

		std::string line;
		std::string current_tag;
		std::string current_text;

		auto FlushEntry = [&]()
			{
				if (current_tag.empty())
					return;

				uint32_t hash = Game::utils::str_to_hash(current_tag.c_str());

				out.push_back({
					hash,
					Utf8ToWide(current_text)
					});

				current_tag.clear();
				current_text.clear();
			};

		bool first_line = true;

		while (std::getline(file, line))
		{
			if (!line.empty() && line.back() == '\r')
				line.pop_back();

			if (first_line)
			{
				StripUtf8Bom(line);
				first_line = false;
			}

			std::string trimmed = Trim(line);

			if (trimmed.empty())
				continue;

			if (trimmed.size() >= 2 && trimmed.front() == '[' && trimmed.back() == ']')
			{
				FlushEntry();

				current_tag = trimmed.substr(1, trimmed.size() - 2);
				current_tag = Trim(current_tag);
				continue;
			}

			// Text line under the current [Tag].
			if (!current_tag.empty())
			{
				if (!current_text.empty())
					current_text += "\n";

				current_text += line;
			}
		}

		FlushEntry();

		return true;
	}

	bool isCutscene()
	{
		return *(bool*)0x02527CF7 || *(bool*)0x02527D14_g;
	}

	wchar_t* GetCustomVoiceString(uint32_t checksum)
	{
		if (!checksum)
			return nullptr;

		auto it = g_CustomVoiceStrings.find(checksum);
		if (it == g_CustomVoiceStrings.end())
			return nullptr;

		return const_cast<wchar_t*>(it->second.c_str());
	}

	SafetyHookInline get_localization_from_hashD;
	wchar_t* _cdecl get_localization_from_hash(uint32_t checksum)
	{
		if (checksum)
		{
			if (isCutscene())
			{
				auto result = GetCustomVoiceString(checksum);
				if (!result)
					return result;
			}

			auto it = g_CustomStrings.find(checksum);
			if (it != g_CustomStrings.end())
				return it->second;
		}
		return get_localization_from_hashD.unsafe_ccall<wchar_t*>(checksum);

	}
	void StartParsing()
	{
		g_ParsedStrings.clear();

		const char* language_cstr = GetCurrentLanguage();
		if (!language_cstr || !*language_cstr)
			return;

		std::string language = language_cstr;

		// Match files like:
		// english.cxt
		// en.cxt
		// whatever GetCurrentLanguage() returns + ".cxt"
		std::string wanted_suffix = language + ".cxt";

		for (auto& entry : DirCache)
		{
			const std::string& filename = entry.first; // lowercase hopefully
			const std::string& filepath = entry.second.FilePath;

			if (!EndsWithInsensitive(filename, wanted_suffix))
				continue;

			std::vector<ParsedString> parsed;

			if (!ParseCxtFile(filepath, parsed))
				continue;

			for (auto& s : parsed)
			{
				wchar_t* text_ptr = AllocLocalizationString(s.Hash, s.Text);

				if (!text_ptr)
					continue;

				g_CustomStrings[s.Hash] = text_ptr;
			}

			loaded_files_push_filename(filepath.c_str());
		}
	}

	struct ParsedVoiceString
	{
		uint32_t Hash;
		std::wstring Text;
	};

	static bool ParseVoiceCxtFile(const std::string& filepath, std::vector<ParsedVoiceString>& out)
	{
		std::ifstream file(filepath, std::ios::binary);
		if (!file)
			return false;

		std::string line;
		std::string current_tag;
		std::string current_text;

		auto FlushEntry = [&]()
			{
				if (current_tag.empty())
					return;

				uint32_t hash = Game::utils::str_to_hash(current_tag.c_str());

				out.push_back({
					hash,
					Utf8ToWide(current_text)
					});

				current_tag.clear();
				current_text.clear();
			};

		bool first_line = true;

		while (std::getline(file, line))
		{
			if (!line.empty() && line.back() == '\r')
				line.pop_back();

			if (first_line)
			{
				StripUtf8Bom(line);
				first_line = false;
			}

			std::string trimmed = Trim(line);

			if (trimmed.empty())
				continue;

			if (trimmed.size() >= 2 && trimmed.front() == '[' && trimmed.back() == ']')
			{
				FlushEntry();

				current_tag = trimmed.substr(1, trimmed.size() - 2);
				current_tag = Trim(current_tag);
				continue;
			}

			if (!current_tag.empty())
			{
				if (!current_text.empty())
					current_text += "\n";

				current_text += line;
			}
		}

		FlushEntry();

		return true;
	}

	void StartParsingVoice()
	{
		const char* language_cstr = GetCurrentLanguage();
		if (!language_cstr || !*language_cstr)
			return;

		std::string language = language_cstr;

		// Match files like:
		// anything_vcEN.cxt
		// anything_vcJP.cxt
		// voice_script_vcenglish.cxt
		std::string wanted_suffix = "_vc" + language + ".cxt";

		for (auto& entry : DirCache)
		{
			const std::string& filename = entry.first; // lowercase hopefully
			const std::string& filepath = entry.second.FilePath;

			if (!EndsWithInsensitive(filename, wanted_suffix))
				continue;

			std::vector<ParsedVoiceString> parsed;

			if (!ParseVoiceCxtFile(filepath, parsed))
				continue;

			for (auto& s : parsed)
			{
				// Assignment copies the string into the map.
				// Returned pointer will be g_CustomVoiceStrings[hash].c_str().
				g_CustomVoiceStrings[s.Hash] = std::move(s.Text);
			}

			loaded_files_push_filename(filepath.c_str());
		}
	}

	//void StartScan()
	//{
	//	const auto max_files = 1024;
	//	const char** files = (const char**)calloc(max_files, sizeof(const char*));
	//	if (files) {
	//		auto number_of_files = cf_find_ext(files, max_files, ".anim_pc");

	//		if (number_of_files && files && *files) {
	//			for (int i = 0; i < number_of_files; ++i)
	//			{
	//				Logger::TypedLog("MSTRINGS", "Found file {}\n", *files[i]);
	//			}
	//		}
	//		if(files)
	//		free(files);
	//	}
	//	GFile file;
	//	file.OpenFile("charlist_JP.txt", "rb");
	//	Logger::TypedLog("MSTRINGS", "Found file with size {}\n",file.GetSize() );

	//}
	SafetyHookInline loadcharlistD;
	char __cdecl loadcharlist()
	{
		auto result = loadcharlistD.unsafe_ccall<char>();
		StartParsing();
		StartParsingVoice();

		if(!g_CustomStrings.empty() || !g_CustomVoiceStrings.empty())
			get_localization_from_hashD = safetyhook::create_inline(0x7F4D60_g, get_localization_from_hash);
		return result;
	}
	SafetyHookMid increase_localization_buffer;
	void Init()
	{
		loadcharlistD = safetyhook::create_inline(0x7F4430_g, loadcharlist);
		increase_localization_buffer = safetyhook::create_mid(0x7F480E_g, [](SafetyHookContext& ctx) 
			{
				// theres a 100% better way to do this, I want to increase the mempool size, want it to estimate from our currnet custom strings (clippy95)
				if (ctx.ebx == 0x02A15730_g)
				{
					uint32_t* max = (uint32_t*)((0x02A15730_g) + 0x3C);
					uint32_t* end = (uint32_t*)((0x02A15730_g) + 0x4C);
					uint32_t* end2 = (uint32_t*)((0x02A15730_g) + 0x50);
					void** pool = (void**)((0x02A15730_g) + 0x38);
					constexpr size_t new_localization_size = 1331200 * 4;
					auto ptr = calloc(new_localization_size, 1);

					if (ptr) 
					{
						*pool = ptr;
						*max = new_localization_size;
						*end = new_localization_size;
						*end2 = new_localization_size;
						(void)increase_localization_buffer.disable();
					}
				}

			});
	}
}