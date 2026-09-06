#include "loose files.h"
#include "FileLogger.h"
#include "GameConfig.h"
#include "Patcher/patch.h"
#include <algorithm>
#include <cctype>


#pragma warning(disable : 4996) // remove fopen warning
#pragma warning(disable : 4244) // remove possible loss of data during conversion warning

// This replaces the function that fills in the FILE_INFO structure if the file exists on the disk, with the file's information.

std::string loaded_files_to_render;

bool __stdcall raw_get_file_info_by_name_inner_wrapped(FILE_INFO* file_info, char* filename, BOOL override_check)
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


// Helper function to count lines in a string
int count_lines(const std::string& str) {
	return std::count(str.begin(), str.end(), '\n');
}

// Helper function to remove lines from the beginning until we're under the limit
void trim_to_max_lines(std::string& str, int max_lines) {
	int line_count = count_lines(str);
	if (line_count <= max_lines) return;

	int lines_to_remove = line_count - max_lines;
	size_t pos = 0;

	// Find the position after the lines we want to remove
	for (int i = 0; i < lines_to_remove && pos != std::string::npos; ++i) {
		pos = str.find('\n', pos);
		if (pos != std::string::npos) {
			pos++; // Move past the newline
		}
	}

	if (pos != std::string::npos && pos < str.length()) {
		str = str.substr(pos);
	}
}

bool __stdcall raw_get_file_info_by_name_inner(FILE_INFO* file_info, char* filename, BOOL override_check) {
	auto result = raw_get_file_info_by_name_inner_wrapped(file_info, filename, override_check);



	if (file_info && file_info->filename[0]) {
		loaded_files_push_filename(file_info->filename);
	}
	return result;
}

void loaded_files_push_filename(const char* string) {
	int MAX_LINES = 41;
	if (!*(bool*)0x025272DD) {
		MAX_LINES = 26;
	}
	loaded_files_to_render += std::string(string) + "\n";
	Logger::TypedLog("LOOSE_FILE", "{}\n", string);
	// Trim to maximum lines to prevent infinite growth
	trim_to_max_lines(loaded_files_to_render, MAX_LINES);
}

void* raw_get_file_info_by_name_inner_cb = &raw_get_file_info_by_name_inner;

