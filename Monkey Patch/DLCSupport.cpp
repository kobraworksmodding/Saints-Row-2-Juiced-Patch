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
#include "GameConfig.h"
#include <cwchar>
using namespace General;
using namespace Game::xml;
using namespace DLC;


#define MAX_VEH 168
#define STORE_ITEM_LIMIT 512
// can be increased to 30! 31 will static_assert, reasons why are below (probably but 10 is also safe) (clippy95)
#define MAX_VEHICLES_PER_UNLOCK 6

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
bool IsDLCVI;

#define AutosaveFlag 0xA0
#define CheatFlag 0xA1
#define DLCFlag 0xA2

bool IsSaveDLC() {
    return (*(char*)(SaveArray + 172 * CurrentIndex + DLCFlag) != 0);
}

const char DLCServerFlag[] = "[DLC]";
const wchar_t DLCServerFlagWide[] = L"[DLC]";

bool IsStringDLCFlagged(const char* str)  {
    return std::string_view(str).starts_with(DLCServerFlag);
}

bool IsStringDLCFlagged(const wchar_t* str) {
    return std::wstring_view(str).starts_with(DLCServerFlagWide);
}

void DLC::SetDLCNameFlagOnline() {
    char* OnlineServerName = (char*)0x0212AA08;
    if (DLCInstalled && !IsStringDLCFlagged(OnlineServerName)) {
        char Temp[64];
        strcpy_s(Temp, 64, OnlineServerName);
        patchSprintf(OnlineServerName, 64, "[DLC]%s", Temp);
    }
}

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

typedef uintptr_t __cdecl load_packfileT(const char* name);
load_packfileT* load_packfile = (load_packfileT*)(0xC0AE90);

struct packfile_entry {
    const char* filename;
    const char* ext;
    char padding[0x14];
};

static_assert(sizeof(packfile_entry) == 0x1C);

struct packfile {
    char padding[0x154];
    uint32_t num_files;
    char padding2[0x18];
    packfile_entry* dir;
};

static_assert(offsetof(packfile, num_files) == 0x154);
static_assert(offsetof(packfile, dir) == 0x170);

constexpr int CUTSCENE_LIST_CAPACITY = 512;
constexpr int DLC_CUTSCENE_NAME_CAPACITY = 256;
constexpr int CUTSCENE_NAME_LENGTH = 260;

char* cutscene_list_expanded[CUTSCENE_LIST_CAPACITY];
char dlc_cutscene_names[DLC_CUTSCENE_NAME_CAPACITY][CUTSCENE_NAME_LENGTH];

const char* GetBasename(const char* path) {
    const char* basename = path;
    for (const char* c = path; c && *c; c++) {
        if (*c == '\\' || *c == '/') basename = c + 1;
    }
    return basename;
}

bool CutsceneListContains(char** list, int count, const char* name) {
    for (int i = 0; i < count; i++) {
        if (_stricmp(list[i], name) == 0) return true;
    }
    return false;
}

void AppendCutscenePackfile(SafetyHookContext& ctx, const char* packfile_name) {
    packfile* pack = (packfile*)load_packfile(packfile_name);
    if (!pack || !pack->dir) return;

    int& count = *(int*)(ctx.ebp - 0x38C);
    char**& list = *(char***)(ctx.ebp - 0x394);

    if (count > CUTSCENE_LIST_CAPACITY) count = CUTSCENE_LIST_CAPACITY;
    for (int i = 0; i < count; i++) cutscene_list_expanded[i] = list[i];
    list = cutscene_list_expanded;

    int dlc_name_count = 0;
    for (uint32_t i = 0; i < pack->num_files && count < CUTSCENE_LIST_CAPACITY && dlc_name_count < DLC_CUTSCENE_NAME_CAPACITY; i++) {
        const packfile_entry& entry = pack->dir[i];
        if (!entry.filename || !*entry.filename) continue;

        char name[CUTSCENE_NAME_LENGTH];
        const char* basename = GetBasename(entry.filename);
        if (entry.ext && *entry.ext) {
            sprintf_s(name, "%s.%s", basename, entry.ext);
        }
        else {
            strcpy_s(name, basename);
        }

        _strlwr(name);
        if (!strstr(name, ".xtbl") || CutsceneListContains(list, count, name)) continue;

        strcpy_s(dlc_cutscene_names[dlc_name_count], name);
        list[count++] = dlc_cutscene_names[dlc_name_count++];
    }
}

void CHooks_cutscene() {
    patchNop((void*)0x006D47A8, 5); // this removes the cutscene array sorting
    // the idea is to have the DLC cutscenes get added at the very end
    // it doesn't break the base VPP because all the base cutscenes will still be sorted alphabetically
    patchDWord((void*)(0x0051DAD0 + 2), (int)&vpp_list); // patch the new list into the startup function.
    patchByte((void*)(0x0051DB36 + 2), sizeof(vpp_list)); // patch number of VPPs it searches for.
    static auto AppendDLCCutscenes = safetyhook::create_mid(0x6D478A, [](SafetyHookContext& ctx) {
        AppendCutscenePackfile(ctx, "dlc_cutscenes.vpp_pc");
        });

}

