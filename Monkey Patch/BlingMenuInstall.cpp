#if !JLITE
#pragma warning( disable : 4834)
#pragma warning( disable : 4309)
#include "GameConfig.h"
#include "Patcher/CPatch.h"
#include "BlingMenu_public.h"
#include "BlingMenuInstall.h"
#include "InternalNameLists.h"
#include <string>
#include "UtilsGlobal.h"


#include "Render/Render3D.h"
#include "Render/Render2D.h"
#include "Player/Behavior.h"
#include "UGC/Debug.h"
#include "General/General.h"
#include "MainHooks.h"
#include "Math/Math.h"
#include "Player/Input.h"
#include "Game/Game.h"
const char* ERROR_MESSAGE = "ERROR";
// MainHooks.cpp
#if !RELOADED
void ToggleNoclip();
#endif
void SlewModeToggle();
void TeleportToWaypoint();
void VehicleSpawner(const char* Name, const char* Var);
void tpCoords(float x, float y, float z);
extern bool NoclipEnabled;
// MainHooks.cpp


namespace BlingMenuInstall
{

    BYTE BM_Juiced_Kobra_Toggle = false;
    // YOU NEED USERDATA AND ACTION!
    const char* BM_ReportVersion(void* userdata, int action) {
        if (action == 1) {
            BM_Juiced_Kobra_Toggle = (BM_Juiced_Kobra_Toggle + 1) % 3;
        }
        else if (action == 0) {
            BM_Juiced_Kobra_Toggle = (BM_Juiced_Kobra_Toggle + 2) % 3;
        }

        static std::string commitStr;

        switch (BM_Juiced_Kobra_Toggle) {
        case 0:
            return UtilsGlobal::juicedversion;
        case 1:
            commitStr = std::string(UtilsGlobal::getShortCommitHash()) + ", " + BUILD_TIME_UTC;
            return commitStr.c_str();
        case 2:
            return "By Kobraworks";
        default:
            return UtilsGlobal::juicedversion;
        }
    }
    static BYTE BM_sleephack = 0; // Default to 0, but actual state is determined dynamically

    const char* BM_SleepHacks(void* userdata, int action) {


        using namespace Render3D;
        if (action == 1) { // Increment
            BM_sleephack = (BM_sleephack + 1) % 4;
        }
        else if (action == 0) { // Decrement
            BM_sleephack = (BM_sleephack == 0) ? 3 : (BM_sleephack - 1);
        }
        else if (action == -1) {
            if (IsSleepHooked) {
                BM_sleephack = 3;
            }
            else if (CPatches_MediumSleepHack.IsApplied()) {
                BM_sleephack = 2;
            }
            else if (CMPatches_PatchLowSleepHack.IsApplied()) {
                BM_sleephack = 1;
            }
            else {
                BM_sleephack = 0;
            }


            switch (BM_sleephack) {
            case 0: return "OFF";
            case 1: return "LOW";
            case 2: return "MEDIUM";
            case 3: return "HIGH";
            }
        }
        switch (BM_sleephack) {
        case 0: // OFF
            UnHookSleep();
            if (CMPatches_PatchLowSleepHack.IsApplied()) CMPatches_PatchLowSleepHack.Restore();
            if (CPatches_MediumSleepHack.IsApplied()) CPatches_MediumSleepHack.Restore();
            return "OFF";
        case 1:
            UnHookSleep();
            if (CPatches_MediumSleepHack.IsApplied()) CPatches_MediumSleepHack.Restore();
            if (!CMPatches_PatchLowSleepHack.IsApplied()) CMPatches_PatchLowSleepHack.Apply();
            return "LOW";

        case 2:
            UnHookSleep();
            if (!CMPatches_PatchLowSleepHack.IsApplied()) CMPatches_PatchLowSleepHack.Apply();
            if (!CPatches_MediumSleepHack.IsApplied()) CPatches_MediumSleepHack.Apply();
            return "MEDIUM";
        case 3:
            if (CMPatches_PatchLowSleepHack.IsApplied()) CMPatches_PatchLowSleepHack.Restore();
            if (CPatches_MediumSleepHack.IsApplied()) CPatches_MediumSleepHack.Restore();
            HookSleep();
            return "HIGH";
        }
        return "OFF"; // Fallback (should never reach here)
    }
    const char* BM_VFXPlus(void* userdata, int action) {
        using namespace Render3D;
        if (action != -1) {
            if (CMPatches_VFXPlus.IsApplied())
                CMPatches_VFXPlus.Restore();
            else {
                #define HDR_toggle *(bool*)0x00E97882
                if(!HDR_toggle)
                HDR_toggle = true;
                CMPatches_VFXPlus.Apply();
            }
        }
        switch (CMPatches_VFXPlus.IsApplied()) {
        case false: return "OFF";
            break;
        case true: return "ON ";
            break;
        }
        return ERROR_MESSAGE;
    }

