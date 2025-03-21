#pragma once
#include "../Patcher/CMultiPatch.h"
namespace Debug {
	extern void Init();
	extern void PatchDatafiles();
	extern bool addBindToggles;
	extern bool LoadLastSave;
	extern bool fixFrametime;
	extern CMultiPatch CMPatches_DisableCheatFlag;
}