void LoadDLCPersonaVoice(SafetyHookContext& ctx) {
    char* str = (char*)(ctx.ebp);
    const char* DLCBank = "SR2_DLC";
    if (strcmp(str, "SR2_VOC_DL") == 0) { // this is a bit of a hack because it assumes you'll be using a voice_pc that marks the DLC audio as DL
        ctx.ebp = (uintptr_t)DLCBank;
    }
}

void IncreaseVoiceMemPool() {
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
        call SetDLCNameFlagOnline
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

uint32_t* max_unlockables_counted = (uint32_t*)0x0145A29C;
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

struct VehRewards
{
    uint32_t Flags;
    int Vehicles[MAX_VEHICLES_PER_UNLOCK];
};

VehRewards* VehRewardsNew;



struct unlocking_item_vehicle
{

    uint32_t vehicle_handles[MAX_VEHICLES_PER_UNLOCK];
    size_t num_vehicles;

};

struct unlocking_item
{
    unlockables* unlockable;
    bool unlocked;
    bool ui;
    unlocking_item_vehicle data;
};

struct unlockable_vehicle_reward
{
    uint32_t vehicle_type;
    const char* variant_name;
};

struct unlockable_vehicle_reward_data
{
    unlockable_vehicle_reward vehicles[MAX_VEHICLES_PER_UNLOCK];
    uint32_t num_vehicles;
};

#define OFFSET_UNLOCKABLE     offsetof(unlocking_item, unlockable)
#define OFFSET_UNLOCKED       offsetof(unlocking_item, unlocked)
#define OFFSET_UI             offsetof(unlocking_item, ui)
#define OFFSET_VEHICLES       offsetof(unlocking_item, data.vehicle_handles)
#define OFFSET_NUM_VEHICLES   offsetof(unlocking_item, data.num_vehicles)
#define OFFSET_TEMPLATE_NUM_VEHICLES offsetof(unlockable_vehicle_reward_data, num_vehicles)
#define OFFSET_UNLOCKABLE_ITEM_TYPE 0x4
#define OFFSET_UNLOCKABLE_ITEM_TEMPLATE_DATA 0x8
#define OFFSET_TEMPLATE_NUM_VEHICLES_IN_ITEM (OFFSET_UNLOCKABLE_ITEM_TEMPLATE_DATA + OFFSET_TEMPLATE_NUM_VEHICLES)

/* (clippy95) offset cant pass UINT8_MAX, if we really need that shit we have to make hooks for
//     const unsigned int OffsetAddresses[] = {
0x006BB1AB, 0x006BB212, 0x006BB295, 0x006BB29C,
0x006BB332, 0x006BB339, 0x006BC004, 0x006BBF23
    }; */
static_assert((OFFSET_TEMPLATE_NUM_VEHICLES_IN_ITEM <= UINT8_MAX));
#define SIZEOF_UNLOCKABLE_ITEM 0xD0

typedef enum {
    FIELD_UNLOCKABLE,
    FIELD_UNLOCKED,
    FIELD_UI,
    FIELD_VEHICLE_HANDLES,
    FIELD_NUM_VEHICLES,
} unlocking_item_field;

struct unlocking_itemold_XRef {
    uintptr_t patch_location;
    unlocking_item_field field;
};

unlocking_itemold_XRef unlocking_itemold_xrefs[] = {
    { 0x006BAB91, FIELD_UNLOCKED        },
    { 0x006BABA1, FIELD_UNLOCKED        },
    { 0x006BBED0, FIELD_UNLOCKABLE      },
    { 0x006BBED8, FIELD_UNLOCKED        },
    { 0x006BBEDF, FIELD_UI              },
    { 0x006BBEFA, FIELD_UNLOCKED        },
    { 0x006BBF01, FIELD_UI              },
    { 0x006BBF1D, FIELD_NUM_VEHICLES    },
    { 0x006BBF43, FIELD_NUM_VEHICLES    },
    { 0x006BBF57, FIELD_VEHICLE_HANDLES },
    { 0x006BBFE4, FIELD_NUM_VEHICLES    },
    { 0x006BBFF1, FIELD_NUM_VEHICLES    },
    { 0x006BC5C2, FIELD_UNLOCKABLE      },
    { 0x006BC5E3, FIELD_UNLOCKED        },
    { 0x006BC5E9, FIELD_UNLOCKABLE      },
    { 0x006BC5F7, FIELD_UNLOCKED        },
    { 0x006BC607, FIELD_NUM_VEHICLES    },
    { 0x006BC61A, FIELD_VEHICLE_HANDLES },
    { 0x006BC6F6, FIELD_NUM_VEHICLES    },
    { 0x006BC6FD, FIELD_UNLOCKED        },
    { 0x006BC706, FIELD_NUM_VEHICLES    },
    { 0x006BC712, FIELD_NUM_VEHICLES    },
    { 0x006BC7B8, FIELD_UNLOCKABLE      },
    { 0x006BC7C5, FIELD_UNLOCKED        },
    { 0x006BCBC8, FIELD_UNLOCKABLE      },
    { 0x006BCBDB, FIELD_UNLOCKED        },
    { 0x006BCBE0, FIELD_UI              },
    { 0x006BCC34, FIELD_UNLOCKABLE      },
    { 0x006BCC3E, FIELD_UNLOCKED        },
    { 0x006BCC44, FIELD_UI              },
    { 0x006BCC62, FIELD_UNLOCKED        },
    { 0x006BCCDB, FIELD_UNLOCKED        },
    { 0x006BD041, FIELD_UNLOCKABLE      },
    { 0x006BD05D, FIELD_UNLOCKED        },
    { 0x006BD074, FIELD_UNLOCKED        },
    { 0x0076228B, FIELD_UNLOCKED        },
    { 0x0076247A, FIELD_UNLOCKABLE      },
    { 0x00762492, FIELD_UNLOCKED        },
};

const size_t unlocking_itemold_xref_count = sizeof(unlocking_itemold_xrefs) / sizeof(unlocking_itemold_xrefs[0]);

static size_t get_field_offset(unlocking_item_field field) {
    switch (field) {
    case FIELD_UNLOCKABLE:      return OFFSET_UNLOCKABLE;
    case FIELD_UNLOCKED:        return OFFSET_UNLOCKED;
    case FIELD_UI:              return OFFSET_UI;
    case FIELD_VEHICLE_HANDLES: return OFFSET_VEHICLES;
    case FIELD_NUM_VEHICLES:    return OFFSET_NUM_VEHICLES;
    default:                    return 0;
    }
}

unlocking_item new_unlocking_item_array;
SafetyHookInline GetCarsUnlockedStringValue{};

void patch_unlocking_itemold_references(void* new_base) {
    for (size_t i = 0; i < unlocking_itemold_xref_count; i++) {
        void* patch_addr = (void*)unlocking_itemold_xrefs[i].patch_location;
        void* new_value = (void*)((uintptr_t)new_base + get_field_offset(unlocking_itemold_xrefs[i].field));

        patchDWord(patch_addr, (uint32_t)new_value);
    }
}

static int GetVehicleUnlockableTotal()
{
    int total = 0;

    for (uint32_t i = 0; i < *max_unlockables_counted; i++) {
        uintptr_t unlockable = (uintptr_t)Unlockables + (i * SIZEOF_UNLOCKABLE_ITEM);
        uint32_t type = *(uint32_t*)(unlockable + OFFSET_UNLOCKABLE_ITEM_TYPE);

        if (type == 0) {
            total += *(uint32_t*)(unlockable + OFFSET_TEMPLATE_NUM_VEHICLES_IN_ITEM);
        }
    }

    return total;
}

// vnswprintf is inlined :DDDD we can hook the main function and just do this, 
// but we need localization as %d of %d can change in other languages, so i just do a midhook (clippy95)

int __cdecl GetCarsUnlockedStringValueHook(wchar_t* buffer, size_t count, int stats_entry)
{
    return swprintf(buffer, count, L"%d of %d", *(int*)(stats_entry + 0x34), GetVehicleUnlockableTotal());
}

void CHooks_unlockable() {

    patchByte((void*)0x006BB213, MAX_VEHICLES_PER_UNLOCK);
    patchByte((void*)0x006BBFC0, MAX_VEHICLES_PER_UNLOCK);


    patch_unlocking_itemold_references(&new_unlocking_item_array);

    const unsigned int OffsetAddresses[] = {
    0x006BB1AB, 0x006BB212, 0x006BB295, 0x006BB29C,
    0x006BB332, 0x006BB339, 0x006BC004, 0x006BBF23
    };

    for (unsigned int Address : OffsetAddresses) SafeWrite8(Address, (uint8_t)OFFSET_TEMPLATE_NUM_VEHICLES_IN_ITEM);

    static auto get_cars_unlocked_midhook = safetyhook::create_mid(0x6B1DEC, [](SafetyHookContext& ctx) {
        ctx.edx = GetVehicleUnlockableTotal();
        });

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

struct VehInfo_ext
{
    wchar_t display_name[64];
    uint32_t slot;
};

VehInfo_ext* VehArr_ext;

VehInfo_ext* GetVehArr_ext(uintptr_t veh_info)
{
    return (VehInfo_ext*)get_uint(veh_info + 0x276, veh_info + 0x3EA);
}

void ReplaceVehInfoEncodeDisplaywithExtended(SafetyHookContext& ctx)
{
    auto vehinfo_ext = GetVehArr_ext(ctx.ecx - 0x22E);
    ctx.ecx = (uintptr_t)vehinfo_ext->display_name;
}

void IncreaseVehLimits() {

    VehArr = (VehiclePadding*)UtilsGlobal::calloc_game(MAX_VEH, sizeof(VehiclePadding));
    PostLoadArr = (PostLoadPadding*)UtilsGlobal::calloc_game(MAX_VEH, sizeof(PostLoadPadding));
    VehArr_ext = (VehInfo_ext*)UtilsGlobal::calloc_game(MAX_VEH, sizeof(VehInfo_ext));


    for (int i = 0; i < MAX_VEH; i++)
    {
        uintptr_t addr = (uintptr_t)&VehArr[i].Padding[0];
        uintptr_t ext = (uintptr_t)&VehArr_ext[i].display_name[0];
        // (clippy95) this is padding empty unused area in the vehicle info struct, never accessed, so we'll write our own 
        // extended pointer which points to a new array for extended stuff for whatever we want to do
        set_uint(ext, addr + 0x276, addr + 0x3EA);
        VehArr_ext[i].slot = i;
    }

    static auto vehicle_display_name_parse = safetyhook::create_mid(0x00AEB93B, [](SafetyHookContext& ctx) {

        const char* display_name_from_xtbl = (const char*)ctx.ecx;
        auto vehinfo_ext = GetVehArr_ext(ctx.ebp);
        wchar_t* display_name_og_24 = (wchar_t*)(ctx.ebp + 0x22E);

        mbstowcs(vehinfo_ext->display_name, display_name_from_xtbl, sizeof(vehinfo_ext->display_name) / sizeof(wchar_t));


        const char* clean_name = display_name_from_xtbl;
        if (clean_name[0] == '[') {
            const char* closing = strchr(clean_name, ']');
            if (closing) {
                clean_name = closing + 1;
                while (*clean_name == ' ') clean_name++;
            }
        }

        mbstowcs(display_name_og_24, clean_name, 24);
        });

    static auto encode_garage_populate = safetyhook::create_mid(0x554398, ReplaceVehInfoEncodeDisplaywithExtended);
    static auto encode_pause_menu_cars = safetyhook::create_mid(0x78A03E, ReplaceVehInfoEncodeDisplaywithExtended);
    /*int Stack = (MAX_VEH * 65) + 8;

    SafeWrite32(0x00AEE5F0 + 1, Stack); // 4 patches to increase the local filename buffer's size
    SafeWrite32(0x00AEE601 + 3, Stack - 4);
    SafeWrite32(0x00AEE8FD + 3, Stack + 12);
    SafeWrite32(0x00AEE90F + 2, Stack);*/

    SafeWrite32(0x00DB2055 + 1, MAX_VEH - 1); // index for the post load init loop & the new array
    SafeWrite32(0x00DB2050 + 1, (UInt32)PostLoadArr);
    SafeWrite32(0x00ADAEB2 + 3, (UInt32)PostLoadArr);

    SafeWrite32(0x0051F0CB + 1, 1003520); // increased vehicle memory pool for other tables
    SafeWrite32(0x0051F0FD + 1, 1003520);

    ReplaceVehArray(); // swap out all the original veh array references
}

char VintSaveNameBuff[512];

void MissingDLCString(SafetyHookContext& ctx) {
    memset(VintSaveNameBuff, 0, 512);
    static wchar_t New[512];
    wchar_t* AutoSaveLabel = nullptr;
    if (*(char*)(ctx.ebx + DLCFlag) == 1) {
        if (!DLCInstalled) {
            __asm pushad
            wchar_t* Missing = RequestString(nullptr, "DLC_SAVE_TITLE_REPLACEMENT");
            __asm popad
            ctx.ecx = (uintptr_t)Missing;
        }
        else {
            uintptr_t String = ctx.ecx;

            if (*(char*)(ctx.ebx + AutosaveFlag) == 1) {
                __asm pushad
                AutoSaveLabel = RequestString(nullptr, "SAVELOAD_AUTOSAVE_LABEL");
                __asm popad
            }
            if (AutoSaveLabel) wsprintf(New, L"[image:ui_dlc_menu_icon]%s - %s", AutoSaveLabel, (wchar_t*)String);
            else wsprintf(New, L"[image:ui_dlc_menu_icon]%s", (wchar_t*)String);
            ctx.ecx = (uintptr_t)New;
        }
    }
    else if (*(char*)(ctx.ebx + AutosaveFlag) == 1)
    {
        __asm pushad
        wchar_t* AutoSaveLabel = RequestString(nullptr, "SAVELOAD_AUTOSAVE_LABEL");
        __asm popad
        wsprintf(New, L"%s - %s", AutoSaveLabel, (wchar_t*)ctx.ecx);
        ctx.ecx = (uintptr_t)New;
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

void NoMatchOnJoin() {
    __asm pushad
    wchar_t* Title = RequestString(nullptr, "MENU_TITLE_NOTICE");
    wchar_t* Message = RequestString(nullptr, "DLC_CONTENT_NO_MATCH_ON_JOIN");
    const wchar_t* Options[] = { RequestString(nullptr, "CONTROL_OKAY") };
    __asm popad
    AddMessageCustomized(Title, Message, Options, 1);
}

void StartTutorialHook(unsigned int TutorialIndex, int a2, char a3, int a4, char a5) {
    ((void(__cdecl*)(unsigned int, int, char, int, char))0x6B7260)(TutorialIndex, a2, a3, a4, a5);
    if (TutorialIndex == 1) NewGameAutoTut = true;
}

void DLCSaveSetup() {

    patchDWord((void*)0x00778308, 508);
    patchDWord((void*)0x00778506, 508);
    patchDWord((void*)0x007782ED, 508);
    patch_lea_to_mov_ptr(0x7782F1, (uintptr_t)VintSaveNameBuff);
    patch_lea_to_mov_ptr(0x778344, (uintptr_t)VintSaveNameBuff);
    patch_lea_to_mov_ptr(0x77830C, (uintptr_t)VintSaveNameBuff);
    static SafetyHookMid MissingDLC = safetyhook::create_mid(0x00778313, &MissingDLCString);
    static SafetyHookMid DontLoad = safetyhook::create_mid(0x00691E10, &DontLoadTest);
    patchCall((void*)0x00A48636, StartTutorialHook);
        
    // hopefully won't override actual save data

    static auto SaveFlag = safetyhook::create_mid(0x00695654, [](SafetyHookContext& ctx) {
        if (DLCInstalled) ctx.ecx += 4;
        });
    static auto ReadFlag = safetyhook::create_mid(0x006958B4, [](SafetyHookContext& ctx) {
        *(char*)(ctx.ebp + DLCFlag) = (ctx.eax & 4) != 0;
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

struct VehFineAim {
    int Hash = -1;
    vector3 Lookat_Offset;
    float Min_Pitch;
    float Max_Pitch;
    float X_Shift;
    float Heading_Range;
    float Heading_Center;
    float base_fov;
    float z_dist;
    float y_dist;
    uint8_t Flags;
};

struct Vehicle_Fine_Aims_array_XRef {
    uintptr_t patch_location;
    size_t offset;
};

Vehicle_Fine_Aims_array_XRef Vehicle_Fine_Aims_array_xrefs[] = {
    { 0x004964D2, 0x0000 },
    { 0x00498646, 0x0000 },
    { 0x0049C269, 0x0000 },
    { 0x0049D979, 0x0000 },
    { 0x0049D9AB, 0x002C },
    { 0x0049D9BA, 0x0028 },
    { 0x0049D9CE, 0x0014 },
    { 0x0049D9D4, 0x0008 },
    { 0x0049D9E3, 0x0010 },
    { 0x0049D9EF, 0x0024 },
    { 0x0049D9FB, 0x0018 },
    { 0x0049DA07, 0x0004 },
    { 0x0049DA0D, 0x000C },
    { 0x0049EAC9, 0x0030 },
    { 0x0049EB0B, 0x0000 },
    { 0x0049EB2F, 0x0030 },
    { 0x00DA4E46, 0x0000 },
    { 0x00DA4E4B, 0x0034 },
    { 0x00DA4E50, 0x0068 },
    { 0x00DA4E55, 0x009C },
    { 0x00DA4E5A, 0x00D0 },
    { 0x00DA4E5F, 0x0104 },
};

const size_t Vehicle_Fine_Aims_array_xref_count = sizeof(Vehicle_Fine_Aims_array_xrefs) / sizeof(Vehicle_Fine_Aims_array_xrefs[0]);

#define Fine_Aim_Vehicles_size 7
VehFineAim fine[Fine_Aim_Vehicles_size]{};
void patch_Vehicle_Fine_Aims_array_references() {
    uintptr_t new_base = (uintptr_t)fine;
    uintptr_t end_base = ((uintptr_t)&fine[Fine_Aim_Vehicles_size - 1]) + sizeof(uintptr_t);

    Memory::VP::Patch(0x49EB1A + 1, end_base);
    Memory::VP::Patch(0x49D98A + 1, end_base);
    for (size_t i = 0; i < Vehicle_Fine_Aims_array_xref_count; i++) {
        void* patch_addr = (void*)Vehicle_Fine_Aims_array_xrefs[i].patch_location;
        void* new_value = (void*)((uintptr_t)new_base + Vehicle_Fine_Aims_array_xrefs[i].offset);

        Memory::VP::Patch<void*>(patch_addr, new_value);
        printf("Patched 0x%p -> 0x%p (offset +0x%zX)\n",
            patch_addr, new_value, Vehicle_Fine_Aims_array_xrefs[i].offset);
    }
}

void __stdcall ParseVehicleFineAim(xtbl_node* XML)
{
    VehFineAim* Entry = nullptr;
    xtbl_node* VehFineAimNode = xtbl_find(XML, "Vehicle_Fine_Aim");
    int Index = 0;
    VehFineAim* VehFineAimCameras = *(VehFineAim**)0x004964D2;
    bool IsGameWidescreen = *(bool*)0x025272DD;

    while (VehFineAimNode) {

        if ((_strcmpi(xtbl_get_req_string_ref(VehFineAimNode, "Aspect"), "widescreen") == 0) == IsGameWidescreen) {
            //MessageBoxA(0, 0, 0, 0);
            Entry = &VehFineAimCameras[Index++];
            Entry->Hash = Game::utils::str_to_hash((char*)xtbl_get_req_string_ref(VehFineAimNode, "name"));
            xtbl_get_vector(&Entry->Lookat_Offset, VehFineAimNode, "Lookat_Offset");
            Entry->Min_Pitch = xtbl_get_float_lazy(VehFineAimNode, "Min_Pitch");
            Entry->Max_Pitch = xtbl_get_float_lazy(VehFineAimNode, "Max_pitch");
            Entry->X_Shift = xtbl_get_float_lazy(VehFineAimNode, "X_Shift");
            Entry->Heading_Center = xtbl_get_float_lazy(VehFineAimNode, "Heading_Center");
            Entry->Heading_Range = xtbl_get_float_lazy(VehFineAimNode, "Heading_Range");
            Entry->base_fov = xtbl_get_float_lazy(VehFineAimNode, "base_fov");
            Entry->y_dist = xtbl_get_float_lazy(VehFineAimNode, "y_dist");
            Entry->z_dist = xtbl_get_float_lazy(VehFineAimNode, "z_dist");
            Entry->Flags = 0;
           
            xtbl_node* FlagNode = xtbl_find(VehFineAimNode, "flags");

            if (FlagNode) {

                xtbl_node* CurrentFlag = xtbl_find(FlagNode, "Flag");
                
                while (CurrentFlag) {
                    const char* FlagName = xtbl_get_req_string_ref(CurrentFlag, 0);

                    if (_stricmp(FlagName, "limit_heading_range") == 0) Entry->Flags |= 1;
                    else if (_stricmp(FlagName, "turn_player_toward_camera") == 0) Entry->Flags |= 2;
                    else if (_stricmp(FlagName, "turn_camera_with_vehicle") == 0) Entry->Flags |= 4;

                    CurrentFlag = xtbl_find_next(FlagNode, CurrentFlag, "Flag");
                }
            }

            Entry->Min_Pitch = UtilsGlobal::DegreetoRadians(Entry->Min_Pitch);
            Entry->Max_Pitch = UtilsGlobal::DegreetoRadians(Entry->Max_Pitch);
            Entry->Heading_Center = UtilsGlobal::DegreetoRadians(Entry->Heading_Center);
            Entry->Heading_Range = UtilsGlobal::DegreetoRadians(Entry->Heading_Range);
            VehFineAimNode = xtbl_find_next(XML, VehFineAimNode, "Vehicle_Fine_Aim");
        }

        else VehFineAimNode = xtbl_find_next(XML, VehFineAimNode, "Vehicle_Fine_Aim");
    }

    for (int j = 0; j < 2; ++j) { // this was the bright idea they came up with for the DLC flatbed truck
        Entry = &VehFineAimCameras[Index++];

        if (j == 0) {
            Entry->Hash = Game::utils::str_to_hash((char*)"Flatbed Primary");
            Entry->Lookat_Offset = { -0.5f, 1.45f, -1.62f };
        }
        else {
            Entry->Hash = Game::utils::str_to_hash((char*)"Flatbed Secondary");
            Entry->Lookat_Offset = { 0.5f, 1.45f, -1.62f };
        }
        

        Entry->Min_Pitch = -1.134464f;
        Entry->Max_Pitch = 0.61086524f;
        Entry->X_Shift = 0.5f;
        Entry->Heading_Center = 3.1415927f;
        Entry->Heading_Range = 0.0f;
        Entry->base_fov = IsGameWidescreen ? 40.0f : 50.0f;
        Entry->y_dist = 0.0f;
        Entry->z_dist = 1.75f;
        Entry->Flags = (Entry->Flags & 0xF8) | 0x06;
    }
}

void ParseVehicle(xtbl_node* TablePointer, int Unk) {
    ((void(__cdecl*)(xtbl_node*, int))0xAEDA10)(TablePointer, Unk);
}

void ParseVehicleTable(const char* TableName)
{
    xtbl_node* Vehicle;
    char Buffer[32 * 65];
    int Count = 0;

    xtbl_node* Node = parse_table_node(TableName, 0);
    for (Vehicle = xtbl_find(Node, "Vehicle"); Vehicle; Vehicle = xtbl_find_next(Node, Vehicle, "Vehicle")) {
        const char* Name = xtbl_get_req_string_ref(Vehicle, "Name");
        char* New = &Buffer[Count * 65];
        Count++;
        sprintf(New, "%s_veh.xtbl", Name);
    }

    xtbl_free();

    for (int i = 0; i < Count; i++) {
        Node = parse_table_node(&Buffer[i * 65], 0);
        ParseVehicle(Node, -1);
        xtbl_free();
    }
}

void AppendVehicles() {
    static auto ParseTable = safetyhook::create_mid(0x00AEE89B, [](SafetyHookContext& ctx)
        {
            if (!ctx.edi) ParseVehicleTable("dlc_vehicles.xtbl");
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
                ctx.eip = 0x00AEE8A0;
            }
        });

    static auto Reset2 = safetyhook::create_mid(0x00AEE8D6, [](SafetyHookContext& ctx)
        {
            if (IsDLC) IsDLC = false;
        });

    patch_Vehicle_Fine_Aims_array_references();
    patchJmp((void*)0x00496490, &ParseVehicleFineAim);
}

SafetyHookInline ParseVIAnim{};
unsigned char __fastcall ParseVIAnimHook(int* AnimCount, int* AnimPool, const char* TableName, int AnimType) {   
    static const char* VIEnter = "dlc_vi_enter.xtbl";
    static const char* VIExit = "dlc_vi_exit.xtbl";
    static const char* VIRide = "dlc_vi_ride.xtbl";

    if (IsDLCVI) {
        switch (AnimType) {
        case 1:
            return ParseVIAnim.unsafe_fastcall<char>(AnimCount, AnimPool, VIEnter, AnimType);
            break;
        case 2:
            return ParseVIAnim.unsafe_fastcall<char>(AnimCount, AnimPool, VIExit, AnimType);
            break;
        case 3:
            return ParseVIAnim.unsafe_fastcall<char>(AnimCount, AnimPool, VIRide, AnimType);
            break;
        }
    }
     return ParseVIAnim.unsafe_fastcall<char>(AnimCount, AnimPool, TableName, AnimType);
}

void AppendVehicleInteraction() {

    static auto Append = safetyhook::create_mid(0x00AEE64A, [](SafetyHookContext& ctx)
        {
            ((void(__thiscall*)())0x00B160F0)();
            IsDLCVI = true;
        });

    static auto ChangePointSetsTable = safetyhook::create_mid(0x00B1534A, [](SafetyHookContext& ctx)
        {
            static const char* Name = "dlc_vehicle_interaction_point_sets.xtbl";
            ctx.eax = (IsDLCVI ? (uintptr_t)Name : (uintptr_t)0x00E48B84);
            ctx.eip = 0x00B1534F;
        });

    static auto PointSetsAlloc = safetyhook::create_mid(0x00B1536E, [](SafetyHookContext& ctx)
        {
            ctx.eax += 12 * 5; // same increasement as TU3, same applies to all other increasements here
            if (IsDLCVI) {
                if (!ctx.esi) ctx.eflags |= (1 << 6);
                else ctx.eflags &= ~(1 << 6);
                ctx.eip = 0x00B15382;
            }
        });

    ParseVIAnim = safetyhook::create_inline(0xB15D00, ParseVIAnimHook);

    static auto VIAnimAlloc = safetyhook::create_mid(0x00B15D4A, [](SafetyHookContext& ctx)
        {
            ctx.eax += 12 * 10;
            if (IsDLCVI) {
                if (!ctx.esi) ctx.eflags |= (1 << 6);
                else ctx.eflags &= ~(1 << 6);
                ctx.eip = 0x00B15D5B;
            }
        });

    static auto ChangeIntInfoTable = safetyhook::create_mid(0x00B15779, [](SafetyHookContext& ctx)
        {
            static const char* Name = "dlc_vehicle_interaction_info.xtbl";
            ctx.eax = (IsDLCVI ? (uintptr_t)Name : (uintptr_t)0x00E48C5C);
            ctx.eip = 0x00B1577E;
        });

    static auto IntInfoAlloc = safetyhook::create_mid(0x00B1579C, [](SafetyHookContext& ctx)
        {
            ctx.eax += 324 * 10;
            if (IsDLCVI) {
                if (!ctx.esi) ctx.eflags |= (1 << 6);
                else ctx.eflags &= ~(1 << 6);
                IsDLCVI = false;
                ctx.eip = 0x00B157B0;
            }
        });
}

void AppendEffects() {
    static bool IsDLC = false;

    static auto Append = safetyhook::create_mid(0x0050DAD4, [](SafetyHookContext& ctx)
        {
            ((void(*)())0x50D000)();
            IsDLC = true;
        });

    static auto ChangeTable = safetyhook::create_mid(0x0050D01A, [](SafetyHookContext& ctx)
        {
            static const char* Name = "dlc_effects.xtbl";
            ctx.eax = (IsDLC ? (uintptr_t)Name : (uintptr_t)0x00DE044C);
            ctx.eip = 0x0050D01F;
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

void ParseWeaponData(char* WeaponPointer, xtbl_node* TablePointer, bool Refresh) {
    ((void(__cdecl*)(char*, xtbl_node*, bool))0x00B6D6E0)(WeaponPointer, TablePointer, Refresh);
}

void ParseWeaponTable(const char* TableName)
{
    xtbl_node* Weapon;
    int& WeaponCount = *(int*)0x22D7BD0;
    char* WeaponBase = *(char**)0x22D7BCC;

    xtbl_node* Node = parse_table_node(TableName, 0);

    for (Weapon = xtbl_find(Node, "Weapon"); Weapon; Weapon = xtbl_find_next(Node, Weapon, "Weapon")) {
        char* Pointer = WeaponBase + WeaponCount * 1176;
        ParseWeaponData(Pointer, Weapon, false);
        WeaponCount++;
    }
    xtbl_free();
}


void AppendWeapons() {

    static auto IncreaseAlloc = safetyhook::create_mid(0x00B710F1, [](SafetyHookContext& ctx)
        {
            ctx.eax += 40; // they add 40 weapon entries in TU3 as well, which is generous
        });

    static auto LoadTable = safetyhook::create_mid(0x00B711F6, [](SafetyHookContext& ctx)
        {
            if(!*(bool*)(ctx.esp + 16)) ParseWeaponTable("dlc_weapons.xtbl");
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
    AppendVehicles();
    AppendVehicleCameras();
    AppendVehicleInteraction();
    AppendEffects();
    AppendCityMissions();
    AppendWeapons();
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
    if (!DLCInstalled || !isMissionCompleted("tss04") || isMissionCompleted("em01") || isMissionUnlocked("em01")) return;
    UnlockEmergentMissionForDLC();
}

struct pc_multi_session_saver
{
    uint32_t server_id;
    char server_name[256];
    
    inline bool isDLC() {

        return IsStringDLCFlagged(server_name);
    }

};
#define info_count 64
pc_multi_session_saver info[info_count];

uint32_t* server_count = (uint32_t*)0x02528D3C;

union multi_flags_1
{
    uint32_t value;

    struct
    {
        uint32_t unk1 : 16;
        uint32_t is_dlc_lobby : 1;  // 0x00010000
        uint32_t pad_17_31 : 15;
    };
};

multi_flags_1* current_multi_flags = (multi_flags_1*)0xEB22C8;
void CHooks_packet()
{
    static auto packet_mod = safetyhook::create_mid(0x830E26, [](SafetyHookContext& ctx) {
        multi_flags_1* flags = (multi_flags_1*)(ctx.ecx + ctx.edi);
        if(DLCInstalled) flags->is_dlc_lobby = 1;

        });
}

void online()
{
    CHooks_packet();

    static auto SetMPServerNameDLC = safetyhook::create_mid(0x0075EC4A, [](SafetyHookContext& ctx) {
        int Type = *(int*)0x1F77A58;
        if (Type == 7) SetDLCNameFlagOnline();
        });

    static auto fill_it_up = safetyhook::create_mid(0x835F80, [](SafetyHookContext& ctx) {
        const char* server_name = (const char*)(ctx.ecx);
        if (*server_count < info_count) {
            auto this_info = &info[*server_count];
            sprintf_s(this_info->server_name, sizeof(this_info->server_name), "%s", server_name);
            info[*server_count].server_id = *server_count;
        }
        });

    static auto join_online_coop_game = safetyhook::create_mid(0x7EF8CF, [](SafetyHookContext& ctx) {

        auto server_id = ctx.eax;
        if (server_id < info_count) {
            auto this_info = &info[server_id];

            if (!DLCInstalled && this_info->isDLC() || DLCInstalled && !this_info->isDLC()) {
                NoMatchOnJoin();
                ctx.eip = 0x7EF8D4;
            }
        }
        });

    static auto hello_lan = safetyhook::create_mid(0x8131B4, [](SafetyHookContext& ctx) {
        multi_flags_1* this_flags = (multi_flags_1*)(ctx.esi + 0x98);
        printf("this_flags->is_dlc_lobby %d\n", this_flags->is_dlc_lobby);
           if (!DLCInstalled && this_flags->is_dlc_lobby || DLCInstalled && !this_flags->is_dlc_lobby) {
               NoMatchOnJoin();
               ctx.eip = 0x8131A9;
           }

        });

}


void DLC::Init() {
#if !RELOADED
    auto EnableDLC = GameConfig::GetValue("DLC", "EnableDLC", 1, "Requires DLC content to be installed otherwise does nothing");
    if (EnableDLC == -1)
        return;
    online();
    // Patch max memory for Cust Data to prevent mod crashes with DLC.
    patchByte((BYTE*)0x0051EE15, 0x0C);
    patchByte((BYTE*)0x0051EE53, 0x0C);
    // -- 737280 > 802816 (This is enough to have GOTR work, which is main priority i suppose.)
    DLCSaveSetup();
    VehArr = (VehiclePadding*)0x2FAD1F8;
    if (!UtilsGlobal::FolderExists("DLC") || !EnableDLC) return;
    AppendSetup();
    PatchFollowerHeads();
    static SafetyHookMid MissionFailure = safetyhook::create_mid(0x00A3994C, &MissionFStringFix);
    CHooks_cutscene();
    CHooks_unlockable();
    WriteRelJump(0x005207FE, (UInt32)&AddInterfacePeg);
    if (GameConfig::GetValue("DLC", "UnlockEmergentMission", 1)) WriteRelCall(0x0068D167, (UInt32)DLCGlobals);
    static SafetyHookMid DLCVoice = safetyhook::create_mid(0x0047AD09, &LoadDLCPersonaVoice);
    IncreaseVoiceMemPool();
    IncreaseVehLimits();
    static auto PlaceholderStringFix = safetyhook::create_mid(0x00B92C0F, [](SafetyHookContext& ctx) {
        if (ctx.eax)
            if (*(short*)ctx.eax == 0x0001) ctx.ebp = ctx.eax + 2; // eax + 2 so we can get the image tags displaying in the outfits section of the wardrobe like in TU3
        });

#endif
}
