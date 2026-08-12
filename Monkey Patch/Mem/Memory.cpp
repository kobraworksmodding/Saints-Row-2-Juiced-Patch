// Memory.cpp (uzis)
// --------------------
// Created: 13/12/2024

#include "../FileLogger.h"
#include "../Patcher/patch.h"
#include "../GameConfig.h"
#include <algorithm>

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

	void ExpandTreeDist()
	{
		// Increases the Tree Fade Distance from 250000 to 500000
		Logger::TypedLog(CHN_MEMORY, "Increasing Tree Fade Distance to 500000.\n");
		patchFloat((BYTE*)0x0252A058, static_cast<float>(NewTreeDist));
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

	struct myLogoItem
	{
		char bytes[24];
	};

#define MAX_CUSTOM_LOGOS   24
	myLogoItem NEW_LOGOS[MAX_CUSTOM_LOGOS];


	void PatchLogos() {
		patchDWord((BYTE*)0x007BD198, (int)&NEW_LOGOS);
		patchDWord((BYTE*)0x007BDF4A, (int)&NEW_LOGOS);
		patchDWord((BYTE*)0x007BE426, (int)&NEW_LOGOS);
		patchDWord((BYTE*)0x007BF99E, (int)&NEW_LOGOS);
		patchDWord((BYTE*)0x007C029D, (int)&NEW_LOGOS);
		patchDWord((BYTE*)0x007C02AE, (int)&NEW_LOGOS);
		patchDWord((BYTE*)0x007C02B9, (int)&NEW_LOGOS);
		patchDWord((BYTE*)0x007C10E9, (int)&NEW_LOGOS);
		patchDWord((BYTE*)0x007C10FA, (int)&NEW_LOGOS);
		patchDWord((BYTE*)0x007C1105, (int)&NEW_LOGOS);
		patchDWord((BYTE*)0x007C2BFB, (int)&NEW_LOGOS);
		patchDWord((BYTE*)0x007C3665, (int)&NEW_LOGOS);
		patchDWord((BYTE*)0x007C38B4, (int)&NEW_LOGOS);
		patchDWord((BYTE*)0x007C40E9, (int)&NEW_LOGOS);
		patchDWord((BYTE*)0x007C40F4, (int)&NEW_LOGOS);
		patchDWord((BYTE*)0x007C40FF, (int)&NEW_LOGOS);
		patchDWord((BYTE*)0x007C410A, (int)&NEW_LOGOS);
		patchDWord((BYTE*)0x007C4C97, (int)&NEW_LOGOS);
		patchDWord((BYTE*)0x007C52A5, (int)&NEW_LOGOS);
		patchDWord((BYTE*)0x007C58D4, (int)&NEW_LOGOS);
		patchDWord((BYTE*)0x007C58E5, (int)&NEW_LOGOS);
		patchDWord((BYTE*)0x007C58F0, (int)&NEW_LOGOS);
		patchDWord((BYTE*)0x007C5919, (int)&NEW_LOGOS);
		patchDWord((BYTE*)0x007C592A, (int)&NEW_LOGOS);
		patchDWord((BYTE*)0x007C5935, (int)&NEW_LOGOS);
		patchDWord((BYTE*)0x007C5972, (int)&NEW_LOGOS);
		patchDWord((BYTE*)0x007C9FE0, (int)&NEW_LOGOS);
		patchDWord((BYTE*)0x007C9FF1, (int)&NEW_LOGOS);
		patchDWord((BYTE*)0x007C9FFC, (int)&NEW_LOGOS);
		patchDWord((BYTE*)0x007CB03C, (int)&NEW_LOGOS);
		patchDWord((BYTE*)0x007CB04D, (int)&NEW_LOGOS);
		patchDWord((BYTE*)0x007CB05A, (int)&NEW_LOGOS);
		patchDWord((BYTE*)0x007CC022, (int)&NEW_LOGOS);
		patchDWord((BYTE*)0x007CC063, (int)&NEW_LOGOS);
		patchDWord((BYTE*)0x007CCE6F, (int)&NEW_LOGOS);

	}
	void Init()
	{
		// ~ PatchLogos();
		if (GameConfig::GetValue("Debug", "GangstaBrawlMemoryExtender", 1)) // Replaces GB MemLimits with SA.
		{
			GangstaBrawlMemoryExt();
		}
#if !JLITE

	int perm_mesh_cpu_new_size = std::clamp((int)GameConfig::GetValue("Mempool", "perm_mesh_cpu", 1114112  * 2, "Default 1114112"), 1114112,INT32_MAX / 4);

			patchInt((void*)(0x0051DED7 + 1), perm_mesh_cpu_new_size);
			patchInt((void*)(0x0051DF0F + 1), perm_mesh_cpu_new_size);
			if(perm_mesh_cpu_new_size != 1114112)
			Logger::TypedLog(CHN_DEBUG, "Expanded perm mesh cpu to {}\n", perm_mesh_cpu_new_size);

	int audio_wavebank_new_size = std::clamp((int)GameConfig::GetValue("Mempool", "audio_wavebank", 102760448, "Default 102760448"), 102760448, INT32_MAX / 4);
			if (audio_wavebank_new_size != 102760448) {
				patchInt((void*)(0x0051EBD7 + 1), audio_wavebank_new_size);
				patchInt((void*)(0x0051EC19 + 1), audio_wavebank_new_size);

				Logger::TypedLog(CHN_DEBUG, "Expanded audio - wavebank to {}\n", audio_wavebank_new_size);

			}

	int animation_new_size = std::clamp((int)GameConfig::GetValue("Mempool", "animation", 17293312 * 2, "Default 17293312"), 17293312, INT32_MAX / 4);
			if (animation_new_size != 17293312) {
				patchInt((void*)(0x51FB30 + 1), animation_new_size);
				Logger::TypedLog(CHN_DEBUG, "Expanded animation to {}\n", animation_new_size);

			}

		//if (GameConfig::GetValue("Graphics", "ExtendedTreeFadeDistance", 0))
		//{
		//	ExpandTreeDist();
		//}

		//if (GameConfig::GetValue("Graphics", "ExtendedShadowRenderDistance", 0))
		//{
		//	ExpandShadowRenderDist();
		//}
#endif
	}
}
