/*
	Hopefully fixes the speed up bug in Saints Row 2.
	The bug is caused by an optimisation in the timing routines that can be inaccurate due to precision errors. The value of QueryPerformanceFrequency must be
	a multiple of 1000000 to nullify these errors.
	The fix is to make QueryPerformanceFrequency report a value of 10000000 and adjust the value of QueryPerformanceCounter to return a value proportionate
	to the PerformanceFrequency of 10000000. (This is what later versions of Windows 10 and 11 do).
	It does this by hooking into the import address table of the main process with our new routines.
*/


#include "QueryPerformance Fix.h"
#include "../FileLogger.h"
#include "../iat_functions.h"

double query_perf_mult=1.0;

BOOL __stdcall hook_QueryPerformanceCounter(LARGE_INTEGER *lpPerformanceCount)
{
	BOOL result;

	result=QueryPerformanceCounter(lpPerformanceCount);

	// Adjust the PerformanceCounter to be proportionate to the fake value of PerformanceFrquency.
	// We are not testing for an overflow as it would take over 100 years for an overflow to occur.
	lpPerformanceCount->QuadPart=LONGLONG(lpPerformanceCount->QuadPart*query_perf_mult);
	
	return(result);
}

BOOL __stdcall hook_QueryPerformanceFrequency(LARGE_INTEGER *lpFrequency)
{
	lpFrequency->QuadPart=FAKE_PERF_FREQ;
	return(TRUE);
}

void InitQueryPerformance()
{
	LARGE_INTEGER system_freq;

	QueryPerformanceFrequency(&system_freq);
	query_perf_mult=(double)FAKE_PERF_FREQ/system_freq.QuadPart;

	Logger::TypedLog(CHN_DLL, "QueryPerformance multiplier is %f\n",query_perf_mult);
	return;
}

void PatchQueryPerformance()
{
	void *old_proc;

	InitQueryPerformance();
	
	HMODULE main_handle=GetModuleHandleA(NULL);

	if(PatchIat(main_handle,"Kernel32.dll", "QueryPerformanceFrequency", (void *)hook_QueryPerformanceFrequency, &old_proc)==S_OK)
		Logger::TypedLog(CHN_DLL, "Patched Kernel32.QueryPerformanceFrequency.\n");
	else
		Logger::TypedLog(CHN_DLL, "Patching Kernel32.QueryPerformanceFrequency failed.\n");
	
	if(PatchIat(main_handle,"Kernel32.dll", "QueryPerformanceCounter", (void *)hook_QueryPerformanceCounter, &old_proc)==S_OK)
		Logger::TypedLog(CHN_DLL, "Patched Kernel32.QueryPerformanceCounter.\n");
	else
		Logger::TypedLog(CHN_DLL, "Patching Kernel32.QueryPerformanceCounter failed.\n");

	return;
}