#include <cstdint>
#include <ctime>
#include "FileLogger.h"
#include "RPCHandler.h"
#include "Patcher/patch.h"
#include <codecvt>
#include "GameConfig.h"
#pragma comment (lib, "../Discord/discord_game_sdk.dll.lib")


namespace RPCHandler {
	struct Application {
		struct IDiscordCore* core;
		struct IDiscordUserManager* users;
		struct IDiscordAchievementManager* achievements;
		struct IDiscordActivityManager* activities;
		struct IDiscordRelationshipManager* relationships;
		struct IDiscordApplicationManager* application;
		struct IDiscordLobbyManager* lobbies;
		DiscordUserId user_id;
	};
	Application app;
	IDiscordUserEvents users_events;
	IDiscordActivityEvents activities_events;
	IDiscordRelationshipEvents relationships_events;
	DiscordCreateParams params;
	DiscordActivity pres;
	uint8_t Enabled;

	typedef int(__cdecl* hostcheckT)(int);
	hostcheckT hostcheck = (hostcheckT)0x7EE0D0;

	std::string wstring_to_string(const std::wstring& wstr) {
		// Create a wide-to-narrow converter
		std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
		return converter.to_bytes(wstr);
	}


	void DiscordCallbacks()
	{
		app.core->run_callbacks(app.core); // do i need to do this every frame
	}

	bool IsCoopOrSP = false;
	bool ShouldFixStereo = false;
	const char *FancyChunkName[2048];

	int mapListAPI(const char* word) {
		if (strcmp(word, "sr2_mp_lobby") == 0) return 1;
		if (strcmp(word, "sr2_mp_gb_frat01") == 0) return 2;
		if (strcmp(word, "sr2_mp_gb_yacht1") == 0) return 3;
		if (strcmp(word, "sr2_mp_gb_dome01") == 0) return 4;
		if (strcmp(word, "sr2_mp_gb_ultorB") == 0) return 5;
		if (strcmp(word, "sr2_mp_gb_hangar") == 0) return 6;
		if (strcmp(word, "sr2_mp_gb_ct") == 0) return 7;
		if (strcmp(word, "sr2_mp_gb_pyramid") == 0) return 8;
		if (strcmp(word, "sr2_mp_gb_ug_mall") == 0) return 9;
		if (strcmp(word, "sr2_mp_gb_st") == 0) return 10;
		if (strcmp(word, "sr2_mp_gb_sci_mu") == 0) return 11;
		if (strcmp(word, "sr2_mp_gb_kanto1") == 0) return 12;
		if (strcmp(word, "sr2_mp_gb_wareh1") == 0) return 13;
		if (strcmp(word, "sr2_mp_sa_ct") == 0) return 14;
		if (strcmp(word, "sr2_mp_sa_marina") == 0) return 15;
		if (strcmp(word, "sr2_mp_sa_nuke") == 0) return 16;
		if (strcmp(word, "sr2_mp_sa_ba2") == 0) return 17;
		if (strcmp(word, "sr2_mp_sa_museum2") == 0) return 18;
		if (strcmp(word, "sr2_mp_sa_casino") == 0) return 19;
		if (strcmp(word, "sr2_mp_sa_docks") == 0) return 20;
		if (strcmp(word, "sr2_mp_gb_demo01") == 0) return 21;
		if (strcmp(word, "sr2_mp_lobby02") == 0) return 22;
		if (strcmp(word, "sr2_mp_lobby03") == 0) return 23;
		if (strcmp(word, "sr2_mp_lobby04") == 0) return 24;
		if (strcmp(word, "sr2_mp_lobbytut") == 0) return 25;
		if (strcmp(word, "sr2_mp_gb_court1") == 0) return 26;
		return 0;
	}

