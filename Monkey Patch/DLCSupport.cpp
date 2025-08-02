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
using namespace Game::xml;

#define MAX_VEH 168
#define STORE_ITEM_LIMIT 512

struct VehiclePadding {
    unsigned char Padding[2000];
};

struct PostLoadPadding {
    unsigned char Padding[12];
};

struct Store {
    const char* Name;
    char Padding[292 + (12 * STORE_ITEM_LIMIT - 4)]; // 73 * 4 = 292 for the offset of the clothing items + (12 * size - 4)
};

VehiclePadding* VehArr;
PostLoadPadding* PostLoadArr;

bool DLCInstalled;
bool NoticesSeen;
bool NewGameAutoTut;
int& SaveArray = *(int*)0x25283A0;
int& CurrentIndex = *(int*)0x25283B4;

bool IsSaveDLC() {
    return (*(char*)(SaveArray + 172 * CurrentIndex + 0xA2) != 0);
}

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

void __declspec(naked) ParseCharacterTable(const char* Extension, int Callback1, const char* TableName, int Callback2, int Pointer) {
    __asm {
        push ebp
        mov ebp, esp
        sub esp, __LOCAL_SIZE

        push Pointer
        push Callback2
        push TableName
        mov edx, Callback1
        mov ecx, Extension
        mov ebx, 0x4A4870
        call ebx

        mov esp, ebp
        pop ebp
        ret
    }
}

void __cdecl CountCallback(int CharacterCount, int* Unk) {
    ((void(__cdecl*)(int, int*))0x4A3B40)(CharacterCount + 15, Unk); // in TU3 this is exactly what they did, hardcoded additional 15 character allocations
}

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
        { "ui_hud_recruit_homie_tera_a", 0xFFFFFFFF },
        { "ui_hud_recruit_gryphon", 0xFFFFFFFF } // this one goes unused in the actual DLC due to an oversight most likely
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

void CHooks_unlockable() {

    static auto unlock_hack1 = safetyhook::create_mid(0x006BAD0D, [](SafetyHookContext& ctx) {
        unlockables* current_unlockable = (unlockables*)ctx.ebp;
        Game::xml::xtbl_get_bool("Purchase_Unlocked", &current_unlockable->dlc_start_unlocked, (xtbl_node*)ctx.ebx);
        });
}

