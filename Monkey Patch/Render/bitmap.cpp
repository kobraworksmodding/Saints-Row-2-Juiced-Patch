// bitmap.cpp (Clippy95 & Tervel)
// --------------------
// Created: 19/6/2025
#include <mutex>
#include <algorithm>
#include <cctype>
#include <limits>
#include <memory>
#include <string>
#include <unordered_map>
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
#include "../Hooker.h"
#include "../Game/CrashFixes.h"

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

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
int* Bm_bogus_static_bitmap = (int*)0xE8314C;
static_assert(sizeof(bitmap_entry) == 12, "SR2 bitmap_entry must remain 12 bytes");


namespace {
// bm_find returns a signed 16-bit handle. 0xFFFF is the not-found sentinel, so
// 0x7FFF is the largest handle that can safely make the round trip through it.
constexpr uint32_t maximum_bitmap_entries = 0x8000;
constexpr uint32_t bitmap_capacity_growth = 256;

struct dynamic_bitmap_record {
    int handle;
    std::string filename;
};

std::unique_ptr<bitmap_entry[]> Dynamic_bm_bitmaps;
uint32_t Dynamic_bm_storage_capacity = 0;
std::unordered_map<std::string, std::unique_ptr<dynamic_bitmap_record>> Dynamic_bm_names;

std::string canonical_bitmap_name(const char* filename);

class scoped_critical_section {
public:
    explicit scoped_critical_section(LPCRITICAL_SECTION critical_section)
        : critical_section_(critical_section)
    {
        EnterCriticalSection(critical_section_);
    }

    ~scoped_critical_section()
    {
        LeaveCriticalSection(critical_section_);
    }

    scoped_critical_section(const scoped_critical_section&) = delete;
    scoped_critical_section& operator=(const scoped_critical_section&) = delete;

private:
    LPCRITICAL_SECTION critical_section_;
};
}

