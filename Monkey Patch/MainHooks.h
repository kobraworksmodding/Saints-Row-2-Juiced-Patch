#pragma once
#include "stdafx.h"
#include <corecrt_math_defines.h>

#define CLANTAG_MAX 5 // do we need a max name define?.
static int *pargc=(int*)0x00ee07c0;
static char ***pargv=(char***)0x00ee07c4;

static bool keepfpslimit=false;

typedef int(WINAPI *WinMain_Type)(HINSTANCE, HINSTANCE, LPSTR, int);
int WINAPI Hook_WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd);

static bool GetVersionExAFirstRun=true;
BOOL __stdcall Hook_GetVersionExA(LPOSVERSIONINFOA lpVersionInformation);
int UpdateByteStuff_Hacked();

struct mouse {
    static constexpr uintptr_t x_delta_address = 0x234F45C;
    static constexpr uintptr_t y_delta_address = 0x234F458;
    static constexpr uintptr_t scroll_delta_address = 0x234EA34;
    // Returns Mouse X delta in signed int32.
    inline int32_t getXdelta() const {
        return *reinterpret_cast<const int32_t*>(x_delta_address);
    }
    // Returns Mouse Y delta in signed int32.
    inline int32_t getYdelta() const {
        return *reinterpret_cast<const int32_t*>(y_delta_address);
    }
    // Returns Mouse Scroll Wheel delta, 120 = one scroll up -120 one scroll down.
    inline int32_t getWheeldelta() const {
        return *reinterpret_cast<const int32_t*>(scroll_delta_address);
    }

};
inline bool menustatus(int status) {
    BYTE currentmenustatus = *(BYTE*)(0x00EBE860);
    if (currentmenustatus == status)
        return true;
    else return false;
}

inline int getplayer() {
    return *(int*)(0x21703D4);

}

inline float DegreetoRadians(float degree) {
    return (float)(degree * (M_PI / 180));
}

inline float RadianstoDegree(float degree) {
    return (float)(degree * (180 / M_PI));
}

template <typename T>
T clamp(T value, T min, T max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
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

