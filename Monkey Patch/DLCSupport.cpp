// DLCSupport.cpp (Tervel, Clippy95)
// --------------------
// Created: 15/06/2025

#include "DLCSupport.h"
#include "../SafeWrite.h"
#include <safetyhook.hpp>
#include <Hooking.Patterns.h>
#include "Patcher/patch.h"
#include "General/General.h" 
#include "Game/Game.h"
#include "UtilsGlobal.h"
#include "FileLogger.h"
using namespace General;

bool DLCInstalled;

typedef uintptr_t __cdecl load_packfileT(const char* name);
load_packfileT* load_packfile = (load_packfileT*)(0xC0AE90);

struct cutscene_hook_checks {
    uint8_t func_init : 1;
    uint8_t switched_to_dlc : 1;
    uint8_t next : 1;
    uint8_t stop : 1;
} cscene_hook{};

char* vpp_list[] = {
    const_cast<char*>("anims.vpp"),
    const_cast<char*>("audio.vpp"),
    const_cast<char*>("chunks1.vpp"),
    const_cast<char*>("chunks2.vpp"),
    const_cast<char*>("chunks3.vpp"),
    const_cast<char*>("chunks4.vpp"),
    const_cast<char*>("common.vpp"),
    const_cast<char*>("cutscenes.vpp"),
    const_cast<char*>("dlc_cutscenes.vpp"),
    const_cast<char*>("meshes.vpp"),
    const_cast<char*>("music1.vpp"),
    const_cast<char*>("music2.vpp"),
    const_cast<char*>("music3.vpp"),
    const_cast<char*>("music4.vpp"),
    const_cast<char*>("pegs.vpp"),
    const_cast<char*>("textures.vpp"),
    const_cast<char*>("patch.vpp"),
};

void CHooks_cutscene() {
    patchNop((void*)0x006D47A8, 5); // this removes the cutscene array sorting
    // the idea is to have the DLC cutscenes get added at the very end
    // it doesn't break the base VPP because all the base cutscenes will still be sorted alphabetically
    patchDWord((void*)(0x0051DAD0 + 2), (int)&vpp_list); // patch the new list into the startup function.
    patchByte((void*)(0x0051DB36 + 2), sizeof(vpp_list)); // patch number of VPPs it searches for.
    static auto chook_start = safetyhook::create_mid(0x6D46C4, [](SafetyHookContext& ctx) {
        if (cscene_hook.func_init == 0) {
            cscene_hook.func_init = 1;
        }

        size_t number_of_files = *(uintptr_t*)(ctx.ebx + 0x154);
        int& i_loop = *(int*)(ctx.ebp + 0x80);
        int& j_loop = *(int*)(ctx.ebp + 0x7C);

        // printf("number_of_files: %d, i_loop %d j_loop %d\n", number_of_files, i_loop, j_loop);

        if ((i_loop + 1 >= number_of_files) && !cscene_hook.switched_to_dlc && !cscene_hook.next) {
            // printf("IS IT DONE?!\n");
            cscene_hook.next = 1;
        }
        if (cscene_hook.next && !cscene_hook.stop) {
            cscene_hook.stop = 1;
            auto packfile = load_packfile("dlc_cutscenes.vpp_pc");
            if (!packfile)
                return;
            //printf("ITS DOING IT\n");
            cscene_hook.switched_to_dlc = 1;
            i_loop = 0;
            j_loop = 0;
            ctx.ebx = (uintptr_t)packfile;
        }
        });

}

void LoadDLCPersonaVoice(SafetyHookContext& ctx) {
    char* str = (char*)(ctx.ebp);
    const char* DLCBank = "SR2_DLC";
    if (strcmp(str, "SR2_VOC_DL") == 0) { // this is a bit of a hack because it assumes you'll be using a voice_pc that marks the DLC audio as DL
        ctx.ebp = (uintptr_t)DLCBank;
    }
}

void IncreaseMemPool() {
    SafeWrite32((UInt32)0x006AD138, 2049100); // doubled size to avoid the game crashing if you merge voice_pc with dlc_voice_xbox2
    SafeWrite32((UInt32)0x006AD151, 2049100);
}

void __declspec(naked) AddInterfacePeg()
{
    static const char* DLC = "ui_dlc.peg";
    static int jmp_continue = 0x00520803;
    __asm {
        mov edi, dword ptr[0x522450]
        call edi
        mov edx, 0x27716E4
        mov ecx, DLC
        call edi
        mov DLCInstalled, al
        jmp jmp_continue
    }
}

