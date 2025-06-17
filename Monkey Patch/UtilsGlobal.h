#pragma once
#include <corecrt_math_defines.h>
#include "..\Generated\BuildInfo.h"
#include "Math\Math.h"
#pragma warning( disable : 4219)
using namespace Math;

namespace UtilsGlobal {

inline uintptr_t getplayer(bool provideaddress = false) {
    if (!provideaddress)
        return *(uintptr_t*)(0x21703D4);
    else return 0x21703D4;

}

inline int RetZero() {
    return 0;
}

// Returns the address last in the chain, for example if value of ADDRESS (0x1)
// 0x1 + 0x2 = 0x4,
// and value of ADDRESS (0x4)
// 0x4 + 0x1 = 0x9, 
// then ReadPointer(0x1,{0x2,0x1}); will return 0x9.
inline uintptr_t ReadPointer(uintptr_t baseAddress, const std::vector<uintptr_t>& offsets) {
    uintptr_t address = baseAddress;

    if (address == 0) {
        return 0;
    }

    for (size_t i = 0; i < offsets.size(); ++i) {
        uintptr_t* nextAddress = reinterpret_cast<uintptr_t*>(address);
        if (nextAddress == nullptr || *nextAddress == 0) {
            return 0;
        }
        address = *nextAddress + offsets[i];
    }

    return address;
}

inline void GetPlayerXYZ(vector3* Dest) {
    memcpy(Dest, (void*)(0x25F5BB4), sizeof(vector3));
}

inline void GetObjectXYZ(vector3* Dest, int ObjectPointer) {
    memcpy(Dest, (void*)(ObjectPointer + 0x14), sizeof(vector3));
}

inline void GetPlayerOrient(matrix* Dest) {
    memcpy(Dest, (void*)(UtilsGlobal::getplayer() + 32), sizeof(matrix));
}

inline bool invert;

inline bool isPaused;

#if JLITE
inline const char* juicedversion = "1.5.1";
#else
inline const char* juicedversion = "7.8.0";
#endif

inline const char* thaRowmenuversion = "0.0.1";
inline const char* thaRowversion = "%s - thaRow 0.1beta";

inline char* lobby_list[2] = {
        const_cast<char*>("sr2_mp_lobby02"),
        const_cast<char*>("sr2_mp_lobby03")
};

inline std::string getShortCommitHash(size_t length = 7) {
    return std::string(GIT_COMMIT_HASH).substr(0, length);
}

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
    inline float getMouseX_sens() const {
        return *reinterpret_cast<const float*>(0x025F5C98);
    }
    inline float getMouseY_sens() const {
        return *reinterpret_cast<const float*>(0x025F5C9C);
    }
};

template <typename T>
T clamp(T value, T min, T max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

inline float DegreetoRadians(float degree) {
    return (float)(degree * (M_PI / 180));
}

inline float RadianstoDegree(float degree) {
    return (float)(degree * (180 / M_PI));
}

}