void ReplaceVehArray() {

    const std::pair<unsigned int, std::vector<unsigned int>> Mappings[] = {
    { 0x000000, {
        0x004840C7, 0x004A7444, 0x00541807, 0x005524FD, 0x0055423A, 0x00554349, 0x00555522, 0x00555743,
        0x00575353, 0x0057BD98, 0x0057CF43, 0x0057D07E, 0x0057DB40, 0x0057DC52, 0x0057E48E, 0x0057E4AB,
        0x0057EADD, 0x0057EAF8, 0x0057F9B4, 0x0057F9D7, 0x00581347, 0x00583060, 0x00584074, 0x005840AE,
        0x005845CC, 0x0058466B, 0x005851C2, 0x005851DF, 0x00585212, 0x0058522F, 0x0058AA94, 0x0058E61F,
        0x00591B2F, 0x005C4F87, 0x005E1083, 0x005EB4A8, 0x005F3F76, 0x005FDCD0, 0x005FE3D6, 0x005FE908,
        0x006004B5, 0x00630854, 0x00630D06, 0x00643853, 0x00643872, 0x006448D7, 0x006448F4, 0x00644CC9,
        0x00644CED, 0x0064A0E8, 0x0064A11C, 0x0065EACB, 0x006711F1, 0x00678647, 0x0067ADB6, 0x0067ADC6,
        0x0067ADF5, 0x0067AE5E, 0x0067AE74, 0x00682CDB, 0x00682D16, 0x00682D7B, 0x006831A5, 0x0068359D,
        0x00683630, 0x0068724D, 0x0069FDBE, 0x006BBF4E, 0x006D046D, 0x00783DCC, 0x0078714F, 0x007871B9,
        0x00789FDD, 0x007A668E, 0x007A7702, 0x007DC0DF, 0x007DC3DD, 0x007DC3F8, 0x007DC433, 0x007DC44E,
        0x007DC489, 0x007DC4A7, 0x007DEC0F, 0x007DED1D, 0x007DED3D, 0x007DED5D, 0x007DEDDB, 0x008734D4,
        0x0088640A, 0x00886415, 0x0088693F, 0x00886A30, 0x00886C63, 0x00886E45, 0x00888B11, 0x00888D73,
        0x0088900E, 0x008A4B19, 0x008A4B81, 0x008A4C38, 0x008A5D48, 0x008AEDB2, 0x00922C16, 0x00923C18,
        0x00924074, 0x009391B8, 0x0093A0AA, 0x0093A459, 0x0093DDD1, 0x0093F79F, 0x009400B0, 0x009400E8,
        0x00943FE0, 0x00944067, 0x009446C6, 0x00944B86, 0x00946269, 0x0094658D, 0x009466F9, 0x00947B58,
        0x00947B8F, 0x00947BE5, 0x00947C0B, 0x00948213, 0x0094825D, 0x009483AD, 0x0094898F, 0x00948B96,
        0x00949244, 0x00949276, 0x0094930D, 0x0094948C, 0x009497E3, 0x00949A09, 0x00949AF0, 0x0094F883,
        0x00956695, 0x009588BC, 0x009588EB, 0x00959EE9, 0x00A47702, 0x00A4E1E5, 0x00A4E214, 0x00A6F7F9,
        0x00A6FA1C, 0x00A6FD1A, 0x00A6FE75, 0x00A6FEAA, 0x00AA834C, 0x00AA8397, 0x00AAF213, 0x00AC73E6,
        0x00AC743D, 0x00ADAEA8, 0x00ADB17C, 0x00ADB4FE, 0x00ADBAE2, 0x00AE2A42, 0x00AE2BCB, 0x00AE2CB2,
        0x00AE2F4E, 0x00AE327D, 0x00AE32BF, 0x00AE32F0, 0x00AE3324, 0x00AE3E78, 0x00AE40D1, 0x00AE4507,
        0x00AE456C, 0x00AE461A, 0x00AE468C, 0x00AE46E9, 0x00AE4777, 0x00AE4BC3, 0x00AEDA57, 0x00AEDB84,
        0x00AEDD8D, 0x00B16469, 0x00B545E1
    }},
    { 0x000004, {
        0x00949153, 0x009497F4, 0x00955C45, 0x00AC7AC0, 0x00AE3E87
    }},
    { 0x00007C, {
        0x00AE235B
    }},
    { 0x000080, {
        0x005797B9, 0x0057B2A7, 0x0057C2EB, 0x005826E1, 0x005833CC, 0x00585DDC, 0x0058615A, 0x0058A2BC,
        0x0058AB39, 0x005EB4D1, 0x005F0B48, 0x005F4470, 0x00886CB8, 0x00886DC4, 0x00886DF2, 0x00886FDD,
        0x00A6ED62, 0x00A70BA1, 0x00AC5897, 0x00AE3DE7
    }},
    { 0x000098, {
        0x0057AF35, 0x0057BD25, 0x0057DAFD, 0x0057DC0F, 0x005823A5, 0x00582FF5, 0x0058544C, 0x005854FB,
        0x005940D8, 0x005942DB, 0x005AD36C, 0x005E70DF, 0x005E71DF, 0x005E8BFC, 0x005F0B05, 0x005F1EB5,
        0x0062D6FC, 0x0063C785, 0x006446B2, 0x006446D9, 0x0064A9DF, 0x0064AA03, 0x0064AA27, 0x0064AA4B,
        0x0064AA82, 0x0065097B, 0x00651E0B, 0x00662DD1, 0x00671165, 0x0067CF39, 0x00683575, 0x00683605,
        0x006926CB, 0x00696331, 0x006BB275, 0x007868B5, 0x00922BE3, 0x00927BC9, 0x00946240, 0x00946555,
        0x00AC7A9F, 0x00AE3EF0, 0x00AE40AF, 0x00AEDB56, 0x00AEDD65
    }},
    { 0x00009C, {
        0x00554596, 0x0055676B, 0x005567AB, 0x005E1219, 0x005E3CBC, 0x00642C9C, 0x00642CA3, 0x006450FC,
        0x00888FB3, 0x0093A4BA, 0x00955FC2, 0x009576E5, 0x00A47733, 0x00AB5179, 0x00AB5209, 0x00AB5296
    }},
    { 0x00022E, {
        0x0055267B, 0x005F0B6D, 0x005FEC29, 0x00AE3DF7
    }},
    { 0x000294, {
        0x005E785F, 0x005E7865, 0x005E786D
    }},
    { 0x0002A0, {
        0x00AA83C6, 0x00AA83CE, 0x00AE28E8
    }},
    { 0x0002A4, {
        0x00AE3E08
    }},
    { 0x000414, {
        0x00DB2098
    }},
    { 0x00053C, {
        0x006BBD9C
    }},
    { 0x00059C, {
        0x00AE200F, 0x00AE3F24, 0x00AE3FE1
    }},
    { 0x0005B0, {
        0x00939ADD, 0x00AE5058
    }},
    };

    for (const auto& Entry : Mappings) {
        unsigned int Offset = Entry.first;
        for (unsigned int Address : Entry.second) {
            SafeWrite32(Address, (UInt32)VehArr + Offset);
        }
    }
}