	void MapDescAPI() {
		char* chunkID = (CHAR*)0x00E99740;
		int code = mapListAPI(chunkID);

		switch (code) {
		case 1:
			*FancyChunkName = "SR1 Lobby";
			break;
		case 2:
			*FancyChunkName = "Frat House Lobby";
			break;
		case 3:
			*FancyChunkName = "Ultor Yacht";
			break;
		case 4:
			*FancyChunkName = "Ultor Dome";
			break;
		case 5:
			*FancyChunkName = "Ultor Ball Room";
			break;
		case 6:
			*FancyChunkName = "The Hangar";
			break;
		case 7:
			*FancyChunkName = "Little Shanghai";
			break;
		case 8:
			*FancyChunkName = "The Pyramid";
			break;
		case 9:
			*FancyChunkName = "Rounds Square Mall";
			break;
		case 10:
			*FancyChunkName = "Shantytown";
			break;
		case 11:
			*FancyChunkName = "Stilwater Science";
			break;
		case 12:
			*FancyChunkName = "Kanto";
			break;
		case 13:
			*FancyChunkName = "Warehouse";
			break;
		case 14:
			*FancyChunkName = "Little Shanghai";
			break;
		case 15:
			*FancyChunkName = "North Shore";
			break;
		case 16:
			*FancyChunkName = "Nuke Plant";
			break;
		case 17:
			*FancyChunkName = "Oldtown";
			break;
		case 18:
			*FancyChunkName = "Partheon";
			break;
		case 19:
			*FancyChunkName = "Poseidon's Palace";
			break;
		case 20:
			*FancyChunkName = "Wharfside";
			break;
		case 21:
			*FancyChunkName = "Demolition Derby";
			break;
		case 22:
			*FancyChunkName = "Purgatory Crib Lobby";
			break;
		case 23:
			*FancyChunkName = "Gymnasium Lobby";
			break;
		case 24:
			*FancyChunkName = "Hangar Lobby";
			break;
		case 25:
			*FancyChunkName = "Tutorial Lobby";
			break;
		case 26:
			*FancyChunkName = "Courthouse";
			break;
		default:
			*FancyChunkName = chunkID;
			break;
		}
	}

	char* ClanTag[3] = {
		const_cast<char*>("["),
		const_cast<char*>("TEST"),
		const_cast<char*>("]")
	};

	char* lobby_list[2] = {
		const_cast<char*>("sr2_mp_gb_frat01"),
		const_cast<char*>("sr2_mp_lobby")
	};

	bool AlreadyAddedClanTag = 0;
	int isDefaultSNameChecked = 0;
	bool UsingClanTag = 0;

