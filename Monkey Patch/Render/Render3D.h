#pragma once
#include "../Patcher/CMultiPatch.h"
#include "../Patcher/CPatch.h"

namespace Render3D
{
    extern void Init();
    extern bool useFPSCam;
    extern bool VFXP_fixFog;
    extern void FPSCamHack();
    void PatchHQTreeShadows();
    extern void HookSleep();
    extern void UnHookSleep();
    extern bool ARfov;
    extern bool ARCutscene;
    extern double FOVMultiplier;
    extern const double fourbythreeAR;
    extern bool IsSleepHooked;
    extern CMultiPatch CMPatches_PatchLowSleepHack;
    extern CMultiPatch CMPatches_ClassicGTAIdleCam;
    extern CMultiPatch CMPatches_DisableFog;
    extern CPatch CPatches_MediumSleepHack;
    extern CPatch CUncapFPS;
    extern CPatch CBetterAO;
}