void IncreaseVehLimits() {

    VehArr = (VehiclePadding*)UtilsGlobal::calloc_game(MAX_VEH, sizeof(VehiclePadding));
    PostLoadArr = (PostLoadPadding*)UtilsGlobal::calloc_game(MAX_VEH, sizeof(PostLoadPadding));

    int Stack = (MAX_VEH * 65) + 8;

    SafeWrite32(0x00AEE5F0 + 1, Stack); // 4 patches to increase the local filename buffer's size
    SafeWrite32(0x00AEE601 + 3, Stack - 4);
    SafeWrite32(0x00AEE8FD + 3, Stack + 12);
    SafeWrite32(0x00AEE90F + 2, Stack);

    SafeWrite32(0x00DB2055 + 1, MAX_VEH - 1); // index for the post load init loop & the new array
    SafeWrite32(0x00DB2050 + 1, (UInt32)PostLoadArr);
    SafeWrite32(0x00ADAEB2 + 3, (UInt32)PostLoadArr);

    ReplaceVehArray(); // swap out all the original veh array references
}

void MissingDLCString(SafetyHookContext& ctx) {
    if (!DLCInstalled && *(char*)(ctx.ebx + 0xA2) == 1) {
        __asm pushad
        wchar_t* Missing = RequestString(nullptr, "DLC_SAVE_TITLE_REPLACEMENT");
        __asm popad
        ctx.ecx = (uintptr_t)Missing;
    }
}

void DontLoadTest(SafetyHookContext& ctx) {
    if (!DLCInstalled && IsSaveDLC()) {
        __asm pushad
        wchar_t* Title = RequestString(nullptr, "SAVELOAD_ERROR");
        wchar_t* Message = RequestString(nullptr, "DLC_CONTENT_NO_MATCH_ON_SAVE_LOAD");
        const wchar_t* Options[] = { RequestString(nullptr, "CONTROL_OKAY") };
        __asm popad
        AddMessageCustomized(Title, Message, Options, 1);
        ctx.eip = (uintptr_t)UtilsGlobal::RetZero;
    }
}

void VehicleNotice(int Unk, int SelectedOption, int Action) {
    if (Action == 2 && SelectedOption == 0) {
        NoticesSeen = true; // the idea is to mimic the logic on console - they show up once per game session so if you reload a save w no DLC again it won't show them again
        NewGameAutoTut = false;
        __asm pushad
        wchar_t* Title = RequestString(nullptr, "MENU_TITLE_NOTICE");
        wchar_t* Message = RequestString(nullptr, "DLC_NEW_VEHICLES_AVAILABLE");
        const wchar_t* Options[] = { RequestString(nullptr, "CONTROL_OKAY") };
        __asm popad
        AddMessageCustomized(Title, Message, Options, 1);
    }
}

