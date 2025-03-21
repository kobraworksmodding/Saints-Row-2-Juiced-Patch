#include "loose files.h"
#include "FileLogger.h"
#include <algorithm>


#pragma warning(disable : 4996) // remove fopen warning
#pragma warning(disable : 4244) // remove possible loss of data during conversion warning

// This replaces the function that fills in the FILE_INFO structure if the file exists on the disk, with the file's information.

bool __stdcall raw_get_file_info_by_name_inner(FILE_INFO* file_info, char* filename, BOOL override_check)
{
	FILE* file_stream;
	_int64 file_size;
	
	FILEDATA *file_data;

	if (override_check)
		return(false);

	file_data = TranslateFilePathData(filename);

	if (file_data == NULL)
	{
		file_stream = fopen(filename, "rb");
		if (!file_stream)
			return(false);

		strncpy_s(file_info->filename, filename, sizeof(file_info->filename));
		
		if (fseek(file_stream, 0, SEEK_END))
			return(false);

		file_size = _ftelli64(file_stream);

		if (file_size >= 0xFFFFFFFF)
			return(false);

		file_info->size = file_size;
		fclose(file_stream);
	}
	else
	{
		//Logger::TypedLog(CHN_DLL, "Redirecting file %s to %s (%u)\n", filename, file_data->FilePath.c_str(), file_data->file_size);
		strncpy_s(file_info->filename, file_data->FilePath.c_str(), sizeof(file_info->filename));
		file_info->size = file_data->file_size;
	}

	file_info->filename[255] = 0;
	file_info->access_method = 0;
	file_info->access_flag = 0;

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



std::map<std::string, FILEDATA> DirCache;

std::string StringToUpper(std::string strToConvert)
{
	std::transform(strToConvert.begin(), strToConvert.end(), strToConvert.begin(), ::toupper);

	return(strToConvert);
}

std::string StringToLower(std::string strToConvert)
{
	std::transform(strToConvert.begin(), strToConvert.end(), strToConvert.begin(), ::tolower);

	return(strToConvert);
}

bool CreateCache(char* DirListFile)
{
	FILE *DirListHandle = fopen(DirListFile, "r");
	if (!DirListHandle)
	{
		Logger::TypedLog(CHN_DLL, "Failed to open directory list file %s\n", DirListFile);
		return(false);
	}

	Logger::TypedLog(CHN_DLL, "Creating cache directory data from %s\n", DirListFile);

	char CurrentDirectory[MAX_PATH];
	char CurrentSearch[MAX_PATH];

	char PathBuffer[MAX_PATH];

	bool SearchRootVPP = false;

	while (fgets(CurrentDirectory, MAX_PATH, DirListHandle) != NULL)
	{
		
		// Remove any control codes from the end of the file path string
		for (int i = strlen(CurrentDirectory) - 1; i >= 0; i--)
		{
			if (CurrentDirectory[i] > 31)
				break;
			CurrentDirectory[i] = 0;
		}

		// If the line is blank or a comment (#) then skip

		if (!CurrentDirectory[0] || CurrentDirectory[0]=='#')
			continue;

		// If the file path is "." then set it to the current directory otherwise FindFirstFileA will search the root directory
		// of your drive.

		//if (!strcmp(CurrentDirectory, "."))
			//GetCurrentDirectoryA(MAX_PATH, CurrentDirectory);

		HANDLE SearchDirHandle;
		WIN32_FIND_DATAA FileData;

		strcpy_s(CurrentSearch, MAX_PATH, CurrentDirectory);
		PathAppendA(CurrentSearch, "*");

		SearchDirHandle = FindFirstFileA(CurrentSearch, &FileData);

		// Check for errors searching the directory
		if (SearchDirHandle == INVALID_HANDLE_VALUE)
		{
			Logger::TypedLog(CHN_DLL, "Unable to find directory %s\n", CurrentDirectory);
			continue;
		}

		Logger::TypedLog(CHN_DLL, "Adding contents of directory %s\n", CurrentDirectory);
		
		do
		{
			// Skip if it's a directory
			if (!strcmp(FileData.cFileName, ".") || !strcmp(FileData.cFileName, "..") || (FileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
				continue;

			char* Extension = PathFindExtensionA(FileData.cFileName);

			// Blacklist of file extensions to skip
			if (!_stricmp(Extension, ".exe") || !_stricmp(Extension, ".dll") || !_stricmp(Extension,".bbsave"))
				continue;

			std::string SearchFileName(FileData.cFileName);
			SearchFileName = StringToLower(SearchFileName);
			std::map<std::string, FILEDATA>::iterator itDirCache;

			itDirCache = DirCache.find(SearchFileName);

			if (itDirCache == DirCache.end())
			{
				FILEDATA PushData;
				strcpy_s(PathBuffer, MAX_PATH, CurrentDirectory);
				PathAppendA(PathBuffer, FileData.cFileName);
				std::string FullFindFilePath(PathBuffer);
				PushData.FilePath = FullFindFilePath;
				PushData.file_size = FileData.nFileSizeLow;
				PushData.MultiDef = false;
				DirCache[SearchFileName] = PushData;
			}
			else
				itDirCache->second.MultiDef = true;

		} while (FindNextFileA(SearchDirHandle, &FileData));

		FindClose(SearchDirHandle);
	}

	if (DirListHandle)
		fclose(DirListHandle);
	
	if (DirCache.empty())
		return(false);

	return(true);
}

void DumpCache()
{
	Logger::TypedLog(CHN_DLL, "Directory cache :\n");
	for (std::map<std::string, FILEDATA>::iterator DumpRecord = DirCache.begin(), itr_end = DirCache.end(); DumpRecord != itr_end; ++DumpRecord)
	{
		if (DumpRecord->second.MultiDef)
			Logger::TypedLog(CHN_DLL, "%s -> %s (Multiple definitions)\n", DumpRecord->first.c_str(), DumpRecord->second.FilePath.c_str());
		else
			Logger::TypedLog(CHN_DLL, "%s -> %s\n", DumpRecord->first.c_str(), DumpRecord->second.FilePath.c_str());
	}
	Logger::TypedLog(CHN_DLL, "*** End of Directory Cache ***\n");
	return;
}

void CacheConflicts()
{
	bool has_conflicts = false;
	Logger::TypedLog(CHN_DEBUG, "Possible loose file conflicts:\n");
	for(std::map<std::string, FILEDATA>::iterator DumpRecord = DirCache.begin(), itr_end = DirCache.end(); DumpRecord != itr_end; ++DumpRecord)
		if (DumpRecord->second.MultiDef)
		{
			Logger::TypedLog(CHN_DEBUG, "    %s\n", DumpRecord->first.c_str());
			has_conflicts = true;
		}
	if (!has_conflicts)
		Logger::TypedLog(CHN_DEBUG, "    none\n");
	return;
}

const char* TranslateFilePath(const char* FilePath)
{
	std::string FilePathString(FilePath);
	FilePathString = StringToLower(FilePathString);


	std::map<std::string, FILEDATA>::iterator FoundFilePath;

	FoundFilePath = DirCache.find(FilePathString);

	if (FoundFilePath == DirCache.end())
		return(NULL);

	return(FoundFilePath->second.FilePath.c_str());
}

FILEDATA* TranslateFilePathData(const char* FilePath)
{
	std::string FilePathString(FilePath);
	FilePathString = StringToLower(FilePathString);


	std::map<std::string, FILEDATA>::iterator FoundFilePath;

	FoundFilePath = DirCache.find(FilePathString);

	if (FoundFilePath == DirCache.end())
		return(NULL);

	return(&FoundFilePath->second);
}

void ClearDirCache()
{
	DirCache.clear();
	return;
}



