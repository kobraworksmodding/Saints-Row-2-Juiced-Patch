#include "../FileLogger.h"
#include "../Patcher/patch.h"
#include "../SafeWrite.h"
#include "../GameConfig.h"
#include "Game.h"
#include <safetyhook.hpp>
namespace Game
{
	namespace Timer {
		// Returns game's frametime in ms
		float GetFrameTime() {
			return *(float*)0xE84380;
		}
		// Returns frametime / 33 ms,
		// use this to fix calculations that are calculated FASTER than they should when game is running at a higher FPS.
		float GetFrameTimeOver33ms_Fix() {
			return GetFrameTime() / (1.f / 30.f);
		}
		// Returns 33 ms / frametime,
		// use this to fix calculations that are calculated SLOWER than they should when game is running at a higher FPS.
		float Get33msOverFrameTime_Fix() {
			return  (1.f / 30.f) / GetFrameTime();
		}
		// Returns Havok's frametime, by default this is stuck to 16.6ms but with Havok ticker it'll tick correctly.
		float GetHavokFrameTime() {
			return *(float*)0x02527DA4;
		}
		// Returns havok frametime / 16.6 ms,
		// use this to fix calculations that are calculated FASTER than they should when game is running at a higher HAVOK FPS.
		float GetHavokFrameTimeOver16ms_Fix() {
			return GetHavokFrameTime() / (1.f / 60.f);
		}
		havok_get_time_this_frameT havok_get_time_this_frame = (havok_get_time_this_frameT)0x6FF860;
	}
	namespace Physical {
		using namespace Timer;
		
		// Maybe should figure out how GTA modders use makeinline from ThirtneenAG's fork of injector which uses SafetyHook? I'm happy with this though.

		SafetyHookMid motorcycle_should_eject_passengers_MIDASMHOOK;
		// improves/fixes bike ejection when Havok Frametime is being ticked.
		void motorcycle_should_eject_passengers_asmhook(safetyhook::Context32& ctx) {

			float* delta_velocity_vector = (float*)ctx.ebp;

			delta_velocity_vector[0] *= GetHavokFrameTimeOver16ms_Fix();
			delta_velocity_vector[1] *= GetHavokFrameTimeOver16ms_Fix();
			delta_velocity_vector[2] *= GetHavokFrameTimeOver16ms_Fix();
			/*printf("motorcycle delta_velocity ebp: 0x%X\n x: %f\n y: %f\n z: %f\n mag3-thisframe: %f \n",
				reinterpret_cast<uintptr_t>(delta_velocity_vector),
				delta_velocity_vector[0],
				delta_velocity_vector[1],
				delta_velocity_vector[2],
				std::sqrtf(
					delta_velocity_vector[0] * delta_velocity_vector[0] +
					delta_velocity_vector[1] * delta_velocity_vector[1] +
					delta_velocity_vector[2] * delta_velocity_vector[2]
				) 
			) / (float)havok_get_time_this_frame();*/
		}
	}
	void CreateSafetyHooks() {
		using namespace Physical;
		motorcycle_should_eject_passengers_MIDASMHOOK = safetyhook::create_mid(0x00AB599F, &motorcycle_should_eject_passengers_asmhook);
	}
	namespace HUD
	{
		int __declspec(naked) vint_message(wchar_t* message_text, vint_message_struct* a2) {
			__asm {
				push ebp
				mov ebp, esp
				sub esp, __LOCAL_SIZE


				mov eax, message_text
				push eax
				mov edi, a2
				push edi

				mov edx, 0x0079CD40
				call edx

				mov esp, ebp
				pop ebp
				ret
			}
		}
	};

	namespace InLoop
	{
		void RemoveWordFromLine(std::string& line, const std::string& word)
		{
			auto n = line.find(word);
			if (n != std::string::npos)
			{
				line.erase(n, word.length());
			}
		}

		bool IsCoopOrSP = false;
		bool ShouldFixStereo = false;

		bool AlreadyAddedClanTag = 0;
		int isDefaultSNameChecked = 0;
		bool UsingClanTag = 0;

