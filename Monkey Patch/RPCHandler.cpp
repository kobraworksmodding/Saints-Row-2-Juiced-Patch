#if !JLITE
#include <cstdint>
#include <ctime>
#include "FileLogger.h"
#include "RPCHandler.h"
#include "Patcher/patch.h"
#include <codecvt>
#include "GameConfig.h"
#include "UtilsGlobal.h"
#include "Game/Game.h"
#include "Hooker.h"
//#pragma comment (lib, "../Discord/discord_game_sdk.dll.lib")
#define DISCORD_LIB_NAME "discord_game_sdk.dll"

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

	std::string wstring_to_string(const std::wstring& wstr) {
		// Create a wide-to-narrow converter
		std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
		return converter.to_bytes(wstr);
	}


	void DiscordCallbacks()
	{
		app.core->run_callbacks(app.core); // do i need to do this every frame
	}

	const char* FancyChunkName[2048];

	//sr2_mp_custBG
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
		if (strcmp(word, "sr2_mp_ptp_pris") == 0) return 27;
		if (strcmp(word, "sr2_mp_gb_reclob") == 0) return 28;
		if (strcmp(word, "sr2_mp_gb_prIL01") == 0) return 29;
		if (strcmp(word, "sr2_mp_gb_traprk") == 0) return 30;
		if (strcmp(word, "sr2_codeblue") == 0) return 31;
		if (strcmp(word, "sr2_mp_gb_hotwrd") == 0) return 32;
		if (strcmp(word, "sr2_mp_gb_hosp01") == 0) return 33;
		if (strcmp(word, "sr2_mp_gb_prisis") == 0) return 34;
		if (strcmp(word, "sr2_mp_gb_prison") == 0) return 35;
		if (strcmp(word, "sr2_mp_gb_pornpl") == 0) return 36;
		if (strcmp(word, "sr2_mp_gb_broth1") == 0) return 37;
		if (strcmp(word, "sr2_mp_lobby05") == 0) return 38;
		return 0;
	}

	void MapDescAPI() {
		char* chunkID = (CHAR*)DynAddress(0x00E99740);
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
		case 27:
			*FancyChunkName = "Prison Island (PTP Blitz)";
			break;
		case 28:
			*FancyChunkName = "Rec Center";
			break;
		case 29:
			*FancyChunkName = "Prison Island";
			break;
		case 30:
			*FancyChunkName = "Trailer Park";
			break;
		case 31:
			*FancyChunkName = "Code Blue 1v1";
			break;
		case 32:
			*FancyChunkName = "Hotwired";
			break;
		case 33:
			*FancyChunkName = "Hospital";
			break;
		case 34:
			*FancyChunkName = "Prison Island";
			break;
		case 35:
			*FancyChunkName = "Cellblock";
			break;
		case 36:
			*FancyChunkName = "Porno Palace";
			break;
		case 37:
			*FancyChunkName = "Brotherhood HQ";
			break;
		case 38:
			*FancyChunkName = "Warehouse Lobby";
			break;
		default:
			*FancyChunkName = chunkID;
			break;
		}
	}

	std::string wchar_to_utf8(const std::wstring& wstr) {
		std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
		return converter.to_bytes(wstr);
	}

	enum gang
	{
		saints = 0x0,
		brotherhood = 0x1,
		ronin = 0x2,
		samedi = 0x3,
		ultor = 0x6,
	};

	enum activities
	{
		none = -1,
		demoderby = 0x0,
		demoderby2 = 0x1,
		escort = 0x2,
		fightclub = 0x3,
		fraud = 0x4,
		fuzz = 0x5,
		trailblaze = 0x7,
		snatch = 0x8,
		trafficking = 0xA,
		crowdcontrol = 0xB,
		septicavenger = 0xC,
		zombieuprising = 0xD,
		racing = 0xE,
		heliassault = 0xF,
		mayhem = 0x10,
	};

	typedef activities Activity_TypeT();
	Activity_TypeT* Activity_Type = (Activity_TypeT*)DynAddress(0x53C4D0);

	std::unordered_map<gang, const char*> gangToString = {
	{saints, "tss"},
	{brotherhood, "brotherhood"},
	{ronin, "ronin"},
	{samedi, "samedi"},
	{ultor, "ultor"}
	};

	std::unordered_map<activities, const char*> activitiesToString = {
	{none, ""},
	{demoderby, "Demo Derby"},
	{demoderby2, "Demo Derby"},
	{escort, "Escort"},
	{fightclub, "Fight Club"},
	{fraud, "Insurance Fraud"},
	{fuzz, "Fuzz"},
	{trailblaze, "Trail Blazing"},
	{snatch, "Snatch"},
	{trafficking, "Drug Trafficking"},
	{crowdcontrol, "Crowd Control"},
	{septicavenger, "Septic Avenger"},
	{zombieuprising, "Zombie Uprising"},
	{racing, "Racing"},
	{heliassault, "Heli Assault"},
	{mayhem, "Mayhem"}
	};

	std::string getActivityLowercase(activities activity) {
		auto it = activitiesToString.find(activity);
		if (it == activitiesToString.end()) {
			return "";
		}
		std::string result = it->second;
		std::transform(result.begin(), result.end(), result.begin(), ::tolower);
		std::replace(result.begin(), result.end(), ' ', '_');
		return result;
	}

	typedef bool(*isCoopT)();
	isCoopT isCoop = (isCoopT)0x007F7AD0;
	// Updates state info for discord.
	void UpdateDiscordParams() {
		BYTE CurrentGamemode = *(BYTE*)DynAddress(0x00E8B210); // Parses the current gamemode from EXE
		BYTE LobbyCheck = *(BYTE*)DynAddress(0x02528C14); // Checks lobby, technically this is another gamemode check but we'll use it for lobby
		BYTE MatchType = *(BYTE*)DynAddress(0x00E8B20C); // Checks match type
		char* playerName = (CHAR*)DynAddress(0x0212AB48); // parses player name
		wchar_t* partnerName = (WCHAR*)DynAddress(0x02CD1870); // parses Co-op Partner name, usually
		BYTE IsInCutscene = *(BYTE*)DynAddress(0x02527D14); // Checks if user is in a cutscene.
		BYTE CurDifficulty = *(BYTE*)DynAddress(0x00E986F4); // Current Difficulty

		std::wstring wPartnerName = partnerName; // parse co-op partner name to a wstring
		std::string f_PartnerName = wstring_to_string(wPartnerName); // THEN to a string
		const char* COOPPartner = f_PartnerName.c_str(); // now to a const char because discord is a picky bitch
		char finalUsername[2048];
		char finalMPDesc[2048];
		char finalCOOPDescCutsc[2048];
		char finalCOOPDesc[2048];
		char finalSPDesc[2048];
		char smalltxtmission[256];

		// -- Get Current Players District
		const wchar_t* district_name = (const wchar_t*)UtilsGlobal::ReadPointer(UtilsGlobal::getplayer(true), { 0x18CC,0x44,0 });
		std::wstring district_test = L"";
		if (district_name) {
			district_test += district_name;
			if (district_test.empty())
				district_test += L"Stilwater";
		}
		// -----

		const wchar_t* mission_name = (const wchar_t*)UtilsGlobal::ReadPointer(0x027B3C60, { 0x34,0 });
		gang* mission_team = (gang*)UtilsGlobal::ReadPointer(0x027B3C60, { 0x40 });
		auto Current_Activity = Activity_Type();
		if (Current_Activity == none) {
			if (mission_team) {

				//wprintf(L"mission team: %i\n", *mission_team);
				strcpy_s(pres.assets.small_image, gangToString[*mission_team]);
			}
			else {
				strcpy_s(pres.assets.small_image, "");
			}
			if (mission_name) {
				sprintf(smalltxtmission, "%s", wchar_to_utf8(mission_name).c_str());
				strcpy_s(pres.assets.small_text, smalltxtmission);
			}
	    }
		else {
			//wprintf(L"mission name: %i\n", Activity_Type());
			strcpy_s(pres.assets.small_text, activitiesToString[Current_Activity]);
			strcpy_s(pres.assets.small_image, getActivityLowercase(Current_Activity).c_str());
		}

		std::string Difficulty = "";
		if (CurDifficulty == 0x0) {
			Difficulty = "Casual";
		}
		else if (CurDifficulty == 0x1) {
			Difficulty = "Normal";
		}
		else {
			Difficulty = "Hardcore";
		}
		//BYTE IsInMission = *(BYTE*)0x27B3C60; // parses mission (?)
		sprintf(finalUsername, "%s", playerName);
		sprintf(finalMPDesc, "%s (In Map: %s)", playerName, *FancyChunkName);
		sprintf(finalCOOPDescCutsc, "Watching a Cutscene with %s", COOPPartner);
		sprintf(finalCOOPDesc, "Exploring %s with %s - %s", wchar_to_utf8(district_test).c_str(), COOPPartner, Difficulty.c_str());
		sprintf(finalSPDesc, "Exploring %s - %s", wchar_to_utf8(district_test).c_str(), Difficulty.c_str());

		static DWORD lastTick = 0;

		DWORD currentTick = GetTickCount();
		if (currentTick - lastTick >= 600) {
			lastTick = currentTick;

			// If GOTR, Change name and logo to GOTR
			if (Game::xtbl_scan_status.gotr_detected == 1) {
				strcpy_s(pres.assets.large_image, "gotr");
				strcpy_s(pres.assets.large_text, "Gentlemen of the Row mod in use.");
			}

			if (!LobbyCheck == 0x0 && CurrentGamemode == 0xFF) // This should be CO-OP / Singleplayer
			{
				if (IsInCutscene == 1)
				{
					if (f_PartnerName == playerName || f_PartnerName.empty() || !isCoop) {
						strcpy_s(pres.details, "Watching a Cutscene");
					}
					else
					{
						strcpy_s(pres.details, finalCOOPDescCutsc);
						strcpy_s(pres.state, finalUsername);
					}
				}
				else
				{
					if (f_PartnerName == playerName || f_PartnerName.empty() || !isCoop) {
						strcpy_s(pres.details, finalSPDesc);
					}
					else
					{
						strcpy_s(pres.details, finalCOOPDesc);
						strcpy_s(pres.state, finalUsername);
					}
				}
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
				if (*FancyChunkName == "Prison Island (PTP Blitz)") {
					if (MatchType == (BYTE)1)
						strcpy_s(pres.details, "Playing MP in PTP Blitz");

					if (MatchType == (BYTE)2)
						strcpy_s(pres.details, "Playing Ranked MP in PTP Blitz");

					if (MatchType == (BYTE)3)
						strcpy_s(pres.details, "Playing Party MP in PTP Blitz");
				}
				else {
					if (MatchType == (BYTE)1)
						strcpy_s(pres.details, "Playing MP in Team Gangsta Brawl");

					if (MatchType == (BYTE)2)
						strcpy_s(pres.details, "Playing Ranked MP in Team Gangsta Brawl");

					if (MatchType == (BYTE)3)
						strcpy_s(pres.details, "Playing Party MP in Team Gangsta Brawl");
				}

				strcpy_s(pres.state, finalMPDesc);
			}
			if (LobbyCheck == 0x44) // Game Lobby
			{
				// Player MP Lobby shows as default for everyone, Ranked and Party seem to only show in rich presence if host
				// so we'll keep the conditions because we need it for friendly fire shit anyway.
				if (MatchType == (BYTE)1) // Player Lobby
					strcpy_s(pres.details, "Waiting in MP Lobby...");

				if (MatchType == (BYTE)2) // Ranked Lobby
				{
					strcpy_s(pres.details, "Waiting in Ranked MP Lobby...");
				}

				if (MatchType == (BYTE)3) // Party Lobby
					strcpy_s(pres.details, "Waiting in Party MP Lobby...");
			}
			if (LobbyCheck == 0x0) // Usually Menus Check
			{
				strcpy_s(pres.details, "In Menus...");
				strcpy_s(pres.state, "");
			}

			app.activities->update_activity(app.activities, &pres, 0, 0);
		}

	}
	void Init() {
		Enabled = GameConfig::GetValue("Misc", "RichPresence", 0);

		if (Enabled)
		{
			Logger::TypedLog(CHN_RPC, "Attempting to initialize Discord RPC...\n");
			HMODULE discord_handle = LoadLibraryA(DISCORD_LIB_NAME);
			if (!discord_handle) {
				Logger::TypedLog(CHN_RPC, "Discord's DLL not found, skipping RPC.\n");
				Enabled = false;
				return;
			}
				DiscordCreateFn DiscordCreate = (DiscordCreateFn)GetProcAddress(discord_handle, "DiscordCreate");
				if (!DiscordCreate) {
					Logger::TypedLog(CHN_RPC, "Discord's DLL ProcAddress fail. \n\n");
					Enabled = false;
					return;
				} else InitRPC(DiscordCreate);
			}
			
			//RPCHandler::DiscordCallbacks(); callbacks needs to be hooked into a game loop
		}
	

	void InitRPC(DiscordCreateFn EDiscordCreate)
	{
		if (!EDiscordCreate)
			Logger::TypedLog(CHN_RPC, "Discord RPC Initialization failed !!!!!!!!\n");
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
		int fail = EDiscordCreate(DISCORD_VERSION, &params, &app.core);
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
		params.client_id = 1381232715705221244;
		params.flags = DiscordCreateFlags_NoRequireDiscord;
		params.event_data = &app;
		params.activity_events = &activities_events;
		params.relationship_events = &relationships_events;
		params.user_events = &users_events;
		//DiscordCreate(DISCORD_VERSION, &params, &app.core);
		int fail = EDiscordCreate(DISCORD_VERSION, &params, &app.core);
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
				strcpy_s(pres.assets.large_image, "tharow");
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
#endif