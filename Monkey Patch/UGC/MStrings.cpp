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
#include "../Player/Input.h"

uintptr_t compress_character_addr = 0x7F4F60_g;
struct cchar
{
	wchar_t unicode;
	wchar_t compressed;
};

wchar_t compress_char(wchar_t c)
{
	unsigned int i = 0;
	size_t& num_cchars = *(size_t*)0x02528B30_g;
	cchar* cc_list = *(cchar**)0x02528B2C_g;

	if (c < 256)
		return c;
	for (i = 0; i < num_cchars; ++i)
	{
		if (cc_list[i].unicode == c)
			return cc_list[i].compressed;
	}
	return c;

}

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
ankerl::unordered_dense::map<uint32_t, wchar_t*> g_CustomPadStrings;
ankerl::unordered_dense::map<uint32_t, std::wstring> g_CustomVoiceStrings;

const wchar_t* display_request_string(const char* id, const wchar_t* default_fallback)
{
	auto result = thiscall_call<const wchar_t*>(0x7F4BC0_g);
	if (result)
	{
		return result;
	}

	if (default_fallback)
	{
		return default_fallback;
	}

	return nullptr;
}

namespace MStrings
{
	struct ParsedString
	{
		uint32_t Hash;
		std::wstring Text;
	};

	struct ParsedCxtResult
	{
		std::vector<ParsedString> Normal;
		std::vector<ParsedString> Pad;
	};

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
				out.push_back(compress_char((wchar_t)c));

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

		// Map each codepoint through the game's character compression table.
		// compress_character() returns the input unchanged when the codepoint
		// is >= 256 (and any other rejection cases), so this is safe for US too.
		for (wchar_t& c : out)
			c = compress_char(c);

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

	static void ParseTagHeader(const std::string& raw_header, std::string& out_tag, bool& out_pad)
	{
		out_tag.clear();
		out_pad = false;

		std::stringstream ss(raw_header);
		std::string part;
		bool first = true;

		while (std::getline(ss, part, ','))
		{
			part = Trim(part);

			if (first)
			{
				out_tag = part;
				first = false;
			}
			else
			{
				if (_stricmp(part.c_str(), "pad") == 0 ||
					_stricmp(part.c_str(), "gamepad") == 0 ||
					_stricmp(part.c_str(), "controller") == 0)
				{
					out_pad = true;
				}
			}
		}
	}

	static bool IsInlineFormatTag(const std::string& tag)
	{
		if (tag.empty())
			return false;

		if (tag.front() == '/')
			return true;

		return _stricmp(tag.c_str(), "format") == 0 ||
			_strnicmp(tag.c_str(), "color:", 6) == 0 ||
			_strnicmp(tag.c_str(), "scale:", 6) == 0 ||
			_strnicmp(tag.c_str(), "image:", 6) == 0;
	}

	static bool IsValidTagIdentifier(const std::string& tag)
	{
		if (tag.empty())
			return false;

		for (unsigned char c : tag)
		{
			if (!(std::isalnum(c) || c == '_'))
				return false;
		}

		return true;
	}

	static bool IsSectionHeaderLine(const std::string& trimmed)
	{
		if (trimmed.size() < 2 || trimmed.front() != '[' || trimmed.back() != ']')
			return false;

		std::string header = Trim(trimmed.substr(1, trimmed.size() - 2));
		if (header.empty())
			return false;

		// Treat only a single bracketed token as a section header.
		// This keeps inline formatting such as
		// [format][color:red]hotdog[/format]
		// or standalone tags like [format] as string content.
		if (header.find('[') != std::string::npos ||
			header.find(']') != std::string::npos ||
			IsInlineFormatTag(header))
		{
			return false;
		}

		return true;
	}

	static bool TryParseSectionHeaderLine(
		const std::string& trimmed,
		std::string& out_header)
	{
		out_header.clear();

		if (IsSectionHeaderLine(trimmed))
		{
			out_header = Trim(trimmed.substr(1, trimmed.size() - 2));
			return true;
		}

		if (trimmed.size() < 2 || trimmed.front() != '@')
			return false;

		std::string header = Trim(trimmed.substr(1));
		if (header.empty())
			return false;
		if (header.find_first_of(" \t") != std::string::npos)
			return false;

		std::string tag;
		bool pad = false;
		ParseTagHeader(header, tag, pad);

		if (!IsValidTagIdentifier(tag))
			return false;

		out_header = std::move(header);
		return true;
	}

