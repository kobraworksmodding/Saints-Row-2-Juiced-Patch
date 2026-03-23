// Player/Behavior.cpp (uzis, Tervel)
// --------------------
// Created: 13/12/2024

#include "Behavior.h"

#include "../FileLogger.h"
#include "../Patcher/patch.h"
#include "../SafeWrite.h"
#include "../GameConfig.h"
#include "../Patcher/CPatch.h"
#include "../Patcher/CMultiPatch.h"
#include <safetyhook.hpp>
#include "../BlingMenu_public.h"
#include "../UtilsGlobal.h"
#include "..\Game\Game.h"
#pragma warning( disable : 4834)
// Use me to store garbagedata when NOP doesn't work.
static float garbagedata = 0;
double bogusPi = 2.90;
double bogusRagForce = 2.5;
double animBlend = 3.0;

namespace Behavior
{
	typedef int __cdecl character_set_anim_setT(DWORD* hp, uint32_t unk, char* name_of_human);
	character_set_anim_setT* character_set_anim_set = (character_set_anim_setT*)(0x0096F940);
	int sticky_cam_timer_add = 0;
	void BetterMovement()
	{
		// Majority of the SR2 movement sluggishness is due to the fact that certain walking anims add an
		// increased latency to walking generally and 180 anims tend to play constantly when trying to strafe, 180 anims didnt exist in SR1.
		Logger::TypedLog(CHN_MOD, "Patching In Better Movement Behavior...\n");
		patchNop((BYTE*)0x00E9216C, 3); // Jog180
		patchNop((BYTE*)0x00E9213C, 3); // Stand180
		patchNop((BYTE*)0x00E92130, 3); // WalkStop
		patchNop((BYTE*)0x00E9237C, 3); // StandToRun
		patchNop((BYTE*)0x00E92370, 3); // RunToStop
		patchNop((BYTE*)0x00E92364, 3); // WalkToStop
		patchNop((BYTE*)0x00E92394, 3); // WalkToStand
		patchNop((BYTE*)0x00E92388, 3); // StandToWalk
	}
	
	int FindMeleeTarget(int NPCPointer) {
		return ((int(__cdecl*)(int))0x974FB0)(NPCPointer);
	}

	bool IsNPCDead(int NPCPointer) {
		return ((bool(__fastcall*)(int))0x9855F0)(NPCPointer);
	}

	int __cdecl DisableMeleeLockon1(int NPCPointer) {
		if (NPCPointer == UtilsGlobal::getplayer()) return 0;
		else return FindMeleeTarget(NPCPointer);
	}

	void __declspec(naked) DisableMeleeLockon2() {
		static int Continue = 0x00974D06;
		int NPCPointer;
		__asm {
			mov NPCPointer, eax
		}
		if (NPCPointer != UtilsGlobal::getplayer()) {
			__asm {
				mov eax, NPCPointer
				mov ecx, 0x973360
				call ecx
			}
		}
		else __asm mov al, 1
		__asm jmp Continue
	}

	void MovingAttacks() {
		patchBytesM((BYTE*)0x00981EE5, (BYTE*)"\xEB\x10", 2);
		patchJmp((void*)0x981F40, UtilsGlobal::RetZero);
		patchJmp((void*)0x982380, UtilsGlobal::RetZero);
	}

	void AllowToggleCrouchWhileWalk() {
		Logger::TypedLog(CHN_DEBUG, "Allow Toggle Crouch to work while walking...\n");
		patchNop((BYTE*)0x004F9944, 2);
	}
CMultiPatch CMPatches_SR1Reloading = {

		[](CMultiPatch& mp) {
			mp.AddPatchNop(0x009F1A9C, 5);
		},

		[](CMultiPatch& mp) {
			mp.AddPatchNop(0x009F1ACE, 5);
		},

		[](CMultiPatch& mp) {
			mp.AddSafeWrite8(0x009F1A4C, 0x75);
		},
};
	void SR1Reloading()
	{
		//This fixes the functionality to be able to reload while sprinting.
		Logger::TypedLog(CHN_MOD, "Patching In ReloadDuringSprint...\n");
		/*patchNop((BYTE*)0x009F1A9C, 5);
		patchNop((BYTE*)0x009F1ACE, 5);
		patchByte((BYTE*)0x009F1A4C, 0x75);*/
		CMPatches_SR1Reloading.Apply();
	}
	CMultiPatch CMPatches_SR1QuickSwitch = {

			[](CMultiPatch& mp) {
				mp.AddPatchNop(0x0079266D, 6);
			},

			[](CMultiPatch& mp) {
				mp.AddPatchNop(0x004F943E, 14);
			},

			[](CMultiPatch& mp) {
				mp.AddPatchNop(0x00797003, 5);
			},
	};

