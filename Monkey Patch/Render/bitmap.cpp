// bitmap.cpp (Clippy95 & Tervel)
// --------------------
// Created: 19/6/2025
#include <mutex>
#include <safetyhook.hpp>
#include "d3d9.h"
#include "../General/General.h"
#include <thread>
#include <vector>
#include "../BlingMenu_public.h"
#include "Render3D.h"
#include "bitmap.h"
#include "../FileLogger.h"
#include "../SafeWrite.h"
#include "../GameConfig.h"
#include "../Patcher/patch.h"

uint32_t string_hash_table::estimate_maximum_memory_usage(uint32_t hash_table_size, uint32_t string_pool_size)
{
    return string_pool_size + 4 * hash_table_size + (hash_table_size << 7);
}

int __declspec(naked) create_empty(string_hash_table* table, int hash_table_size, mempool* to_use, int string_pool_size) {
    __asm {
        push ebp
        mov ebp, esp
        sub esp, __LOCAL_SIZE

        mov     ecx, to_use
        push    hash_table_size
        push    table
        mov     eax, string_pool_size

        mov esi, 0xC07A80
        call esi

        mov esp, ebp
        pop ebp
        ret
    }
}

int __declspec(naked) add_string(string_hash_table* table, char* text, int user_data) {
    __asm {

        push ebp
        mov ebp, esp
        sub esp, __LOCAL_SIZE

        push user_data
        push text
        mov eax, table

        mov esi, 0xC07AF0
        call esi

        mov esp, ebp
        pop ebp
        ret
    }
}

int Bm_discovery_callback(bitmap_entry* fuck) {
    return ((int(__cdecl*)(bitmap_entry*))0x51D700)(fuck);
}

uint32_t* Bm_entry_count = (uint32_t*)0x2348904;
uint32_t* Bm_bitmap_count = (uint32_t*)0x2348908;
bitmap_entry** Bm_bitmaps = (bitmap_entry**)0x234890C;
void __cdecl file_remove_extension(char* filename, size_t ext, const char* new_filename_array_size)
{
    const char* new_filename;
    size_t num_characters_to_copy;
    new_filename = strrchr(new_filename_array_size, 46);
    if (new_filename)
    {
        num_characters_to_copy = new_filename - new_filename_array_size;
        if (new_filename - new_filename_array_size > ext - 1)
            num_characters_to_copy = ext - 1;
        strncpy(filename, new_filename_array_size, num_characters_to_copy);
        filename[num_characters_to_copy] = 0;
    }
    else
    {
        strncpy(filename, new_filename_array_size, ext);
        filename[ext - 1] = 0;
    }
}

string_hash_table* Extra_bm_filename_hash_table = (string_hash_table*)0xEC5D38;
mempool* pool = (mempool*)0xEC5DA8;

void bitmap_testf(SafetyHookContext& ctx) {
    pool->snapshot_pool();
    uint32_t estimate = Extra_bm_filename_hash_table->estimate_maximum_memory_usage((ctx.ebx - (*Bm_bitmap_count - 1000)), (ctx.ebx - (*Bm_bitmap_count - 1000)));
    if (GameConfig::GetValue("Modding", "extra_hash_table_size", 0) > 15000)
        estimate = (uint32_t)GameConfig::GetValue("Modding", "extra_hash_table_size", 15000);
    pool->alloc_aligned(estimate, 0);
    pool->set_pool_used(-1);
    create_empty(Extra_bm_filename_hash_table, (ctx.ebx - (*Bm_bitmap_count - 1000)), pool, 24 * (ctx.ebx - (*Bm_bitmap_count - 1000)));
}
void set_thread_ownership(mempool* test) {
    test->field_30 = GetCurrentThreadId();
}
LPCRITICAL_SECTION Bm_add_lock = (LPCRITICAL_SECTION)0x33DA354;

int __cdecl bm_add_bitmap(const char* filename)
{
    bitmap_entry* b;
    int handle = 0;
    if (filename == NULL)
        return -1;
    if ((strcmp("null", filename) == 0) || (strcmp(".tga", filename) == 0))
        return -1;
    if (*Bm_entry_count <= *Bm_bitmap_count + 1)
    {
        int f = 0;
        bitmap_entry* frame = nullptr;
        char extension_less[64] = {};
        int entries_needed = 0;
        EnterCriticalSection(Bm_add_lock);
        set_thread_ownership(pool);
        set_thread_ownership((mempool*)Extra_bm_filename_hash_table);
        handle = (*Bm_bitmap_count)++;
        bitmap_entry* Bm_bitmaps = (bitmap_entry*)*(uintptr_t*)(0x234890C);
        b = (bitmap_entry*)&Bm_bitmaps[handle];;
        file_remove_extension(extension_less, 0x40u, filename);
        b->filename_ptr = (char*)add_string(Extra_bm_filename_hash_table, extension_less, handle);
        b->frame_number = 0;
        b->this_peg = (peg_entry*)*(uintptr_t*)0x0252A560;
        Bm_discovery_callback(b);
        LeaveCriticalSection(Bm_add_lock);
    }
    else
    {
        bitmap_entry* Bogus_static_bitmap_entry = (bitmap_entry*)*(int*)(0x252A564);
        if (b != nullptr)
            *b = *Bogus_static_bitmap_entry;
    }
    return handle;
}