	static std::string UnescapeBackslashSequences(const std::string& input)
	{
		std::string out;
		out.reserve(input.size());

		for (size_t i = 0; i < input.size(); ++i)
		{
			char c = input[i];
			if (c != '\\' || i + 1 >= input.size())
			{
				out.push_back(c);
				continue;
			}

			char next = input[++i];
			switch (next)
			{
			case 'n':
				out.push_back('\n');
				break;
			case 'r':
				out.push_back('\r');
				break;
			case 't':
				out.push_back('\t');
				break;
			case '\\':
				out.push_back('\\');
				break;
			case '"':
				out.push_back('"');
				break;
			default:
				out.push_back('\\');
				out.push_back(next);
				break;
			}
		}

		return out;
	}

	static bool TryParseQuotedEntryLine(
		const std::string& trimmed,
		std::string& out_tag,
		std::string& out_text)
	{
		out_tag.clear();
		out_text.clear();

		if (trimmed.size() < 3 || trimmed.front() != '"')
			return false;

		std::string raw_tag;
		raw_tag.reserve(trimmed.size());

		size_t i = 1;
		bool escape = false;
		for (; i < trimmed.size(); ++i)
		{
			char c = trimmed[i];
			if (escape)
			{
				raw_tag.push_back(c);
				escape = false;
				continue;
			}

			if (c == '\\')
			{
				escape = true;
				raw_tag.push_back(c);
				continue;
			}

			if (c == '"')
				break;

			raw_tag.push_back(c);
		}

		if (i >= trimmed.size() || trimmed[i] != '"')
			return false;

		size_t value_start = i + 1;
		while (value_start < trimmed.size() &&
			(trimmed[value_start] == ' ' || trimmed[value_start] == '\t'))
		{
			++value_start;
		}

		if (value_start >= trimmed.size())
			return false;

		out_tag = Trim(UnescapeBackslashSequences(raw_tag));
		if (out_tag.empty())
			return false;

		out_text = UnescapeBackslashSequences(trimmed.substr(value_start));
		return true;
	}