	CMultiPatch CMPatches_NoMeleeLockOn = {

			[](CMultiPatch& mp) {
				mp.AddWriteRelCall(0x0097C7BC, (uintptr_t)&DisableMeleeLockon1);
				mp.AddWriteRelCall(0x0097F29F, (uintptr_t)&DisableMeleeLockon1);
				mp.AddWriteRelJump(0x00974D01, (uintptr_t)&DisableMeleeLockon2);
			},
	};

	CPatch CSR1CrouchCam = CPatch::PatchNop(0x0049B718, 2);
	void SR1QuickSwitch()
	{
		// Fixes broken weapon wheel implementation and brings back quick switching.

		Logger::TypedLog(CHN_MOD, "Patching in Weapon Quick Switching...\n");
		/*patchNop((BYTE*)0x0079266D, 6);
		patchNop((BYTE*)0x004F943E, 14);
		patchNop((BYTE*)0x00797003, 5);*/
		CMPatches_SR1QuickSwitch.Apply();
	}

	void __declspec(naked) TauntLeft()
	{
		static int jmp_continue = 0x004F8323;
		__asm {
			push 1
			push 1
			push 0
			push - 1
			push - 1
			push 1
			push 1
			push 1
			jmp jmp_continue
		}
	}

	void __declspec(naked) TauntRight()
	{
		static int jmp_continue = 0x004F833F;
		__asm {
			push 1
			push 1
			push 0
			push - 1
			push - 1
			push 1
			push 1
			push 0
			jmp jmp_continue
		}
	}
	CMultiPatch CMPatches_TauntCancelling = {

	[](CMultiPatch& mp) {
		mp.AddWriteRelJump(0x004F8315, (UInt32)&TauntLeft);
	},

	[](CMultiPatch& mp) {
		mp.AddWriteRelJump(0x004F8332, (UInt32)&TauntRight);
	},

	[](CMultiPatch& mp) {
		mp.AddSafeWriteBuf(0x00964F77 + 1, "\x00\x00",2);
	},
	};
	void TauntCancelling()
	{
		// Makes it so you can cancel out Taunts. 
		Logger::TypedLog(CHN_MOD, "Patching In TauntCancelling...\n");
		CMPatches_TauntCancelling.Apply();
	}
	CMultiPatch CMPatches_UseWeaponAfterEmpty = {

	[](CMultiPatch& mp) {
		mp.AddSafeWrite8(0x9D95F0, 0xC3);
	},

	[](CMultiPatch& mp) {
		mp.AddPatchNop(0x0055B496, 2);
	},
	};
	void WeaponJam()
	{
		Logger::TypedLog(CHN_MOD, "Patching In UseWeaponAfterEmpty...\n");
		CMPatches_UseWeaponAfterEmpty.Apply();
	}

	void HigherMaxSpeed()
	{
		Logger::TypedLog(CHN_MOD, "Patching Higher Max Speed...\n");
		patchNop((void*)0x00AEB541, 2);
		patchNop((void*)(0x00AEB541 + 0x61), 2);
	}

	void FasterDoors()
	{
		Logger::TypedLog(CHN_DEBUG, "Patching Fast Doors...\n");
		patchNop((BYTE*)0x00E92268, 3);
		patchNop((BYTE*)0x00E9225C, 3);
	}
	CPatch CBetterDBC = CPatch::SafeWrite32(0x00498689 + 2, reinterpret_cast<uint32_t>(&garbagedata));
	void BetterDBC()
	{
		Logger::TypedLog(CHN_DEBUG, "Patching Better Drive-by Cam...\n");
		//patchBytesM((BYTE*)0x00498689 + 2, (BYTE*)"\x71\x5D", 2);
		CBetterDBC.Apply();

	}
	CPatch CBetterHBC = CPatch::SafeWrite8(0xE99886, 0);
	void BetterHBC()
	{
		Logger::TypedLog(CHN_DEBUG, "Patching Better Handbrake Cam...\n");
		//patchBytesM((BYTE*)0x004992a2 + 2, (BYTE*)"\x71\x5D", 2);
		CBetterHBC.Apply();
	}
	CMultiPatch CMPatches_DisableLockedClimbCam = {

		[](CMultiPatch& mp) {
			mp.AddSafeWrite32(0x0049BD70 + 2, (uint32_t)&garbagedata);
		},

		[](CMultiPatch& mp) {
			mp.AddSafeWrite32(0x0049BD9C + 2, (uint32_t)&garbagedata);
		},
	};
	void DisableLockedClimbCam()
	{
		Logger::TypedLog(CHN_DEBUG, "Disable Camera Lock during climb...\n");
		//SafeWrite32(0x0049BD70 + 2, (UInt32)&garbagedata); // X-Axis
		//SafeWrite32(0x0049BD9C + 2, (UInt32)&garbagedata); // Y-Axis
		CMPatches_DisableLockedClimbCam.Apply();
	}

