// Render3D.cpp (uzis)
// --------------------
// Created: 15/12/2024

#include "../FileLogger.h"
#include "../Patcher/patch.h"

namespace Render3D
{
	void PatchHQTreeShadows() 
	{
		//10x resolution for TreeShadows
		patchBytesM((BYTE*)0x0053833F, (BYTE*)"\x68\x00\x3C\x00\x00", 5);
		patchBytesM((BYTE*)0x00538344, (BYTE*)"\x68\x00\x3C\x00\x00", 5);
		patchBytesM((BYTE*)0x0051CE75, (BYTE*)"\x68\x00\x3C\x00\x00", 5);
		patchBytesM((BYTE*)0x0051CE7A, (BYTE*)"\x68\x00\x3C\x00\x00", 5);
		patchBytesM((BYTE*)0x00D1F8B2, (BYTE*)"\x68\x00\x3C\x00\x00", 5);
		patchBytesM((BYTE*)0x00D1F8B7, (BYTE*)"\x68\x00\x3C\x00\x00", 5);
		patchBytesM((BYTE*)0x0051FE40, (BYTE*)"\x68\x00\x3C\x00\x00", 5);
		patchBytesM((BYTE*)0x0051FE45, (BYTE*)"\x68\x00\x3C\x00\x00", 5);
	}
}
