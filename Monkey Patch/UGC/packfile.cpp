#include "packfile.h"

// packfile.cpp (uzis)
// --------------------
// Created: 8/06/2025

#include "../FileLogger.h"
#include "../Patcher/patch.h"
#include "../GameConfig.h"
#include "../RPCHandler.h"

namespace packfile {

	char* vpp_list[] = { // Include original packfiles and queue new ones in after patch.vpp
		const_cast<char*>("anims.vpp"),
		const_cast<char*>("audio.vpp"),
		const_cast<char*>("chunks1.vpp"),
		const_cast<char*>("chunks2.vpp"),
		const_cast<char*>("chunks3.vpp"),
		const_cast<char*>("chunks4.vpp"),
		const_cast<char*>("common.vpp"),
		const_cast<char*>("cutscenes.vpp"),
		const_cast<char*>("meshes.vpp"),
		const_cast<char*>("music1.vpp"),
		const_cast<char*>("music2.vpp"),
		const_cast<char*>("music3.vpp"),
		const_cast<char*>("music4.vpp"),
		const_cast<char*>("pegs.vpp"),
		const_cast<char*>("textures.vpp"),
		const_cast<char*>("patch.vpp"),
		const_cast<char*>("tharow_ui.vpp"),
		const_cast<char*>("tharow_maps.vpp"),
		const_cast<char*>("tharow_weapons.vpp"),
		const_cast<char*>("tharow_tweaks.vpp"),
		const_cast<char*>("tharow_misc.vpp")
	};

	void PatchThaRowPackfiles() {
		patchDWord((void*)(0x0051DAD0 + 2), (int)&vpp_list); // patch the new list into the startup function.
		patchByte((void*)(0x0051DB36 + 2), sizeof(vpp_list)); // patch number of VPPs it searches for.
	}
}