	// Updates state info for discord.
	void UpdateDiscordParams() { 
		BYTE CurrentGamemode = *(BYTE*)0x00E8B210; // Parses the current gamemode from EXE
		BYTE LobbyCheck = *(BYTE*)0x02528C14; // Checks lobby, technically this is another gamemode check but we'll use it for lobby
		BYTE MatchType = *(BYTE*)0x00E8B20C; // Checks match type
		char* playerName = (CHAR*)0x0212AB48; // parses player name
		wchar_t* partnerName = (WCHAR*)0x02CD1870; // parses Co-op Partner name, usually
		BYTE AbleToStartGame = *(BYTE*)0x02528D90; // Determines whether the gamemode is able to start or not (we'll force this on when we can, nice QOL feature.)
		BYTE IsInCutscene = *(BYTE*)0x02527D14; // Checks if user is in a cutscene.
		BYTE GamespyStatus = *(BYTE*)0x02529334; // Checks the current gamespy status.

		std::wstring wPartnerName = partnerName; // parse co-op partner name to a wstring
		std::string f_PartnerName = wstring_to_string(wPartnerName); // THEN to a string
		const char* COOPPartner = f_PartnerName.c_str(); // now to a const char because discord is a picky bitch
		char finalUsername[2048];
		char finalMPDesc[2048];
		char finalCOOPDescCutsc[2048];
		char finalCOOPDesc[2048];

	    sprintf(finalUsername, "Username: %s", playerName);
		sprintf(finalMPDesc, "Username: %s | In Map: %s", playerName, *FancyChunkName);
		sprintf(finalCOOPDescCutsc, "Watching a Cutscene with %s", COOPPartner);
		sprintf(finalCOOPDesc, "Playing CO-OP with %s", COOPPartner);

		static DWORD lastTick = 0;

		DWORD currentTick = GetTickCount();
		if (currentTick - lastTick >= 600) {
			lastTick = currentTick;
			if (!LobbyCheck == 0x0 && CurrentGamemode == 0xFF) // This should be CO-OP / Singleplayer
			{
				if (IsInCutscene == 1) 
				{
					if (ShouldFixStereo == true) {
						patchNop((BYTE*)0x00482658, 5); // nop mono aud
						patchNop((BYTE*)0x0046CAC8, 5); // nop ambient aud
					}

					if (f_PartnerName == playerName || f_PartnerName.empty()) {
						strcpy_s(pres.details, "Watching a Cutscene");
					}
					else 
					{
						strcpy_s(pres.details, finalCOOPDescCutsc);
						strcpy_s(pres.state, finalUsername);
					}
					IsCoopOrSP = true;
				}
				else 
				{
					if (ShouldFixStereo == true) {
						patchBytesM((BYTE*)0x00482658, (BYTE*)"\xE8\x43\xFD\xFF\xFF", 5); // patch mono aud back in
						patchBytesM((BYTE*)0x0046CAC8, (BYTE*)"\xE8\x83\xA7\x00\x00", 5); // patch ambient aud back in
					}

					if (f_PartnerName == playerName || f_PartnerName.empty()) {
						strcpy_s(pres.details, "Playing Story Mode");
					}
					else
					{
						strcpy_s(pres.details, finalCOOPDesc);
						strcpy_s(pres.state, finalUsername);
					}
					IsCoopOrSP = true;
				}
			}
			else 
			{
				IsCoopOrSP = false;
			}
			if (CurrentGamemode == 0xD) // Strong Arm
			{
				MapDescAPI();
				if (MatchType == (BYTE)1)
					strcpy_s(pres.details, "Playing MP in Strong Arm");

				if (MatchType == (BYTE)2)
					strcpy_s(pres.details, "Playing Ranked MP in Strong Arm");

				if (MatchType == (BYTE)3)
					strcpy_s(pres.details, "Playing Party MP in Strong Arm");

				strcpy_s(pres.state, finalMPDesc);
			}
			if (CurrentGamemode == 0xB) // Gangsta Brawl
			{
				MapDescAPI();
				if (MatchType == (BYTE)1)
					strcpy_s(pres.details, "Playing MP in Gangsta Brawl");

				if (MatchType == (BYTE)2)
					strcpy_s(pres.details, "Playing Ranked MP in Gangsta Brawl");

				if (MatchType == (BYTE)3)
					strcpy_s(pres.details, "Playing Party MP in Gangsta Brawl");

				strcpy_s(pres.state, finalMPDesc);
			}
			if (CurrentGamemode == 0xC) // Team Gangsta Brawl
			{
				MapDescAPI();
				if (MatchType == (BYTE)1)
					strcpy_s(pres.details, "Playing MP in Team Gangsta Brawl");

				if (MatchType == (BYTE)2)
					strcpy_s(pres.details, "Playing Ranked MP in Team Gangsta Brawl");

				if (MatchType == (BYTE)3)
					strcpy_s(pres.details, "Playing Party MP in Team Gangsta Brawl");

				strcpy_s(pres.state, finalMPDesc);
			}
			if (LobbyCheck == 0x44) // Game Lobby
			{
				if (MatchType == (BYTE)2) { // If in ranked
					if (!CurrentGamemode == 0xD || !CurrentGamemode == 0xC || CurrentGamemode == 0xB) // And gamemode is not TGB or Strong Arm but is Gangsta Brawl
					{
						AbleToStartGame = 1; // Force Able to Start
					}

				}
				else 
				{
					AbleToStartGame = 1;
				}

				// Player MP Lobby shows as default for everyone, Ranked and Party seem to only show in rich presence if host
				// so we'll keep the conditions because we need it for friendly fire shit anyway.
				if (MatchType == (BYTE)1) // Player Lobby
					strcpy_s(pres.details, "Waiting in MP Lobby...");

				if (MatchType == (BYTE)2) // Ranked Lobby
				{
					strcpy_s(pres.details, "Waiting in Ranked MP Lobby...");
                    #if RELOADED
					    *(BYTE*)0x02A4D134 = 0x1; // Force Friendly Fire to Full Damage.
                    #endif
				}

				if (MatchType == (BYTE)3) // Party Lobby
				    strcpy_s(pres.details, "Waiting in Party MP Lobby...");
			}
			if (LobbyCheck == 0x0) // Usually Menus Check
			{
				AbleToStartGame = 0; // Reset Able to Start to 0 in Main Menu
                #if RELOADED
				    *(BYTE*)0x02A4D134 = 0x0; // Force Friendly Fire to Off.
					if (UsingClanTag == 1)
					{
						char* currentPlayerName = playerName;
						std::string Clanresult = ClanTag[0];
						Clanresult = Clanresult + ClanTag[1] + ClanTag[2] + " " + currentPlayerName;
						const char* finalClanstring = Clanresult.c_str();

						if (GamespyStatus == 0x4) {
							if (AlreadyAddedClanTag == 0) {
								char* newPlayerName = reinterpret_cast<char*>(playerName);
								strcpy(newPlayerName, (const char*)finalClanstring);
								AlreadyAddedClanTag = 1;
							}
						}
					}
                #endif
				strcpy_s(pres.details, "In Menus...");
				strcpy_s(pres.state, "");
			}

			*(BYTE*)0x02528D90 = AbleToStartGame;

			app.activities->update_activity(app.activities, &pres, 0, 0);
		}

	}

