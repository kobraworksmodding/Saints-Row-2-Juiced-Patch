#pragma once
#include "../stdafx.h"

// This is the PerformanceFrequency to fake. It must be a multiple of and greater than 1 million to fix the speed up bug. (10000000 to emulate later Windows versions).
// However it can be set to any value for testing purposes.
#define FAKE_PERF_FREQ 10000000
// #define FAKE_PERF_FREQ 1999999	// Fake the speed up bug

void InitQueryPerformance();
void PatchQueryPerformance();
BOOL __stdcall hook_QueryPerformanceCounter(LARGE_INTEGER *lpPerformanceCount);
BOOL __stdcall hook_QueryPerformanceFrequency(LARGE_INTEGER *lpFrequency);