void ClothingNotice() {
    if (!NoticesSeen && DLCInstalled && !IsSaveDLC()) {
        __asm pushad
        wchar_t* Title = RequestString(nullptr, "MENU_TITLE_NOTICE");
        wchar_t* Message = RequestString(nullptr, "DLC_NEW_CLOTHING_AVAILABLE");
        const wchar_t* Options[] = { RequestString(nullptr, "CONTROL_OKAY") };
        __asm popad
        int Result = AddMessageCustomized(Title, Message, Options, 1);
        *(void**)(Result + 0x930) = &VehicleNotice; // definitely not what Volition did but the next notice is supposed to come right after so I think this is better
        // pretty much just daisy chaining
    }
}

void StartTutorialHook(unsigned int TutorialIndex, int a2, char a3, int a4, char a5) {
    ((void(__cdecl*)(unsigned int, int, char, int, char))0x6B7260)(TutorialIndex, a2, a3, a4, a5);
    if (TutorialIndex == 1) NewGameAutoTut = true;
}

void DLCSaveSetup() {
    static SafetyHookMid MissingDLC = safetyhook::create_mid(0x00778313, &MissingDLCString);
    static SafetyHookMid DontLoad = safetyhook::create_mid(0x00691E10, &DontLoadTest);
    patchCall((void*)0x00A48636, StartTutorialHook);

    // hopefully won't override actual save data

    static auto SaveFlag = safetyhook::create_mid(0x00695654, [](SafetyHookContext& ctx) {
        if (DLCInstalled) ctx.ecx += 4;
        });
    static auto ReadFlag = safetyhook::create_mid(0x006958B4, [](SafetyHookContext& ctx) {
        *(char*)(ctx.ebp + 0xA2) = (ctx.eax & 4) != 0;
        });
    static auto UpdateContinueIndex = safetyhook::create_mid(0x007791BA, [](SafetyHookContext& ctx) {
        *(int*)0x25283B4 = ctx.ebx; // the game does not update the current save index if you use the continue option in the main menu
        });
    static auto SaveContinueWorkaround = safetyhook::create_mid(0x00779270, [](SafetyHookContext& ctx) {
        if (!DLCInstalled && IsSaveDLC()) ctx.eip = 0x0077928E; // workaround to prevent softlocking if you have no DLC and you use the continue button to load a DLC save
        });
    static auto DLCNoticesAndUnlocks = safetyhook::create_mid(0x0073B7D7, [](SafetyHookContext& ctx) {
        DLC_Unlocks();
        if (isMissionCompleted("tss01")) ClothingNotice(); // the mission check is to make it not instantly show up when you start a new game
        });
    static auto DLCNoticesNewGame = safetyhook::create_mid(0x006B6AD0, [](SafetyHookContext& ctx) {
        if (NewGameAutoTut) ClothingNotice(); // post-tutorial like console
        });
}

// Tervel - the appending/merging could be better but I had issues with doing regular lambdas and using writerelcall, otherwise the name swaps & calls could work without mid hooks

void AppendFollowerHeads()
{
    static bool IsDLC = false;

    static auto Append = safetyhook::create_mid(0x00792AC7, [](SafetyHookContext& ctx)
        {
            ((void(*)())0x790A10)(); // we call it once and then when it resumes the game will call it again
            IsDLC = true;
        });

    static auto AddCount = safetyhook::create_mid(0x00790A63, [](SafetyHookContext& ctx)
        {
            *(int*)0x2528514 += (int)ctx.eax;
            ctx.eip = 0x00790A68;
        });

    static auto ChangeTable = safetyhook::create_mid(0x00790A19, [](SafetyHookContext& ctx)
        {
            static const char* Name = "dlc_follower_heads.xtbl";
            ctx.eax = (IsDLC ? (uintptr_t)Name : (uintptr_t)0x00E1E190);
            ctx.eip = 0x00790A1E;
        });

    static auto IncreaseAlloc = safetyhook::create_mid(0x00790A73, [](SafetyHookContext& ctx)
        {
            ctx.eax = ctx.eax + 40; // love it or hate it, this is exactly what they did in the DLC - why calculate it! HARDCODE IT!!!
        });
    static auto SkipAlloc = safetyhook::create_mid(0x00790A71, [](SafetyHookContext& ctx)
        {
            if (IsDLC) {
                IsDLC = false;
                ctx.eip = 0x00790A8B;
            }
        });
}