	void UpdateNoDiscParams() { // Updates state info for non discord users.
		BYTE CurrentGamemode = *(BYTE*)0x00E8B210; // Parses the current gamemode from EXE
		BYTE LobbyCheck = *(BYTE*)0x02528C14; // Checks lobby, technically this is another gamemode check but we'll use it for lobby
		BYTE MatchType = *(BYTE*)0x00E8B20C; // Checks match type
		BYTE AbleToStartGame = *(BYTE*)0x02528D90; // Determines whether the gamemode is able to start or not (we'll force this on when we can, nice QOL feature.)
		BYTE IsInCutscene = *(BYTE*)0x02527D14; // Checks if user is in a cutscene.
		char* playerName = (CHAR*)0x0212AB48; // parses player name
		BYTE GamespyStatus = *(BYTE*)0x02529334; // Checks the current gamespy status.

		static DWORD lastTick = 0;

		DWORD currentTick = GetTickCount();

		if (currentTick - lastTick >= 600) {
			lastTick = currentTick;
			if (LobbyCheck == 0x44) // Game Lobby
			{
				if (MatchType == (BYTE)2) { // If in ranked
                    #if RELOADED
					    *(BYTE*)0x02A4D134 = 0x1; // Force Friendly Fire to Full Damage.
                    #endif
					if (!CurrentGamemode == 0xD || !CurrentGamemode == 0xC || CurrentGamemode == 0xB) // And gamemode is not TGB or Strong Arm but is Gangsta Brawl
					{
						AbleToStartGame = 1; // Force Able to Start
					}

				}
				else
				{
					AbleToStartGame = 1;
				}
			}
			if (LobbyCheck == 0x0) // Usually Menus Check
			{
                #if RELOADED
				    *(BYTE*)0x02A4D134 = 0x0; // Force Friendly Fire to Off.
					if (UsingClanTag == 1)
					{
						char* currentPlayerName = playerName;
						std::string Clanresult = ClanTag[0];
						Clanresult = Clanresult + ClanTag[1] + ClanTag[2] + " " + currentPlayerName;
						const char* finalClanstring = Clanresult.c_str();

						if (GamespyStatus == 0x4) {
							if (AlreadyAddedClanTag == 0) {
								char* newPlayerName = reinterpret_cast<char*>(playerName);
								strcpy(newPlayerName, (const char*)finalClanstring);
								AlreadyAddedClanTag = 1;
							}
						}
					}
                #endif
				AbleToStartGame = 0; // Reset Able to Start to 0 in Main Menu
			}
			if (!LobbyCheck == 0x0 && CurrentGamemode == 0xFF) // This should be CO-OP / Singleplayer
			{
				IsCoopOrSP = true;
				if (IsInCutscene == 1)
				{
					if (ShouldFixStereo == true) {
						patchNop((BYTE*)0x00482658, 5); // nop mono aud
						patchNop((BYTE*)0x0046CAC8, 5); // nop ambient aud
					}
				}
				else
				{
					if (ShouldFixStereo == true) {
						patchBytesM((BYTE*)0x00482658, (BYTE*)"\xE8\x43\xFD\xFF\xFF", 5); // patch mono aud back in
						patchBytesM((BYTE*)0x0046CAC8, (BYTE*)"\xE8\x83\xA7\x00\x00", 5); // patch ambient aud back in
					}
				}
			}
			else 
			{
				IsCoopOrSP = false;
			}
			*(BYTE*)0x02528D90 = AbleToStartGame;
		}
	}


