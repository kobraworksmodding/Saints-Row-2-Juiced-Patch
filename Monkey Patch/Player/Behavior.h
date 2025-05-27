#pragma once
#include "../Patcher/CMultiPatch.h"
#include "../Patcher/CPatch.h"
#include <safetyhook.hpp>
namespace Behavior
{
	extern int sticky_cam_timer_add;
	extern void Init();
	extern void BetterMovement();
	extern CPatch CBetterDBC;
	extern CPatch CBetterHBC;
	extern CPatch CAnimBlend;
	extern CPatch CSR1CrouchCam;
	extern CMultiPatch CMPatches_DisableLockedClimbCam;
	extern CMultiPatch CMPatches_SR1QuickSwitch;
	extern CMultiPatch CMPatches_SR1Reloading;
	extern CMultiPatch CMPatches_UseWeaponAfterEmpty;
	extern CMultiPatch CMPatches_TauntCancelling;
	extern SafetyHookMid cf_do_control_mode_sticky_MIDASMHOOK;
	extern CPatch CAllowWeaponSwitchInAllCases_KBM;
	extern float vehicle_camera_follow_aggressiveness_modifier;
	extern void LessAgressiveCameraVehicleFollow_hook_enable_disable();
}
