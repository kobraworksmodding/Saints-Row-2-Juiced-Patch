// Math.cpp (Clippy95)
// --------------------
// Created: 1/15/2025

#include "../FileLogger.h"
#include "../Patcher/patch.h"
#include "../GameConfig.h"
#include "Math.h"
#include "../Player/Input.h"
#include "../Hooker.h"
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
		SafetyHookInline sub_9EE620{};
		signed char SSE_hack = 1;
		DWORD* _stdcall sub_9EE620_sse4(DWORD* a1, float a2, float a3, float a4, float a5)
		{
			__m128 v5;
			__m128 v6;
			__m128 v7;
			__m128i v8;
			__m128i v9;
			__m128i v10;
			DWORD* result;
			unsigned int v12;

			v5 = _mm_set_ps(a5, a4, a3, a2);


			v6 = _mm_set1_ps(1.0f);
			v5 = _mm_add_ps(v5, v6);

			v7 = _mm_set1_ps(127.5f);
			v5 = _mm_mul_ps(v5, v7);


			v8 = _mm_cvtps_epi32(v5);


			v9 = _mm_packus_epi32(v8, v8);


			v10 = _mm_packus_epi16(v9, v9);


			v12 = _mm_extract_epi32(v10, 0);

			result = a1;
			*a1 = v12;

			return result;
		}
		DWORD* _stdcall sub_9EE620_sse2(DWORD* a1, float a2, float a3, float a4, float a5)
		{
			__m128 v5;
			__m128 v6;
			__m128 v7;
			__m128i v8;
			DWORD* result;
			union {
				__m128i vec;
				unsigned int arr[4];
			} v10;

			v5 = _mm_set_ps(a5, a4, a3, a2);

			v6 = _mm_set1_ps(1.0f);
			v5 = _mm_add_ps(v5, v6);


			v7 = _mm_set1_ps(127.5f);
			v5 = _mm_mul_ps(v5, v7);

			v8 = _mm_cvtps_epi32(v5);

			v10.vec = v8;

			result = a1;

			*a1 = (v10.arr[0] & 0xFF) |
				((v10.arr[1] & 0xFF) << 8) |
				((v10.arr[2] & 0xFF) << 16) |
				((v10.arr[3] & 0xFF) << 24);

			return result;
		}
		DWORD* _stdcall sub_9EE620_dbg(DWORD* a1, float a2, float a3, float a4, float a5) {
			switch (SSE_hack) {
			case 0:
				return sub_9EE620.stdcall<DWORD*>(a1, a2, a3, a4, a5);
				break;
			case 1:
				return sub_9EE620_sse2(a1, a2, a3, a4, a5);
				break;
			case 2:
				return sub_9EE620_sse4(a1, a2, a3, a4, a5);
				break;
			default:
				return sub_9EE620_sse2(a1, a2, a3, a4, a5);
				break;
			}
		}

		int sub_BDB4F0_SSE4(unsigned char* a1, float* a2, float* a3) {
			__m128i bytes = _mm_loadu_si32(a1);
			__m128i int_vec = _mm_cvtepu8_epi32(bytes);
			__m128 float_vec = _mm_cvtepi32_ps(int_vec);

			const __m128 scale = _mm_set1_ps(2.0f / 255.0f);
			__m128 result_vec = _mm_mul_ps(float_vec, scale);
			result_vec = _mm_sub_ps(result_vec, _mm_set1_ps(1.0f));

			_mm_store_ss(a2, result_vec);

			__m128 shuffled = _mm_shuffle_ps(result_vec, result_vec, _MM_SHUFFLE(1, 1, 1, 1));
			_mm_store_ss(a2 + 1, shuffled);

			shuffled = _mm_shuffle_ps(result_vec, result_vec, _MM_SHUFFLE(2, 2, 2, 2));
			_mm_store_ss(a2 + 2, shuffled);

			shuffled = _mm_shuffle_ps(result_vec, result_vec, _MM_SHUFFLE(3, 3, 3, 3));
			_mm_store_ss(a3, shuffled);

			return a1[3];
		}
		signed char FixWater = 1;
		bool SimulateWaterBug = false;
		void Init() {
			int cpuinfo[4]{};
			__cpuid(cpuinfo, 1);
			bool sse1 = cpuinfo[3] & (1 << 25) || false;
			bool sse2 = cpuinfo[3] & (1 << 26) || false;
			bool sse3 = cpuinfo[2] & (1 << 0) || false;
			bool ssse3 = cpuinfo[2] & (1 << 9) || false;
			bool sse4_1 = cpuinfo[2] & (1 << 19) || false;
			bool sse4_2 = cpuinfo[2] & (1 << 20) || false;

			bool allowMathFix = (sse2 || sse4_1) && GameConfig::GetValue("Debug", "FastMath", 1);
			bool allowMathFixdbg = (sse2 || sse4_1) && (GameConfig::GetValue("Debug", "FastMath", 1) == 255);
			// Idea to fix issue #14 IK/Foot issue getting messed up, the actual call cause is at 0x0x00CE9600, but rn im doing this globally as it makes the most sense - Clippy95
			matrix_operator_multiplication_midhook = safetyhook::create_mid(0x00BE2F57, [](SafetyHookContext& ctx) {
				matrix* result = (matrix*)ctx.eax;
				matrix* thisa = (matrix*)ctx.edx;
				const matrix* m = (matrix*)ctx.ecx;
				result->multiply(thisa, m);
				//matrix_multiply_safe(result, thisa, m);

				ctx.eip = 0x00BE313F;
				});
			if (!allowMathFix)
				return;
			SSE_hack = 1;
			//if (allowMathFix) {
			//	if (sse2 && !sse4_1) {
			//		sub_9EE620 = safetyhook::create_inline(0x9EE620, &sub_9EE620_sse2);
			//		Logger::TypedLog("MATH", "Patching several math functions for better performance: SSE%d\n", 2);
			//		//patchJmp((void*)0x9EE620, &sub_9EE620_sse2);
			//	}
			//	else if (sse4_1) {
			//		sub_9EE620 = safetyhook::create_inline(0x9EE620, &sub_9EE620_sse4);
			//	//patchJmp((void*)0x9EE620, &sub_9EE620_sse4);
			//	Logger::TypedLog("MATH", "Patching several math functions for better performance: SSE%d\n", 4);
			//		SSE_hack = 2;
			//	}
			//}
			//else {
			
			// Okay for some reason the dbg switch case function is faster than letting it route to the SSE2/SSE4 functions?? -- Clippy95
				sub_9EE620 = safetyhook::create_inline(0x9EE620, &sub_9EE620_dbg);
				if (sse4_1)
					SSE_hack = 2;
				else if (sse2)
					SSE_hack = 1;
				Logger::TypedLog("MATH", "Patching several math functions for better performance: SSE%d\n", 4);
			if (sse4_1) {
				auto static sub_BDB4F0_hook = safetyhook::create_mid(0x00BDB4F1, [](SafetyHookContext& ctx) {
					if (SSE_hack == 0)
						return;
					unsigned char* a1 = (unsigned char*)ctx.eax;
					float* a2 = (float*)ctx.ecx;
					float* a3 = (float*)(ctx.esp + 8);
					sub_BDB4F0_SSE4(a1, a2, a3);

					static auto this_return = DynAddress(0x00BDB54E);
					ctx.eip = this_return;
					});
			}

			static vector2 smoothed_cam_dir = vector2::zero();
			static bool smooth_init = false;
			static constexpr float SMOOTH_RATE = 0.08f;
			static float bug_timer = 0.0f;

			// This is a work around and not a proper fix. -- Clippy
			if(GameConfig::GetValue("Debug","FixWaterVolumeCameraBug",1))
			static auto watervol_midhook1 = safetyhook::create_mid(0x72635C, [](SafetyHookContext& ctx) {
				vector2* cam_forward = (vector2*)(ctx.esp + 0x14);

				/*if (SimulateWaterBug) {
					float game_frametime = *(float*)0xE84380;
					game_frametime = (std::min)(game_frametime, 0.1f);

					bug_timer += game_frametime * 15.0f;  // Higher frequency for rapid changes
					float crazy_x = std::sin(bug_timer * 25.0f) * 1.5f;  // Realistic magnitude ~1.5
					float crazy_y = std::cos(bug_timer * 20.0f) * 1.3f;  // Different frequency, ~1.3 max

					// Apply realistic orientation changes
					*cam_forward = vector2(crazy_x, crazy_y);
				}
				*/


					if (Input::g_lastInput != Input::GAME_LAST_INPUT::MOUSE) {
						return;
					}

					if (!smooth_init) {
						smoothed_cam_dir = *cam_forward;
						smooth_init = true;
						return;
					}

					vector2 delta = *cam_forward - smoothed_cam_dir;
					float delta_magnitude = delta.magnitude();
					bool is_rapid_change = delta_magnitude > 1.0f;

					//if (is_rapid_change)
					//	printf("rapid change detected: %.2f\n", delta_magnitude);

					if (!cam_forward->isNull()) {
						float adaptive_rate = is_rapid_change ? 0.005f : SMOOTH_RATE; 


						smoothed_cam_dir = smoothed_cam_dir.lerp(*cam_forward, adaptive_rate);
						*cam_forward = smoothed_cam_dir;
					}
					else {
						// If current vector is null, gradually decay the smoothed vector
						smoothed_cam_dir = smoothed_cam_dir * (1.0f - SMOOTH_RATE);
						// If smoothed vector becomes very small, zero it out
						if (smoothed_cam_dir.magnitude() < 1e-6f) {
							smoothed_cam_dir = vector2::zero();
						}
					}
					return;
				
				});

		}
	}
	void Init() {
		Math::Fixes::Init();
	}
}

void matrix::multiply(const matrix* lhs, const matrix* rhs)
{
	Math::matrix_multiply_safe(this,lhs,rhs);
}
