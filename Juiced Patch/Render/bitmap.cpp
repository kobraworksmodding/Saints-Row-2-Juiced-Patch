// bitmap.cpp (Clippy95 & Tervel)
// --------------------
// Created: 19/6/2025
#include <mutex>
#include <array>
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


volatile LONG Dynamic_bm_frame_redirects[maximum_bitmap_entries]{};
uint16_t Dynamic_bm_frame_counts[maximum_bitmap_entries]{};

uint32_t resolve_dynamic_bitmap(uint32_t handle) {
    if (handle >= maximum_bitmap_entries) return handle;
    while (const LONG next = Dynamic_bm_frame_redirects[handle])
        handle = next - 1;
    return handle;
}

uint32_t dynamic_frame_capacity(uint32_t handle) {
    return handle < maximum_bitmap_entries ? Dynamic_bm_frame_counts[resolve_dynamic_bitmap(handle)] : 0;
}
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
        !ensure_bitmap_capacity(handle + 1))
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
        Dynamic_bm_frame_counts[handle] = 1;
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

namespace {
int reserve_dynamic_frames(int handle, uint32_t frames) {
    scoped_critical_section lock(Bm_add_lock);
    if (handle < 0 || !dynamic_frame_capacity(handle)) return handle;
    bitmap_entry empty{};
    empty.this_peg = *(peg_entry**)0x0252A560;
    const auto old_capacity = dynamic_frame_capacity(handle);
    const uint32_t old = resolve_dynamic_bitmap(handle);
    int result = static_cast<int>(old);
    if (!frames || frames > maximum_bitmap_entries) {
        result = -1;
    } else if (frames > old_capacity) {
        const bool at_end = old + old_capacity == *Bm_bitmap_count;
        const uint32_t target = at_end ? old : *Bm_bitmap_count;
        if (target > maximum_bitmap_entries - frames || !ensure_bitmap_capacity(target + frames)) {
            result = -1;
        } else {
            bitmap_entry* entries = *Bm_bitmaps;
            if (!at_end)
                for (uint32_t i = 0; i < old_capacity; ++i)
                    entries[target + i] = entries[old + i];
            for (uint32_t i = old_capacity; i < frames; ++i) {
                entries[target + i] = empty;
                entries[target + i].filename_ptr = entries[target].filename_ptr;
                entries[target + i].frame_number = static_cast<uint16_t>(i);
                Bm_discovery_callback(entries + target + i);
            }
            Dynamic_bm_frame_counts[target] = static_cast<uint16_t>(frames);
            *Bm_bitmap_count = target + frames;
            if (!at_end) {
                Dynamic_bm_frame_counts[old] = 0;
                for (uint32_t i = 0; i < old_capacity; ++i) {
                    entries[old + i] = empty;
                    InterlockedExchange(Dynamic_bm_frame_redirects + old + i, target + i + 1);
                }
            }
            result = static_cast<int>(target);
        }
    }
    if (result < 0) {
        AssertHandler::AssertOnce("Bm_animated_capacity", "Cannot reserve consecutive addon bitmap animation frames");
    } else {
        for (uint32_t i = 0; i < frames; ++i)
            (*Bm_bitmaps)[result + i].frame_number = static_cast<uint16_t>(i);
    }
    if (result >= 0 && frames > old_capacity) {
        Logger::TypedLog("Modding", "Addon animated bitmap {}: {} frames, handle {} -> {}\n",
            (*Bm_bitmaps)[result].filename_ptr, frames, handle, result);
    }
    return result;
}

void register_dynamic_frames(SafetyHookContext& ctx) {
    scoped_critical_section lock(Bm_add_lock);
    const int handle = static_cast<int16_t>(ctx.eax);
    if (handle < 0 || !dynamic_frame_capacity(handle)) return;
    const auto frames = ctx.ebp;
    const int resolved = reserve_dynamic_frames(handle, frames);
    if (resolved < 0) {
        // Skip the complete group; never register its remaining descriptors as unrelated single images after an allocation failure.
        *reinterpret_cast<uint32_t*>(ctx.esp + 0x10) += frames;
        ctx.eip = 0xC084A3;
        return;
    }
    ctx.ebx = resolved;
    // Bypass the native "addon = one frame" clamp; EBP is the authored count.
    ctx.eip = 0xC0844C;
}

void stream_dynamic_frames(SafetyHookContext& ctx) {
    scoped_critical_section lock(Bm_add_lock);
    const int handle = static_cast<int16_t>(ctx.eax);
    if (handle < 0 || !dynamic_frame_capacity(handle)) return;
    const auto frames = *reinterpret_cast<uint16_t*>(ctx.esi + 16);
    const int resolved = reserve_dynamic_frames(handle, frames);
    if (resolved < 0) {
        const auto gpu = *reinterpret_cast<uint32_t*>(ctx.ebx + 4);
        for (uint32_t i = 0; i < frames; ++i) {
            auto& data = *reinterpret_cast<uint32_t*>(ctx.esi + 48 * i);
            data = data == 0xFFFFFFFF ? 0 : data + gpu;
        }
        *reinterpret_cast<uint32_t*>(ctx.ebx + 8) += frames;
        ctx.eip = 0xC09025;
        return;
    }
    ctx.ebp = resolved;
    ctx.eax = frames;
    // Bypass the separate native 20-frame addon limit in streamed PEGs.
    ctx.eip = 0xC08FBB;
}

void unregister_dynamic_frames(SafetyHookContext& ctx) {
    scoped_critical_section lock(Bm_add_lock);
    const int handle = static_cast<int16_t>(ctx.eax);
    if (handle < 0 || !dynamic_frame_capacity(handle)) return;
    if (ctx.edi > dynamic_frame_capacity(handle)) {
        // Registration failed before this group acquired any bitmap nodes.
        ctx.eax = 0xFFFFFFFF;
        return;
    }
    ctx.eax = resolve_dynamic_bitmap(handle);
}

void install_animated_bitmap_hooks() {
    static auto registration = safetyhook::create_mid(0xC08429, register_dynamic_frames, safetyhook::MidHook::StartDisabled);
    static auto streaming = safetyhook::create_mid(0xC08F71, stream_dynamic_frames, safetyhook::MidHook::StartDisabled);
    static auto unregister = safetyhook::create_mid(0xC08559, unregister_dynamic_frames, safetyhook::MidHook::StartDisabled);
    static auto lookup = safetyhook::create_mid(0xC094E0, [](SafetyHookContext& ctx) {
        ctx.eax = resolve_dynamic_bitmap(ctx.eax);
    }, safetyhook::MidHook::StartDisabled);
    if (!registration || !streaming || !unregister || !lookup
        || !lookup.enable() || !unregister.enable() || !streaming.enable() || !registration.enable()) {
        registration.reset(); streaming.reset(); unregister.reset(); lookup.reset();
        AssertHandler::AssertOnce("Bm_animated_install", "Unable to install all addon animation hooks");
        return;
    }
    // Unloading must only find existing entries. It must never create a new
    // one-slot placeholder and then remove a multi-frame PEG through it.
    WriteRelCall(0xC08551, (int)&bm_find_og);
}
}