__declspec(naked) wchar_t* RequestMFailedString(const char* Label) {
    __asm {
        push ebp
        mov ebp, esp
        sub esp, __LOCAL_SIZE

        mov eax, ds:Label
        mov esi, ds : 0x2529838
        mov ecx, 0x612460
        call ecx

        mov esp, ebp
        pop ebp
        ret
    }
}

void MissionFStringFix(SafetyHookContext& ctx) {
    ctx.eax = (uintptr_t)RequestMFailedString((const char*)(ctx.esi));
    if (wcscmp((wchar_t*)ctx.eax, L"NULL") == 0) { // DLC ditches the mission_help.xtbl route so the LUA passes in MSN_ directly
        __asm pushad
        ctx.eax = (uintptr_t)RequestString(nullptr, (const char*)(ctx.esi));
        __asm popad
    }
    ctx.eip = 0x00A39959;
}

void PatchFollowerHeads() {

    struct FollowerHead {
        const char* Name;
        uint32_t Value;
    };

    static FollowerHead FollowerHeads[] = { // gotta love Volition!
        { "ui_hud_recruit_wb_f_asian", 0xFFFFFFFF },
        { "ui_hud_recruit_wb_f_black", 0xFFFFFFFF },
        { "ui_hud_recruit_wb_f_hispanic", 0xFFFFFFFF },
        { "ui_hud_recruit_wb_f_white", 0xFFFFFFFF },
        { "ui_hud_recruit_wb_m_asian", 0xFFFFFFFF },
        { "ui_hud_recruit_wb_m_black", 0xFFFFFFFF },
        { "ui_hud_recruit_wb_m_hispanic", 0xFFFFFFFF },
        { "ui_hud_recruit_wb_m_white", 0xFFFFFFFF },
        { "ui_hud_recruit_ho_asian", 0xFFFFFFFF },
        { "ui_hud_recruit_ho_black", 0xFFFFFFFF },
        { "ui_hud_recruit_ho_hispanic", 0xFFFFFFFF },
        { "ui_hud_recruit_ho_white", 0xFFFFFFFF },
        { "ui_hud_recruit_drug_tobias", 0xFFFFFFFF },
        { "ui_hud_recruit_homie_laura", 0xFFFFFFFF },
        { "ui_hud_recruit_homie_jane", 0xFFFFFFFF },
        { "ui_hud_recruit_homie_lee", 0xFFFFFFFF },
        { "ui_hud_recruit_misc_bouncer", 0xFFFFFFFF },
        { "ui_hud_recruit_misc_derby", 0xFFFFFFFF },
        { "ui_hud_recruit_misc_fuzz", 0xFFFFFFFF },
        { "ui_hud_recruit_misc_luz", 0xFFFFFFFF },
        { "ui_hud_recruit_misc_richie", 0xFFFFFFFF },
        { "ui_hud_recruit_coop", 0xFFFFFFFF },
        { "ui_hud_recruit_story_jgat", 0xFFFFFFFF },
        { "ui_hud_recruit_story_julius", 0xFFFFFFFF },
        { "ui_hud_recruit_story_troy", 0xFFFFFFFF },
        { "ui_hud_recruit_story_carlos", 0xFFFFFFFF },
        { "ui_hud_recruit_story_carlos_z", 0xFFFFFFFF },
        { "ui_hud_recruit_story_donnie", 0xFFFFFFFF },
        { "ui_hud_recruit_story_maero", 0xFFFFFFFF },
        { "ui_hud_recruit_story_pierce", 0xFFFFFFFF },
        { "ui_hud_recruit_story_shaundi", 0xFFFFFFFF },
        { "ui_hud_recruit_story_jessica", 0xFFFFFFFF },
        { "ui_hud_recruit_homie_tera_a", 0xFFFFFFFF }
    };

    auto pattern = hook::pattern("44 D8 E8 00");
    pattern.for_each_result([](hook::pattern_match match) {
        void* addr = match.get<void*>();
        SafeWrite32((UInt32)addr, (UInt32)&FollowerHeads[0].Value);
        });

    pattern = hook::pattern("40 D8 E8 00");
    pattern.for_each_result([](hook::pattern_match match) {
        void* addr = match.get<void*>();
        SafeWrite32((UInt32)addr, (UInt32)&FollowerHeads);
        });
    
    SafeWrite8((UInt32)0x007909FE, sizeof(FollowerHeads) / sizeof(FollowerHeads[0]));
    SafeWrite16((UInt32)0x00792A44, sizeof(FollowerHeads));
}

