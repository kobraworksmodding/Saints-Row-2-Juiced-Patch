#include "loose files.h"

#pragma warning(disable : 4996) // remove fopen warning
#pragma warning(disable : 4244) // remove possible loss of data during conversion warning

// This replaces the function that fills in the FILE_INFO structure if the file exists on the disk, with the file's information.

bool __stdcall raw_get_file_info_by_name_inner(FILE_INFO* file_info, char* filename, BOOL override_check)
{
	FILE* file_stream;
	_int64 file_size;
	char amend_filename[256];

	if (override_check)
		return(false);

	sprintf_s(amend_filename, sizeof(amend_filename), "%s//%s", "loose", filename);

	file_stream = fopen(amend_filename, "rb");

	if (!file_stream)
	{
		file_stream = fopen(filename, "rb");
		if (!file_stream)
			return(false);
	}
	else
		filename = amend_filename;

	strncpy_s(file_info->filename, filename, sizeof(file_info->filename));
	file_info->filename[255] = 0;
	file_info->access_method = 0;
	file_info->access_flag = 0;

	if (fseek(file_stream, 0, SEEK_END))
		return(false);

	file_size = _ftelli64(file_stream);

	if (file_size >= 0xFFFFFFFF)
		return(false);

	file_info->size = file_size;
	fclose(file_stream);
	return(true);
}

// Wrapper to convert bool __usercall func@<eax> (FILE_INFO *file_info@<edi>, char * filename, BOOL override_check) to
// bool __stdcall func (FILE_INFO *file_info, char *filename, BOOL override_check)

bool __declspec(naked) hook_raw_get_file_info_by_name(char* filename, BOOL override_check)
{
	__asm {
		push	ebp
		mov	ebp, esp
		mov	eax, DWORD PTR 12[ebp]
		push	eax
		mov	ecx, DWORD PTR 8[ebp]
		push	ecx
		push	edi
		call raw_get_file_info_by_name_inner
		pop	ebp
		ret	0
	}
}

// Changes the order in which files are searched. With loose files being the highest priority instead of the lowest.

_declspec(naked) void hook_loose_files()
{
	__asm {
		mov cl, 1
		mov edi, 1
		xor esi, esi
		mov eax, 0x00BFDB50
		call eax
		mov cl, 1
		xor edi, edi
		mov esi, 0
		mov eax, 0x00BFDB50
		call eax
		mov eax, 0x0051DAC9
		jmp eax
	}
}