	CPatch CDisableSprintCamShake = CPatch::SafeWrite32(0xE997FC,0);

	SafetyHookMid cf_do_control_mode_sticky_MIDASMHOOK;

	// This is modifiable in the game's xtbl, they're called `Default_Time` and `Stopped_Time`, 
	// same effect could probably be achieved with modifying the 1000's ms doubles at 0049EA59+ but this is just to showcase safetyhook and I guess to seperate Stopped_Time
	void sticky_cam_modifier(safetyhook::Context32& ctx) {
		if (sticky_cam_timer_add > 0) {
			// eax = return of LTO'd?? compiled timestamp:set which seems to have made a custom usercall function just for vehicles, not complaining lol.
			// if Default_Time (assuming default xtbl values)
			if(ctx.eax > 500)
			ctx.eax += sticky_cam_timer_add;
			// else it's Stopped_Time
			else
			ctx.eax += (sticky_cam_timer_add / 2);
		}
		//printf("Sticky cam timestamp %d \n", ctx.eax);
	}
	SafetyHookMid slewmode_mousefix_rewrite;

	 SAFETYHOOK_NOINLINE void slewmode_control_rewrite(safetyhook::Context32& ctx) {
		using namespace UtilsGlobal;

		float mouse_x = (mouse().getXdelta() / 30.f) * mouse().getMouseX_sens();
		// re-inverted because Tervel inverted it somewhere.
		float mouse_y = (-mouse().getYdelta() / 30.f) * mouse().getMouseY_sens();
		float* rs_x = (float*)(0x023485B4);
		float* rs_y = (float*)(0x023485B8);
		float& FOV = *(float*)0x25F5BA8;
		mouse_x *= Game::Timer::Get33msOverFrameTime_Fix();

		mouse_y *= Game::Timer::Get33msOverFrameTime_Fix();

		ctx.xmm1.f32[0] = (mouse_x + *rs_x) * (FOV / 58.f);
		ctx.xmm0.f32[0] = (mouse_y + *rs_y) * (FOV / 58.f);
	}

#if !JLITE
	CPatch CAnimBlend = CPatch::SafeWrite32(0x006F1CA6 + 2, (uint32_t)&animBlend);
	CPatch CAllowWeaponSwitchInAllCases_KBM = CPatch::PatchNop(0x004F7F1E, 0x23);
#endif
	SafetyHookInline player_data_loadT{};
	unsigned int __cdecl player_data_load(int save_ptr, char arg4) {
		unsigned int result = player_data_loadT.unsafe_ccall<unsigned int>(save_ptr, arg4);
		int gender = *(int*)(save_ptr + 0xE754);
		if (gender == 1) // female
			character_set_anim_set((DWORD*)UtilsGlobal::getplayer(false), 0, (char*)"GFL1");
		else
			character_set_anim_set((DWORD*)UtilsGlobal::getplayer(false), 0, (char*)"GML1");
		return result;
	}
	// Lower values = camera slower panning around car
	float vehicle_camera_follow_modifier = -49.f;
	SafetyHookMid LessCameraVehicleFollow{};
	void LessCameraVehicleFollow_hook_enable_disable() {
		if (vehicle_camera_follow_modifier != 1.f)
			(void)LessCameraVehicleFollow.enable();
		else
			(void)LessCameraVehicleFollow.disable();
		GameConfig::SetDoubleValue("Gameplay", "vehicle_camera_follow_modifier", vehicle_camera_follow_modifier);
	}

	void __fastcall vehicle_get_velocity(void* vehicle, vector3* velocity, uint32_t sub_object_id = -1) {
		((vector3*(__thiscall*)(void*,vector3*,uint32_t))0xAA5150)(vehicle,velocity,sub_object_id);
	}

	inline double __fastcall vehicle_get_brake_value(void* a1, void* a2) {
		return ((double(__fastcall*)(void*, void*))0xAA64A0)(a1, a2);
	}

