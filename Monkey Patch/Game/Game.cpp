#include "../FileLogger.h"
#include "../Patcher/patch.h"
#include "../SafeWrite.h"
#include "../GameConfig.h"
#include "Game.h"
#include <safetyhook.hpp>
#include "../Patcher/CPatch.h"
#include "../Player/Input.h"
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
	CPatch CDisable_Tutorials = CPatch::SafeWrite8(0x006B7260, 0xC3);
	struct UIElementProperties {
		std::string name;
		int startX;
		int startY;
		int imageWidth;
		int imageHeight;
		float imageScale = 1.0f;
		int clip = 0;
		std::string persistFilename;

		UIElementProperties(const std::string& n, int x, int y, int w, int h, const std::string& filename)
			: name(n), startX(x), startY(y), imageWidth(w), imageHeight(h), imageScale(1.0f), clip(0), persistFilename(filename) {}

		UIElementProperties(const std::string& n, int x, int y, int w, int h, float scale, int c, const std::string& filename)
			: name(n), startX(x), startY(y), imageWidth(w), imageHeight(h), imageScale(scale), clip(c), persistFilename(filename) {}
	};

	bool elementExistsInXML(const std::string& xml_content, const std::string& elementName) {
		return xml_content.find("<Name>" + elementName + "</Name>") != std::string::npos;
	}

	bool createUIElement(std::string& xml_content, const std::string& targetSheet, const UIElementProperties& props) {
		size_t sheet_pos = xml_content.find("<Name>" + targetSheet + "</Name>");
		if (sheet_pos == std::string::npos) {
			Logger::TypedLog(CHN_XTBL, ("Target sheet " + targetSheet + " not found \n").c_str());
			return false;
		}

		size_t images_start = xml_content.find("<Images>", sheet_pos);
		if (images_start == std::string::npos) {
			Logger::TypedLog(CHN_XTBL, ("Images section not found in " + targetSheet + "\n").c_str());
			return false;
		}

		size_t images_end = xml_content.find("</Images>", images_start);
		if (images_end == std::string::npos) {
			Logger::TypedLog(CHN_XTBL, ("Images end not found in " + targetSheet + "\n").c_str());
			return false;
		}

		size_t existing_element = xml_content.find("<Name>" + props.name + "</Name>", images_start);
		if (existing_element != std::string::npos && existing_element < images_end) {
			Logger::TypedLog(CHN_XTBL, (props.name + " already exists in " + targetSheet + ", skipping creation\n").c_str());
			return true;
		}

		std::string new_properties = R"(			<Properties>
				<Name>)" + props.name + R"(</Name>
				<StartX>)" + std::to_string(props.startX) + R"(</StartX>
				<StartY>)" + std::to_string(props.startY) + R"(</StartY>
				<ImageWidth>)" + std::to_string(props.imageWidth) + R"(</ImageWidth>
				<ImageHeight>)" + std::to_string(props.imageHeight) + R"(</ImageHeight>
				<ImageScale>)" + std::to_string(props.imageScale) + R"(</ImageScale>
				<Clip>)" + std::to_string(props.clip) + R"(</Clip>
				<PersistFilename>)" + props.persistFilename + R"(</PersistFilename>
			</Properties>
)";

		xml_content.insert(images_end, new_properties);
		Logger::TypedLog(CHN_XTBL, ("Successfully created " + props.name + " in " + targetSheet + "\n").c_str());
		return true;
	}

	bool moveUIElement(std::string& xml_content, const std::string& fromSheet, const std::string& toSheet, const UIElementProperties& props) {
		size_t from_pos = xml_content.find("<Name>" + fromSheet + "</Name>");
		if (from_pos != std::string::npos) {
			size_t images_start = xml_content.find("<Images>", from_pos);
			if (images_start != std::string::npos) {
				size_t images_end = xml_content.find("</Images>", images_start);
				if (images_end != std::string::npos) {
					size_t element_start = xml_content.find("<Name>" + props.name + "</Name>", images_start);
					if (element_start != std::string::npos && element_start < images_end) {
						size_t properties_start = xml_content.rfind("<Properties>", element_start);
						if (properties_start != std::string::npos && properties_start > images_start) {
							size_t properties_end = xml_content.find("</Properties>", element_start);
							if (properties_end != std::string::npos) {
								properties_end += strlen("</Properties>");
								xml_content.erase(properties_start, properties_end - properties_start);
								Logger::TypedLog(CHN_XTBL, ("Removed " + props.name + " from " + fromSheet + "\n").c_str());
							}
						}
					}
				}
			}
		}

		size_t to_pos = xml_content.find("<Name>" + toSheet + "</Name>");
		if (to_pos == std::string::npos) {
			Logger::TypedLog(CHN_XTBL, (toSheet + " not found" + "\n").c_str());
			return false;
		}

		size_t to_images_start = xml_content.find("<Images>", to_pos);
		if (to_images_start == std::string::npos) {
			Logger::TypedLog(CHN_XTBL, ("Images section not found in " + toSheet + "\n").c_str());
			return false;
		}

		size_t to_images_end = xml_content.find("</Images>", to_images_start);
		if (to_images_end == std::string::npos) {
			Logger::TypedLog(CHN_XTBL, ("Images end not found in " + toSheet).c_str());
			return false;
		}

		size_t existing_element = xml_content.find("<Name>" + props.name + "</Name>", to_images_start);
		if (existing_element != std::string::npos && existing_element < to_images_end) {
			Logger::TypedLog(CHN_XTBL, (props.name + " already exists in " + toSheet + ", skipping" + "\n").c_str());
			return true;
		}

		std::string new_properties = R"(			<Properties>
				<Name>)" + props.name + R"(</Name>
				<StartX>)" + std::to_string(props.startX) + R"(</StartX>
				<StartY>)" + std::to_string(props.startY) + R"(</StartY>
				<ImageWidth>)" + std::to_string(props.imageWidth) + R"(</ImageWidth>
				<ImageHeight>)" + std::to_string(props.imageHeight) + R"(</ImageHeight>
				<ImageScale>)" + std::to_string(props.imageScale) + R"(</ImageScale>
				<Clip>)" + std::to_string(props.clip) + R"(</Clip>
				<PersistFilename>)" + props.persistFilename + R"(</PersistFilename>
			</Properties>
)";

		xml_content.insert(to_images_end, new_properties);
		Logger::TypedLog(CHN_XTBL, ("Successfully added " + props.name + " to " + toSheet + "\n").c_str());
		return true;
	}

	bool moveOrCreateUIElement(std::string& xml_content, const std::string& fromSheet, const std::string& toSheet, const UIElementProperties& props) {
		if (elementExistsInXML(xml_content, props.name)) {
			return moveUIElement(xml_content, fromSheet, toSheet, props);
		}
		else {
			Logger::TypedLog(CHN_XTBL, (props.name + " doesn't exist anywhere, creating in " + toSheet + "\n").c_str());
			return createUIElement(xml_content, toSheet, props);
		}
	}
	SafetyHookMid xtbl_read_and_parse_file_hook{};
	void Init() {
		static bool unhook_after_patching_xtbl_read_and_parse_file = GameConfig::GetValue("Debug", "unhook_after_patching_xtbl_read_and_parse_file", 1);
		if(Input::EnableDynamicPrompts)
			xtbl_read_and_parse_file_hook = safetyhook::create_mid(0xBFBA44, [](SafetyHookContext& ctx) {
			char* xtbl_filename = (char*)ctx.edi;
			char* buffer = (char*)ctx.esi;

			if (strcmp(xtbl_filename, "bitmap_sheetsen.xtbl") == 0 ||
				strcmp(xtbl_filename, "bitmap_sheetsdk.xtbl") == 0 ||
				strcmp(xtbl_filename, "bitmap_sheetsde.xtbl") == 0 ||
				strcmp(xtbl_filename, "bitmap_sheetses.xtbl") == 0 ||
				strcmp(xtbl_filename, "bitmap_sheetsfr.xtbl") == 0 ||
				strcmp(xtbl_filename, "bitmap_sheetsit.xtbl") == 0 ||
				strcmp(xtbl_filename, "bitmap_sheetsnl.xtbl") == 0 ||
				strcmp(xtbl_filename, "bitmap_sheetspl.xtbl") == 0 ||
				strcmp(xtbl_filename, "bitmap_sheetsru.xtbl") == 0) {
				std::string xml_content(buffer);

				std::vector<UIElementProperties> elementsToMove = {
					{
						"ui_ctrl_360_btn_a",
						450, 289, 34, 34,
						"D:\\projects\\sr2art\\Interface\\bms\\controls\\ui_ctrl_360_btn_a.tga"
					},
					{
						"ui_ctrl_360_btn_x",
						346, 989, 34, 34,
						"path\\to\\file.tga"
					},
					{
						"ui_ctrl_ps3_btn_cross",
						450, 359, 34, 34,
						"path\\to\\file.tga"
					}
				};

				std::vector<UIElementProperties> elementsToMove_00 = {
					{
						"ui_ctrl_360_dpad_lr",
						391, 892, 42, 42,
						"path\\to\\file.tga"
					},
					{
						"ui_ctrl_360_dpad_ud",
						391, 849, 42, 42,
						"path\\to\\file.tga"
					}
				};

				for (const auto& element : elementsToMove) {
					moveUIElement(xml_content, "ui_bms_10", "ui_bms_01", element);
				}
				for (const auto& element : elementsToMove_00) {
					moveUIElement(xml_content, "ui_bms_10", "ui_bms_00", element);
				}

				std::vector<std::pair<std::string, UIElementProperties>> elementsToCreate = {
					{
						"ui_bms_10",
						UIElementProperties(
							"ui_ctrl_pc_dpad_lr_juiced",
							400, 269, 73, 44, 0.7f, 0,
							"D:\\projects\\sr2art\\Interface\\bms\\new\\ui_brand_new_element.tga"
						)
					},
					{
						"ui_bms_10",
						UIElementProperties(
							"ui_ctrl_pc_dpad_up_juiced",
							322, 269, 73, 44, 0.7f, 0,
							"D:\\projects\\sr2art\\Interface\\bms\\new\\ui_brand_new_element.tga"
						)
					}
				};

				for (const auto& [targetSheet, element] : elementsToCreate) {
					if (!elementExistsInXML(xml_content, element.name)) {
						createUIElement(xml_content, targetSheet, element);
					}
					else {
						Logger::TypedLog(CHN_XTBL, (element.name + " already exists, skipping creation" + "\n").c_str());
					}
				}

				size_t new_size = xml_content.length() + 1;
				char* new_buffer = new char[new_size];
				strcpy_s(new_buffer, new_size, xml_content.c_str());

				ctx.esi = (uintptr_t)new_buffer;

				Logger::TypedLog(CHN_XTBL, "XML modification completed \n");\
				// We'll unhook because it seems like bitmaps_sheets{lang}.xtbl is only parsed once, 
				// as it doesn't make sense to keep on strcmp xtbl after patching the xtbl, 
				// if this statement is incorrect, we'll have to remove this unhook. 
				if(unhook_after_patching_xtbl_read_and_parse_file)
				xtbl_read_and_parse_file_hook.disable();
			}
			});
		if (GameConfig::GetValue("Gameplay", "DisableTutorials", 0))
			CDisable_Tutorials.Apply();

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