SafetyHookInline bm_findT{};
__int16 __fastcall bm_find(void* dummy1, void* dummy2, uintptr_t a2, char* String2) {
    int hndl = 0;
    if (a2 == 0xEC5D38) {
        char extension_less[64] = {};
        file_remove_extension(extension_less, 0x40u, String2);
        hndl = bm_findT.thiscall<__int16>(dummy1, a2, extension_less);
    }
    else
    {
        hndl = bm_findT.thiscall<__int16>(dummy1, a2, String2);
    }
    if (a2 == 0xEC5D38 && hndl == -1) {
        return bm_add_bitmap(String2);
    }
    return hndl;
}

SAFETYHOOK_NOINLINE __int16 __fastcall bm_find_og(void* dummy1, void* dummy2, uintptr_t a2, char* String2) {
    int hndl = 0;
    if (a2 == 0xEC5D38) {
        char extension_less[64] = {};
        file_remove_extension(extension_less, 0x40u, String2);
        hndl = bm_findT.thiscall<__int16>(dummy1, a2, extension_less);
    }
    else {
        hndl = bm_findT.thiscall<__int16>(dummy1, a2, String2);
    }
    return hndl;
}

__declspec(naked) void LoadBitmapTableasm(const char* FileName) {


    __asm {

        push ebp
        mov ebp, esp

        sub esp, __LOCAL_SIZE
        mov eax, FileName

        mov edx, 0xB87540

        call edx

        mov esp, ebp

        pop ebp

        ret

    }
}

bitmap_statusT bitmap_status{};

void LoadExtraBitMapTable(const char* fileName) {
    patchJmp((void*)0xB875B0, (void*)0xB875C4);
    LoadBitmapTableasm(fileName);
    patchDWord((void*)0xB875B0, 0xA1A005C7);
}
SafetyHookInline load_pegT;
bool __fastcall load_peg_hook(const char* filename, uintptr_t mempool) {

    if (mempool == 0x27716E4 && strcmp(filename, "interface-backend.peg") == 0) {
        load_pegT.fastcall<bool>(filename, mempool);
        bool result = load_pegT.fastcall<bool>("juiced-ui.peg", mempool);
        bitmap_status.juiced_ui_loaded = result;
        //load_pegT.disable();
        return result;
    }

    return load_pegT.fastcall<bool>(filename, mempool);
}

SafetyHookInline sub_51D290T;

uintptr_t sub_51D290Lang() {
    LoadExtraBitMapTable("juiced-ui.xtbl");
    return sub_51D290T.ccall<uintptr_t>();
}

namespace bitmap_loader {
    void Init() {
        if (GameConfig::GetValue("Modding", "addon_bitmaps", 1)) {
            static auto interface_gpu_increase = safetyhook::create_mid(0x51E322, [](SafetyHookContext& ctx) {
                if (double interface_gpu_new_size = GameConfig::GetDoubleValue("Mempool", "interface_gpu_multi", 1.5); interface_gpu_new_size >= 1.0) {
                    ctx.esi *= interface_gpu_new_size;
                    Logger::TypedLog("Mempool", "Patched interface_gpu size to 0x%X\n", ctx.esi);
                }
                });
            load_pegT = safetyhook::create_inline(0x522450, &load_peg_hook);
            sub_51D290T = safetyhook::create_inline(0x51D290, sub_51D290Lang);
            SafeWrite32(0x00C08803 + 1, 1806336);
            SafeWrite32(0x00C08817 + 1, 1806336);
            bm_findT = safetyhook::create_inline(0xC07160, &bm_find);
            static auto bitmap_test = safetyhook::create_mid(0xC083AB, &bitmap_testf);
            if (GameConfig::GetValue("Modding", "addon_bitmaps", 0) == 180) {
                WriteRelCall(0xC08421, (int)&bm_find_og);
                WriteRelCall(0xC08F69, (int)&bm_find_og);
                WriteRelCall(0xC08551, (int)&bm_find_og);
            }
        }
    }
}