namespace {
constexpr int render_readback_count = 29;

struct native_render_readback {
    BYTE pending;
    BYTE padding[3];
    BYTE* pixels;
    int pitch;
    IDirect3DSurface9* surface;
};
static_assert(sizeof(native_render_readback) == 16);

struct render_readback_storage {
    BYTE* pixels = nullptr;
    size_t size = 0;
    UINT width = 0;
    UINT height = 0;
    D3DFORMAT format = D3DFMT_UNKNOWN;
    IDirect3DSurface9* surface = nullptr;
    IDirect3DDevice9* device = nullptr;
};

render_readback_storage Render_readbacks[render_readback_count];
CRITICAL_SECTION Render_readback_lock;

void release_render_readback(render_readback_storage& storage) {
    if (storage.surface) storage.surface->Release();
    if (storage.pixels) HeapFree(GetProcessHeap(), 0, storage.pixels);
    storage = {};
}

void reset_render_readbacks(SafetyHookContext&) {
    scoped_critical_section lock(&Render_readback_lock);
    auto* native = reinterpret_cast<native_render_readback*>(0x22FD678);
    for (int target = 0; target < render_readback_count; ++target) {
        auto& storage = Render_readbacks[target];
        if (storage.surface) storage.surface->Release();
        storage.surface = nullptr;
        storage.device = nullptr;
        // Water code caches the returned CPU pointer. Keep its allocation stable
        // through device resets, but discard the old frame and staging surface.
        if (storage.pixels) memset(storage.pixels, 0, storage.size);
        native[target] = {};
        native[target].pixels = storage.pixels;
        if (storage.height) native[target].pitch = static_cast<int>(storage.size / storage.height);
    }
}

void* __cdecl read_render_target(int target, int* pitch, void* output) {
    if (pitch) *pitch = 0;
    if (target < 0 || target >= render_readback_count) return nullptr;

    scoped_critical_section lock(&Render_readback_lock);
    auto& native = reinterpret_cast<native_render_readback*>(0x22FD678)[target];
    auto& storage = Render_readbacks[target];
    auto* device = *reinterpret_cast<IDirect3DDevice9**>(0x252A2D0);
    auto* source = reinterpret_cast<IDirect3DSurface9**>(0x22FDBA8)[target];
    if (!device) return nullptr;

    D3DSURFACE_DESC desc{};
    desc.Width = reinterpret_cast<UINT*>(0xDC8F00)[target];
    desc.Height = reinterpret_cast<UINT*>(0xDC8E78)[target];
    desc.Format = reinterpret_cast<D3DFORMAT*>(0xDC8FB0)[target];
    if (source && FAILED(source->GetDesc(&desc))) return nullptr;

    size_t pixel_bytes = 0;
    switch (desc.Format) {
    case D3DFMT_A8R8G8B8:
    case D3DFMT_X8R8G8B8:
    case D3DFMT_D24S8:
    case D3DFMT_R32F: pixel_bytes = 4; break;
    case D3DFMT_R5G6B5: pixel_bytes = 2; break;
    case D3DFMT_G32R32F: pixel_bytes = 8; break;
    default: return nullptr;
    }
    if (!desc.Width || !desc.Height || desc.Width > INT_MAX / pixel_bytes) return nullptr;
    const size_t row_bytes = desc.Width * pixel_bytes;
    if (desc.Height > SIZE_MAX / row_bytes) return nullptr;
    const size_t size = row_bytes * desc.Height;

    if (storage.device != device) {
        if (storage.surface) storage.surface->Release();
        storage.surface = nullptr;
        storage.device = device;
    }
    if (!storage.pixels || storage.width != desc.Width || storage.height != desc.Height
        || storage.format != desc.Format) {
        auto* pixels = static_cast<BYTE*>(HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size));
        if (!pixels) return nullptr;
        release_render_readback(storage);
        storage.pixels = pixels;
        storage.size = size;
        storage.width = desc.Width;
        storage.height = desc.Height;
        storage.format = desc.Format;
        storage.device = device;
        if (output) memset(output, 0, size);
    }

