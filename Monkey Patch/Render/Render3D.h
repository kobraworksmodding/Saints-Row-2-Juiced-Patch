#pragma once
#include "../Patcher/CMultiPatch.h"
#include "../Patcher/CPatch.h"

#include <safetyhook.hpp>

struct shaderOptions {
    char X360Gamma;
    char ShadowFilter;
};

namespace Render3D
{
    extern void Init();
    extern bool useFPSCam;
    extern bool VFXP_fixFog;
    extern void FPSCamHack();
    void PatchHQTreeShadows();
    extern void HookSleep();
    extern void UnHookSleep();
    extern void patch_render_batch();
    extern bool ARfov;
    extern double FOVMultiplier;
    extern const double fourbythreeAR;
    extern bool IsSleepHooked;
    extern CMultiPatch CMPatches_PatchLowSleepHack;
    extern CMultiPatch CMPatches_ClassicGTAIdleCam;
    extern CMultiPatch CMPatches_DisableFog;
    extern CMultiPatch CMPatches_VFXPlus;
    extern CMultiPatch CMPatches_DisableSkyRefl;
    extern CPatch CPatches_MediumSleepHack;
    extern CPatch CUncapFPS;
    extern CPatch CBetterAO;
    extern CPatch CRemoveBlackBars;
    extern CPatch CIncreaseVehicleDespawnDistance;
    extern bool crash;
    extern SafetyHookMid add_to_entry_test;
    extern void AspectRatioFix(bool update_aspect_ratio = false);
    extern shaderOptions ShaderOptions;
    extern void ChangeShaderOptions();
    extern void VFXBrightnesstoggle();

    extern int OVERRIDE_SHADER_LOD;
    extern int SHADER_LOD;
    extern float SHADER_DISTANCE_SQUARED_MULT;
    extern SafetyHookMid screen_3d_to_2d_midhook;
    extern bool use_old;
    extern void apply_old();
    extern bool push_back_force_old_size;
    extern float RenderDistance_old;
}