void AppendHomies() {
    static bool IsDLC = false;

    static auto Append = safetyhook::create_mid(0x005202A7, [](SafetyHookContext& ctx)
        {
            ((void(*)())0x7863E0)();
            IsDLC = true;
        });

    static auto ChangeTable = safetyhook::create_mid(0x007863F4, [](SafetyHookContext& ctx)
        {
            static const char* Name = "dlc_homies.xtbl";
            ctx.eax = (IsDLC ? (uintptr_t)Name : (uintptr_t)0x00E1D5AC);
            IsDLC = false;
            ctx.eip = 0x007863F9;
        });
}

void AppendUnlockables() {
    static bool IsDLC = false;

    static auto Append = safetyhook::create_mid(0x006BCBBF, [](SafetyHookContext& ctx)
        {
            ((void(*)())0x006BC920)();
            IsDLC = true;
        });

    static auto ChangeTable = safetyhook::create_mid(0x006BC928, [](SafetyHookContext& ctx)
        {
            static const char* Name = "dlc_unlockables.xtbl";
            ctx.eax = (IsDLC ? (uintptr_t)Name : (uintptr_t)0x00E0C434);
            IsDLC = false;
            ctx.eip = 0x006BC937;
        });
}

void AppendFoley() {
    static bool IsDLC = false;

    static auto Append = safetyhook::create_mid(0x0046514F, [](SafetyHookContext& ctx)
        {
            ((void(*)())0x00479990)();
            IsDLC = true;
        });

    static auto SkipMemset = safetyhook::create_mid(0x00479993, [](SafetyHookContext& ctx)
        {
            if (IsDLC) ctx.eip = 0x004799A7;
        });

    static auto ChangeTable = safetyhook::create_mid(0x004799AC, [](SafetyHookContext& ctx)
        {
            static const char* Name = "dlc_foley.xtbl";
            ctx.eax = (IsDLC ? (uintptr_t)Name : (uintptr_t)0x00DD87BC);
            IsDLC = false;
            ctx.eip = 0x004799B1;
        });
}

void AppendVoice() {
    static bool IsDLC = false;

    static auto Append = safetyhook::create_mid(0x00465154, [](SafetyHookContext& ctx)
        {
            ((void(*)())0x0047AE30)();
            IsDLC = true;
        });

    static auto SkipMemset = safetyhook::create_mid(0x0047AE33, [](SafetyHookContext& ctx)
        {
            if (IsDLC) ctx.eip = 0x0047AE47;
        });

    static auto SkipTable = safetyhook::create_mid(0x0047AEC5, [](SafetyHookContext& ctx)
        {
            if (IsDLC) {
                IsDLC = false;
                ctx.eip = 0x0047AF63;
            }
        });

    static auto ChangeTable = safetyhook::create_mid(0x0047AE49, [](SafetyHookContext& ctx)
        {
            static const char* Name = "dlc_voice.xtbl";
            ctx.eax = (IsDLC ? (uintptr_t)Name : (uintptr_t)0x00DD886C);
            ctx.eip = 0x0047AE4E;
        });
}

void AppendBitmaps() {
    static bool IsDLC = false;

    static auto Append = safetyhook::create_mid(0x0051F648, [](SafetyHookContext& ctx)
        {
            LoadBitmapTable((const char*)(ctx.esp + 24));
            IsDLC = true;
            LoadBitmapTable("dlc_bitmap_sheets.xtbl");
            ctx.eip = 0x0051F651;
        });

    static auto Skip = safetyhook::create_mid(0x00B875B0, [](SafetyHookContext& ctx)
        {
            if (IsDLC) {
                IsDLC = false;
                ctx.eip = 0x00B875C4;
            }
        });
}

