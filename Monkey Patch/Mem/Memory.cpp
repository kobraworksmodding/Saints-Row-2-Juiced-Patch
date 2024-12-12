// Memory.cpp (uzis)
// --------------------
// Created: 13/12/2024

#include "../FileLogger.h"
#include "../Patcher/patch.h"

namespace Memory
{

	int NewTreeDist = 500000;
	int NewShadowDist = 255;

	void GangstaBrawlMemoryExt()
	{
		Logger::TypedLog(CHN_RL, "Patching GangstaBrawlMemoryExtender to Strong Arm Pools...\n");
		patchBytesM((BYTE*)0x00835879, (BYTE*)"\x6A\x02", 2); // client
		patchBytesM((BYTE*)0x00833A52, (BYTE*)"\x6A\x02", 2); // host
		patchBytesM((BYTE*)0x0082FD84, (BYTE*)"\x83\xC3\x08", 3); // Limit Gangsta Brawl/TGB player cap to 8 from 12.
	}

	void ExpandGeneralPools()
	{
		Logger::TypedLog(CHN_DEBUG, "Expanding Memory Pools.\n");
		patchBytesM((BYTE*)0x0051DED7, (BYTE*)"\x68\x00\x00\x15\x00", 5); // perm mesh cpu
		patchBytesM((BYTE*)0x0051DF0F, (BYTE*)"\xB8\x00\x00\x15\x00", 5); // perm mesh cpu
		Logger::TypedLog(CHN_DEBUG, "Expanded perm mesh cpu to 1376256\n");
	}

	void ExpandRenderDist()
	{
		// Increases the Render Distance by x1.85
		// Might be glitchy, would've loved to increase this to x3.00 or x4.00 but the LOD starts bugging out
		Logger::TypedLog(CHN_MEMORY, "Increasing LOD Distance by x1.85. (Only a slight increase.)\n");
		patchBytesM((BYTE*)0x00E996B4, (BYTE*)"\x00\x00\xEC\x3F", 4);
	}

	void ExpandTreeDist()
	{
		// Increases the Tree Fade Distance from 250000 to 500000
		Logger::TypedLog(CHN_MEMORY, "Increasing Tree Fade Distance to 500000.\n");
		patchFloat((BYTE*)0x0252A058, NewTreeDist);
	}

	void ExpandShadowRenderDist()
	{
		// Increases the Shadow Render Distance from 125 to 255, Actually a considerable difference.
		Logger::TypedLog(CHN_MEMORY, "Increasing Shadow Render Distance to 255.\n");
		patchDWord((void*)0x0279778C, NewShadowDist); // Day_ShadowRenderDist > 255
		patchDWord((void*)0x02797790, NewShadowDist); // Night_ShadowRenderDist > 255
		// im a bit scared about these nops but it works?... ~ NOPs calls that set these values from xtbl
		patchNop((BYTE*)0x0054DFEE, 15);
		patchNop((BYTE*)0x0054DFDE, 15);
	}

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

	void ExpandCustItemsPool()
	{
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

	/*void PatchHomies() {
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

	}*/

}