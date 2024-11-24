#include "GameConfig.h"
#include "Patcher/patch.h"
#include "FileLogger.h"

namespace Clothing {
	struct myItem
	{
		char bytes[120];
	};
	struct myHomie
	{
		char bytes[940];
	};
#define MAX_CUSTOM_ITEMS    3000
	myItem NEW_ITEMS[MAX_CUSTOM_ITEMS];

#define MAX_CUSTOM_HOMIES    24
	myHomie NEW_HOMIES[MAX_CUSTOM_HOMIES];

	void PatchLimit() {
		//Patch Clothing limit from 1050 to 3000 BITCHES.

		Logger::TypedLog(CHN_DEBUG, "Increasing Customization Memory from 1050 to 3000...\n");

		patchBytesM((BYTE*)0x007BBAC6 + 1, (BYTE*)"\x08\xBB", 2);
		patchBytesM((BYTE*)0x007BCC14 + 6, (BYTE*)"\x08\xBB", 2);
		patchDWord((BYTE*)0x0053FD41, (int)&NEW_ITEMS);
		patchDWord((BYTE*)0x006109C5, (int)&NEW_ITEMS);
		patchDWord((BYTE*)0x00611898, (int)&NEW_ITEMS);
		patchDWord((BYTE*)0x00640F5C, (int)&NEW_ITEMS);
		patchDWord((BYTE*)0x006BB709, (int)&NEW_ITEMS);
		patchDWord((BYTE*)0x007BBAE1, (int)&NEW_ITEMS);
		patchDWord((BYTE*)0x007BCDB7, (int)&NEW_ITEMS);
		patchDWord((BYTE*)0x007BD67B, (int)&NEW_ITEMS);
		patchDWord((BYTE*)0x007BDBF5, (int)&NEW_ITEMS);
		patchDWord((BYTE*)0x007BE789, (int)&NEW_ITEMS);
		patchDWord((BYTE*)0x007C1A4B, (int)&NEW_ITEMS);
		patchDWord((BYTE*)0x007C1A86, (int)&NEW_ITEMS);
		patchDWord((BYTE*)0x007C1AAD, (int)&NEW_ITEMS);
		patchDWord((BYTE*)0x007C272C, (int)&NEW_ITEMS);
		patchDWord((BYTE*)0x007C2CE6, (int)&NEW_ITEMS);
		patchDWord((BYTE*)0x007C354F, (int)&NEW_ITEMS);
		patchDWord((BYTE*)0x007C3BAF, (int)&NEW_ITEMS);
		patchDWord((BYTE*)0x007C3E45, (int)&NEW_ITEMS);
		patchDWord((BYTE*)0x007C3FA0, (int)&NEW_ITEMS);
		patchDWord((BYTE*)0x007C5885, (int)&NEW_ITEMS);
		patchDWord((BYTE*)0x007C5891, (int)&NEW_ITEMS);
		patchDWord((BYTE*)0x007C58A7, (int)&NEW_ITEMS);
		patchDWord((BYTE*)0x007C58B0, (int)&NEW_ITEMS);
		patchDWord((BYTE*)0x007C6E97, (int)&NEW_ITEMS);
		patchDWord((BYTE*)0x007C715F, (int)&NEW_ITEMS);
		patchDWord((BYTE*)0x007C7366, (int)&NEW_ITEMS);
		patchDWord((BYTE*)0x007C74D6, (int)&NEW_ITEMS);
		patchDWord((BYTE*)0x007C8E2C, (int)&NEW_ITEMS);
		patchDWord((BYTE*)0x007C8E6F, (int)&NEW_ITEMS);
		patchDWord((BYTE*)0x007C8E7F, (int)&NEW_ITEMS);
		patchDWord((BYTE*)0x007C8E88, (int)&NEW_ITEMS);
		patchDWord((BYTE*)0x007CA32D, (int)&NEW_ITEMS);
		patchDWord((BYTE*)0x007CA7C6, (int)&NEW_ITEMS);
		patchDWord((BYTE*)0x007CA9CE, (int)&NEW_ITEMS);
		patchDWord((BYTE*)0x007CB2D3, (int)&NEW_ITEMS);
		patchDWord((BYTE*)0x007CB985, (int)&NEW_ITEMS);
		patchDWord((BYTE*)0x007CC45C, (int)&NEW_ITEMS);
		patchDWord((BYTE*)0x007CC826, (int)&NEW_ITEMS);
		patchDWord((BYTE*)0x007CC865, (int)&NEW_ITEMS);
		patchDWord((BYTE*)0x007CC87B, (int)&NEW_ITEMS);
		patchDWord((BYTE*)0x007CC884, (int)&NEW_ITEMS);
		patchDWord((BYTE*)0x007D43EA, (int)&NEW_ITEMS);
		patchDWord((BYTE*)0x007D45CD, (int)&NEW_ITEMS);
		patchDWord((BYTE*)0x007D641E, (int)&NEW_ITEMS);
		patchDWord((BYTE*)0x0081ABF0, (int)&NEW_ITEMS);
		patchDWord((BYTE*)0x0081ADB9, (int)&NEW_ITEMS);
		patchDWord((BYTE*)0x0081B1B2, (int)&NEW_ITEMS);
		patchDWord((BYTE*)0x0081B8BD, (int)&NEW_ITEMS);
		patchDWord((BYTE*)0x0081C18B, (int)&NEW_ITEMS);
		patchDWord((BYTE*)0x0081C6AA, (int)&NEW_ITEMS);
		patchDWord((BYTE*)0x0081CBD2, (int)&NEW_ITEMS);
		patchDWord((BYTE*)0x0081D828, (int)&NEW_ITEMS);
		patchDWord((BYTE*)0x0081E102, (int)&NEW_ITEMS);
		patchDWord((BYTE*)0x0088DE77, (int)&NEW_ITEMS);
		patchDWord((BYTE*)0x009A02FE, (int)&NEW_ITEMS);

	}