	double __declspec(naked) vehicle_get_accelerator_value(int vehicle_handle) {
		static uintptr_t vehicle_get_accelerator_value_addr = 0xAA63A0;
		__asm {
			push ebp
			mov ebp, esp
			sub esp, __LOCAL_SIZE


			mov eax, vehicle_handle
			call vehicle_get_accelerator_value_addr

			mov esp, ebp
			pop ebp
			ret
		}
	}

	bool getVehicleType(uintptr_t vehicle_pointer, int type) {
		if (vehicle_pointer == NULL)
			return false;

		uintptr_t first_ptr = *(uintptr_t*)(vehicle_pointer + 0x84E4);
		if (first_ptr == NULL)
			return false;

		return *(uintptr_t*)(first_ptr + 0x9C) == type;
	}

	double __fastcall vehicle_brakelight_fix(void* a1, uintptr_t a2) {
		vector3 vehicle_vel{};
		vehicle_get_velocity((void*)a2, &vehicle_vel);
		if (getVehicleType(a2,0) && vehicle_vel.magnitudeSquared() < 0.005 && vehicle_get_accelerator_value(*(int*)(a2 + 0x44)) == 0.0)
			return 0.3;
		else
			return vehicle_get_brake_value(a1, (void*)a2);
	}

	inline void camera_switch_to_vehicle(uintptr_t player, uintptr_t vehicle,bool tp) {
		((void(__cdecl*)(uintptr_t player, uintptr_t vehicle, bool tp))0xB19F40)(player,vehicle, tp);
	}

	void effect_stop(int param_effect_handle, BOOL hard_stop, int player_sync, int instance_id) {
		static uintptr_t effect_stop_addr = 0x50DDB0;

		__asm {
			pushad
			mov esi, param_effect_handle
			push instance_id
			push player_sync
			push hard_stop
			call effect_stop_addr
			add esp,0xC
			popad
		}

	}

