// Blank DFEngine.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"
#include "DFEngine.h"
#include "FileLogger.h"
#include "SafeWrite.h"
#include "MainHooks.h"
#include "iat_functions.h"
#include "GameConfig.h"
#include "LAAPatcher.h"
#include <vector>
#include "Ext/Hooking.Patterns.h"
#include <Windows.h>
#include <direct.h>

static CDFEngine DFEngine;
static CDFObjectInstance fake_CDFObject;

int address_offset = 0;

static std::vector<std::wstring> find_billboards_list;
static int number_of_billboard_files;

VOID startup(LPCTSTR lpApplicationName)
{
	// additional information
	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	// set the size of the structures
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));

	// start the program up
	CreateProcess(lpApplicationName,   // the path
		NULL,        // Command line
		NULL,           // Process handle not inheritable
		NULL,           // Thread handle not inheritable
		FALSE,          // Set handle inheritance to FALSE
		0,              // No creation flags
		NULL,           // Use parent's environment block
		NULL,           // Use parent's starting directory 
		&si,            // Pointer to STARTUPINFO structure
		&pi             // Pointer to PROCESS_INFORMATION structure (removed extra parentheses)
	);
	// Close process and thread handles. 
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
}
LONG WINAPI CustomUnhandledExceptionFilter(LPEXCEPTION_POINTERS ExceptionInfo);
BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	if(ul_reason_for_call==DLL_PROCESS_ATTACH)
	{
		if (GameConfig::GetValue("Logger", "ExceptionHandler", 1))
		{
			Logger::TypedLog(CHN_DLL, "Enabling ExceptionHandler.\n");
			_mkdir("Juiced");
			_mkdir("Juiced\\CrashDumps");
			_mkdir("Juiced\\logs");
			SetUnhandledExceptionFilter(CustomUnhandledExceptionFilter);
			uint32_t ret = 0x900004C2; //ret4
			DWORD protect[2];
			VirtualProtect(&SetUnhandledExceptionFilter, sizeof(ret), PAGE_EXECUTE_READWRITE, &protect[0]);
			memcpy(&SetUnhandledExceptionFilter, &ret, sizeof(ret));
			VirtualProtect(&SetUnhandledExceptionFilter, sizeof(ret), protect[0], &protect[1]);
			// Create the log directory structure

		}


		void * old_proc;
		GameConfig::Initialize();
		Logger::Initialize();

		HMODULE main_handle = GetModuleHandleA(NULL);

		if ((unsigned int)main_handle != 0x0400000)
		{
			address_offset = (unsigned int)main_handle - 0x0400000;
			Logger::TypedLog(CHN_DLL, "Executable base doesn't match default. Base = 0x%08X offset %i\n", (unsigned int)main_handle, address_offset);
		}
		UInt32 winmaindata = *((UInt32*)offset_addr(0x00520ba0));
		if (winmaindata != 0x83ec8b55) {
			Logger::TypedLog(CHN_DLL, "WinMain sanity check failed. Probably running the Steam encrypted version.\n");

			auto pattern = hook::pattern("FD A1 CB 99 95 E9 F1 31 94 B8 5D 09 17 41");
			if (!pattern.empty()) {
				Logger::TypedLog(CHN_DLL, "We've now ensured that the running exe is a Steam EXE. Patching using xdelta to ensure unencrypted data.\n\n\n");
				std::wstring rt = L"JuicedSteamXDel.bat";
				startup(rt.c_str());
				exit(0);
			}

			if (MessageBoxA(NULL, "Possible Steam game executable detected.\n\nAs a massive warning, the steam executable for SR2 is really un-stable and will cause more crashes than you should ever need.\n\nIf you want increased stability it is recommended you either find a download for the GOG executable for your game as it's a fixed version of the SR2 executable and is compatible with the steam game files, or you run Steamless on your Steam SR2 Game Executable.\n\nIf you run steamless on your Steam SR2 Executable and run SR2 with Juiced again, Juiced Patch will LAA (Large Address Aware) patch your game executable preventing most crashes.\n\nWould you like to ignore this warning and keep playing?", "Saints Row 2 Juiced Patch", MB_ICONEXCLAMATION | MB_YESNO) == IDNO) {
				exit(0);
			}
		}
		else 
		{

			LAAPatcher::LAACheck();
		}

/*
We can't hook WinMain yet as the Steam version is still encrypted at this point, so we need to do some coding gymnastics
to get it to work. We will hook the import address table entry for GetVersionExA. This is part of the Kernel32.dll which 
should be one of the first dlls to load, as the loading routines of other dlls require the routines in it. GetVersionExA
is also called by the common runtime startup routines, which initialise the program before the constructor or WinMain 
are called. Fortunately by this time Steam has decrypted the program data. We then use the hooked routine to hook the 
WinMain entry point.
*/

		if(PatchIat(main_handle,(char*)"Kernel32.dll", (char*)"GetVersionExA", (void *)Hook_GetVersionExA, &old_proc)==S_OK)
			Logger::TypedLog(CHN_DLL, "Patched Kernel32.GetVersionExA.\n");
		else
			Logger::TypedLog(CHN_DLL, "Patching Kernel32.GetVersionExA failed.\n");

	}
	
    return TRUE;
}

