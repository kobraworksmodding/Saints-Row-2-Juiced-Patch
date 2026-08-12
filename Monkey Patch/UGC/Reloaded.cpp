// Reloaded.cpp (uzis)
// --------------------
// Created: 13/12/2024

#include "../FileLogger.h"
#include "../Patcher/patch.h"
#include "../GameConfig.h"
#include "Reloaded.h"
#include "../RPCHandler.h"
#include "../Game/Game.h"
#include "AT.h"


namespace Reloaded
{

	void PatchTables() 
	{
		patchBytesM((BYTE*)0x00E06CC4, (BYTE*)"\x72\x65\x65\x6C", 4); // reeload.tbl
		patchBytesM((BYTE*)0x00E06CD0, (BYTE*)"\x72\x65\x65\x6C", 4); // reeload_anims.tbl
		AT::patchCalls();
	}

	void PatchKillfeed() 
	{
		Logger::TypedLog(CHN_RL, "Making Killfeed Better...\n");
		patchBytesM((BYTE*)0x0087F096, (BYTE*)"\x8B\x3D\x10\xD2\xEA\x00", 6);
	}


	// Vanilla = 0xC9 // 201
	// thaRow first ver = 0xA1 // 161

	void Init()
	{
		patchByte((BYTE*)0x00882CDA, 0x74); // Fixes the Kick Menu for host in MP.

		// Patch version number to prevent vanilla players from seeing reloaded lobbies.

		// TODO: FIX THIS SHIT, WHY IS IT NOT WORKING ANYMORE??!?!?!

		/*patchByte((BYTE*)0x008FCB98, 0xA1);
		patchByte((BYTE*)0x00812BDC, 0xA1);
		patchByte((BYTE*)0x008D01F7, 0xA1);
		patchByte((BYTE*)0x00812CB6, 0xA1);
		patchByte((BYTE*)0x00812D68, 0xA1);
		patchByte((BYTE*)0x00815D42, 0xA1);
		patchByte((BYTE*)0x00816F9B, 0xA1);
		patchByte((BYTE*)0x00812573, 0xA1);
		patchByte((BYTE*)0x008159CE, 0xA1);*/

		// Unlock SA Spray Cam
		patchNop((BYTE*)0x0089F895, 7);
		patchNop((BYTE*)0x0089F89C, 7);

		PatchTables();
		if (GameConfig::GetValue("Multiplayer", "BetterKillfeed", 1))
		{
			PatchKillfeed();
		}

		// -- TODO: Find more cleaner way to patch name for Clan Tag, still saves to user account.
		// Adds Clan tag to name
		if (GameConfig::GetValue("Multiplayer", "UseClanTag", 0))
		{
			// we use our own CLANTAG_
			//variable to max out the limit of the string to 5.
			static char ClanName[CLANTAG_MAX];
			Logger::TypedLog(CHN_RL, "Adding Clan to Name...\n");

			GameConfig::GetStringValue("Multiplayer", "ClanTag", "SR2RL", ClanName);

			std::string MyMaxClanTag(ClanName, ClanName + 5);
			static char EndClanName[CLANTAG_MAX];
			strcpy(EndClanName, MyMaxClanTag.c_str());

			Game::InLoop::ClanTag[1] = EndClanName;
			Logger::TypedLog(CHN_RL, "You Joined Clan: %s\n", Game::InLoop::ClanTag[1]);
			Game::InLoop::UsingClanTag = 1;

		}
	}
}
