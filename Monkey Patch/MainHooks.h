#pragma once
#include "stdafx.h"

#define CLANTAG_MAX 5 // do we need a max name define?.
static int *pargc=(int*)0x00ee07c0;
static char ***pargv=(char***)0x00ee07c4;

static bool keepfpslimit=false;

typedef int(WINAPI *WinMain_Type)(HINSTANCE, HINSTANCE, LPSTR, int);
int WINAPI Hook_WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd);

static bool GetVersionExAFirstRun=true;
BOOL __stdcall Hook_GetVersionExA(LPOSVERSIONINFOA lpVersionInformation);
int UpdateByteStuff_Hacked();

inline bool menustatus(int status) {
    BYTE currentmenustatus = *(BYTE*)(0x00EBE860);
    if (currentmenustatus == status)
        return true;
    else return false;
}

enum menustatus {
    gameplay = 3,
    busy1 = 2,
    loading = 13,
    buying = 15,
    pausemenu = 20,
    // Occurs while scrolling.
    pausemenescroll1 = 21,
    // Occurs while scrolling.
    pausemenuscroll2 = 22,
    // Might also occur while scrolling.
    pausemenuphone = 23,
    pausemenuphonebook= 24,
};