    const char* BM_ToggleNoclip(void* userdata, int action) {
#if !RELOADED
        if (action != -1) {
            ToggleNoclip();
        }
        switch (NoclipEnabled) {
        case false: return "OFF";
            break;
        case true: return "ON ";
            break;
        }
#endif
        return ERROR_MESSAGE;
    }

    const char* BM_AllowCheatFlagging(void* userdata, int action) {
        using namespace Debug;
        if (action != -1) {
            if (CMPatches_DisableCheatFlag.IsApplied())
                CMPatches_DisableCheatFlag.Restore();
            else {
                CMPatches_DisableCheatFlag.Apply();
                // Will remove cheat flag if we disable the flag, but wont restore it since we don't know the previous status.
                *(bool*)0x2527B5A = false;
                *(bool*)0x2527BE6 = false;
            }
        }
        switch (CMPatches_DisableCheatFlag.IsApplied()) {
            // SWITCHED OFF/ON RETURNS SINCE DEFAULT IS ON!!
        case true: return "OFF";
            break;
        case false: return "ON ";
            break;
        }
        return ERROR_MESSAGE;
    }
    const char* BM_ClippyTextureExceptionHandle(void* userdata, int action) {
        using namespace Render3D;
        if (action != -1) {
            if (add_to_entry_test.enabled()) {
                add_to_entry_test.disable();
            }
            else {
                add_to_entry_test.enable();
            }
        }
        switch (add_to_entry_test.enabled()) {
        case false: return "OFF";
            break;
        case true: return "ON ";
            break;
        }
        return ERROR_MESSAGE;
    }

    const char* BM_TervelCrashWorkAround(void* userdata, int action) {
        using namespace General;
        if (action != -1) {
            if (CMPatches_TervelTextureCrashWorkaround_be_as_pe.IsApplied()) {
                CMPatches_TervelTextureCrashWorkaround_be_as_pe.Restore();
            }
            else {
                CMPatches_TervelTextureCrashWorkaround_be_as_pe.Apply();
            }
        }
        return CMPatches_TervelTextureCrashWorkaround_be_as_pe.IsApplied() ? "ON " : "OFF";
    }

    template<typename PatchType>
    const char* BM_GenericPatchFunction(PatchType& patch, int action, const char* appName = nullptr, const char* keyName = nullptr) {
        if (action != -1) {
            if (patch.IsApplied()) {
                patch.Restore();
            }
            else {
                patch.Apply();
            }

            if (appName && keyName) {
                GameConfig::SetValue(appName, keyName, (uint32_t)patch.IsApplied());
            }
        }

        return patch.IsApplied() ? "ON " : "OFF";
    }
    template<typename SafetyHookType>
    const char* BM_GenericSafetyHookFunction(SafetyHookType& hook, int action, const char* appName = nullptr, const char* keyName = nullptr) {
        if (action != -1) {
            if (hook.enabled()) {
                hook.disable();
            }
            else {
                hook.enable();
            }

            if (appName && keyName) {
                GameConfig::SetValue(appName, keyName, (uint32_t)hook.enabled());
            }
        }

        return hook.enabled() ? "ON " : "OFF";
    }
        BM_MakeCPatchFunction(ClassicGTAIdleCam, CMPatches_ClassicGTAIdleCam, Render3D)
        BM_MakeCPatchFunction(UncapFPS, CUncapFPS, Render3D)
        BM_MakeCPatchFunction(BetterAO, CBetterAO, Render3D)
        BM_MakeCPatchFunction(DisableSkyRefl, CMPatches_DisableSkyRefl, Render3D)
        BM_MakeCPatchFunction(DisableFog, CMPatches_DisableFog, Render3D)
        BM_MakeCPatchFunction(DBC, CBetterDBC, Behavior)
        BM_MakeCPatchFunction(HBC, CBetterHBC, Behavior)
        BM_MakeCPatchFunction(AnimBlend, CAnimBlend, Behavior)
        BM_MakeCPatchFunction(SR1Reloading, CMPatches_SR1Reloading, Behavior)
        BM_MakeCPatchFunction(SR1QuickSwitch, CMPatches_SR1QuickSwitch, Behavior)
        BM_MakeCPatchFunctionSaveConfig(DisableCutSceneBlackBars, CRemoveBlackBars, Render3D, "Graphics", "RemoveBlackBars")
        BM_MakeSafetyHookFunction(Math_multiplication_NAN_fix, matrix_operator_multiplication_midhook,Math::Fixes)
        BM_MakeSafetyHookFunction(UI_hook, final_2d_render, Render2D)
        BM_MakeSafetyHookFunction(FixFrametimeVehicleSkids, FixFrametimeVehicleSkids, Game)
        BM_MakeSafetyHookFunction(screen_3d_to_2d_midhook, screen_3d_to_2d_midhook, Render3D)
    void BM_restoreHavok() {
        if(!Debug::fixFrametime)
            *(float*)(0x02527DA4) = 0.01666666666f;

}
// Not the real console_do_command, but rather a helper function to set the bool if a function is called or not to true then back to false, 
// probably could just keep it as true but for safety we'll reset to false.
        __declspec(noinline) void callconsolecommand(int func_address) {
        bool* console_flag_function_call = (bool*)0x02345B68;
       *console_flag_function_call = true;
       ((void(__cdecl*)(void))func_address)();
       *console_flag_function_call = false;
    }


