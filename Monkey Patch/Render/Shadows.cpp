#include <mutex>
#include <safetyhook.hpp>
#include "d3d9.h"
#include "../General/General.h"
#include <thread>
#include <vector>
#include "../BlingMenu_public.h"
#include "Render3D.h"
#include "../FileLogger.h"
#include "../SafeWrite.h"
#include "../GameConfig.h"
struct mempool;
struct mempool_vt
{
    int(__fastcall* func_00)(mempool* thisa, int unused);
    bool(__fastcall* is_enabled)(mempool* thisa, int unused);
    char* (__fastcall* always_returns_false)(mempool* thisa, int unused);
    int(__fastcall* get_free_space_left)(mempool* thisa, int unused);
    int(__fastcall* pool_in_use)(mempool* thisa, int unused);
    int(__fastcall* get_max_pool_size)(mempool* thisa, int unused);
    bool(__fastcall* is_aligned_space_available)(mempool* thisa, int unused, int a1, int a2);
    void* (__fastcall* alloc)(mempool* thisa, int unused, int allocation_size, unsigned int alloc_alignment);
    int(__fastcall* realloc)(mempool* thisa, int unused, int a2, int a3);
    bool(__fastcall* is_from_this_pool)(mempool* thisa, int unused, void* addr);
    int(__fastcall* reset)(mempool* thisa, int unused);
    void* (__fastcall* get_start_of_pool)(mempool* thisa, int unused);
    int(__fastcall* snapshot_pool_mark)(mempool* thisa, int unused);
    bool(__fastcall* set_pool_used_restore_to_mark)(mempool* thisa, int unused, int a2);
    int(__fastcall* shrink_pool_used_by)(mempool* thisa, int unused, int a2);
    bool(__fastcall* align_pool)(mempool* thisa, int unused, unsigned int a2);
    mempool* (__fastcall* dtor)(mempool* thisa, int unused, char flags$);
    void(__fastcall* create)(mempool* thisa, int unused, void* start, int size, char* name, int alignment, char a6);
    int(__fastcall* alloc_all_free_space)(mempool* thisa, int unused, int alignment);
    char(__fastcall* func_19)(mempool* thisa, int unused);
    void* (__fastcall* create_in_parent_pool)(mempool* thisa, int unused, void* a2, int size, char* name, int alignment, int a6);
    int(__fastcall* func_21)(mempool* thisa, int unused);
    bool(__fastcall* func_22)(mempool* thisa, int unused, int a2);
    bool(__fastcall* func_23)(mempool* thisa, int unused);
    bool(__fastcall* func_24)(mempool* thisa, int unused);
};

struct mempool
{
	mempool_vt* vt;
	bool is_disabled;
	int field_8;
	char name[32];
	char flags;
	int field_30;
	int field_34;
	void* start_of_pool;
	int max_pool_size;
	int default_alignment;
	int pool_used;
	int field_48;
	int size_2;
	int size_3;
};
struct rl_renderable_base_inst;
struct rl_renderable_base_inst_vt
{
    char(__fastcall* func_00)(rl_renderable_base_inst* thisa, int unused);
    int(__fastcall* func_01)(rl_renderable_base_inst* thisa, int unused, int);
    char(__fastcall* func_02)(rl_renderable_base_inst* thisa, int unused, int, int);
    char(__fastcall* func_03)(rl_renderable_base_inst* thisa, int unused, int, int, int);
    char(__fastcall* do_shadows_calucation)(rl_renderable_base_inst* thisa, int unused, int, float*, mempool*);
    void(__fastcall* func_05)(rl_renderable_base_inst* thisa, int unused, int, int);
    char(__fastcall* func_06)(rl_renderable_base_inst* thisa, int unused);
    int(__fastcall* func_07)(rl_renderable_base_inst* thisa, int unused);
    double(__fastcall* func_08)(rl_renderable_base_inst* thisa, int unused, float*, int);
};

struct rl_renderable_base_inst
{
    rl_renderable_base_inst_vt* table;
    int type;
    char name[32];
};


struct render_load
{
    rl_renderable_base_inst* renderable;
   int flags;
   int unk;
   int unk1;
   float unk3;
};


typedef mempool* __fastcall mempoolConstructorT(mempool *thisa, int unused);
mempoolConstructorT* mempool_init = (mempoolConstructorT*)(0xBF48F0);

LPCRITICAL_SECTION shadow_jobs_lock = (LPCRITICAL_SECTION)0xFA23BC;

int* shadow_job_count = (int*)0x25273DC;

