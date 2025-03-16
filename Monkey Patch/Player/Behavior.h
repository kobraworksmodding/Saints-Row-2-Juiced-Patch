#pragma once
#include "../Patcher/CMultiPatch.h"
#include "../Patcher/CPatch.h"

namespace Behavior
{
	extern int sticky_cam_timer_add;
	extern void Init();
	extern void BetterMovement();
	extern CPatch CBetterDBC;
	extern CPatch CBetterHBC;
	extern CPatch CAnimBlend;
	extern CMultiPatch CMPatches_DisableLockedClimbCam;
	extern CMultiPatch CMPatches_SR1QuickSwitch;
	extern CMultiPatch CMPatches_SR1Reloading;
}