    char __declspec(naked) load_weapons_xtbl(const char* filename,int refresh)
    {
        _asm {
            push ebp
            mov ebp, esp
            sub esp, __LOCAL_SIZE

            push refresh
            mov eax, filename
            mov edx, 0xB710C0
            call edx

            mov esp, ebp
            pop ebp
            ret
        }
    }

   void AddOptions() {
       if (BlingMenuLoad()) {
       //BlingMenuAddFuncCustom("Juiced", "SleepHack", NULL, &BM_SleepHacks, NULL);
       BlingMenuAddBool("Juiced", "Fix Havok Frametime",&Debug::fixFrametime, BM_restoreHavok);
       BlingMenuAddFuncCustom("Juiced", "FixFrametimeVehicleSkids",NULL ,&BM_FixFrametimeVehicleSkids, NULL);
       BlingMenuAddFuncCustom("Juiced", "Uncap FPS", NULL, &BM_UncapFPS, NULL);
       BlingMenuAddInt8("Juiced", "OSD", (signed char*)&useJuicedOSD, NULL, 1, 0, 3);
       BlingMenuAddFuncCustom("Juiced", "VanillaFXPlus", NULL, &BM_VFXPlus, NULL);
       BlingMenuAddFuncCustom("Juiced", "Disable Sky Reflections", NULL, &BM_DisableSkyRefl, NULL);
       BlingMenuAddFuncCustom("Juiced", "Better Ambient Occlusion", NULL, &BM_BetterAO, NULL);
       BlingMenuAddFuncCustom("Juiced", "Disable Cutscene black-bars", NULL, &BM_DisableCutSceneBlackBars, NULL);
       BlingMenuAddFuncCustom("Juiced", "Disable Fog", NULL, &BM_DisableFog, NULL);
       BlingMenuAddBool("Juiced", "X360Gamma", (bool*)&Render3D::ShaderOptions.X360Gamma, &Render3D::ChangeShaderOptions);
       BlingMenuAddBool("Juiced", "ShadowFiltering", (bool*)&Render3D::ShaderOptions.ShadowFilter, &Render3D::ChangeShaderOptions);
       BlingMenuAddFuncCustom("Juiced", "X360GammaUI", NULL, &BM_UI_hook, NULL);
       BlingMenuAddDouble("Juiced", "FOV Multiplier", &Render3D::FOVMultiplier, []() {
           Render3D::AspectRatioFix();
           GameConfig::SetDoubleValue("Gameplay", "FOVMultiplier", Render3D::FOVMultiplier);
           }, 0.01, 0.1, 5.0);
       BlingMenuAddInt("Juiced", "Vehicle Auto Center Modifer", &Behavior::sticky_cam_timer_add, []() {
           if (Behavior::sticky_cam_timer_add != 0)
               Behavior::cf_do_control_mode_sticky_MIDASMHOOK.enable();
           else Behavior::cf_do_control_mode_sticky_MIDASMHOOK.disable();
           
           GameConfig::SetValue("Gameplay", "VehicleAutoCenterModifer", Behavior::sticky_cam_timer_add);
           }, 250, 0, INT_MAX - 1500);
       BlingMenuAddFloat("Juiced", "vehicle_camera_follow_modifier", &Behavior::vehicle_camera_follow_modifier, &Behavior::LessCameraVehicleFollow_hook_enable_disable, 1.25f, -100.f, 100.f);
       BlingMenuAddFuncCustom("Juiced", "Better Drive-by Cam", NULL, &BM_DBC, NULL);
       BlingMenuAddFuncCustom("Juiced", "Better Handbrake Cam", NULL, &BM_HBC, NULL);
       BlingMenuAddFuncCustom("Juiced", "Anim Blend", NULL, &BM_AnimBlend, NULL);
       BlingMenuAddFuncCustom("Juiced", "Classic GTA Idle Cam", NULL, &BM_ClassicGTAIdleCam, NULL);
       BlingMenuAddFuncCustom("Juiced", "SR1 Reloading", NULL, &BM_SR1Reloading, NULL);
       BlingMenuAddFuncCustom("Juiced", "SR1 Quick Switching", NULL, &BM_SR1QuickSwitch, NULL);
       BlingMenuAddFuncCustom("Juiced", "Juiced", NULL, &BM_ReportVersion, NULL);
       BlingMenuAddBool("Juiced Misc", "HUD (WILL DISABLE MENU RENDERING, PRESS F2 TO RESTORE)", (bool*)0x0252737C, NULL);
       BlingMenuAddFuncCustom("Juiced Cheats", "Noclip", NULL, &BM_ToggleNoclip, NULL);
       BlingMenuAddFuncCustom("Juiced Cheats", "Toggle Cheats flagging saves", NULL, &BM_AllowCheatFlagging, NULL);
       BlingMenuAddFunc("Juiced Cheats", "Teleport to Waypoint", TeleportToWaypoint);
       BlingMenuAddFunc("Juiced Cheats", "Teleport to Camera's current position", []() {
           float* camera_pos = (float*)0x025F5B14;
           tpCoords(camera_pos[0], camera_pos[1], camera_pos[2]);
           });
       BlingMenuAddFunc("Juiced Cheats", "Toggle Slew Mode", SlewModeToggle);
       BlingMenuAddFunc("Juiced play_as", "RESTORE PLAYER", []() {
           if (!UtilsGlobal::getplayer())
               return;
           General::ResetCharacter(0, 255);

           });
       for (const char* character : Characters) {
           BlingMenuAddFuncStd("Juiced play_as", character, [character]() {
               if (!UtilsGlobal::getplayer())
                   return;
               int* character_id = General::GetCharacterID(character);
               General::ChangeCharacter(&character_id);
               });
       }
       BlingMenuAddFunc("Juiced spawn_npc", "Delete all spawned NPCs", General::YeetAllNPCs);
       for (const char* character : Characters) {
           BlingMenuAddFuncStd("Juiced spawn_npc", character, [character]() {
               if (!UtilsGlobal::getplayer())
                   return;
               General::NPCSpawner(character);
               });
       }

       for (const auto& vehicle : AllVehicles) {
           BlingMenuAddFuncStd("Juiced Vehicle Spawner", vehicle.first.c_str(), [vehicle]() {
               if (!UtilsGlobal::getplayer())
                   return;
               VehicleSpawner(vehicle.second, "-1");
               });
       }
       BlingMenuAddFunc("Reload table", "Following are untested.", NULL);
       BlingMenuAddFunc("Reload table", "reload weather & weather_time_of_day.xtbl", []() {

           ((void(__cdecl*)(void))0x004DBDE0)();
           ((void(__cdecl*)(void))0x004DD390)();

           });
       BlingMenuAddFunc("Reload table", "reload camera_shake.xtbl", (void(*)())0x004A0870);
       BlingMenuAddFunc("Reload table", "reload lightning.xtbl", (void(*)())0x004CA220);
       BlingMenuAddFunc("Reload table", "reload motion_blur.xtbl", (void(*)())0x004CB850);
       BlingMenuAddFunc("Reload table", "reload rain.xtbl", (void(*)())0x004CD230);
       BlingMenuAddFunc("Reload table", "reload time_of_day & lens_flares.xtbl", []() { callconsolecommand(0x004D2610); });
       BlingMenuAddFunc("Reload table", "reload wind.xtbl", (void(*)())0x004E51A0);
       // Doesn't work, crashes.
       //BlingMenuAddFunc("Reload table", "reload effects", []() { callconsolecommand(0x0050CA70); });
       BlingMenuAddFunc("Reload table", "reload player_image_data.xtbl", (void(*)())0x00530AA0);
       BlingMenuAddFunc("Reload table", "reload fraud_globals.xtbl foley", (void(*)())0x00649A30);
       BlingMenuAddFunc("Reload table", "reload fraud_globals.xtbl adrenaline", (void(*)())0x00649B20);
       BlingMenuAddFunc("Reload table", "reload fraud_globals.xtbl globals", (void(*)())0x00650570);
       BlingMenuAddFunc("Reload table", "reload melee.xtbl", []() { callconsolecommand(0x00982450); });
       BlingMenuAddFunc("Reload table", "reload distant_ped_spawn_parameters.xtbl", (void(*)())0x00B7BFF0);
       BlingMenuAddFunc("Reload table", "reload distant_vehicles xtbls", []() { callconsolecommand(0x00B7EF20); });

       // Custom loading, that are not leftover in the game or from SR1.

       BlingMenuAddFunc("Reload table", "reload weapons.xtbl", []() {
           //This works but buffer doesn't scale up for some reason.
           load_weapons_xtbl("weapons.xtbl", 1);
           });
       //BlingMenuAddBool("Juiced Debug", "simulate a add_to_entry crash(only works when below handler is hooked)", &Render3D::crash, NULL);
       BlingMenuAddInt8("Juiced Debug", "FastMath", &Math::Fixes::SSE_hack, NULL,1,0,2);
       BlingMenuAddFuncCustom("Juiced Debug", "Hook a (eh) exception-ish handler to stop add_to_entry crash", NULL, &BM_ClippyTextureExceptionHandle, NULL);
       BlingMenuAddFuncCustom("Juiced Debug", "Hook / make add_to_entry(bitmap_entry *be,peg_entry *pe) to just use pe as both args", NULL, &BM_TervelCrashWorkAround, NULL);
       BlingMenuAddBool("Juiced Debug", "UseDynamicRenderDistance", (bool*)&Debug::UseDynamicRenderDistance, NULL);
       BlingMenuAddFloat("Juiced Debug", "MAX_RENDER", &Debug::MAX_RENDER, NULL, 0.1f, 1.f, 50.f);
       BlingMenuAddFloat("Juiced Debug", "TRANSITION_SPEED", &Debug::TRANSITION_SPEED, NULL, 0.1f, 1.f, 50.f);
       BlingMenuAddInt("Juiced Debug", "SIZE_MIN", &Debug::SIZE_MIN, NULL,5,1,153600);
       BlingMenuAddInt("Juiced Debug", "MAX", &Debug::MAX, NULL, 5, 1, 153600);
       BlingMenuAddInt("Juiced Debug", "ShaderOverride",&Render3D::OVERRIDE_SHADER_LOD, NULL, 1, 0, 2);
       BlingMenuAddInt("Juiced Debug", "SHADER_LOD", (int*)&Render3D::SHADER_LOD, NULL, 1, -2, 10);
       BlingMenuAddFloat("Juiced Debug", "SHADER_DISTANCE_SQUARED_MULT", &Render3D::SHADER_DISTANCE_SQUARED_MULT, NULL, 0.1f, 1.f, 250.f);
       BlingMenuAddFuncCustom("Juiced Debug", "Math_multiplication_NAN_fix", NULL, &BM_Math_multiplication_NAN_fix, NULL);
       BlingMenuAddFuncCustom("Juiced Debug", "screen_3d_to_2d_midhook", NULL, &BM_screen_3d_to_2d_midhook, NULL);
       BlingMenuAddBool("Juiced Debug", "useTextPrompts", &Input::useTextPrompts, NULL);
       BlingMenuAddInt("Juiced Debug", "Use PS3 Prompts", &Input::usePS3Prompts, NULL, 1, 0, 1);
       BlingMenuAddFloat("Juiced Debug", "Render Distance", (float*)0x00E996B4, NULL, 0.65f, 1.f, 500.f);
       BlingMenuAddBool("Juiced Debug", "push_back_force_old_size", &Render3D::push_back_force_old_size, NULL);
       //BlingMenuAddInt8("Juiced Debug", "Fix mouse water", &Math::Fixes::FixWater, NULL,1,0,2);
       //BlingMenuAddBool("Juiced Debug", "Simulate mouse water", &Math::Fixes::SimulateWaterBug, NULL);
       BlingMenuAddBool("Juiced Debug", "old sleep hack 1", &Render3D::use_old, Render3D::apply_old);
       }
    }
}
#endif