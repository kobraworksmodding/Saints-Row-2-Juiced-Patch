#include "GFile.h"
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
#include "../Patcher/patch.h"
#include "../Hooker.h"


SAFETYHOOK_NOINLINE unsigned int cf_find_ext(const char** files, int maxfiles, const char* ext)
{
	return cdecl_call<uint32_t>(0xBFF1B0, files);
	const char* original_ext = nullptr;
	int original_maxfiles = 10240;
	Read(0xBFF2FA + 1, original_ext);
	Patch(0xBFF2FA + 1, ext);
	Patch(0xBFF2B3 + 1, ext);
	Patch(0xC1DC1A + 1, ext);
	Patch(0xBFF271 + 2, maxfiles);
	Patch(0xBFF2D0 + 2, maxfiles);
	auto numfiles = cdecl_call<uint32_t>(0xBFF1B0, files);
	Patch(0xBFF271 + 2, original_maxfiles);
	Patch(0xBFF2D0 + 2, original_maxfiles);

	Patch(0xBFF2FA + 1, original_ext);
	Patch(0xBFF2B3 + 1, original_ext);
	Patch(0xC1DC1A + 1, original_ext);
	return numfiles;

}
uintptr_t cf_get_file_info_by_name_addr = DynAddress(0xBFD880);
bool __cdecl cf_get_file_info_by_name(FILE_INFO* found, const char* filename)
{
	char result;
	__asm
	{
		mov eax, found
		push filename
		call cf_get_file_info_by_name_addr
		add esp, 4
		mov result, al
	}
	return result;
}

uintptr_t cf_open_addr = DynAddress(0xBFDC20);
cfile* __cdecl cf_open(const char* filename, const char* mode, bool async)
{
	cfile* result;
	__asm
	{
		pushad
		mov ecx, filename
		push async
		push mode
		call cf_open_addr
		add esp, 8
		mov result, eax
		popad
	}
	return result;
}

uintptr_t cf_read_addr = DynAddress(0xBFE170);
size_t __cdecl cf_read(cfile* file, void* lpBuffer, size_t size_buf)
{
	size_t result;
	__asm
	{
		pushad
		mov eax, size_buf
		push file
		push lpBuffer
		call cf_open_addr
		add esp, 8
		mov result, eax
		popad
	}
	return result;
}

bool __cdecl cf_close(cfile* file)
{
	return cdecl_call<bool>(DynAddress(0xBFDF30), file);
}

cfile* GFile::OpenFile(const char* filename, const char* mode, bool async)
{
	auto result = cf_open(filename, mode, async);
	t_cfile = result;
	return result;
}

cfile* GFile::GetFile(unsigned int index) const { return t_cfile; }

size_t GFile::GetSize(unsigned int index) const
{
	if (t_cfile) return t_cfile->size;
	return 0;
}

size_t GFile::Read(void* buffer, size_t maxsize)
{
	if (t_cfile)
		return cf_read(t_cfile, buffer, maxsize);
	return 0;
}

GFile::~GFile()
{
	if (t_cfile)
		cf_close(t_cfile);
}