void loose_files_render_status(bool get_ready_to_append_to_render) {
	if (!get_ready_to_append_to_render) {
		raw_get_file_info_by_name_inner_cb = &raw_get_file_info_by_name_inner_wrapped;
		loaded_files_to_render.clear();
	}
	else {
		raw_get_file_info_by_name_inner_cb = &raw_get_file_info_by_name_inner;
	}
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
		call raw_get_file_info_by_name_inner_cb
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



LooseFileCache DirCache;
LooseFileCache DLCCache;

namespace
{
    // filename struct is 65 bytes. Vanilla descriptors use
    // at most 35 bytes including their leading underscore.
    constexpr size_t MAX_MODPACK_SAVE_PREFIX_LENGTH = 29;
    constexpr size_t SAVEGAME_DESCRIPTOR_FILENAME_SIZE = 65;

    std::string ModpackSavePrefix;
    SafetyHookInline CreateSavegameDescriptorHook;

    const char* get_legacy_save_descriptor(const char* filename)
    {
        if (!filename || ModpackSavePrefix.empty())
            return filename;

        const size_t prefix_length = ModpackSavePrefix.length();
        if (!_strnicmp(filename, ModpackSavePrefix.c_str(), prefix_length) && filename[prefix_length] == '_')
            return filename + prefix_length;

        return filename;
    }

    int __cdecl format_modpack_save_search(char* buffer, const char*, const char* save_path, const char* extension)
    {
        return _snprintf_s(buffer, MAX_PATH, _TRUNCATE, "%s\\%s_*%s", save_path, ModpackSavePrefix.c_str(), extension);
    }

    bool __cdecl parse_legacy_save_descriptor(void* descriptor_info, const char* filename)
    {
        using ParseSavegameDescriptor = bool(__cdecl*)(void*, const char*);
        constexpr uintptr_t PARSE_SAVEGAME_DESCRIPTOR = 0x00695740;
        return reinterpret_cast<ParseSavegameDescriptor>(PARSE_SAVEGAME_DESCRIPTOR)(descriptor_info, get_legacy_save_descriptor(filename));
    }

    void __cdecl create_modpack_save_descriptor(char* descriptor, void* save_game_data, bool is_autosave)
    {
        CreateSavegameDescriptorHook.ccall<void>(descriptor, save_game_data, is_autosave);

        char prefixed_descriptor[SAVEGAME_DESCRIPTOR_FILENAME_SIZE]{};
        _snprintf_s(prefixed_descriptor, sizeof(prefixed_descriptor), _TRUNCATE, "%s%s",
            ModpackSavePrefix.c_str(), get_legacy_save_descriptor(descriptor));
        strcpy_s(descriptor, SAVEGAME_DESCRIPTOR_FILENAME_SIZE, prefixed_descriptor);
    }

    bool is_valid_modpack_save_prefix(const std::string& prefix)
    {
        if (prefix.empty() || prefix.length() > MAX_MODPACK_SAVE_PREFIX_LENGTH)
            return false;

        static constexpr char invalid_filename_characters[] = "<>:\"/\\|?*";
        return std::none_of(prefix.begin(), prefix.end(), [](unsigned char character) {
            return character < 32 || strchr(invalid_filename_characters, character) != nullptr;
        }) && prefix.back() != ' ' && prefix.back() != '.';
    }

    bool read_modpack_save_prefix(const char* path, std::string& prefix)
    {
        FILE* file = fopen(path, "rb");
        if (!file)
            return false;

        char buffer[256]{};
        const size_t bytes_read = fread(buffer, 1, sizeof(buffer) - 1, file);
        const bool read_error = ferror(file) != 0;
        fclose(file);
        if (read_error)
            return false;

        prefix.assign(buffer, bytes_read);
        if (prefix.size() >= 3 && static_cast<unsigned char>(prefix[0]) == 0xEF &&
            static_cast<unsigned char>(prefix[1]) == 0xBB && static_cast<unsigned char>(prefix[2]) == 0xBF)
            prefix.erase(0, 3);

        const auto is_space = [](unsigned char character) { return std::isspace(character) != 0; };
        prefix.erase(prefix.begin(), std::find_if_not(prefix.begin(), prefix.end(), is_space));
        prefix.erase(std::find_if_not(prefix.rbegin(), prefix.rend(), is_space).base(), prefix.end());
        return true;
    }
}

void initialize_modpack_save_prefix()
{
    FILEDATA* prefix_file = TranslateFilePathData("modpack_save.txt");
    if (!prefix_file)
    {
        Logger::TypedLog(CHN_DLL, "No loose modpack_save.txt found; using the vanilla save namespace.\n");
        return;
    }

    std::string prefix;
    if (!read_modpack_save_prefix(prefix_file->FilePath.c_str(), prefix))
    {
        Logger::TypedLog(CHN_DLL, "Unable to read modpack save prefix from {}.\n", prefix_file->FilePath.c_str());
        return;
    }

    if (!is_valid_modpack_save_prefix(prefix))
    {
        Logger::TypedLog(CHN_DLL, "Ignoring invalid modpack save prefix from {} (must be 1-{} valid filename characters).\n",
            prefix_file->FilePath.c_str(), MAX_MODPACK_SAVE_PREFIX_LENGTH);
        return;
    }

    ModpackSavePrefix = std::move(prefix);

    InjectHook(0x00691CBB, reinterpret_cast<void*>(format_modpack_save_search));
    InjectHook(0x00691D6E, reinterpret_cast<void*>(parse_legacy_save_descriptor));
    CreateSavegameDescriptorHook = safetyhook::create_inline(
        0x00695450,
        reinterpret_cast<void*>(create_modpack_save_descriptor));

    Logger::TypedLog(CHN_DLL, "Using modpack save prefix '{}' from {}.\n", ModpackSavePrefix, prefix_file->FilePath.c_str());
}

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

// Recursively scan DLC directory

bool ScanDLCDir(const char* Directory)
{
    if (!(GameConfig::GetValue("DLC", "EnableDLC", 1) >= 1)) return false;
    char CurrentSearch[MAX_PATH];
    char PathBuffer[MAX_PATH];
    WIN32_FIND_DATAA FileData;
    HANDLE SearchDirHandle;

    strcpy_s(CurrentSearch, MAX_PATH, Directory);
    PathAppendA(CurrentSearch, "*");
    SearchDirHandle = FindFirstFileA(CurrentSearch, &FileData);

    if (SearchDirHandle == INVALID_HANDLE_VALUE)
    {
        Logger::TypedLog(CHN_DLL, "Unable to find directory {}\n", Directory);
        return false;
    }

    else {
        Logger::TypedLog(CHN_DLL, "Adding contents of directory {}\n", Directory);
    }

    do
    {
        if (!strcmp(FileData.cFileName, ".") || !strcmp(FileData.cFileName, ".."))
            continue;

        strcpy_s(PathBuffer, MAX_PATH, Directory);
        PathAppendA(PathBuffer, FileData.cFileName);

        if (FileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            ScanDLCDir(PathBuffer);
            continue;
        }

        char* Extension = PathFindExtensionA(FileData.cFileName);
        if (!_stricmp(Extension, ".exe") || !_stricmp(Extension, ".dll") || !_stricmp(Extension, ".bbsave"))
            continue;

        std::string SearchFileName(FileData.cFileName);
        SearchFileName = StringToLower(SearchFileName);

        auto itDLCCache = DLCCache.find(SearchFileName);
        if (itDLCCache == DLCCache.end())
        {
            FILEDATA PushData;
            std::string FullFindFilePath(PathBuffer);
            PushData.FilePath = FullFindFilePath;
            PushData.file_size = FileData.nFileSizeLow;
            PushData.MultiDef = false;
            DLCCache[SearchFileName] = PushData;
        }
        else
            itDLCCache->second.MultiDef = true;

    } while (FindNextFileA(SearchDirHandle, &FileData));

    FindClose(SearchDirHandle);
    return true;
}

bool CreateCache(char* DirListFile)
{   
	FILE *DirListHandle = fopen(DirListFile, "r");
	if (!DirListHandle)
	{
		Logger::TypedLog(CHN_DLL, "Failed to open directory list file {}\n", DirListFile);
		return(false);
	}

	Logger::TypedLog(CHN_DLL, "Creating cache directory data from {}\n", DirListFile);

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
			Logger::TypedLog(CHN_DLL, "Unable to find directory {}\n", CurrentDirectory);
			continue;
		}

		Logger::TypedLog(CHN_DLL, "Adding contents of directory {}\n", CurrentDirectory);
		
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
            auto itDirCache = DirCache.find(SearchFileName);

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
	for (auto DumpRecord = DirCache.begin(), itr_end = DirCache.end(); DumpRecord != itr_end; ++DumpRecord)
	{
		if (DumpRecord->second.MultiDef)
			Logger::TypedLog(CHN_DLL, "{} -> {} (Multiple definitions)\n", DumpRecord->first.c_str(), DumpRecord->second.FilePath.c_str());
		else
			Logger::TypedLog(CHN_DLL, "{} -> {}\n", DumpRecord->first.c_str(), DumpRecord->second.FilePath.c_str());
	}
	Logger::TypedLog(CHN_DLL, "*** End of Directory Cache ***\n");
	return;
}