	void InitRPC()
	{
        #if !RELOADED
		memset(&app, 0, sizeof(Application));
		memset(&users_events, 0, sizeof(users_events));
		memset(&activities_events, 0, sizeof(activities_events));
		memset(&relationships_events, 0, sizeof(relationships_events));

		DiscordCreateParamsSetDefault(&params);
		params.client_id = 1263159254903164948;
		params.flags = DiscordCreateFlags_NoRequireDiscord;
		params.event_data = &app;
		params.activity_events = &activities_events;
		params.relationship_events = &relationships_events;
		params.user_events = &users_events;
		int fail = DiscordCreate(DISCORD_VERSION, &params, &app.core);
		if (fail)
		{
			Logger::TypedLog(CHN_RPC, "Discord RPC Initialization failed !!!!!!!!\n");
			Logger::TypedLog(CHN_RPC, "Error code: %d\n", fail);
			Enabled = 0;
		}
		else 
		{
			Enabled = 1;
			Logger::TypedLog(CHN_RPC, "Discord RPC Initialization Succeeded!\n");
			if (Enabled)
			{
				Logger::TypedLog(CHN_RPC, "Sending Info to Rich Presence.\n");
				app.users = app.core->get_user_manager(app.core);
				app.achievements = app.core->get_achievement_manager(app.core);
				app.activities = app.core->get_activity_manager(app.core);
				app.application = app.core->get_application_manager(app.core);
				app.lobbies = app.core->get_lobby_manager(app.core);

				std::time_t CurrentTime = std::time(0);
				//Initialize a basic Rich Presence.
				pres.timestamps.start = CurrentTime;
				//pres.timestamps.end = 0;
				strcpy_s(pres.assets.large_image, "sr2");
				app.activities->update_activity(app.activities, &pres, 0, 0);
				// ---------------------------------
			}
		}
        #else 
		memset(&app, 0, sizeof(Application));
		memset(&users_events, 0, sizeof(users_events));
		memset(&activities_events, 0, sizeof(activities_events));
		memset(&relationships_events, 0, sizeof(relationships_events));

		DiscordCreateParamsSetDefault(&params);
		params.client_id = 1257751836375519332;
		params.flags = DiscordCreateFlags_NoRequireDiscord;
		params.event_data = &app;
		params.activity_events = &activities_events;
		params.relationship_events = &relationships_events;
		params.user_events = &users_events;
		//DiscordCreate(DISCORD_VERSION, &params, &app.core);
		int fail = DiscordCreate(DISCORD_VERSION, &params, &app.core);
		if (fail)
		{
			Logger::TypedLog(CHN_RPC, "Discord RPC Initialization failed !!!!!!!!\n");
			Logger::TypedLog(CHN_RPC, "Error code: %d\n", fail);
			Enabled = 0;
		}
		else
		{
			Enabled = 1;
			Logger::TypedLog(CHN_RPC, "Discord RPC Initialization Succeeded!\n");
			if (Enabled)
			{
				Logger::TypedLog(CHN_RPC, "Sending Info to Rich Presence.\n");
				app.users = app.core->get_user_manager(app.core);
				app.achievements = app.core->get_achievement_manager(app.core);
				app.activities = app.core->get_activity_manager(app.core);
				app.application = app.core->get_application_manager(app.core);
				app.lobbies = app.core->get_lobby_manager(app.core);

				std::time_t CurrentTime = std::time(0);
				//Initialize a basic Rich Presence.
				pres.timestamps.start = CurrentTime;
				strcpy_s(pres.assets.large_image, "sr2mprr");
				app.activities->update_activity(app.activities, &pres, 0, 0);
				// ---------------------------------
			}
		}
        #endif
	}

	void ChangeDetails(const char* newdetails) 
	{
		strcpy_s(pres.details, newdetails);
		app.activities->update_activity(app.activities, &pres, 0, 0);
	}

	void ShutdownRPC()
	{
		app.activities->clear_activity(app.activities, 0, 0);
		// does it immediately upon exiting game sometimes
		// (unless that's because i don't have the game
		// manually added in app settings)
	}
}
