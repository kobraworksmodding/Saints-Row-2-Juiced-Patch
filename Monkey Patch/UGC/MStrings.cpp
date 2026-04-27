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
namespace MStrings
{
	void StartScan()
	{
		const auto max_files = 1024;
		const char** files = (const char**)calloc(max_files, sizeof(const char*));
		if (files) {
			auto number_of_files = cf_find_ext(files, max_files, ".anim_pc");

			if (number_of_files && files && *files) {
				for (int i = 0; i < number_of_files; ++i)
				{
					Logger::TypedLog("MSTRINGS", "Found file {}\n", *files[i]);
				}
			}
			if(files)
			free(files);
		}
		GFile file;
		file.OpenFile("charlist_JP.txt", "rb");
		Logger::TypedLog("MSTRINGS", "Found file with size {}\n",file.GetSize() );

	}
	SafetyHookInline loadcharlistD;
	char __cdecl loadcharlist()
	{
		auto result = loadcharlistD.unsafe_ccall<char>();
		StartScan();
		return result;
	}
	void Init()
	{
		return;
		loadcharlistD = safetyhook::create_inline(0x7F4430, loadcharlist);
	}
}