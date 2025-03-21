#pragma once
#include "stdafx.h"
#include <map>
#include <string>

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



struct FILEDATA
{
	std::string FilePath;			// Filepath to redirect to
	unsigned int file_size;			// We will need the file size so store it here
	bool MultiDef;					// For debug purposes
};

bool CreateCache(char* DirListFile);
void DumpCache();
void CacheConflicts();
void ClearDirCache();

const char* TranslateFilePath(const char* FilePath);
FILEDATA* TranslateFilePathData(const char* FilePath);