void CacheConflicts()
{
    bool has_conflicts = false;
    Logger::TypedLog(CHN_DEBUG, "Possible loose file conflicts:\n");
	for(auto DumpRecord = DirCache.begin(), itr_end = DirCache.end(); DumpRecord != itr_end; ++DumpRecord)
		if (DumpRecord->second.MultiDef)
		{
			Logger::TypedLog(CHN_DEBUG, "    {}\n", DumpRecord->first.c_str());
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

    auto FoundFilePath = DirCache.find(FilePathString);
    if (FoundFilePath != DirCache.end())
        return(FoundFilePath->second.FilePath.c_str());

    FoundFilePath = DLCCache.find(FilePathString);
    if (FoundFilePath != DLCCache.end())
        return(FoundFilePath->second.FilePath.c_str());

    return(NULL);
}

FILEDATA* TranslateFilePathData(const char* FilePath)
{
    std::string FilePathString(FilePath);
    FilePathString = StringToLower(FilePathString);

    auto FoundFilePath = DirCache.find(FilePathString);
    if (FoundFilePath != DirCache.end())
        return(&FoundFilePath->second);

    FoundFilePath = DLCCache.find(FilePathString);
    if (FoundFilePath != DLCCache.end())
        return(&FoundFilePath->second);

    return(NULL);
}

void ClearDirCache()
{
    DirCache.clear();
    DLCCache.clear();
    return;
}

struct VPPFile
{
    const char* FileName;
    const char* Extension;
};

struct VPPFileData
{
    unsigned int Hash;
    VPPFile* Data;
};

int FileHashExists(const char* FileName) {
    return ((int(__cdecl*)(const char*))0xC0A3B0)(FileName);
}

void AddFileHash(VPPFileData* Struct) {
    ((void(__cdecl*)(VPPFileData*))0xC0A350)(Struct);
}

int GetStringHash(const char* String) {
    return ((int(__fastcall*)(int, const char*))0xBF2BD0)(0, String);
}

void ProcessCacheHashes(LooseFileCache& Cache) {

    const char* validExts[] = {
        ".cmesh_pc", ".g_cmesh_pc", ".peg_pc", ".g_peg_pc", ".pcm_pc", ".sim_pc", ".cvtf", ".morph_pc",
        ".car_pc", ".g_car_pc", ".smesh_pc", ".g_smesh_pc", ".chunk_pc", ".g_chunk_pc", ".effect_pc",
        ".g_effect_pc"
    };

    int extCount = sizeof(validExts) / sizeof(validExts[0]);

    for (auto it = Cache.begin(), it_end = Cache.end(); it != it_end; ++it) {
        const std::string& filename = it->first;
        const char* matchedExt = NULL;
        for (int i = 0; i < extCount; i++) {
            const char* ext = validExts[i];
            size_t extLen = strlen(ext);
            if (filename.length() >= extLen) {
                if (filename.compare(filename.length() - extLen, extLen, ext) == 0) {
                    matchedExt = ext;
                    break;
                }
            }
        }
        if (!matchedExt)
            continue;

        if (!FileHashExists(filename.c_str())) {
            int Hash = GetStringHash(filename.c_str());
            std::string BaseName = filename.substr(0, filename.length() - strlen(matchedExt));
            std::string Extension = (matchedExt[0] == '.') ? matchedExt + 1 : matchedExt;
            char* tempBase = _strdup(BaseName.c_str());
            char* tempExt = _strdup(Extension.c_str());
            VPPFile* Entry = new VPPFile();
            Entry->FileName = tempBase;
            Entry->Extension = tempExt;
            VPPFileData* Data = new VPPFileData();
            Data->Hash = Hash;
            Data->Data = Entry;
            AddFileHash(Data);
        }
    }
}

void InsertFileHashes(SafetyHookContext& ctx) {
    ProcessCacheHashes(DirCache);
    ProcessCacheHashes(DLCCache);
}
