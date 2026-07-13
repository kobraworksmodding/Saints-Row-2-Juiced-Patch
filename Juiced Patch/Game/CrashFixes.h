#pragma once
namespace CrashFixes {
	extern void Init();
}
namespace AssertHandler {
	inline void AssertOnce(const char* id, const char* message, bool hide_by_default = false);
}