void AppendVehicleCameras() {
    static bool IsDLC = false;

    static auto Append = safetyhook::create_mid(0x00AEE8A2, [](SafetyHookContext& ctx)
        {
            static const char* Name = "dlc_vehicle_cameras.xtbl";
            ctx.eax = (IsDLC ? (uintptr_t)Name : (uintptr_t)0x00E48100);
            ctx.eip = 0x00AEE8A7;
        });

    static auto Reset = safetyhook::create_mid(0x00AEE8C9, [](SafetyHookContext& ctx)
        {
            if (!IsDLC) {
                IsDLC = true;
                ctx.eip = 0x0000AEE8A0;
            }
        });

    static auto Reset2 = safetyhook::create_mid(0x00AEE8D6, [](SafetyHookContext& ctx)
        {
            if (IsDLC) IsDLC = false;
        });
}

void AppendCityMissions() {
    static bool IsDLC = false;

    static auto Append = safetyhook::create_mid(0x005207EF, [](SafetyHookContext& ctx)
        {
            ((void(*)())0x00696C70)();
            IsDLC = true;
        });

    static auto Append2 = safetyhook::create_mid(0x006A410E, [](SafetyHookContext& ctx)
        {
            static const char* Name = "_missions_dlc.xtbl";
            SafeWrite32(0x0069F13D + 1, 0x00E00E6C);
            ((void(*)())0x0069F120)();
            IsDLC = true;
            SafeWrite32(0x0069F13D + 1, (UInt32)Name);
        });

    static auto SkipLoop = safetyhook::create_mid(0x0069F9C2, [](SafetyHookContext& ctx)
        {
            if (!IsDLC) ctx.eip = 0x69FA38; // the idea is to avoid the loop at the end of the function on the first call
            else IsDLC = false;
        });

    static auto ChangeTable = safetyhook::create_mid(0x00696C8A, [](SafetyHookContext& ctx)
        {
            static const char* Name = "sr2_city_missions_dlc.xtbl";
            ctx.eax = (IsDLC ? (uintptr_t)Name : (uintptr_t)0x00E00850);
            IsDLC = false;
            ctx.eip = 0x00696C99;
        });

}

void AppendCityCTS(const char* LevelName) {
    LoadCTS(LevelName);
    // only try loading the dlc one in sr2_city, other levels include mp maps
    if (_stricmp(LevelName, "sr2_city") == 0) LoadCTS("dlc_mission_start_sr2_city");
}

void AppendAudioBanks() {
    static bool IsDLC = false;

    static auto Append = safetyhook::create_mid(0x004819E9, [](SafetyHookContext& ctx)
        {
            ((void(*)())0x00481450)();
            IsDLC = true;
        });

    static auto ChangeTable = safetyhook::create_mid(0x00481469, [](SafetyHookContext& ctx)
        {
            static const char* Name = "dlc_audio_bank.xtbl";
            ctx.eax = (IsDLC ? (uintptr_t)Name : (uintptr_t)0x00DD88F0);
            ctx.eip = 0x0048146E;
        });

    static auto Increase = safetyhook::create_mid(0x00481486, [](SafetyHookContext& ctx)
        {
            ctx.eax += 1; // this is pretty crap but there only other "correct" way to do it would be to get the bank count from the table..
        });

    static auto SkipAlloc = safetyhook::create_mid(0x00481475, [](SafetyHookContext& ctx)
        {
            if (!ctx.eax) ctx.eip = 0x00481690; // vanilla game never assumes the file can be missing so without this it crashes if it can't find it
            else
                if (IsDLC) {
                    IsDLC = false;
                    ctx.eip = 0x004814EF;
                }
        });
}

void AppendItemsInventory() {
    static bool IsDLC = false;

    static auto Append = safetyhook::create_mid(0x0092F5AC, [](SafetyHookContext& ctx)
        {
            ((void(*)())0x0091CF50)();
            IsDLC = true;
        });

    static auto ChangeTable = safetyhook::create_mid(0x0091CF59, [](SafetyHookContext& ctx)
        {
            static const char* Name = "dlc_items_inventory.xtbl";
            ctx.eax = (IsDLC ? (uintptr_t)Name : (uintptr_t)0x00E38D24);
            IsDLC = false;
            ctx.eip = 0x0091CF63;
        });
}