CDFEngine* CreateDFEngine(void)
{
	HMODULE dll_loaded;
	typedef CDFEngine* (__stdcall* CreateDFEngine_type)();
	CreateDFEngine_type redirect_CreateDFEngine;
	Logger::TypedLog(CHN_MOD, "Calling CreateDFEngine.\n");


	/*dll_loaded = LoadLibraryA("pass_DFEngine.dll");

	if (dll_loaded)
	{
		Logger::TypedLog(CHN_DLL, "Passthrough DFEngine exists. Attempting passthrough.\n");
		redirect_CreateDFEngine = (CreateDFEngine_type)GetProcAddress(dll_loaded, "CreateDFEngine");
		if (redirect_CreateDFEngine)
		{
			Logger::TypedLog(CHN_DLL, "Passthrough succeeded.\n");
			return(redirect_CreateDFEngine());
		}
		else
			Logger::TypedLog(CHN_DLL, "Passthrough failed. Using fake DFEngine.\n");
	}*/

	set_up_billboard_stuff();

	return(&DFEngine);
}

void set_up_billboard_stuff()
{
	HANDLE find_handle;
	WIN32_FIND_DATAW find_file_data;

	wchar_t game_directory[MAX_PATH];
	wchar_t search_path[MAX_PATH];
	int path_size;

	//PrintLog->PrintSys("Setting up billboards.\n");

	GetCurrentDirectoryW(MAX_PATH, game_directory);
	path_size = wcslen(game_directory);

	wcscpy(search_path, game_directory);
	wcscpy(&search_path[path_size], L"\\billboards\\*.*");

	memset(&find_file_data, 0, sizeof(find_file_data));
	find_handle = FindFirstFileW(search_path, &find_file_data);

	if (find_handle == INVALID_HANDLE_VALUE)
	{
		number_of_billboard_files = 0;
	}
	else
	{
		while (FindNextFileW(find_handle, &find_file_data))
		{
			if (wcslen(find_file_data.cFileName) > 3)
			{
				find_billboards_list.push_back(find_file_data.cFileName);
			}
		}
		number_of_billboard_files = find_billboards_list.size();
	}

	Logger::TypedLog(CHN_MOD, "Found %i billboard files\n", number_of_billboard_files);

	FindClose(find_handle);
	return;
}

int	CDFObjectInstance::GetAbsoluteFilename(void* that, wchar_t* filenamepath_out, int param_3)
{
	wchar_t* default_ad_dir = (wchar_t*)L"\\data\\DFEngine\\cache\\data\\Default\\Default.tga";

	wchar_t game_directory[MAX_PATH];
	int path_size;

	//PrintLog->PrintSys("CDFObjectInstance::GetAbsoluteFilename\n");

	GetCurrentDirectoryW(MAX_PATH, game_directory);
	path_size = wcslen(game_directory);

	if (number_of_billboard_files == 0)
	{
		wcscpy(filenamepath_out, game_directory);
		wcscpy(&filenamepath_out[path_size], default_ad_dir);
		Logger::TypedLog(CHN_DLL, "Billboard directory not found or empty. Using default billboard.\n");
		//PrintLog->PrintSys("CDFObjectInstance::GetAbsoluteFilename(%S)\n",filenamepath_out);
	}
	else
	{
		//PrintLog->PrintSys("Number of files: %i",number_of_billboard_files);
		wcscpy(filenamepath_out, game_directory);
		wcscpy(&filenamepath_out[path_size], L"\\billboards\\");
		wcscpy(&filenamepath_out[path_size + 12], find_billboards_list[rand() % number_of_billboard_files].c_str());

		//PrintLog->PrintSys("CDFObjectInstance::GetAbsoluteFilename(%S)\n",filenamepath_out);
	}
	return 0;
}

int	CDFObjectInstance::UpdateOnEvent(void* that, int param_2, float* param_3)
{
	//PrintLog->PrintSys("CDFObjectInstance::UpdateOnEvent\n");
	return 0;
}

int	CDFObjectInstance::SetLocalBoundingBox(void* that, float* param_2, float* param_3)
{
	//PrintLog->PrintSys("CDFObjectInstance::SetLocalBoundingBox\n");
	return 0;
}

float* CDFObjectInstance::SetLocalLookAt(void* that, float* param_2)
{
	//PrintLog->PrintSys("CDFObjectInstance::SteLocalLookAt\n");
	return param_2;
}

int	CDFEngine::Start(void* param_1, int version, wchar_t** data_directory)
{
	//PrintLog->PrintSys("DFEngine::Start(%x, %S)\n", version, *data_directory);
	return 0;
}

int CDFEngine::StartZone(void* param_1, char* ZoneName)
{
	//PrintLog->PrintSys("DFEngine::StartZone(%s)\n", ZoneName);
	return 0;
}

int	CDFEngine::CreateDFObject(void* param_1, char* ObjectIdent, CDFObjectInstance** param_3)
{
	//PrintLog->PrintSys("DFEngine::CreateDFObject(%s, %x)\n", ObjectIdent, param_3);
	*param_3 = &fake_CDFObject;
	return 0;
}

void CDFEngine::Update(void* param_1, float TimeSinceLastUpdate)
{
	//PrintLog->PrintSys("DFEngine::Update(%f)\n",TimeSinceLastUpdate);
	return;
}

void CDFEngine::SetCameraViewMatrix(void* param_1, float* pViewMatrix)
{
	//PrintLog->PrintSys("DFEngine::SetCameraViewMatrix(%f, %f, %f, %f)",pViewMatrix[0],pViewMatrix[1],pViewMatrix[2],pViewMatrix[3]);
	return;
}

void CDFEngine::SetCameraProjMatrix(void* param_1, float* pProjMatrix)
{
	//PrintLog->PrintSys("DFEngine::SetCameraProjMatrix(%f, %f, %f, %f)",pProjMatrix[0],pProjMatrix[1],pProjMatrix[2],pProjMatrix[3]);
	return;
}