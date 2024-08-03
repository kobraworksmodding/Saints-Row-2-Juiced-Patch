#include <cstdint>
#include <ctime>
#include "FileLogger.h"
#include "RPCHandler.h"
#include "Patcher/patch.h"
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



	void DiscordCallbacks()
	{
		app.core->run_callbacks(app.core); // do i need to do this every frame
	}

	bool IsCoopOrSP = false;
	bool ShouldFixStereo = false;

	void UpdateDiscordParams() { // Updates state info for discord.
		BYTE CurrentGamemode = *(BYTE*)0x00E8B210; // Parses the current gamemode from EXE
		BYTE LobbyCheck = *(BYTE*)0x02528C14; // Checks lobby, technically this is another gamemode check but we'll use it for lobby
		BYTE MatchType = *(BYTE*)0x00E8B20C; // Checks match type
		char* playerName = (CHAR*)0x0212AB48; // parses player name
		BYTE AbleToStartGame = *(BYTE*)0x02528D90; // Determines whether the gamemode is able to start or not (we'll force this on when we can, nice QOL feature.)
		BYTE IsInCutscene = *(BYTE*)0x02527D14; // Checks if user is in a cutscene.

		char finalUsername[2048];
		sprintf(finalUsername, "Username: %s", playerName);

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

					strcpy_s(pres.details, "Playing SP / CO-OP | Watching a Cutscene");
					strcpy_s(pres.state, finalUsername);
					IsCoopOrSP = true;
				}
				else 
				{
					if (ShouldFixStereo == true) {
						patchBytesM((BYTE*)0x00482658, (BYTE*)"\xE8\x43\xFD\xFF\xFF", 5); // patch mono aud back in
						patchBytesM((BYTE*)0x0046CAC8, (BYTE*)"\xE8\x83\xA7\x00\x00", 5); // patch ambient aud back in
					}
					strcpy_s(pres.details, "Playing SP / CO-OP");
					strcpy_s(pres.state, finalUsername);
					IsCoopOrSP = true;
				}
			}
			else 
			{
				IsCoopOrSP = false;
			}
			if (CurrentGamemode == 0xD) // Strong Arm
			{
				if (MatchType == (BYTE)1)
					strcpy_s(pres.details, "Playing Player MP in Strong Arm");

				if (MatchType == (BYTE)2)
					strcpy_s(pres.details, "Playing Ranked MP in Strong Arm");

				if (MatchType == (BYTE)3)
					strcpy_s(pres.details, "Playing Party MP in Strong Arm");

				strcpy_s(pres.state, finalUsername);
			}
			if (CurrentGamemode == 0xB) // Gangsta Brawl
			{
				if (MatchType == (BYTE)1)
					strcpy_s(pres.details, "Playing Player MP in Gangsta Brawl");

				if (MatchType == (BYTE)2)
					strcpy_s(pres.details, "Playing Ranked MP in Gangsta Brawl");

				if (MatchType == (BYTE)3)
					strcpy_s(pres.details, "Playing Party MP in Gangsta Brawl");

				strcpy_s(pres.state, finalUsername);
			}
			if (CurrentGamemode == 0xC) // Team Gangsta Brawl
			{
				if (MatchType == (BYTE)1)
					strcpy_s(pres.details, "Playing Player MP in Team Gangsta Brawl");

				if (MatchType == (BYTE)2)
					strcpy_s(pres.details, "Playing Ranked MP in Team Gangsta Brawl");

				if (MatchType == (BYTE)3)
					strcpy_s(pres.details, "Playing Party MP in Team Gangsta Brawl");

				strcpy_s(pres.state, finalUsername);
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

				if (MatchType == (BYTE)1) // Player Lobby
					strcpy_s(pres.details, "Waiting in Player MP Lobby...");

				if (MatchType == (BYTE)2) // Ranked Lobby
					strcpy_s(pres.details, "Waiting in Ranked MP Lobby...");

				if (MatchType == (BYTE)3) // Party Lobby
				    strcpy_s(pres.details, "Waiting in Party MP Lobby...");
			}
			if (LobbyCheck == 0x0) // Usually Menus Check
			{
				AbleToStartGame = 0; // Reset Able to Start to 0 in Main Menu
				strcpy_s(pres.details, "In Menus...");
				strcpy_s(pres.state, "");
			}

			*(BYTE*)0x02528D90 = AbleToStartGame;

			app.activities->update_activity(app.activities, &pres, 0, 0);

			//Logger::TypedLog(CHN_DEBUG, "CurrentGamemode: %x, LobbyCheck: %x, PlayerName: %s, AbleToStartGame: %x\n", CurrentGamemode, LobbyCheck, playerName, AbleToStartGame);
		}

	}

	void UpdateNoDiscParams() { // Updates state info for discord.
		BYTE CurrentGamemode = *(BYTE*)0x00E8B210; // Parses the current gamemode from EXE
		BYTE LobbyCheck = *(BYTE*)0x02528C14; // Checks lobby, technically this is another gamemode check but we'll use it for lobby
		BYTE MatchType = *(BYTE*)0x00E8B20C; // Checks match type
		BYTE AbleToStartGame = *(BYTE*)0x02528D90; // Determines whether the gamemode is able to start or not (we'll force this on when we can, nice QOL feature.)
		BYTE IsInCutscene = *(BYTE*)0x02527D14; // Checks if user is in a cutscene.


		static DWORD lastTick = 0;

		DWORD currentTick = GetTickCount();

		if (currentTick - lastTick >= 600) {
			lastTick = currentTick;
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
			}
			if (LobbyCheck == 0x0) // Usually Menus Check
			{
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
				//pres.timestamps.end = 0;
				//strcpy_s(pres.details, "Reclaiming Stilwater...");
				strcpy_s(pres.assets.large_image, "sr2");
				app.activities->update_activity(app.activities, &pres, 0, 0);
				//ChangeDetails("test");
				// ---------------------------------
			}
		}
		// trash app

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

	void AddFunctions()
	{
		if (Enabled)
		{
			//CFuncs::RedirectFunction("SetRichPresenceMode", (void*)CFunc_SetRichPresenceMode);
		}
	}
}
