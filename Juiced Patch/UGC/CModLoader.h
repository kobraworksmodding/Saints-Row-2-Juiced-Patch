#pragma once
#include "../Patcher/CPatch.h"
#include <safetyhook.hpp>
struct SR_FILE_READER
{
	char* TXT;
	char* TXT_READ;
	char FILENAME[129];
	int unk0;
	int size;
	char unk1;
	uint32_t something_mempool;
};
namespace CModLoader {
	extern void Init();
}
