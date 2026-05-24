#pragma once
#include "stdafx.h"
#include <string>
#include <ankerl/unordered_dense.h>
#include <safetyhook.hpp>

#include "Shlwapi.h"
#pragma comment(lib,"Shlwapi.lib")

// ******************************************************************************************
// **                                                                                      **
// ** Allows the loading of loose files. Contributed by Scanti.                            **
// **                                                                                      **
// ******************************************************************************************

// Local variable / type declarations.

struct FILE_INFO
{
	int access_method;
	int access_flag;
	char filename[256];
	unsigned int size;
	void* file_data;
};

// Internal function declarations.

bool __stdcall raw_get_file_info_by_name_inner(FILE_INFO* file_info, char* filename, BOOL override_check);

// Global function declarations.

extern bool  hook_raw_get_file_info_by_name(char* filename, BOOL override_check);
extern void hook_loose_files();

extern void loose_files_render_status(bool get_ready_to_append_to_render = false);

extern std::string loaded_files_to_render;


struct FILEDATA
{
	std::string FilePath;			// Filepath to redirect to
	unsigned int file_size;			// We will need the file size so store it here
	bool MultiDef;					// For debug purposes
};

using LooseFileCache = ankerl::unordered_dense::map<std::string, FILEDATA>;

bool CreateCache(char* DirListFile);
bool ScanDLCDir(const char* Dir);
void DumpCache();
void CacheConflicts();
void ClearDirCache();
void InsertFileHashes(SafetyHookContext& ctx);

extern LooseFileCache DirCache;
extern LooseFileCache DLCCache;
const char* TranslateFilePath(const char* FilePath);
FILEDATA* TranslateFilePathData(const char* FilePath);

extern void loaded_files_push_filename(const char* string);