// CHooks = ClippyHooks as in dont expect the best

struct unlockables
{
    char padding[0xC4];
    bool unlocked;
    bool dlc_start_unlocked;
    char unk[0xA];
};

int* max_unlockables_counted = (int*)0x0145A29C;
unlockables* Unlockables = (unlockables*)0x027DD018;
typedef void __fastcall unlock_unlockablesT(unlockables* item,int unused);
unlock_unlockablesT* unlock_item = (unlock_unlockablesT*)(0x6BBD50);

void DLC_Unlocks() {
    if (!*max_unlockables_counted)
        return;
    for (int i = 0; i < *max_unlockables_counted; i++) {
        if (Unlockables[i].dlc_start_unlocked && !Unlockables[i].unlocked) {
            unlock_item(&Unlockables[i],1);
        }
    }
}
SafetyHookInline sub_73B430T{};
// Enter game?
    // - Clippy if this causes issues, either change unsafe_call to call or will just have to do unlocks in a loop when in game
int sub_73B430_hook() {
    auto result = sub_73B430T.unsafe_call<int>();
    DLC_Unlocks();
    return result;
}

void CHooks_unlockable() {

    static auto unlock_hack1 = safetyhook::create_mid(0x006BAD0D, [](SafetyHookContext& ctx) {
        unlockables* current_unlockable = (unlockables*)ctx.ebp;
        Game::xml::xtbl_get_bool("Purchase_Unlocked", &current_unlockable->dlc_start_unlocked, (xtbl_node*)ctx.ebx);
        });
    sub_73B430T = safetyhook::create_inline(0x73B430, &sub_73B430_hook);
}

void MissingDLCString(SafetyHookContext& ctx) {
    if (!DLCInstalled && *(char*)(ctx.ebx + 0xA2) == 1) {
        __asm pushad
        wchar_t* Missing = General::RequestString(nullptr, "DLC_SAVE_TITLE_REPLACEMENT");
        __asm popad
        ctx.ecx = (uintptr_t)Missing;
    }
}

void DontLoadTest(SafetyHookContext& ctx) {
    int SaveArray = *(int*)0x25283A0;
    int CurrentIndex = *(int*)0x25283B4;
    if (!DLCInstalled && *(char*)(SaveArray + 172 * CurrentIndex + 0xA2) == 1) {
        __asm pushad
        wchar_t* Title = General::RequestString(nullptr, "SAVELOAD_ERROR");
        wchar_t* Message = General::RequestString(nullptr, "DLC_CONTENT_NO_MATCH_ON_SAVE_LOAD");
        const wchar_t* Options[] = { General::RequestString(nullptr, "CONTROL_OKAY") };
        __asm popad
        General::AddMessageCustomized(Title, Message, Options, 1);
        ctx.eip = (uintptr_t)UtilsGlobal::RetZero;
    }
}

void DLCSaveSetup() {
    static SafetyHookMid MissingDLC = safetyhook::create_mid(0x00778313, &MissingDLCString);
    static SafetyHookMid DontLoad = safetyhook::create_mid(0x00691E10, &DontLoadTest);

    // hopefully won't override actual save data

    static auto SaveFlag = safetyhook::create_mid(0x00695654, [](SafetyHookContext& ctx) {
        if (DLCInstalled) ctx.ecx += 4;
        });
    static auto ReadFlag = safetyhook::create_mid(0x006958B4, [](SafetyHookContext& ctx) {
        *(char*)(ctx.ebp + 0xA2) = (ctx.eax & 4) != 0;
        });
}

void DLCSetup() {
#if !RELOADED
    DLCSaveSetup();
    PatchFollowerHeads();
    static SafetyHookMid MissionFailure = safetyhook::create_mid(0x00A3994C, &MissionFStringFix);
    CHooks_cutscene();
    CHooks_unlockable();
    WriteRelJump(0x005207FE, (UInt32)&AddInterfacePeg);
    static SafetyHookMid DLCVoice = safetyhook::create_mid(0x0047AD09, &LoadDLCPersonaVoice);
    IncreaseMemPool();
#endif
}