void AppendCustomizationItems() {
    static bool IsDLC = false;

    static auto Append = safetyhook::create_mid(0x007BF9CE, [](SafetyHookContext& ctx)
        {
            ((void(*)())0x7BBA50)();
            IsDLC = true;
        });

    static auto ChangeTable = safetyhook::create_mid(0x007BBA5C, [](SafetyHookContext& ctx)
        {
            static const char* Name = "dlc_customization_items.xtbl";
            ctx.eax = (IsDLC ? (uintptr_t)Name : (uintptr_t)0x00E20E64);
            ctx.eip = 0x007BBA61;
            IsDLC = false;
        });
}

void AppendCustomizationOutfits() {
    static bool IsDLC = false;

    static auto Append = safetyhook::create_mid(0x007BF9E2, [](SafetyHookContext& ctx)
        {
            ((void(*)())0x7BE4B0)();
            IsDLC = true;
        });

    static auto ChangeTable = safetyhook::create_mid(0x007BE4B9, [](SafetyHookContext& ctx)
        {
            static const char* Name = "dlc_customization_outfits.xtbl";
            ctx.eax = (IsDLC ? (uintptr_t)Name : (uintptr_t)0x00E20E80);
            ctx.eip = 0x007BE4BE;
            IsDLC = false;
        });
}

void AppendCustomizationStores() {
    static bool IsDLC = false;
    #define STORE_COUNT *(int*)0x25288F0
    patchNop((void*)0x007BD95C, 10); // removing some of the init
    patchNop((void*)0x007BD5C2, 6);

    int Size = sizeof(Store);
    SafeWrite32(0x007BD62A, STORE_ITEM_LIMIT - 1); // doubled store item limit - this is necessary, otherwise you have too many tattoos in Rusty's Needle for example and past the limit they won't register
    SafeWrite32(0x007BD281, Size); // patches to change the indexing of the array to be * new size
    SafeWrite32(0x007BD2F1, Size);
    SafeWrite32(0x007C4F42, Size);
    SafeWrite32(0x007BFB00, Size);
    SafeWrite32(0x007C4F95, Size);

    static auto Append = safetyhook::create_mid(0x007BF9E7, [](SafetyHookContext& ctx)
        {
            ((void(*)())0x7BD250)();
            IsDLC = true;
        });

    static auto ChangeTable = safetyhook::create_mid(0x007BD259, [](SafetyHookContext& ctx)
        {
            static const char* Name = "dlc_customization_stores.xtbl";
            ctx.eax = (IsDLC ? (uintptr_t)Name : (uintptr_t)0x00E20E9C);
            ctx.eip = 0x007BD25E;
        });

    static auto SkipAlloc = safetyhook::create_mid(0x007BD273, [](SafetyHookContext& ctx)
        {
            if (IsDLC) {
                ctx.esp += 4;
                *(uintptr_t*)(ctx.esp + 68) = STORE_COUNT;
                ctx.eax = (uintptr_t)0x25288EC;
                ctx.eip = 0x007BD2AC;
            }
        });

    static auto StoreSkip = safetyhook::create_mid(0x007BD2E7, [](SafetyHookContext& ctx) // re-parsing the base store data needs to be avoided, so we just handle the store items directly
        {
            if (IsDLC) {
                *(uintptr_t*)(ctx.esp + 60) = STORE_COUNT;
                Store* StoreArray = *(Store**)0x25288EC;
                for (int i = 0; i < STORE_COUNT; i++)
                {
                    const char* StoreName = xtbl_get_req_string_ref(*(xtbl_node**)(ctx.esp + 24), "Name");
                    if (StoreName && strcmp(StoreArray[i].Name, StoreName) == 0)
                    {
                        ctx.ebp = (uintptr_t)&StoreArray[i];
                        *(uintptr_t*)(ctx.esp + 20) = (uintptr_t)&StoreArray[i];
                        break;
                    }
                }
                ctx.eip = 0x007BD553;
            }
        });
}

