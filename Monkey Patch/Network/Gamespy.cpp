// Gamespy.cpp (uzis, Tervel)
// --------------------
// Created: 25/02/2025

#include "../FileLogger.h"
#include "../Patcher/patch.h"
#include "../GameConfig.h"
#include "../SafeWrite.h"
#include "Gamespy.h"
#include "../UtilsGlobal.h"
#include "STUNNat.h"
#include <Wininet.h>
#include "safetyhook.hpp"
#include "MPStorage.h"
#pragma comment(lib, "wininet.lib")

enum E_NAME
{
	ELAN,
	EGAMESPY
};

E_NAME& name_to_pick = *(E_NAME*)0x02528BC4;

namespace Gamespy
{
	bool IsInternetAvailable()
	{
		DWORD flags;
		return InternetGetConnectedState(&flags, 0);
	}

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

	char* lobby_list[2] = {
		const_cast<char*>("sr2_mp_lobby02"),
		const_cast<char*>("sr2_mp_lobby03")
	};

	uintptr_t get_pc_session()
	{
		return *(uintptr_t*)0x02528C14;
	}

	__declspec(noinline) uintptr_t get_extra_info(uintptr_t session)
	{
		return *(uintptr_t*)(session + 0xC);
	}

	bool isSessionCoop(uintptr_t session)
	{
		auto extra_info = get_extra_info(session);
		return (*(uint32_t*)(extra_info + 0x38)) == 2;
	}

	__declspec(noinline) bool get_is_syslink(uintptr_t session = NULL)
	{
		if(!session)
		session = get_pc_session();

		if (session) {
			auto extra_info = get_extra_info(session);
			if (extra_info) {
				return *(bool*)(extra_info + 0x47);
			}
		}
		return false;
	}

	void SetNameToLAN(SafetyHookContext& ctx)
	{
		auto is_lan = get_is_syslink();
		if(get_is_syslink())
		name_to_pick = ELAN;
	}
	int ip_sprintf_s_parsing(char* buffer, const size_t BufferCount, const char* format, uint8_t ip1, uint8_t ip2, uint8_t ip3, uint8_t ip4)
	{
		// we want it empty
		return sprintf_s(buffer, BufferCount, " ");
	}
	void Init() {
		coopPausePatch = 0;
		MPStorage::Init();

		// Obfuscate IP display in server browser 
		InjectHook(0x8360F8, ip_sprintf_s_parsing);

		// Display nat type
		if (IsInternetAvailable()) {
			// Check basic NAT type on port 4200
			std::string publicIp;
			int publicPort = 0;
			int localPort = 4200;

			std::string natType = detectNatType(localPort, publicIp, publicPort);
			UtilsGlobal::mynat_type = natType;
			Logger::TypedLog(CHN_NET, "{}\n", UtilsGlobal::mynat_type.c_str());
			// ----
		}

	if (GameConfig::GetValue("Multiplayer", "FixNetworkBinding", 1, "Fixes the multiple network adapter issue which makes it so you can't use online functionalities if you had more than 1 network adapter."))
		{
			Logger::TypedLog(CHN_NET, "Fixing Network Adapter Binding...\n");
			patchNetworkBind();
		}
#if !JLITE
	if (GameConfig::GetValue("Multiplayer", "NewLobbyList", 1, "Changes the lobby maps."))
		{
			static char newLobby1[MAX_PATH];
			static char newLobby2[MAX_PATH];

			Logger::TypedLog(CHN_DEBUG, "Changing Lobby List...\n");

	GameConfig::GetStringValue("Multiplayer", "Lobby1", "sr2_mp_lobby02", newLobby1);
	GameConfig::GetStringValue("Multiplayer", "Lobby2", "sr2_mp_lobby03", newLobby2);

			Logger::TypedLog(CHN_DEBUG, "Lobby Map 1 Found: {}\n", newLobby1);
			Logger::TypedLog(CHN_DEBUG, "Lobby Map 2 Found: {}\n", newLobby2);
			lobby_list[0] = newLobby1;
			lobby_list[1] = newLobby2;

			patchDWord((void*)(0x0073EABA + 3), (int)&lobby_list);
			patchDWord((void*)(0x0073EA0B + 3), (int)&lobby_list);
			patchDWord((void*)(0x007E131A + 3), (int)&lobby_list);
			patchDWord((void*)(0x007E161E + 3), (int)&lobby_list);
			patchDWord((void*)(0x007E7670 + 3), (int)&lobby_list);
			patchDWord((void*)(0x007E774F + 3), (int)&lobby_list);
			patchDWord((void*)(0x0082F2E9 + 3), (int)&lobby_list);
			patchDWord((void*)(0x0082F4CC + 3), (int)&lobby_list);
			patchDWord((void*)(0x00842497 + 3), (int)&lobby_list);
		}

		if (*(int*)0x819570 == 0x0104EC81)
			patchJmp((void*)0x819570, &UtilsGlobal::RetZero);

	if (GameConfig::GetValue("Multiplayer", "FreeMPClothing", 0, "Makes all clothing free in Multiplayer."))
		{
			Logger::TypedLog(CHN_DEBUG, "Making MP Clothing Free...\n");
			patchNop((BYTE*)0x0088DDBD, 5); // nop the float call from xtbl > exe that parses clothing prices.
		}

		// Disabled by default FOR NOW, too many issues arise when dealing with missions. Otherwise it works beautifully in freeroam.
		//if (GameConfig::GetValue("Gameplay", "coopPausePatch", 0)) // Tervel W streak
		//{
		//	Logger::TypedLog(CHN_DEBUG, "Disabling CO-OP pause...\n");
		//	coopPausePatch = 1;
		//	patchNop((BYTE*)0x00779C5E, 5); // Prevent the game from pausing your co-op partner if you open the pause menu.
		//	patchBytesM((BYTE*)0x0068CA79, (BYTE*)"\x83\x3D\xF6\x2C\x7B\x02\x00", 7); // new pause check address
		//	patchBytesM((BYTE*)0x00BF0A1B, (BYTE*)"\x83\x3D\xF6\x2C\x7B\x02\x00", 7); // new particle pause check address
		//	patchBytesM((BYTE*)0x00BDCFFD, (BYTE*)"\x83\x3D\xF6\x2C\x7B\x02\x00", 7);  // new particle pause check address 2
		//	patchBytesM((BYTE*)0x006B793F, (BYTE*)"\x83\x3D\xF6\x2C\x7B\x02\x00", 7);  // new particle pause check address 3
		//}
#endif

	// Fixes an issue where the game would use your gamespy name instead of the LAN one if you are logged into gamespy (clippy95)
	static auto UseLAN_NameInSysLink_Load = safetyhook::create_mid(0x778F05, SetNameToLAN);

	static auto multi_session_create_midhook = safetyhook::create_mid(0x80DB1E, [](SafetyHookContext& ctx) {
		if(ctx.ebx && isSessionCoop(ctx.ebx))
		name_to_pick = (E_NAME)!get_is_syslink(ctx.ebx);
		});

	}
}
