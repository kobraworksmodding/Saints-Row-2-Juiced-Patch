// Gamespy.cpp (uzis, Tervel)
// --------------------
// Created: 25/02/2025

#include "../FileLogger.h"
#include "../Patcher/patch.h"
#include "../GameConfig.h"
#include "../SafeWrite.h"
#include "../UGC/Reloaded.h"
#include "Gamespy.h"

namespace Gamespy
{
	int(__stdcall* theirbind)(SOCKET, const struct sockaddr_in*, int) = nullptr;

	int __stdcall bindWrapper(SOCKET socket, struct sockaddr_in* address, int namelen) {
		address->sin_addr.S_un.S_addr = INADDR_ANY; // bind to all interfaces instead of just one

		int result = theirbind(socket, address, namelen);

		//Logger::TypedLog(CHN_NET, "Redirecting network bind to %i\n", result);

		return result;
	}

	void patchNetworkBind() { // Binds local ip to any instead of 127.0.0.1 which fucks up if you have more than 1 network adapter.
		uint32_t* bindaddr = reinterpret_cast<uint32_t*>(0x00C1BC76 + 1);

		theirbind = reinterpret_cast<int(_stdcall*)(SOCKET, const struct sockaddr_in*, int)>(reinterpret_cast<intptr_t>(bindaddr) + 4 + *bindaddr);

		patchCall((BYTE*)0x00C1BC76, bindWrapper); // join
		patchCall((BYTE*)0x00D2526F, bindWrapper); // host
		//patchCall((BYTE*)0x0090A779, bindWrapper);
		//patchCall((BYTE*)0x008E7681, bindWrapper);
	}

	void Init() {
		coopPausePatch = 0;
		if (GameConfig::GetValue("Multiplayer", "FixNetworkBinding", 1))
		{
			Logger::TypedLog(CHN_NET, "Fixing Network Adapter Binding...\n");
			patchNetworkBind();
		}
#if !JLITE
		if (GameConfig::GetValue("Multiplayer", "NewLobbyList", 1))
		{
			char newLobby1[MAX_PATH];
			char newLobby2[MAX_PATH];

			Logger::TypedLog(CHN_DEBUG, "Changing Lobby List...\n");

			GameConfig::GetStringValue("Multiplayer", "Lobby1", "sr2_mp_lobby03", newLobby1);
			GameConfig::GetStringValue("Multiplayer", "Lobby2", "sr2_mp_lobby02", newLobby2);

			Logger::TypedLog(CHN_DEBUG, "Lobby Map 1 Found: %s\n", newLobby1);
			Logger::TypedLog(CHN_DEBUG, "Lobby Map 2 Found: %s\n", newLobby2);
			Reloaded::lobby_list[0] = newLobby1;
			Reloaded::lobby_list[1] = newLobby2;

			patchDWord((void*)(0x0073EABA + 3), (int)&Reloaded::lobby_list);
			patchDWord((void*)(0x0073EA0B + 3), (int)&Reloaded::lobby_list);
			patchDWord((void*)(0x007E131A + 3), (int)&Reloaded::lobby_list);
			patchDWord((void*)(0x007E161E + 3), (int)&Reloaded::lobby_list);
			patchDWord((void*)(0x007E7670 + 3), (int)&Reloaded::lobby_list);
			patchDWord((void*)(0x007E774F + 3), (int)&Reloaded::lobby_list);
			patchDWord((void*)(0x0082F2E9 + 3), (int)&Reloaded::lobby_list);
			patchDWord((void*)(0x0082F4CC + 3), (int)&Reloaded::lobby_list);
			patchDWord((void*)(0x00842497 + 3), (int)&Reloaded::lobby_list);
		}

		if (GameConfig::GetValue("Multiplayer", "FreeMPClothing", 1))
		{
			Logger::TypedLog(CHN_DEBUG, "Making MP Clothing Free...\n");
			patchNop((BYTE*)0x0088DDBD, 5); // nop the float call from xtbl > exe that parses clothing prices.
		}

		// Disabled by default FOR NOW, too many issues arise when dealing with missions. Otherwise it works beautifully in freeroam.
		if (GameConfig::GetValue("Gameplay", "coopPausePatch", 1)) // Tervel W streak
		{
			Logger::TypedLog(CHN_DEBUG, "Disabling CO-OP pause...\n");
			coopPausePatch = 1;
			patchNop((BYTE*)0x00779C5E, 5); // Prevent the game from pausing your co-op partner if you open the pause menu.
			patchBytesM((BYTE*)0x0068CA79, (BYTE*)"\x83\x3D\xF6\x2C\x7B\x02\x00", 7); // new pause check address
			patchBytesM((BYTE*)0x00BF0A1B, (BYTE*)"\x83\x3D\xF6\x2C\x7B\x02\x00", 7); // new particle pause check address
			patchBytesM((BYTE*)0x00BDCFFD, (BYTE*)"\x83\x3D\xF6\x2C\x7B\x02\x00", 7);  // new particle pause check address 2
			patchBytesM((BYTE*)0x006B793F, (BYTE*)"\x83\x3D\xF6\x2C\x7B\x02\x00", 7);  // new particle pause check address 3
		}
#endif
	}
}