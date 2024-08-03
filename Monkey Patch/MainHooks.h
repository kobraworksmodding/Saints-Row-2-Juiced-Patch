#pragma once
#include "stdafx.h"

static int *pargc=(int*)0x00ee07c0;
static char ***pargv=(char***)0x00ee07c4;

static bool keepfpslimit=false;

typedef int(WINAPI *WinMain_Type)(HINSTANCE, HINSTANCE, LPSTR, int);
int WINAPI Hook_WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd);

static bool GetVersionExAFirstRun=true;
BOOL __stdcall Hook_GetVersionExA(LPOSVERSIONINFOA lpVersionInformation);
int UpdateByteStuff_Hacked();