		char* ClanTag[3] = {
        	const_cast<char*>("["),
        	const_cast<char*>(""),
	        const_cast<char*>("]")
		};
#if RELOADED
		bool ChangedRLServerName = 0;

		void RLServerNameUpdateOnce() {
			char* playerName = (CHAR*)0x0212AB48;
			char finalSName[2048];
			sprintf(finalSName, "%s - SR2 RL 1\.0b4", playerName);
			char* GameName = reinterpret_cast<char*>(0x0212AA08);
			strcpy(GameName, finalSName);
		}
#endif

		void FrameChecks() { // Updates some specific stuff we need to loop all the time
			BYTE CurrentGamemode = *(BYTE*)0x00E8B210; // Parses the current gamemode from EXE
			BYTE LobbyCheck = *(BYTE*)0x02528C14; // Checks lobby, technically this is another gamemode check but we'll use it for lobby
			BYTE MatchType = *(BYTE*)0x00E8B20C; // Checks match type
			BYTE AbleToStartGame = *(BYTE*)0x02528D90; // Determines whether the gamemode is able to start or not (we'll force this on when we can, nice QOL feature.)
			BYTE IsInCutscene = *(BYTE*)0x02527D14; // Checks if user is in a cutscene.
			char* playerName = (CHAR*)0x0212AB48; // parses player name
			BYTE GamespyStatus = *(BYTE*)0x02529334; // Checks the current gamespy status.

			static DWORD lastTick2 = 0;

			DWORD currentTick2 = GetTickCount();

			if (currentTick2 - lastTick2 >= 600) {
				lastTick2 = currentTick2;
				if (LobbyCheck == 0x44) // Game Lobby
				{
#if RELOADED
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
					if (MatchType == (BYTE)2) { // If in ranked
#if RELOADED
						* (BYTE*)0x02A4D134 = 0x1; // Force Friendly Fire to Full Damage.
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
					if (GamespyStatus == 0x4) {
						if (ChangedRLServerName == 0) {
							RLServerNameUpdateOnce();
							ChangedRLServerName = 1;
						}
					}
					if (UsingClanTag == 1)
					{
						if (GamespyStatus == 0x4) {
							if (AlreadyAddedClanTag == 1) {
								std::string NameResult = playerName;
								std::string ClanTagresult = ClanTag[0];
								ClanTagresult = ClanTagresult + ClanTag[1] + ClanTag[2] + " ";
								RemoveWordFromLine(NameResult, ClanTagresult);
								const char* finalNameString = NameResult.c_str();
								char* newPlayerName = reinterpret_cast<char*>(playerName);
								strcpy(newPlayerName, finalNameString);
								AlreadyAddedClanTag = 0;
							}
						}
					}
					*(BYTE*)0x02A4D134 = 0x0; // Force Friendly Fire to Off.
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
	}

// maybe expose read_and_parse_file for outside reloaded but currently I don't have a use for it in Juiced -- Clippy95
#if RELOADED
	namespace xml {
		read_and_parse_fileT read_and_parse_file = (read_and_parse_fileT)0x00966720;

		uint32_t checksum(xtbl_node* root, uint32_t accumulator)
		{
			unsigned int result = accumulator;
			if (root)
			{
				if (root->name)
					result = Game::utils::str_to_hash(root->name) ^ result;

				if (root->text)
					result = Game::utils::str_to_hash(root->text) ^ result;

				for (xtbl_node* child = root->elements; child; child = child->next)
				{
					result = checksum(child, result);
				}
			}
			return result;
		}
		__declspec(naked) xtbl_node* parse_table_node(const char* filename, int* override_xtbl_mempool) {
			__asm {
				push ebp
				mov ebp, esp
				sub esp, __LOCAL_SIZE

				mov eax, filename
				mov ecx, override_xtbl_mempool
				mov edx, 0x00B743F0
				call edx

				mov esp, ebp
				pop ebp
				ret
			}
		}
	}
	namespace utils {
		crc_strT str_to_hash = (crc_strT)0x00BDC9B0;
	}
#endif
};