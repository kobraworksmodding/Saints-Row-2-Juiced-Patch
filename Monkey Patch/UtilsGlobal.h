#pragma once
#include <corecrt_math_defines.h>
namespace UtilsGlobal {

inline uintptr_t getplayer(bool provideaddress = false) {
    if (!provideaddress)
        return *(uintptr_t*)(0x21703D4);
    else return 0x21703D4;

}

inline bool invert;

inline bool isPaused;

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