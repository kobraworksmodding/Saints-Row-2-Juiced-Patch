// Math.cpp (Clippy95)
// --------------------
// Created: 1/15/2025

#include "../FileLogger.h"
#include "../Patcher/patch.h"
#include "../GameConfig.h"
#include "Math.h"
namespace Math
{
	void matrix_multiply_safe(matrix* result, const matrix* lhs, const matrix* rhs) {
		auto safe_dot = [](float a1, float a2, float a3) -> float {
			return (std::isnan(a1) || std::isnan(a2) || std::isnan(a3)) ? 0.0f : a1 + a2 + a3;
			};

		auto safe_mul = [](float a, float b) -> float {
			return (std::isnan(a) || std::isnan(b)) ? 0.0f : a * b;
			};

		result->rvec.x = safe_dot(safe_mul(lhs->rvec.x, rhs->rvec.x),
			safe_mul(lhs->rvec.y, rhs->uvec.x),
			safe_mul(lhs->rvec.z, rhs->fvec.x));

		result->rvec.y = safe_dot(safe_mul(lhs->rvec.x, rhs->rvec.y),
			safe_mul(lhs->rvec.y, rhs->uvec.y),
			safe_mul(lhs->rvec.z, rhs->fvec.y));

		result->rvec.z = safe_dot(safe_mul(lhs->rvec.x, rhs->rvec.z),
			safe_mul(lhs->rvec.y, rhs->uvec.z),
			safe_mul(lhs->rvec.z, rhs->fvec.z));

		result->uvec.x = safe_dot(safe_mul(lhs->uvec.x, rhs->rvec.x),
			safe_mul(lhs->uvec.y, rhs->uvec.x),
			safe_mul(lhs->uvec.z, rhs->fvec.x));

		result->uvec.y = safe_dot(safe_mul(lhs->uvec.x, rhs->rvec.y),
			safe_mul(lhs->uvec.y, rhs->uvec.y),
			safe_mul(lhs->uvec.z, rhs->fvec.y));

		result->uvec.z = safe_dot(safe_mul(lhs->uvec.x, rhs->rvec.z),
			safe_mul(lhs->uvec.y, rhs->uvec.z),
			safe_mul(lhs->uvec.z, rhs->fvec.z));

		result->fvec.x = safe_dot(safe_mul(lhs->fvec.x, rhs->rvec.x),
			safe_mul(lhs->fvec.y, rhs->uvec.x),
			safe_mul(lhs->fvec.z, rhs->fvec.x));

		result->fvec.y = safe_dot(safe_mul(lhs->fvec.x, rhs->rvec.y),
			safe_mul(lhs->fvec.y, rhs->uvec.y),
			safe_mul(lhs->fvec.z, rhs->fvec.y));

		result->fvec.z = safe_dot(safe_mul(lhs->fvec.x, rhs->rvec.z),
			safe_mul(lhs->fvec.y, rhs->uvec.z),
			safe_mul(lhs->fvec.z, rhs->fvec.z));
	}

	namespace Fixes {
		SafetyHookMid matrix_operator_multiplication_midhook{};
		void Init() {
			// Idea to fix issue #14 IK/Foot issue getting messed up, the actual call cause is at 0x0x00CE9600, but rn im doing this globally as it makes the most sense - Clippy95
			matrix_operator_multiplication_midhook = safetyhook::create_mid(0x00BE2F57, [](SafetyHookContext& ctx) {
				matrix* result = (matrix*)ctx.eax;
				matrix* thisa = (matrix*)ctx.edx;
				const matrix* m = (matrix*)ctx.ecx;
				result->multiply(thisa, m);
				//matrix_multiply_safe(result, thisa, m);

				ctx.eip = 0x00BE313F;
				});
		}
	}
	void Init() {
		if(GameConfig::GetValue("Debug","MathFixes",1))
		Math::Fixes::Init();
	}
}

void matrix::multiply(const matrix* lhs, const matrix* rhs)
{
	Math::matrix_multiply_safe(this,lhs,rhs);
}