struct shadow_job
{
    int u1;
    int u2;
    int unk;
};
shadow_job* shadow_job_list = (shadow_job*)0xFA23DC;

render_load* render_1 = (render_load*)0x277D190;

float* flt_2612D10 = (float*)0x02612D10;

LONG* shadows_jobs_working = (LONG*)0x25273E0;


static std::mutex g_WorkMutex;
static std::condition_variable g_WorkCondition;
static volatile bool g_ShouldShutdown = false;
static volatile bool g_HasWork = false;

void __stdcall shadow_job_thread(LPVOID lpThreadParameter)
{
    void* v1;
    int indexing_1;
    int v3;
    int u2;
    render_load* shadow_job_class;
    mempool shadow_job_pool{};

    mempool_init(&shadow_job_pool, 0);
    v1 = VirtualAlloc(0, 819200, 0x1000, 4);
    if (!v1)
        GetLastError();
    *(int*)0x252A5D0 += 819200;
    strncpy(shadow_job_pool.name, "shadow job pool", 0x20);
    shadow_job_pool.name[31] = 0;
    shadow_job_pool.field_30 = 0;
    shadow_job_pool.flags = 0;
    shadow_job_pool.field_8 = 0;
    shadow_job_pool.is_disabled = 0;
    shadow_job_pool.start_of_pool = v1;
    shadow_job_pool.max_pool_size = 819200;
    shadow_job_pool.default_alignment = 16;
    shadow_job_pool.pool_used = 0;
    shadow_job_pool.field_48 = 0;
    shadow_job_pool.size_2 = 819200;
    shadow_job_pool.size_3 = 819200;

    while (!g_ShouldShutdown)
    {
        {
            std::unique_lock<std::mutex> lock(g_WorkMutex);
            g_WorkCondition.wait(lock, []() {
                return g_ShouldShutdown || g_HasWork;
                });
        }

        if (g_ShouldShutdown)
        {
            break;
        }

        bool workProcessed = false;

        EnterCriticalSection(shadow_jobs_lock);
        if (*shadow_job_count > 0)
        {
            indexing_1 = --(*shadow_job_count);
            workProcessed = true;
            if (*shadow_job_count == 0)
            {
                std::lock_guard<std::mutex> cvLock(g_WorkMutex);
                g_HasWork = false;
            }
        }
        LeaveCriticalSection(shadow_jobs_lock);

        if (workProcessed)
        {
            u2 = shadow_job_list[indexing_1].u2;
            shadow_job_class = &render_1[shadow_job_list[indexing_1].u1];

            shadow_job_pool.vt->snapshot_pool_mark(&shadow_job_pool, 0);
            shadow_job_class->renderable->table->do_shadows_calucation(
                shadow_job_class->renderable, 0,
                u2,
                &flt_2612D10[160 * u2],
                &shadow_job_pool);
            shadow_job_pool.vt->set_pool_used_restore_to_mark(&shadow_job_pool, 0, -1);

            InterlockedExchangeAdd(shadows_jobs_working, -1);
        }
    }
    if (v1)
    {
        VirtualFree(v1, 0, MEM_RELEASE);
        *(int*)0x252A5D0 -= 819200;
    }
}

void SignalWorkAvailable()
{
    {
        std::lock_guard<std::mutex> lock(g_WorkMutex);
        g_HasWork = true;
    }
    g_WorkCondition.notify_all();
}
void InitializeShadowWorkerSync()
{
    g_ShouldShutdown = false;
    g_HasWork = false;
}

void ShutdownShadowWorkers()
{
    {
        std::lock_guard<std::mutex> lock(g_WorkMutex);
        g_ShouldShutdown = true;
    }
    g_WorkCondition.notify_all();
}

namespace Shadows {
    void Init() {
        if (GameConfig::GetValue("Debug", "sync_shadows_threads", 1)) {
            InitializeShadowWorkerSync();
            SafeWrite32((0x00528539 + 1), (uint32_t)&shadow_job_thread);
            static auto add_shadow_job = safetyhook::create_mid(0x5285E2, [](SafetyHookContext& ctx) {
                SignalWorkAvailable();
                });
        }
        Logger::TypedLog(CHN_DEBUG, "Patching amount of Shadow job threads to be %d\n", std::clamp((int)GameConfig::GetValue("Debug", "ShadowThreadCount", 2), 1, 64));
        SafeWrite32(0x528524, std::clamp((int)GameConfig::GetValue("Debug", "ShadowThreadCount", 2), 1, 64));
    }

    void Cleanup() {
        ShutdownShadowWorkers();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

    }
}