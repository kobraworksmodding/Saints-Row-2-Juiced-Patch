#pragma once

namespace Render3D
{
    extern void Init();
    extern bool useFPSCam;
    extern bool VFXP_fixFog;
    extern void FPSCamHack();
    void PatchHQTreeShadows();

    extern bool ARfov;
    extern bool ARCutscene;
    extern double FOVMultiplier;
    extern const double fourbythreeAR;
}