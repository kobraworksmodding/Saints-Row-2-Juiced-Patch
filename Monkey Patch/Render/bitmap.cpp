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

uint32_t string_hash_table::estimate_maximum_memory_usage(uint32_t hash_table_size, uint32_t string_pool_size)
{
    return string_pool_size + 4 * hash_table_size + (hash_table_size << 7);
}

int __declspec(naked) create_empty(string_hash_table* table, int hash_table_size, mempool* to_use, int string_pool_size) {
    __asm {
        push ebp
        mov ebp, esp
        push esi

        mov eax, [ebp + 20]
        mov ecx, [ebp + 16]
        push[ebp + 12]
        push[ebp + 8]

        mov esi, 0xC07A80
        call esi

        pop esi
        mov esp, ebp
        pop ebp
        ret
    }
}

int __declspec(naked) add_string(string_hash_table* table, char* text, short user_data) {
    __asm {
        push ebp
        mov ebp, esp
        push esi

        mov eax, [ebp + 8]
        movzx edx, word ptr[ebp + 16]
        push edx
        push dword ptr[ebp + 12]

        mov esi, 0xC07AF0
        call esi

        pop esi
        mov esp, ebp
        pop ebp
        ret
    }
}


int Bm_discovery_callback(bitmap_entry* fuck) {
    return ((int(__cdecl*)(bitmap_entry*))0x51D700)(fuck);
}

Tex_register_callback get_Tex_register_callback() {
    return *(Tex_register_callback*)0x252A554;
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
    if (*Bm_entry_count <= *Bm_bitmap_count + 1)
    {
        int f = 0;
        bitmap_entry* frame = nullptr;
        char no_extension_fname[64];
        int entries_needed = 0;
        EnterCriticalSection(Bm_add_lock);
        set_thread_ownership(pool);
        set_thread_ownership((mempool*)Extra_bm_filename_hash_table);
        handle = *Bm_bitmap_count++;
        b = (bitmap_entry*)&Bm_bitmaps[handle];
        file_remove_extension(no_extension_fname, 0x40u, filename);
        b->filename_ptr = (char*)add_string(Extra_bm_filename_hash_table, no_extension_fname, handle);
        b->frame_number = 0;
        Bm_discovery_callback(b);
        LeaveCriticalSection(Bm_add_lock);
    }
    else
    {
        bitmap_entry* Bogus_static_bitmap_entry = (bitmap_entry*)*(int*)(0x252A564);
        if(b != nullptr)
        *b = *Bogus_static_bitmap_entry;
    }
    return handle;
}

SafetyHookInline bm_findT{};
__int16 __fastcall bm_find(void* dummy1, void* dummy2, uintptr_t a2, char* String2) {
    auto hndl = bm_findT.thiscall<__int16>(dummy1, a2, String2);
    if (a2 == 0xEC5D38 && hndl == -1) {
        return bm_add_bitmap(String2);
    }
    return hndl;
}
namespace bitmap_loader {
    void Init() {
        if (GameConfig::GetValue("Modding", "addon_bitmaps", 0)) {
            SafeWrite32(0x00C08803 + 1, 1806336);
            SafeWrite32(0x00C08817 + 1, 1806336);
            bm_findT = safetyhook::create_inline(0xC07160, &bm_find);
            static auto bitmap_test = safetyhook::create_mid(0xC083AB, &bitmap_testf);
        }
    }
}
