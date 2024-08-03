// Blank DFEngine.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"
#include "DFEngine.h"
#include "FileLogger.h"
#include "SafeWrite.h"
#include "MainHooks.h"
#include "iat_functions.h"
#include "GameConfig.h"

static CDFEngine DFEngine;

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	if(ul_reason_for_call==DLL_PROCESS_ATTACH)
	{
		void * old_proc;
		GameConfig::Initialize();
		Logger::Initialize();

		UInt32 winmaindata=*((UInt32*)0x00520ba0);
		if(winmaindata!=0x83ec8b55)
			Logger::TypedLog(CHN_DLL, "WinMain sanity check failed. Probably running the Steam encrypted version.\n");

/*
We can't hook WinMain yet as the Steam version is still encrypted at this point, so we need to do some coding gymnastics
to get it to work. We will hook the import address table entry for GetVersionExA. This is part of the Kernel32.dll which 
should be one of the first dlls to load, as the loading routines of other dlls require the routines in it. GetVersionExA
is also called by the common runtime startup routines, which initialise the program before the constructor or WinMain 
are called. Fortunately by this time Steam has decrypted the program data. We then use the hooked routine to hook the 
WinMain entry point.
*/

		HMODULE main_handle=GetModuleHandleA(NULL);
		if(PatchIat(main_handle,"Kernel32.dll", "GetVersionExA", (void *)Hook_GetVersionExA, &old_proc)==S_OK)
			Logger::TypedLog(CHN_DLL, "Patched Kernel32.GetVersionExA.\n");
		else
			Logger::TypedLog(CHN_DLL, "Patching Kernel32.GetVersionExA failed.\n");

	}
	
    return TRUE;
}

CDFEngine *CreateDFEngine(void)
{
	// PrintLog->PrintSys("Calling CreateDFEngine.\n");	
	HMODULE dll_loaded;
	typedef CDFEngine* (__stdcall* CreateDFEngine_type)();
	CreateDFEngine_type redirect_CreateDFEngine;


	dll_loaded = LoadLibraryA("pass_DFEngine.dll");

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
	}

	return(&DFEngine);
}

int	CDFEngine::Start(void* param_1, int version, wchar_t** data_directory)
{
	//PrintLog->PrintSys("DFEngine::Start(%x, %S)\n", version, *data_directory);
	return -1;
}

int CDFEngine::StartZone(void* param_1, char* ZoneName)
{
	//PrintLog->PrintSys("DFEngine::StartZone(%s)\n", ZoneName);
	return 0;
}

int	CDFEngine::CreateDFObject(void* param_1, char* ObjectIdent, void* param_3)
{
	//PrintLog->PrintSys("DFEngine::CreateDFObject(%s, %x)\n", ObjectIdent, param_3);
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