	void PatchHomies() {
		patchDWord((BYTE*)0x0069205D, (int)&NEW_HOMIES);
		patchDWord((BYTE*)0x00693CD0, (int)&NEW_HOMIES);
		patchDWord((BYTE*)0x00783182, (int)&NEW_HOMIES);
		patchDWord((BYTE*)0x00783D16, (int)&NEW_HOMIES);
		patchDWord((BYTE*)0x00786091, (int)&NEW_HOMIES);
		patchDWord((BYTE*)0x007861EA, (int)&NEW_HOMIES);
		patchDWord((BYTE*)0x0078627A, (int)&NEW_HOMIES);
		patchDWord((BYTE*)0x007862F6, (int)&NEW_HOMIES);
		patchDWord((BYTE*)0x0078644E, (int)&NEW_HOMIES);
		patchDWord((BYTE*)0x00786D77, (int)&NEW_HOMIES);
		patchDWord((BYTE*)0x00786F13, (int)&NEW_HOMIES);
		patchDWord((BYTE*)0x007870A3, (int)&NEW_HOMIES);
		patchDWord((BYTE*)0x007870C3, (int)&NEW_HOMIES);
		patchDWord((BYTE*)0x00787103, (int)&NEW_HOMIES);
		patchDWord((BYTE*)0x00787137, (int)&NEW_HOMIES);
		patchDWord((BYTE*)0x0078718C, (int)&NEW_HOMIES);
		patchDWord((BYTE*)0x00787207, (int)&NEW_HOMIES);
		patchDWord((BYTE*)0x007872CC, (int)&NEW_HOMIES);
		patchDWord((BYTE*)0x0078739C, (int)&NEW_HOMIES);
		patchDWord((BYTE*)0x007875C1, (int)&NEW_HOMIES);
		patchDWord((BYTE*)0x00787738, (int)&NEW_HOMIES);
		patchDWord((BYTE*)0x0078776B, (int)&NEW_HOMIES);
		patchDWord((BYTE*)0x00787791, (int)&NEW_HOMIES);
		patchDWord((BYTE*)0x007877D0, (int)&NEW_HOMIES);
		patchDWord((BYTE*)0x007878DC, (int)&NEW_HOMIES);
		patchDWord((BYTE*)0x00787B0F, (int)&NEW_HOMIES);
		patchDWord((BYTE*)0x00787B55, (int)&NEW_HOMIES);
		patchDWord((BYTE*)0x00787BEF, (int)&NEW_HOMIES);
		patchDWord((BYTE*)0x00788482, (int)&NEW_HOMIES);
		patchDWord((BYTE*)0x00788C26, (int)&NEW_HOMIES);
		patchDWord((BYTE*)0x00789105, (int)&NEW_HOMIES);
		patchDWord((BYTE*)0x00789409, (int)&NEW_HOMIES);
		patchDWord((BYTE*)0x007899FF, (int)&NEW_HOMIES);

	}
}