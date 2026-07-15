#pragma once
#include <filesystem>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <regex>
#include "../loose files.h"
// same as SR3
enum cf_search_types : __int32
{
	CF_SEARCH_NONE = -1,
	CF_SEARCH_STANDARD = 0x0,
	CF_SEARCH_PACKFILE = 0x1,
	CF_SEARCH_VDIR = 0x2,
	CF_SEARCH_NUM_TYPES = 0x3,
};

enum cf_io_access_types : __int32
{
	CF_IO_ACCESS_NONE = -1,
	CF_IO_ACCESS_STANDARD = 0x0,
	CF_IO_ACCESS_PACKFILE = 0x1,
	CF_IO_ACCESS_MEMORY = 0x2,
	CF_IO_ACCESS_UNKNOWN = 0x3,
	CF_IO_ACCESS_NUM_TYPES = 0x4,
};

struct cf_pc_open_file
{
	void* m_file_handle;
	unsigned int m_attrs_flags;
	unsigned int m_flags;
	char uh_over_lapped[20];
	unsigned int m_async_num_bytes_to_transfer;
	unsigned int m_async_pad_bytes;
	unsigned int m_async_num_pad_bytes_to_transfer;
};

union cfile_raw_data
{
	cf_pc_open_file* file_handle;
	// and more shit but we dont need it rn
};

enum cf_io_media_types : __int32
{
	CF_IO_MEDIA_NONE = -1,
	CF_IO_MEDIA_HDD = 0x0,
	CF_IO_MEDIA_DVD = 0x1,
	CF_IO_MEDIA_HOST = 0x2,
	CF_IO_MEDIA_NUM_TYPES = 0x3,
};

enum cfmode_type : __int32
{
	CF_MODE_NONE = 0x0,
	CF_MODE_READ = 0x1,
	CF_MODE_WRITE = 0x2,
};

enum cf_error_codes : __int32
{
	CF_ERROR_NONE = 0x0,
	CF_ERROR_READ = 0x1,
	CF_ERROR_WRITE = 0x2,
	CF_ERROR_ABORT = 0x3,
	CF_ERROR_NUM_CODES = 0x4,
};


class cfile
{
public:
	char name[65];
	char full_pathname[257];
	cf_search_types m_searched_system;
	cf_io_access_types m_access_type;
	cfile_raw_data raw_data;
	cf_io_media_types m_media_type;
	cfmode_type mode;
	cf_error_codes m_error_code;
	unsigned int pos;
	unsigned int size;
	int max_write_size;
	unsigned int m_flags;
};

extern unsigned int cf_find_ext(const char** files, int maxfiles, const char* ext);
extern bool cf_get_file_info_by_name(FILE_INFO* found, const char* filename);
extern cfile* __cdecl cf_open(const char* filename, char* mode, bool async = false);
class GFile
{
private:
	cfile* t_cfile;
public:
	GFile() : t_cfile(nullptr) {}

	~GFile();
	cfile* OpenFile(const char* filename, const char* mode, bool async = false);
	cfile* GetFile(unsigned int index = 0) const;
	size_t GetSize(unsigned int index = 0) const;
	size_t Read(void* buffer, size_t maxsize);
	GFile(const char* filename, const char* mode, bool async = false) {
	
		(void)OpenFile(filename, mode, async);
	};
};