    // Water callers keep this pointer between render commands. Publish owned
    // pixels, never LockRect's pointer, whose lifetime ends at UnlockRect.
    native.pixels = storage.pixels;
    native.pitch = static_cast<int>(row_bytes);
    native.surface = storage.surface;
    if (pitch) *pitch = native.pitch;

    if (native.pending && source && SUCCEEDED(device->TestCooperativeLevel())) {
        native.pending = 0;
        if (!storage.surface) {
            device->CreateOffscreenPlainSurface(desc.Width, desc.Height, desc.Format,
                D3DPOOL_SYSTEMMEM, &storage.surface, nullptr);
            native.surface = storage.surface;
        }
        bool copied = false;
        if (storage.surface && SUCCEEDED(device->GetRenderTargetData(source, storage.surface))) {
            D3DLOCKED_RECT locked{};
            if (SUCCEEDED(storage.surface->LockRect(&locked, nullptr, D3DLOCK_READONLY))) {
                if (locked.pBits && locked.Pitch >= static_cast<int>(row_bytes)) {
                    for (UINT row = 0; row < desc.Height; ++row)
                        memcpy(storage.pixels + row * row_bytes,
                            static_cast<const BYTE*>(locked.pBits) + row * static_cast<size_t>(locked.Pitch), row_bytes);
                    copied = true;
                }
                storage.surface->UnlockRect();
            }
        }
        if (!copied) native.pending = 1;
    }
    return storage.pixels;
}

void install_render_readback_hooks() {
    InitializeCriticalSection(&Render_readback_lock);
    static auto readback = safetyhook::create_inline(0xD1C530, read_render_target, safetyhook::InlineHook::StartDisabled);
    // Clear our owned cache before native initialization/reset rebuilds targets.
    // These sites avoid the existing CreateRTs hook in Render3D.cpp.
    static auto initial = safetyhook::create_mid(0xD1F6EC, reset_render_readbacks, safetyhook::MidHook::StartDisabled);
    static auto reset = safetyhook::create_mid(0xD1F893, reset_render_readbacks, safetyhook::MidHook::StartDisabled);
    if (!readback || !initial || !reset || !initial.enable() || !reset.enable() || !readback.enable()) {
        readback.reset(); initial.reset(); reset.reset();
        AssertHandler::AssertOnce("Bm_render_readback", "Unable to install owned render-target readbacks");
        return;
    }
    Logger::TypedLog("Bitmap", "Installed owned render-target readbacks (including water).\n");
}
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
    // Preserve all five bytes, including the DLC append hook at this address.
    std::array<uint8_t, 5> saved;
    memcpy(saved.data(), reinterpret_cast<const void*>(0xB875B0), saved.size());
    patchJmp((void*)0xB875B0, (void*)0xB875C4);
    FlushInstructionCache(GetCurrentProcess(), reinterpret_cast<void*>(0xB875B0), saved.size());
    LoadBitmapTableasm(fileName);
    Memory::VP::Patch(0xB875B0, saved);
    FlushInstructionCache(GetCurrentProcess(), reinterpret_cast<void*>(0xB875B0), saved.size());
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
        install_render_readback_hooks();
        static auto cube_surface_release = safetyhook::create_mid(0xD198B7, [](SafetyHookContext& ctx) {
            // pc_gr_texture_register has just unlocked one cubemap face/mip.
            // GetCubeMapSurface added a reference that the native path never releases.
            auto* surface = *reinterpret_cast<IDirect3DSurface9**>(ctx.esp + 0x20);
            if (surface)
                surface->Release();
        });
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
            install_animated_bitmap_hooks();
            if (GameConfig::GetValue("Modding", "addon_bitmaps", 0) == 180) {
                WriteRelCall(0xC08421, (int)&bm_find_og);
                WriteRelCall(0xC08F69, (int)&bm_find_og);
                WriteRelCall(0xC08551, (int)&bm_find_og);
            }
        }
    }
}
