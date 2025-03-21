// Math.cpp (Clippy95)
// --------------------
// Created: 1/15/2025

#include "../FileLogger.h"
#include "../Patcher/patch.h"
#include "../GameConfig.h"
#include "Math.h"
namespace Math
{
	Velocity CalculateVelocity(uintptr_t CoordsPointer) {
		if (!CoordsPointer)
			return { 0.f, 0.f, 0.f, 0.f };

		static float previousX = 0.0f;
		static float previousY = 0.0f;
		static float previousZ = 0.0f;
		static auto lastTime = std::chrono::high_resolution_clock::now();

		// read current position, maybe we should just pass in x,y,z so it can be applied to anything rather than what was player pointer?

		float* x = (float*)(*(int*)CoordsPointer + 0x30);
		float* y = (float*)(*(int*)CoordsPointer + 0x34);
		float* z = (float*)(*(int*)CoordsPointer + 0x38);

		// Maybe should take game's deltatime as an arg instead.
		auto currentTime = std::chrono::high_resolution_clock::now();
		std::chrono::duration<float> elapsedTime = currentTime - lastTime;


		float deltaTime = elapsedTime.count();
		if (deltaTime <= 0.0f) deltaTime = 1.0f; // dont divide by 0 lol

		float vx = (*x - previousX) / deltaTime;
		float vy = (*y - previousY) / deltaTime;
		float vz = (*z - previousZ) / deltaTime;


		float magnitude = std::sqrt(vx * vx + vy * vy + vz * vz);

		previousX = *x;
		previousY = *y;
		previousZ = *z;
		lastTime = currentTime;

		return { vx, vy, vz, magnitude };
	}
}