void AppendCharacterPresets() {
    static bool IsDLC = false;

    static auto Append = safetyhook::create_mid(0x00520082, [](SafetyHookContext& ctx)
        {
            ((void(*)())0x4A5DE0)();
            IsDLC = true;
        });

    static auto ChangeTable = safetyhook::create_mid(0x004A5DFE, [](SafetyHookContext& ctx)
        {
            static const char* Name = "dlc_character_presets.xtbl";
            ctx.eax = (IsDLC ? (uintptr_t)Name : (uintptr_t)0x00DDAB84);
            ctx.eip = 0x004A5E03;
            IsDLC = false;
        });
}

void AppendCharacterMorphs() {
    int Callback = 0x6DFD80;
    ParseCharacterTable("cha", Callback, "character.xtbl", 0, 0);
    ParseCharacterTable("cha", Callback, "dlc_character.xtbl", 0, 0);
}

void AppendCharacter() {
    SafeWrite32(0x004A3F95, (UInt32)&CountCallback);
    patchCall((void*)0x006E08C1, AppendCharacterMorphs); // morphs are parsed from the same table
    static auto Append = safetyhook::create_mid(0x004A3FB7, [](SafetyHookContext& ctx)
        {
            ParseCharacterTable("cha", 0x4A3760, "dlc_character.xtbl", 0, *(uintptr_t*)(ctx.esp + 8));
        });
}

void AppendCharacterDesign() {
    int Callback = 0x4A3B90;
    ParseCharacterTable("des", Callback, "character_design.xtbl", 0, 0);
    ParseCharacterTable("des", Callback, "dlc_character_design.xtbl", 0, 0);
}

void AppendSetup() {
    AppendFollowerHeads();
    AppendHomies();
    AppendUnlockables();
    AppendFoley();
    AppendVoice();
    AppendAudioBanks();
    AppendBitmaps();
    AppendVehicleCameras();
    AppendCityMissions();
    AppendItemsInventory();
    AppendCustomizationItems();
    AppendCustomizationOutfits();
    AppendCustomizationStores();
    AppendCharacter();
    AppendCharacterPresets();
    patchCall((void*)0x004A3FCB, AppendCharacterDesign);
    WriteRelCall(0x00A248A3, (UInt32)AppendCityCTS);
}

void UnlockEmergentMissionForDLC()
{
    if ((*(char*)0xFA239B & 0x10) == 0)
    {
        __asm pushad
        wchar_t* String = RequestString(nullptr, "EMERGENT_MISSION_UNLOCKED");
        __asm popad
        AddInfoMessage(String, (int*)0xEAD1B8);
        unsigned int PhoneHash = Game::utils::str_to_hash((char*)"5552628");
        UnlockPhoneNumber(&PhoneHash, 1);
    }

    ((void(__cdecl*)(bool))0x6A8FF0)(0); // no idea why but they kept the mission unlock outside the if statement
}

void DLCGlobals() { // this was done in LUA because they assumed they'd add more globals probably but due to how small it is it's not worth the loading hassle
    ((void(*)())0x54C220)();
    if (!isMissionCompleted("tss04") || isMissionCompleted("em01") || isMissionUnlocked("em01")) return;
    UnlockEmergentMissionForDLC();
}

void DLCSetup() {
#if !RELOADED
    AppendSetup();
    DLCSaveSetup();
    PatchFollowerHeads();
    static SafetyHookMid MissionFailure = safetyhook::create_mid(0x00A3994C, &MissionFStringFix);
    CHooks_cutscene();
    CHooks_unlockable();
    WriteRelJump(0x005207FE, (UInt32)&AddInterfacePeg);
    WriteRelCall(0x0068D167, (UInt32)DLCGlobals);
    static SafetyHookMid DLCVoice = safetyhook::create_mid(0x0047AD09, &LoadDLCPersonaVoice);
    IncreaseMemPool();
    IncreaseVehLimits();
    static auto PlaceholderStringFix = safetyhook::create_mid(0x00B92C0F, [](SafetyHookContext& ctx) {
        if (ctx.eax)
            if (*(short*)ctx.eax == 0x0001) ctx.ebp = ctx.eax + 2; // eax + 2 so we can get the image tags displaying in the outfits section of the wardrobe like in TU3
        });
#endif
}