#pragma once
#include <vector>
namespace Memory
{

	extern void Init();
}

struct mempool_entry_s {
	const char* name;
	bool updated_this_session = false;
};
inline std::vector<mempool_entry_s> mempool_registry;