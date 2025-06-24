#include "DLCSupport.h"
#include "../SafeWrite.h"
#include <safetyhook.hpp>
#include "Patcher/patch.h"
#include "General/General.h" 

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
    const char* ReqString = (const char*)(ctx.esi);
    ctx.eax = (uintptr_t)RequestMFailedString(ReqString);
    if (wcscmp((wchar_t*)ctx.eax, L"NULL") == 0) { // DLC ditches the mission_help.xtbl route so the LUA passes in MSN_ directly
        wprintf(L"Mission falure string is %s\n", (const wchar_t*)ctx.eax);
        __asm pushad
        ctx.eax = (uintptr_t)General::RequestString(nullptr, ReqString);
        __asm popad
        wprintf(L"Attempted to fix it, new string is %s\n", (const wchar_t*)ctx.eax);
    }
    ctx.eip = 0x00A39959;
}

void DLCSetup() {
#if !RELOADED
    static SafetyHookMid MissionFailure = safetyhook::create_mid(0x00A3994C, &MissionFStringFix);
    CHooks_cutscene();
    WriteRelJump(0x005207FE, (UInt32)&AddInterfacePeg);
    static SafetyHookMid DLCVoice = safetyhook::create_mid(0x0047AD09, &LoadDLCPersonaVoice);
    IncreaseMemPool();
#endif
}