void __cdecl file_remove_extension(char* filename, size_t ext, const char* new_filename_array_size)
{
    const char* new_filename;
    size_t num_characters_to_copy;
    new_filename = strrchr(new_filename_array_size, 46);
    if (new_filename)
    {
        num_characters_to_copy = static_cast<size_t>(new_filename - new_filename_array_size);
        if (num_characters_to_copy > ext - 1)
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

namespace {
std::string canonical_bitmap_name(const char* filename)
{
    char extension_less[64] = {};
    file_remove_extension(extension_less, sizeof(extension_less), filename);

    std::string result(extension_less);
    std::transform(result.begin(), result.end(), result.begin(), [](unsigned char character) {
        return static_cast<char>(std::tolower(character));
    });
    return result;
}

bool initialize_dynamic_bitmap_storage(uint32_t original_entry_count)
{
    if (Dynamic_bm_bitmaps)
        return true;

    if (!*Bm_bitmaps || original_entry_count == 0)
        return false;

    // Keep the table contiguous because PEG registration, unregistration and
    // multiframe animation all perform pointer arithmetic on bitmap_entry.
    Dynamic_bm_storage_capacity = std::max(original_entry_count, maximum_bitmap_entries);

    try {
        auto new_storage = std::make_unique<bitmap_entry[]>(Dynamic_bm_storage_capacity);
        std::copy_n(*Bm_bitmaps, original_entry_count, new_storage.get());
        Dynamic_bm_bitmaps = std::move(new_storage);
    }
    catch (const std::bad_alloc&) {
        Dynamic_bm_storage_capacity = 0;
        AssertHandler::AssertOnce("Bm_dynamic_storage_allocation", "Unable to allocate the dynamic bitmap entry table");
        return false;
    }

    *Bm_bitmaps = Dynamic_bm_bitmaps.get();
    *Bm_entry_count = original_entry_count;

    Logger::TypedLog(
        "Modding",
        "Relocated {} bitmap entries into stable dynamic storage ({} entry limit)\n",
        original_entry_count,
        Dynamic_bm_storage_capacity);
    return true;
}

bool ensure_bitmap_capacity(uint32_t entries_needed)
{
    if (!Dynamic_bm_bitmaps || entries_needed > Dynamic_bm_storage_capacity)
        return false;

    if (entries_needed <= *Bm_entry_count)
        return true;

    const uint32_t grown_capacity = std::min(
        Dynamic_bm_storage_capacity,
        std::max(entries_needed, *Bm_entry_count + bitmap_capacity_growth));
    *Bm_entry_count = grown_capacity;

    Logger::TypedLog("Modding", "Grew bitmap entry capacity to {}\n", grown_capacity);
    return true;
}

int find_dynamic_bitmap_locked(const std::string& canonical_name)
{
    const auto found = Dynamic_bm_names.find(canonical_name);
    return found == Dynamic_bm_names.end() ? -1 : found->second->handle;
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
    if (filename == NULL)
        return -1;
    if ((strcmp("null", filename) == 0) || (strcmp(".tga", filename) == 0))
        return -1;

    const std::string canonical_name = canonical_bitmap_name(filename);
    scoped_critical_section lock(Bm_add_lock);

    if (const int existing_handle = find_dynamic_bitmap_locked(canonical_name); existing_handle >= 0)
        return existing_handle;

    const uint32_t handle = *Bm_bitmap_count;
    if (handle > static_cast<uint32_t>(std::numeric_limits<int16_t>::max()) ||
        !ensure_bitmap_capacity(handle + 2))
    {
        AssertHandler::AssertOnce("Bm_entry_count_over", "bm_add_bitmap exhausted the signed 16-bit bitmap handle space");
        return *Bm_bogus_static_bitmap;
    }

    try {
        auto record = std::make_unique<dynamic_bitmap_record>();
        record->handle = static_cast<int>(handle);

        char extension_less[64] = {};
        file_remove_extension(extension_less, sizeof(extension_less), filename);
        record->filename = extension_less;

        const auto [record_it, inserted] = Dynamic_bm_names.emplace(canonical_name, std::move(record));
        if (!inserted)
            return record_it->second->handle;

        bitmap_entry& entry = (*Bm_bitmaps)[handle];
        entry = {};
        entry.filename_ptr = record_it->second->filename.data();
        entry.this_peg = *(peg_entry**)0x0252A560;
        entry.frame_number = 0;
        ++(*Bm_bitmap_count);

        Bm_discovery_callback(&entry);
        return static_cast<int>(handle);
    }
    catch (const std::bad_alloc&) {
        AssertHandler::AssertOnce("Bm_dynamic_name_allocation", "Unable to allocate a dynamic bitmap name record");
        return *Bm_bogus_static_bitmap;
    }
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
    if (a2 == 0xEC5D38 && hndl == -1) {
        const std::string canonical_name = canonical_bitmap_name(String2);
        scoped_critical_section lock(Bm_add_lock);
        hndl = find_dynamic_bitmap_locked(canonical_name);
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
    //LoadExtraBitMapTable("ui_bms_btnmash_j.xtbl");
    LoadExtraBitMapTable("juiced-ui.xtbl");
    return sub_51D290T.ccall<uintptr_t>();
}
#define first_increase 12000 * 2
#define second_increase 73728
namespace bitmap_loader {
constexpr size_t permanent_default = 0x00800000;
#define KB (size_t)(1024)
#define MB (size_t)(1024 * KB)
#define GB (size_t)(1024 * MB)
uintptr_t bm_load_bitmaps_file_og = 0;
int bm_load_bitmaps_file()
{
    auto result = cdecl_call<int>(bm_load_bitmaps_file_og);
    if (!result)
        return result;

    uintptr_t Bm_bitmaps_data = *(uintptr_t*)(0x023478A4_g);
    *Bm_entry_count = *(int*)(Bm_bitmaps_data + 0x4);
    initialize_dynamic_bitmap_storage(*Bm_entry_count);
    return result;
}
    void Init() {
        if (GameConfig::GetValue("Modding", "addon_bitmaps", 1)) {
            static auto interface_gpu_increase = safetyhook::create_mid(0x51E322, [](SafetyHookContext& ctx) {
                if (double interface_gpu_new_size = GameConfig::GetDoubleValue("Mempool", "interface_gpu_multi", 1.5); interface_gpu_new_size >= 1.0) {
                    ctx.esi = static_cast<uintptr_t>(ctx.esi * interface_gpu_new_size);
                    Logger::TypedLog("Mempool", "Patched interface_gpu size to {}\n", ctx.esi);
                }
                });

            InterceptCall(0xC0884A, bm_load_bitmaps_file_og, bm_load_bitmaps_file);

            size_t new_permanent_size = std::clamp(GameConfig::GetValue("Mempool", "permanent", permanent_default + (permanent_default / 2)), permanent_default, GB / 2);

            size_t first_increase_hastable = std::clamp(GameConfig::GetValue("Mempool", "Bitmap_Image_Names_hashtable", (size_t)first_increase), (size_t)12000, GB);

            size_t second_increase_BitMap_Image_names = std::clamp(GameConfig::GetValue("Mempool", "Bitmap_Image_Names", (size_t)second_increase), (size_t)49152, GB);

            Logger::TypedLog("Mempool", "Patched permanent to {}\n", new_permanent_size);
            Logger::TypedLog("Mempool", "Patched Bitmap_Image_Names_hashtable to {}\n", first_increase_hastable);
            Logger::TypedLog("Mempool", "Patched Bitmap_Image_Names to {}\n", second_increase_BitMap_Image_names);

            SafeWrite32((0x51DCB4 + 1), new_permanent_size);
            SafeWrite32((0x51DDC8 + 1), new_permanent_size);
            // Increase size for bitmap string hash table
            SafeWrite32((0xB8723D + 1), first_increase_hastable);

            SafeWrite32((0xB87280 + 1), second_increase_BitMap_Image_names);
            SafeWrite32((0xB872C3 + 6), second_increase_BitMap_Image_names);
            SafeWrite32((0xB87304 + 6), second_increase_BitMap_Image_names);



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
