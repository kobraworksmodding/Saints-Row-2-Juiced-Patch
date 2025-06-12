#pragma once
#include "../Patcher/CMultiPatch.h"
#include "../Patcher/CPatch.h"

#include <safetyhook.hpp>
class mempool
{
public:
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

    virtual int func_00();
    virtual bool is_enabled();
    virtual char* always_returns_false();
    virtual int get_free_space_left();
    virtual int pool_in_use();
    virtual int get_max_pool_size();
    virtual bool is_aligned_space_available(int a1, int a2);
    virtual void* alloc_aligned(int allocation_size, unsigned int alloc_alignment);
    virtual int realloc(int a2, int a3);
    virtual bool is_from_this_pool(void* addr);
    virtual int reset();
    virtual void* get_start_of_pool();
    virtual int snapshot_pool();
    virtual bool set_pool_used(int a2);
    virtual int shrink_pool_used_by(int a2);
    virtual bool align_pool(unsigned int a2);
    virtual ~mempool(); // Destructor (was dtor)
    virtual void create(void* start, int size, char* name, int alignment, char a6);
    virtual int alloc_all_free_space(int alignment);
    virtual char func_19();
    virtual void* create_in_parent_pool(void* a2, int size, char* name, int alignment, int a6);
    virtual int func_21();
    virtual bool func_22(int a2);
    virtual bool func_23();
    virtual bool func_24();
};

class string_hash_table
{
public:
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
    int field_54;
    int field_58;
    char dynamic;
    int field_60;
    int field_64;
    mempool* mempool_to_use;
    int field_6C;

    virtual int func_00();
    virtual bool is_enabled();
    virtual char* always_returns_false();
    virtual int get_free_space_left();
    virtual int pool_in_use();
    virtual int get_max_pool_size();
    virtual bool is_aligned_space_available(int a1, int a2);
    virtual void* alloc_aligned(int allocation_size, unsigned int alloc_alignment);
    virtual int realloc(int a2, int a3);
    virtual bool is_from_this_pool(void* addr);
    virtual int reset();
    virtual void* get_start_of_pool();
    virtual int snapshot_pool();
    virtual bool set_pool_used(int a2);
    virtual int shrink_pool_used_by(int a2);
    virtual bool align_pool(unsigned int a2);
    virtual ~string_hash_table(); // Destructor (was dtor)
    virtual void create(void* start, int size, char* name, int alignment, char a6);
    virtual int alloc_all_free_space(int alignment);
    virtual char func_19();
    virtual void* create_in_parent_pool(void* a2, int size, char* name, int alignment, int a6);
    virtual int func_21();
    virtual bool func_22(int a2);
    virtual bool func_23();
    virtual bool func_24();
    uint32_t estimate_maximum_memory_usage(
        uint32_t hash_table_size,
        uint32_t string_pool_size);
};

struct bitmap_entry
{
    char* filename_ptr;
    uintptr_t* this_peg;
    unsigned __int16 frame_number;
    unsigned __int16 user_data;
};
typedef void(__cdecl* Tex_register_callback)(bitmap_entry*);

namespace bitmap_loader {
    extern void Init();
}