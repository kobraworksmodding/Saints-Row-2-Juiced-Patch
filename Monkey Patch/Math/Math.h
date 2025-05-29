#pragma once
#include <chrono>
#include <safetyhook.hpp>
namespace Math
{
	extern void Init();
    extern struct Velocity {
        float vx;
        float vy;
        float vz;
        float magnitude;
    };
	struct vector3
	{
		float x;
		float y;
		float z;
	};

	struct matrix
	{
		vector3 rvec;
		vector3 uvec;
		vector3 fvec;
	};
	namespace Fixes {
		extern SafetyHookMid matrix_operator_multiplication_midhook;
	}

	inline bool isVectorNull(vector3& vec) {
		if (vec.x != 0 || vec.y != 0 || vec.z != 0) return false;
		else return true;
	}

}
