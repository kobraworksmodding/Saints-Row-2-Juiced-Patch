#if !RELOADED && !JLITE
#include "GameConfig.h"
#include "Patcher/CPatch.h"
#include "BlingMenu_public.h"
#include "BlingMenuInstall.h"
#include "InternalNameLists.h"
#include <string>
#include "UtilsGlobal.h"


#include "Render/Render3D.h"
#include "Player/Behavior.h"
#include "UGC/Debug.h"
#include "General/General.h"
#include "MainHooks.h"
const char* ERROR_MESSAGE = "ERROR";
// MainHooks.cpp
void ToggleNoclip();
void AspectRatioFix();
void SlewModeToggle();
void TeleportToWaypoint();
void VehicleSpawner(const char* Name, const char* Var);
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

    const char* BM_ClassicGTAIdleCam(void* userdata, int action) {
        using namespace Render3D;
        if (action != -1) {
            if (CMPatches_ClassicGTAIdleCam.IsApplied())
                CMPatches_ClassicGTAIdleCam.Restore();
            else
                CMPatches_ClassicGTAIdleCam.Apply();
        }
        switch (CMPatches_ClassicGTAIdleCam.IsApplied()) {
        case false: return "OFF";
            break;
        case true: return "ON ";
            break;
        }
        return ERROR_MESSAGE;
 }

    const char* BM_UncapFPS(void* userdata, int action) {
        using namespace Render3D;
        if (action != -1) {
            if (CUncapFPS.IsApplied())
                CUncapFPS.Restore();
            else
                CUncapFPS.Apply();
        }
        switch (CUncapFPS.IsApplied()) {
        case false: return "OFF";
            break;
        case true: return "ON ";
            break;
        }
        return ERROR_MESSAGE;
 }

    const char* BM_BetterAO(void* userdata, int action) {
        using namespace Render3D;
        if (action != -1) {
            if (CBetterAO.IsApplied())
                CBetterAO.Restore();
            else
                CBetterAO.Apply();
        }
        switch (CBetterAO.IsApplied()) {
        case false: return "OFF";
            break;
        case true: return "ON ";
            break;
        }
        return ERROR_MESSAGE;
    }

    const char* BM_DisableFog(void* userdata, int action) {
        using namespace Render3D;
        if (action != -1) {
            if (CMPatches_DisableFog.IsApplied())
                CMPatches_DisableFog.Restore();
            else
                CMPatches_DisableFog.Apply();
        }
        switch (CMPatches_DisableFog.IsApplied()) {
        case false: return "OFF";
            break;
        case true: return "ON ";
            break;
        }
        return ERROR_MESSAGE;
    }

    const char* BM_DisableCutSceneBlackBars(void* userdata, int action) {
        using namespace Render3D;
        if (action != -1) {
            if (CRemoveBlackBars.IsApplied())
                CRemoveBlackBars.Restore();
            else
                CRemoveBlackBars.Apply();
            // Just showing that you can save with BM.
            GameConfig::SetValue("Graphics", "RemoveBlackBars", (uint32_t)CRemoveBlackBars.IsApplied());
        }
        switch (CRemoveBlackBars.IsApplied()) {
        case false: return "OFF";
            break;
        case true: return "ON ";
            break;
        }
        return ERROR_MESSAGE;
    }

    const char* BM_DBC(void* userdata, int action) {
        using namespace Behavior;
        if (action != -1) {
            if (CBetterDBC.IsApplied())
                CBetterDBC.Restore();
            else
                CBetterDBC.Apply();
        }
        switch (CBetterDBC.IsApplied()) {
        case false: return "OFF";
            break;
        case true: return "ON ";
            break;
        }
        return ERROR_MESSAGE;
}
    const char* BM_HBC(void* userdata, int action) {
        using namespace Behavior;
        if (action != -1) {
            if (CBetterHBC.IsApplied())
                CBetterHBC.Restore();
            else
                CBetterHBC.Apply();
        }
        switch (CBetterHBC.IsApplied()) {
        case false: return "OFF";
            break;
        case true: return "ON ";
            break;
        }
        return ERROR_MESSAGE;
}
    const char* BM_AnimBlend(void* userdata, int action) {
        using namespace Behavior;
        if (action != -1) {
            if (CAnimBlend.IsApplied())
                CAnimBlend.Restore();
            else
                CAnimBlend.Apply();
        }
        switch (CAnimBlend.IsApplied()) {
        case false: return "OFF";
            break;
        case true: return "ON ";
            break;
        }
        return ERROR_MESSAGE;
}

    const char* BM_SR1QuickSwitch(void* userdata, int action) {
        using namespace Behavior;
        if (action != -1) {
            if (CMPatches_SR1QuickSwitch.IsApplied())
                CMPatches_SR1QuickSwitch.Restore();
            else
                CMPatches_SR1QuickSwitch.Apply();
        }
        switch (CMPatches_SR1QuickSwitch.IsApplied()) {
        case false: return "OFF";
            break;
        case true: return "ON ";
            break;
        }
        return ERROR_MESSAGE;
    }

    const char* BM_SR1Reloading(void* userdata, int action) {
        using namespace Behavior;
        if (action != -1) {
            if (CMPatches_SR1Reloading.IsApplied())
                CMPatches_SR1Reloading.Restore();
            else
                CMPatches_SR1Reloading.Apply();
        }
        switch (CMPatches_SR1Reloading.IsApplied()) {
        case false: return "OFF";
            break;
        case true: return "ON ";
            break;
        }
        return ERROR_MESSAGE;
    }

    const char* BM_FixHorizontalMouseSensitivity(void* userdata, int action) {
        using namespace General;
        if (action != -1) {
            if (CFixHorizontalMouseSensitivity.IsApplied())
                CFixHorizontalMouseSensitivity.Restore();
            else
                CFixHorizontalMouseSensitivity.Apply();
        }
        switch (CFixHorizontalMouseSensitivity.IsApplied()) {
        case false: return "OFF";
            break;
        case true: return "ON ";
            break;
        }
        return ERROR_MESSAGE;
    }

    const char* BM_ToggleNoclip(void* userdata, int action) {
        if (action != -1) {
            ToggleNoclip();
        }
        switch (NoclipEnabled) {
        case false: return "OFF";
            break;
        case true: return "ON ";
            break;
        }
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
            General::CMPatches_TervelTextureCrashWorkaround_be_as_pe.Restore();
            if (CMPatches_ClippysIdiotTextureCrashExceptionHandle.IsApplied())
                CMPatches_ClippysIdiotTextureCrashExceptionHandle.Restore();
            else
                CMPatches_ClippysIdiotTextureCrashExceptionHandle.Apply();
        }
        switch (CMPatches_ClippysIdiotTextureCrashExceptionHandle.IsApplied()) {
        case false: return "OFF";
            break;
        case true: return "ON ";
            break;
        }
        return ERROR_MESSAGE;
    }

    const char* BM_TervelCrashWorkAround(void* userdata, int action) {
        using namespace General;

        if (Render3D::CMPatches_ClippysIdiotTextureCrashExceptionHandle.IsApplied()) {
            return "EXCEPTION-ish HANDLER HOOKED";
        }
        if (action != -1 && !Render3D::CMPatches_ClippysIdiotTextureCrashExceptionHandle.IsApplied()) {
            if (CMPatches_TervelTextureCrashWorkaround_be_as_pe.IsApplied()) {
                CMPatches_TervelTextureCrashWorkaround_be_as_pe.Restore();
            }
            else {
                CMPatches_TervelTextureCrashWorkaround_be_as_pe.Apply();
            }
        }
        return CMPatches_TervelTextureCrashWorkaround_be_as_pe.IsApplied() ? "ON " : "OFF";
    }



    void BM_restoreHavok() {
        if(!Debug::fixFrametime)
            *(float*)(0x02527DA4) = 0.01666666666f;

}
// Not the real console_do_command, but rather a helper function to set the bool if a function is called or not to true then back to false, 
// probably could just keep it as true but for safety we'll reset to false.
    void callconsolecommand(int func_address) {
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
       BlingMenuAddFuncCustom("Juiced", "SleepHack", NULL, &BM_SleepHacks, NULL);
       BlingMenuAddBool("Juiced", "Fix Havok Frametime",&Debug::fixFrametime, BM_restoreHavok);
       BlingMenuAddFuncCustom("Juiced", "Uncap FPS", NULL, &BM_UncapFPS, NULL);
       BlingMenuAddInt8("Juiced", "OSD", (signed char*)&useJuicedOSD, NULL, 1, 0, 3);
       BlingMenuAddFuncCustom("Juiced", "Better Ambient Occlusion", NULL, &BM_BetterAO, NULL);
       BlingMenuAddFuncCustom("Juiced", "Disable Cutscene black-bars", NULL, &BM_DisableCutSceneBlackBars, NULL);
       BlingMenuAddFuncCustom("Juiced", "Disable Fog", NULL, &BM_DisableFog, NULL);
       BlingMenuAddDouble("Juiced", "FOV Multiplier", &Render3D::FOVMultiplier, []() {
           AspectRatioFix();
           GameConfig::SetDoubleValue("Gameplay", "FOVMultiplier", Render3D::FOVMultiplier);
           }, 0.01, 0.1, 5.0);
       BlingMenuAddInt("Juiced","Vehicle Auto Center Modifer",&Behavior::sticky_cam_timer_add,NULL,250,0, INT_MAX - 1500);
       BlingMenuAddFuncCustom("Juiced", "Better Drive-by Cam", NULL, &BM_DBC, NULL);
       BlingMenuAddFuncCustom("Juiced", "Better Handbrake Cam", NULL, &BM_HBC, NULL);
       BlingMenuAddFuncCustom("Juiced", "Fix Horizontal Mouse Sensitivity", NULL, &BM_FixHorizontalMouseSensitivity, NULL);
       BlingMenuAddFuncCustom("Juiced", "Anim Blend", NULL, &BM_AnimBlend, NULL);
       BlingMenuAddFuncCustom("Juiced", "Classic GTA Idle Cam", NULL, &BM_ClassicGTAIdleCam, NULL);
       BlingMenuAddFuncCustom("Juiced", "SR1 Reloading", NULL, &BM_SR1Reloading, NULL);
       BlingMenuAddFuncCustom("Juiced", "SR1 Quick Switching", NULL, &BM_SR1QuickSwitch, NULL);
       BlingMenuAddFuncCustom("Juiced", "Juiced", NULL, &BM_ReportVersion, NULL);
       BlingMenuAddBool("Juiced Misc", "HUD (WILL DISABLE MENU RENDERING, PRESS F2 TO RESTORE)", (bool*)0x0252737C, NULL);
       BlingMenuAddFuncCustom("Juiced Cheats", "Noclip", NULL, &BM_ToggleNoclip, NULL);
       BlingMenuAddFuncCustom("Juiced Cheats", "Toggle Cheats flagging saves", NULL, &BM_AllowCheatFlagging, NULL);
       BlingMenuAddFunc("Juiced Cheats", "Teleport to Waypoint", TeleportToWaypoint);
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
       BlingMenuAddFunc("Reload table", "reload camera_shake.xtbl", []() {((void(__cdecl*)(void))0x004A0870)(); });
       BlingMenuAddFunc("Reload table", "reload lightning.xtbl", []() {((void(__cdecl*)(void))0x004CA220)(); });
       BlingMenuAddFunc("Reload table", "reload motion_blur.xtbl", []() {((void(__cdecl*)(void))0x004CB850)(); });
       BlingMenuAddFunc("Reload table", "reload rain.xtbl", []() {((void(__cdecl*)(void))0x004CD230)(); });
       BlingMenuAddFunc("Reload table", "reload time_of_day & lens_flares.xtbl", []() {callconsolecommand(0x004D2610); });
       BlingMenuAddFunc("Reload table", "reload wind.xtbl", []() {((void(__cdecl*)(void))0x004E51A0)(); });
       // doesn't work, crashes.
       //BlingMenuAddFunc("Reload table", "reload effects", []() {callconsolecommand(0x0050CA70); });
       BlingMenuAddFunc("Reload table", "reload player_image_data.xtbl", []() {((void(__cdecl*)(void))0x00530AA0)(); });
       BlingMenuAddFunc("Reload table", "reload fraud_globals.xtbl foley", []() {((void(__cdecl*)(void))0x00649A30)(); });
       BlingMenuAddFunc("Reload table", "reload fraud_globals.xtbl adrenaline", []() {((void(__cdecl*)(void))0x00649B20)(); });
       BlingMenuAddFunc("Reload table", "reload fraud_globals.xtbl globals", []() {((void(__cdecl*)(void))0x00650570)(); });
       BlingMenuAddFunc("Reload table", "reload melee.xtbl", []() {callconsolecommand(0x00982450); });
       BlingMenuAddFunc("Reload table", "reload distant_ped_spawn_parameters.xtbl", []() {((void(__cdecl*)(void))0x00B7BFF0)(); });
       BlingMenuAddFunc("Reload table", "reload distant_vehicles xtbls", []() {callconsolecommand(0x00B7EF20); });

       // Custom loading, that are not leftover in the game or from SR1.

       BlingMenuAddFunc("Reload table", "reload weapons.xtbl", []() {
           //This works but buffer doesn't scale up for some reason.
           load_weapons_xtbl("weapons.xtbl", 1);
           });
       BlingMenuAddBool("Juiced Debug", "simulate a add_to_entry crash(only works when below handler is hooked)", &Render3D::crash, NULL);
       BlingMenuAddFuncCustom("Juiced Debug", "Hook a (eh) exception-ish handler to stop add_to_entry crash", NULL, &BM_ClippyTextureExceptionHandle, NULL);
       BlingMenuAddFuncCustom("Juiced Debug", "Hook / make add_to_entry(bitmap_entry *be,peg_entry *pe) to just use pe as both args", NULL, &BM_TervelCrashWorkAround, NULL);

       }
    }
}
#endif