	static bool ParseCxtFile(const std::string& filepath, ParsedCxtResult& out)
	{
		std::ifstream file(filepath, std::ios::binary);
		if (!file)
			return false;

		std::string line;
		std::string current_tag;
		std::string current_text;
		bool current_pad = false;

		auto FlushEntry = [&]()
			{
				if (current_tag.empty())
					return;

				uint32_t hash = Game::utils::str_to_hash(current_tag.c_str());

				ParsedString parsed{
					hash,
					Utf8ToWide(current_text)
				};

				if (current_pad)
					out.Pad.push_back(std::move(parsed));
				else
					out.Normal.push_back(std::move(parsed));

				current_tag.clear();
				current_text.clear();
				current_pad = false;
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
			std::string header;
			std::string inline_tag;
			std::string inline_text;

			if (trimmed.empty())
				continue;

			if (TryParseSectionHeaderLine(trimmed, header))
			{
				FlushEntry();

				ParseTagHeader(header, current_tag, current_pad);
				continue;
			}

			if (TryParseQuotedEntryLine(trimmed, inline_tag, inline_text))
			{
				FlushEntry();

				uint32_t hash = Game::utils::str_to_hash(inline_tag.c_str());
				ParsedString parsed{
					hash,
					Utf8ToWide(inline_text)
				};

				out.Normal.push_back(std::move(parsed));
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

	wchar_t* _cdecl get_localization_from_hash_og(uint32_t checksum)
	{
		return cdecl_call<wchar_t*>(0x7F4D60, checksum);
	}

	wchar_t* _cdecl get_localization_from_hash(uint32_t checksum)
	{
		if (checksum)
		{
			if (isCutscene())
			{
				if (wchar_t* result = GetCustomVoiceString(checksum))
					return result;
			}

			bool isGamePadConnected =
				Input::LastInputUI() == Input::GAME_LAST_INPUT::CONTROLLER;

			if (isGamePadConnected)
			{
				auto pad_it = g_CustomPadStrings.find(checksum);
				if (pad_it != g_CustomPadStrings.end())
					return pad_it->second;
			}

			auto it = g_CustomStrings.find(checksum);
			if (it != g_CustomStrings.end())
				return it->second;
		}

		return get_localization_from_hashD.unsafe_ccall<wchar_t*>(checksum);
	}
	void StartParsing()
	{

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

			ParsedCxtResult parsed;

			if (!ParseCxtFile(filepath, parsed))
				continue;

			for (auto& s : parsed.Normal)
			{
				wchar_t* text_ptr = AllocLocalizationString(s.Hash, s.Text);
				if (text_ptr)
					g_CustomStrings[s.Hash] = text_ptr;
			}

			for (auto& s : parsed.Pad)
			{
				wchar_t* text_ptr = AllocLocalizationString(s.Hash, s.Text);
				if (text_ptr)
					g_CustomPadStrings[s.Hash] = text_ptr;
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
			std::string header;
			std::string inline_tag;
			std::string inline_text;

			if (trimmed.empty())
				continue;

			if (TryParseSectionHeaderLine(trimmed, header))
			{
				FlushEntry();

				bool unused_pad = false;
				ParseTagHeader(header, current_tag, unused_pad);
				continue;
			}

			if (TryParseQuotedEntryLine(trimmed, inline_tag, inline_text))
			{
				FlushEntry();

				uint32_t hash = Game::utils::str_to_hash(inline_tag.c_str());
				out.push_back({
					hash,
					Utf8ToWide(inline_text)
					});
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

	// US fallback: when the current language isn't US, walk *US.cxt files and
	// inject any entries the game's native localization doesn't have. Lets us
	// add new strings (e.g. J_* keys for the Juiced menu) without having to
	// duplicate them into every localized cxt.
	// Skips entries that already exist in our overlay (native cxt overrides US)
	// or in the game's native string table (don't fight the official translation).
	void StartParsingUSFallback()
	{
		const char* language_cstr = GetCurrentLanguage();
		std::string language = (language_cstr && *language_cstr) ? language_cstr : "";

		// Already covered by StartParsing() in the US case.
		if (_stricmp(language.c_str(), "US") == 0)
			return;

		const std::string wanted_suffix     = "us.cxt";
		const std::string voice_suffix_skip = "_vcus.cxt";

		for (auto& entry : DirCache)
		{
			const std::string& filename = entry.first; // lowercase hopefully
			const std::string& filepath = entry.second.FilePath;

			if (!EndsWithInsensitive(filename, wanted_suffix))
				continue;
			// Voice cxts (*_vcUS.cxt) are handled by StartParsingVoiceUSFallback.
			if (EndsWithInsensitive(filename, voice_suffix_skip))
				continue;

			ParsedCxtResult parsed;
			if (!ParseCxtFile(filepath, parsed))
				continue;

			size_t added_normal = 0;
			size_t added_pad    = 0;

			for (auto& s : parsed.Normal)
			{
				// Already injected from the native cxt — keep the localized text.
				if (g_CustomStrings.find(s.Hash) != g_CustomStrings.end())
					continue;
				// Game already has a native string for this hash — don't override.
				if (get_localization_from_hash_og(s.Hash) != nullptr)
					continue;

				wchar_t* text_ptr = AllocLocalizationString(s.Hash, s.Text);
				if (text_ptr)
				{
					g_CustomStrings[s.Hash] = text_ptr;
					++added_normal;
				}
			}

			for (auto& s : parsed.Pad)
			{
				if (g_CustomPadStrings.find(s.Hash) != g_CustomPadStrings.end())
					continue;
				if (get_localization_from_hash_og(s.Hash) != nullptr)
					continue;

				wchar_t* text_ptr = AllocLocalizationString(s.Hash, s.Text);
				if (text_ptr)
				{
					g_CustomPadStrings[s.Hash] = text_ptr;
					++added_pad;
				}
			}

			if (added_normal || added_pad)
			{
				Logger::TypedLog("MSTRINGS",
					"US fallback: {} normal + {} pad strings from {}\n",
					added_normal, added_pad, filepath);
				loaded_files_push_filename(filepath.c_str());
			}
		}
	}

	void StartParsingVoiceUSFallback()
	{
		const char* language_cstr = GetCurrentLanguage();
		std::string language = (language_cstr && *language_cstr) ? language_cstr : "";

		if (_stricmp(language.c_str(), "US") == 0)
			return;

		const std::string wanted_suffix = "_vcus.cxt";

		for (auto& entry : DirCache)
		{
			const std::string& filename = entry.first;
			const std::string& filepath = entry.second.FilePath;

			if (!EndsWithInsensitive(filename, wanted_suffix))
				continue;

			std::vector<ParsedVoiceString> parsed;
			if (!ParseVoiceCxtFile(filepath, parsed))
				continue;

			size_t added = 0;
			for (auto& s : parsed)
			{
				if (g_CustomVoiceStrings.find(s.Hash) != g_CustomVoiceStrings.end())
					continue;
				if (get_localization_from_hash_og(s.Hash) != nullptr)
					continue;

				g_CustomVoiceStrings[s.Hash] = std::move(s.Text);
				++added;
			}

			if (added)
			{
				Logger::TypedLog("MSTRINGS",
					"US voice fallback: {} strings from {}\n", added, filepath);
				loaded_files_push_filename(filepath.c_str());
			}
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

		// Run the US-fallback passes *before* installing our hook so that
		// get_localization_from_hash_og() — which calls 0x7F4D60 directly —
		// hits the genuine original game function rather than bouncing through
		// our overlay. After this point the hook takes over for all lookups.
		StartParsingUSFallback();
		StartParsingVoiceUSFallback();

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
