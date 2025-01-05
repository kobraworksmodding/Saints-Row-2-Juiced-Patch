#include "metrics dump.h"

__declspec(naked) char * hook_metrics_get_name(int index)
{
	__asm
	{
		push ebp
		mov	ebp, esp
		push ecx
		mov eax, DWORD PTR 8[ebp]
		mov ecx, 0x00C0CBF0
		call ecx
		pop ecx
		pop	ebp
		ret	0
	}
}

__declspec(naked) void hook_metrics_get_frame_time(int index, double *value_out)
{
	__asm
	{
		push ebp
		mov	ebp, esp
		push ecx
		mov eax, DWORD PTR 8[ebp]
		mov ecx, 0x00C0CC60
		call ecx
		mov ecx, DWORD PTR 12[ebp]
		fstp QWORD PTR 0[ecx]
		pop ecx
		pop	ebp
		ret	0
	}
}

void patch_metrics()
{
	patchCall((BYTE*)0x0068E4E2, hook_metrics_dump);
	return;
}

void hook_metrics_dump()
{
	int i;
	char* name;
	double frame_time;

	for (i = 0; i < 1000; i++)
	{
		name = hook_metrics_get_name(i);
		if (!name)
			break;
		hook_metrics_get_frame_time(i, &frame_time);
		frame_time = frame_time * 1000.00;
		if (frame_time > 500.0 && frame_time < 1000000.00)
			Logger::TypedLog(CHN_DEBUG, "Exceeded frame delay in metric %s (%.2fms)\n", name, frame_time);
	}
	return;
}

