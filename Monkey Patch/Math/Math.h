#pragma once
#include <chrono>
namespace Math
{
    extern struct Velocity {
        float vx;
        float vy;
        float vz;
        // m/s
        float magnitude;
    };
    extern Velocity CalculateVelocity(uintptr_t CoordsPointer);
}