	void Init()
	{
		// (clippy95) fixes https://github.com/kobraworksmodding/SR2IssuesList/issues/46
		// Kneecapper visuals weren't stopped when vehicle is stopped.
		static auto vehicle_kneecapper_unequip_midhook = safetyhook::create_mid(0xADDD92, [](SafetyHookContext& ctx) {

			uintptr_t kneecapper_feature = ctx.edx;
			int* m_effect_handles = (int*)(kneecapper_feature + 0x38);
			auto& wheel_index = ctx.edi;
			effect_stop(m_effect_handles[wheel_index], true, 1, 0);

			});
		if (GameConfig::GetValue("Gameplay", "SR1VehicleCameraTransition", 0) != 0) {
			patchNop((void*)0xB0CB81, 5);
			static auto veh_enter_test = safetyhook::create_mid(0xB0ECBD, [](SafetyHookContext& ctx) {
				uintptr_t vehicle = *(uintptr_t*)(ctx.esp + 0x34);
				uintptr_t human = *(uintptr_t*)(ctx.esp + 0x38);
				if (human && vehicle && human == UtilsGlobal::getplayer()) {
					camera_switch_to_vehicle(human, vehicle, false);
				}
				});
		}
		if(GameConfig::GetValue("Graphics","ProperBrakeLights",1) != 0)
		patchCall((void*)0xAD5C29, vehicle_brakelight_fix);
		if (GameConfig::GetValue("Gameplay", "DisableSprintCamShake", 0)) {
			Logger::TypedLog(CHN_DEBUG, "DisableSprintCamShake..\n");
			CDisableSprintCamShake.Apply();
		}
		LessCameraVehicleFollow = safetyhook::create_mid(0x498B5A, [](SafetyHookContext& ctx) {
			float* follow_camera = (float*)(ctx.ebx + 0x5C);
			if (*follow_camera != -1.f)
				*follow_camera = vehicle_camera_follow_modifier;
			if (*follow_camera != -1.f) {
				ctx.eip = 0x498B60;
			}
			else {
				ctx.eip = 0x498D14;
			}
			},safetyhook::MidHook::StartDisabled);
		// Lower values = camera slower panning around car
		vehicle_camera_follow_modifier = (float)GameConfig::GetDoubleValue("Gameplay", "vehicle_camera_follow_modifier", 1.f);
		LessCameraVehicleFollow_hook_enable_disable();
		if (GameConfig::GetValue("Debug", "FixGFL1_for_female_playas", 1))
			player_data_loadT = safetyhook::create_inline(0x00693EB0, &player_data_load);
		/*patchDWord((void*)(0x00D96A50 + 2), (uint32_t)&bogusRagForce);
		patchDWord((void*)(0x00D974B0 + 2), (uint32_t)&bogusRagForce);
		patchDWord((void*)(0x00D97AE8 + 2), (uint32_t)&bogusRagForce);
		patchDWord((void*)(0x00D981E0 + 2), (uint32_t)&bogusRagForce);*/
	//	if (GameConfig::GetValue("Gameplay", "BetterRagdollJoints", 0))
	//	{
	//		patchDWord((void*)(0x00D26587 + 2), (uint32_t)&bogusPi);
		//}
#if !JLITE
#if !RELOADED
		if (GameConfig::GetValue("Gameplay", "AlwaysDisarmOnRagdoll", 0))
		{
			patchNop((BYTE*)0x009AC517, 11); // NOP out an if statement that holsters on ragdoll.
			patchNop((BYTE*)0x009AC52B, 8);
			patchByte((BYTE*)0x009C3AB8, 0xEB); // jmp a check in can drop weapons.
		}
#endif
		if (GameConfig::GetValue("Gameplay", "SprintWhileOnFire", 0))
		{
			patchCall((BYTE*)0x004F8B45, UtilsGlobal::RetZero);
		}
		if (GameConfig::GetValue("Gameplay", "AllowWeaponSwitchInAllCases", 0))
		{
			CAllowWeaponSwitchInAllCases_KBM.Apply();
		}

		if (GameConfig::GetValue("Gameplay", "BetterAnimBlend", 0))
		{
			CAnimBlend.Apply();
		}

		if (GameConfig::GetValue("Gameplay", "BetterHandbrakeCam", 0)) // Fixes Car CAM Axis while doing handbrakes.
		{
			BetterHBC();
		}

		if (GameConfig::GetValue("Gameplay", "BetterDriveByCam", 1)) // Fixes Car CAM Axis while doing drive-bys.
		{
			BetterDBC();
		}

		if (GameConfig::GetValue("Gameplay", "AllowToggleCrouchWhileWalk", 1))
		{
			AllowToggleCrouchWhileWalk();
		}

		if (GameConfig::GetValue("Gameplay", "DisableCameraLockForClimb", 1))
		{
			DisableLockedClimbCam();
		}

		if (GameConfig::GetValue("Gameplay", "FastDoors", 0)) // removes the anim for kicking or opening doors.
		{
			FasterDoors();
		}

		if (GameConfig::GetValue("Gameplay", "SR1Reloading", 0))
		{
			SR1Reloading();
		}

		if (GameConfig::GetValue("Gameplay", "NoMeleeLockOn", 0))
		{
			CMPatches_NoMeleeLockOn.Apply();	
		}

		if (GameConfig::GetValue("Gameplay", "SR1CrouchCam",1)) {
			Logger::TypedLog(CHN_DEBUG, "Patching SR1CrouchCam...\n");
			CSR1CrouchCam.Apply();
		}
		if (GameConfig::GetValue("Gameplay", "SR1QuickSwitch", 1))
		{
			SR1QuickSwitch();
		}

		if (GameConfig::GetValue("Gameplay", "TauntCancelling", 1))
		{
			TauntCancelling();
		}
		if (GameConfig::GetValue("Gameplay", "UseWeaponAfterEmpty", 1))
		{
			WeaponJam();
		}
		if (GameConfig::GetValue("Gameplay", "BetterMovementBehaviour", 0))
		{
			BetterMovement();
		}
		if (GameConfig::GetValue("Gameplay", "SR1MovingAttacks", 0)) // requires XTBL modding to fully use
		{
			MovingAttacks();
		}
		if (GameConfig::GetValue("Gameplay", "HigherMaxSpeed", 0))
		{
			HigherMaxSpeed();
		}
			slewmode_mousefix_rewrite = safetyhook::create_mid(0x00C011FB, &slewmode_control_rewrite);
			cf_do_control_mode_sticky_MIDASMHOOK = safetyhook::create_mid(0x0049C102, &sticky_cam_modifier,safetyhook::MidHook::StartDisabled);
			// it's expecting time in ms, so 1000 = 1 second
			if (int user_cam_modifier = GameConfig::GetValue("Gameplay", "VehicleAutoCenterModifer", 0); user_cam_modifier > 0) {
				(void)cf_do_control_mode_sticky_MIDASMHOOK.enable();
				sticky_cam_timer_add = user_cam_modifier;
			}

#endif
	}
}