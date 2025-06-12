// General.cpp (uzis, Tervel, Clippy95)
// --------------------
// Created: 22/02/2025

/* This CPP Contains crucial runtime patches for Juiced Patch that we must check for
and / or run completely on startup or after we check everything else.*/

#include "../FileLogger.h"
#include "../Patcher/patch.h"
#include "../Patcher/CPatch.h"
#include "../GameConfig.h"
#include "../SafeWrite.h"
#include "General.h"
#include <safetyhook.hpp>
#include "../Render/Render3D.h"
#include "../Render/Render2D.h"
#include "..\LUA\InGameConfig.h"
#include "../Player/Input.h"
#include "../Math/Math.h"
#include "../UtilsGlobal.h"
#include "../Game/Game.h"
#include "JuicedAPI.h"

#include "../Render/bitmap.h"
using namespace Math;
#pragma warning( disable : 4409)
namespace General {
	bool DeletionMode;
	bool allowJuicedAPI = true;
	const wchar_t* SaveMessage = L"Are you sure you want to delete this save?"; // ultimately, if we get extra strings to load, we should use a string label and request the string instead of hardcoding it
	const char* JVLib = "juiced_vint_lib";
	const char* JStr = "juiced";
	bool IsSpawning = false;
	bool* EnterPressed = (bool*)0x02348CD0;
	bool* InCutscene = (bool*)0x2527D14;
	bool* InMultiplayer = (bool*)0x2528B48;
	bool IsQuickSaving = false;
	char* CurrentGamemode = (char*)0x00E8B210;
	UINT16* GameResX = (UINT16*)0x022F63F8;
	UINT16* GameResY = (UINT16*)0x022F63FC;
	char* LobbyCheck = (char*)0x02528C14; // Copied from Rich Presence stuff, just using it so we can limit LUA Executor to SP/CO-OP.
	char* InMission = (char*)0x27B3C60;
	char* GameLoaded = (char*)0x00E94D3E;
	int CurrentNPC = 0;
	int SpawnedNPCs[10] = { 0 }; // we could make this a vector maybe, i don't mind it being like this though

	int userResX = GetSystemMetrics(SM_CXSCREEN);
	int userResY = GetSystemMetrics(SM_CYSCREEN);
	std::string patchedRes;

	bool resFound = false;

	GetCharacterIDT GetCharacterID = (GetCharacterIDT)0x4A5A90;
	ChangeCharacterT ChangeCharacter = (ChangeCharacterT)0x6856A0;
	ResetCharacterT ResetCharacter = (ResetCharacterT)0x685D50;
	DeleteNPCT DeleteNPC = (DeleteNPCT)0x960240;

	bool IsSRFocused()
	{
		DWORD pid;
		HWND Window = GetForegroundWindow();
		GetWindowThreadProcessId(GetForegroundWindow(), &pid);
		if (Window != ConsoleWindow) {
			ClipCursor(0); // Make SR2 let go of the Cursor, hopefully.
			return pid == GetCurrentProcessId();
		}
		return false;
	}


	void __declspec(naked) CutscenePauseWorkaround()
	{
		static int Continue = 0x0068CAA7;
		static int SkipAddr = 0x0068CAFA;
		__asm {
			jnz Check
			jmp Resume

			Check :
			mov al, ds : byte ptr[0x2527D14]
			cmp al, 0
			jz Skip
			jmp Resume

			Skip :
			jmp SkipAddr

			Resume :
			mov edi, dword ptr[0x6C6870]
			call edi
			jmp Continue

		}
	}

	void __declspec(naked) CutscenePauseCheck()
	{
		static int Continue = 0x006D8E10;
		static int SkipAddr = 0x006D8F6F;
		__asm {
			jnz Skip
			mov edi, ds:dword ptr[0x2527C08]
			cmp edi, 0
			jnz Skip
			jmp Continue

			Skip :
			jmp SkipAddr
		}
	}

	void __cdecl SaveDelCallback(int Unk, bool Result, int Action) {
		if (Action == 2) {
			*(bool*)0x25283B0 = true;
			*(bool*)0x25283B1 = Result ? false : true;
			if (Result) {
				DeletionMode = false;
			}
		}
	}

	void __declspec(naked) ChangeSOCallback()
	{
		static int Continue = 0x007787D5;
		__asm {
			mov		cl, byte ptr[DeletionMode]
			test	cl, cl
			jnz     Replace
			mov     ecx, dword ptr[0x7786E0]
			push	ecx
			jmp		Continue

			Replace :
			push SaveDelCallback
			jmp Continue
		}
	}

	void __declspec(naked) DeletionModeCheck()
	{
		static int Continue = 0x00779534;
		__asm {
			mov		cl, byte ptr[DeletionMode]
			test	cl, cl
			jnz     Skip
			mov		edx, 0x778A40
			call	edx
			jmp		Continue
		}
		Skip:
		__asm{
			mov eax, 1
			mov		ds: DeletionMode, 0
			mov     ecx, [EnterPressed]
			mov     ds : byte ptr[ecx], 0
			jmp Continue
		}
	}

	void __declspec(naked) ReplaceSOMessage()
	{
		static int Continue = 0x00778800;
		__asm {
			mov		cl, byte ptr[DeletionMode]
			test	cl, cl
			jnz     Skip
			mov     ecx, dword ptr[0x7F49E0]
			call	ecx
			jmp		Continue

			Skip :
			mov eax, SaveMessage
			mov     ecx, [EnterPressed]
			mov     ds : byte ptr[ecx], 0
			jmp Continue
		}
	}

	void __declspec(naked) SkipSaving()
	{
		static int Continue = 0x007788C3;
		static int SkipAddr = 0x007788DC;
		__asm {
			mov		ecx, dword ptr[0x695150]
			call	ecx
			mov		cl, byte ptr[DeletionMode]
			test	cl, cl
			jnz     Skip
			jmp		Continue

			Skip :
			add esp, 4
			mov byte ptr[DeletionMode], 0
			mov ds : byte ptr[0x2528377], 1
			jmp SkipAddr
		}
	}

	void __declspec(naked) ShadowsFix()
	{
		static int jmp_continue = 0x00773783;
		__asm {
			cmp ds : byte ptr[0xE98994], 0
			jz Skip
			mov ds : byte ptr[0x252A37C], 1
			jmp jmp_continue

			Skip :
			mov ds : byte ptr[0x252A37C], 0
			jmp jmp_continue
		}
	}

	typedef void __cdecl HudControlT(bool Hide);
	HudControlT* HudControl = (HudControlT*)(0x793D60);

	void IdleFix(bool Hide) {

		patchByte((BYTE*)0x004F81EE, Hide ? 0x00 : 0x32);
		patchByte((BYTE*)0x004F81CE, Hide ? 0x00 : 0x33);

		return HudControl(Hide);
	}

	typedef int __cdecl TextureTestT(int idk1, int idk2);
	TextureTestT* TextureTest = (TextureTestT*)(0xC080C0);

	int TextureCrashFix(int idk1, int idk2) {

		__asm pushad
		idk1 = *(int*)(idk2); // making the first arg be the same as the second seems to not break the game and could maybe fix the crash
		__asm popad
		return TextureTest(idk1, idk2);
	}

	char __declspec(naked) SpawnNPC(int NPCPointer) {
		__asm {
			push ebp
			mov ebp, esp
			sub esp, __LOCAL_SIZE

			mov     eax, NPCPointer
			push	eax
			mov     eax, ds: 0x21703D4
			mov     ecx, 0x98E400
			call    ecx

			mov esp, ebp
			pop ebp
			ret
		}
	}

	void NPCSpawner(const char* Name) {
		static int Index = 0;
		int CharID = (int)GetCharacterID(Name);
		if (CharID != NULL) {
			IsSpawning = true;
			if (SpawnedNPCs[Index] != 0) {
				DeleteNPC(SpawnedNPCs[Index], 0); // deletes the oldest npc once you go past 10 (feel free to change the max amount)
			}
			SpawnNPC(CharID);
			SpawnedNPCs[Index] = CurrentNPC;
			Index = (Index + 1) % 10;
			IsSpawning = false;
		}
	}

	void YeetAllNPCs() {
		for (int i = 0; i < 10; i++) {
			if (SpawnedNPCs[i] != 0) {
				DeleteNPC(SpawnedNPCs[i], 0);
				SpawnedNPCs[i] = 0;
			}
		}
	}

	/*void __declspec(naked) TextureCrashFixDefinitive() {

	static bitmap_entry* BMP;
	static peg_entry* Peg;
	__asm {
		mov BMP, edi
		mov Peg, esi
	}
	__asm pushad
	add_to_entry_list(BMP, Peg);
	__asm popad
}*/

/*bool IsMemoryReadable(void* address) {
MEMORY_BASIC_INFORMATION mbi;

if (VirtualQuery(address, &mbi, sizeof(mbi))) {
	bool isReadable = (mbi.Protect & PAGE_READONLY) ||
		(mbi.Protect & PAGE_READWRITE) ||
		(mbi.Protect & PAGE_EXECUTE_READ) ||
		(mbi.Protect & PAGE_EXECUTE_READWRITE);

	printf("Address: %p | Protect: 0x%lx | Readable: %s\n",
		address, mbi.Protect, isReadable ? "Yes" : "No");

	return isReadable;
}
printf("Address: %p | VirtualQuery failed\n", address);
return false;
}

void __declspec(naked) TextureCrashFixRemasteredByGroveStreetGames()
{
	static int jmp_continue = 0x00C080F0;
	static int jmp_skip = 0x00C080F2;
	__asm {
		pushad
		lea edi, dword ptr[eax + 4]
		push edi
		call IsMemoryReadable
		pop edi
		test eax, eax
		jz skip
		popad
		mov bp, [eax + 4]
		cmp bp, [eax + 4]
		jmp jmp_continue

		skip :
		popad
			jmp jmp_skip
	}
}
*/
	void replace_all( // taken from https://stackoverflow.com/questions/5878775/how-to-find-and-replace-string - we could code our own if there's a problem
		std::string& s,
		std::string const& toReplace,
		std::string const& replaceWith
	) {
		std::string buf;
		std::size_t pos = 0;
		std::size_t prevPos;

		buf.reserve(s.size());

		while (true) {
			prevPos = pos;
			pos = s.find(toReplace, pos);
			if (pos == std::string::npos)
				break;
			buf.append(s, prevPos, pos - prevPos);
			buf += replaceWith;
			pos += toReplace.size();
		}

		buf.append(s, prevPos, s.size() - prevPos);
		s.swap(buf);
	}
	char* currentModifiedBuffer = nullptr;
	char* currentModifiedBuffer_general_lua = nullptr;
	void generalluaLoadBuff(safetyhook::Context32& ctx) {
		if (!(ctx.esp + 0x14))
			return;
		const char* buff = (const char*)ctx.eax;
		if (!buff)
			return;
		const char* filename = (const char*)*(uintptr_t*)(ctx.esp + 0x14);
		size_t& sz = ctx.ecx;
		std::string convertedBuff(buff, sz);
		const char* currentBuff = buff;
		bool modified = false;


		std::string finalContent;
		finalContent = std::string(currentBuff, sz);
		if (strcmp(filename, "vint_lib.lua") == 0 || 
			strcmp(filename, "hud_btnmash.lua") == 0 || 
			strcmp(filename, "completion.lua") == 0 ||
			strcmp(filename, "pause_map.lua") == 0) {
			if (!modified) {
				// If we haven't created finalContent yet, do it now
				finalContent = std::string(currentBuff, sz);
			}
			if (currentModifiedBuffer_general_lua != nullptr) {
				delete[] currentModifiedBuffer_general_lua;
				currentModifiedBuffer_general_lua = nullptr;
			}
			// Lambda function for performing simple replacements
			auto performReplacement = [&](const std::string& searchStr, const std::string& replaceStr) -> size_t {
				size_t count = 0;
				size_t pos = 0;
				while ((pos = finalContent.find(searchStr, pos)) != std::string::npos) {
					finalContent.replace(pos, searchStr.length(), replaceStr);
					pos += replaceStr.length(); // Move past the replacement to avoid infinite loop
					count++;
					modified = true;
				}
				//if (count > 0) {
				//	Logger::TypedLog(CHN_LUA, "Replaced %zu instances of '%s' with '%s' in %s \n",
				//		count, searchStr.c_str(), replaceStr.c_str(), filename);
				//}
				return count;
				};

			// Lambda function for platform-specific image replacements
			auto replacePlatformImage = [&](const std::string& platform, const std::string& originalImage, const std::string& newImage) -> size_t {
				// Pattern to match: if vint_get_avg_processing_time("INPUT") == "PLATFORM" then
				//                   image = "original_image"
				std::string searchPattern = "if vint_get_avg_processing_time(\"INPUT\") == \"" + platform + "\" then\n\t\timage = \"" + originalImage + "\"";
				std::string replacePattern = "if vint_get_avg_processing_time(\"INPUT\") == \"" + platform + "\" then\n\t\timage = \"" + newImage + "\"";

				size_t count = 0;
				size_t pos = 0;

				// Try the exact pattern first
				while ((pos = finalContent.find(searchPattern, pos)) != std::string::npos) {
					finalContent.replace(pos, searchPattern.length(), replacePattern);
					pos += replacePattern.length();
					count++;
					modified = true;
				}

				// If no exact matches, try a more flexible approach
				if (count == 0) {
					// Look for the platform condition and then find the image assignment
					std::string platformCondition = "vint_get_avg_processing_time(\"INPUT\") == \"" + platform + "\"";
					pos = 0;

					while ((pos = finalContent.find(platformCondition, pos)) != std::string::npos) {
						// Look for "image = " after this condition
						size_t imageStart = finalContent.find("image = \"", pos);
						if (imageStart != std::string::npos) {
							// Check if this image assignment is within reasonable distance (same if block)
							size_t distanceCheck = imageStart - pos;
							if (distanceCheck < 200) { // Reasonable distance for same block

								size_t imageValueStart = imageStart + 9; // length of "image = \""
								size_t imageValueEnd = finalContent.find("\"", imageValueStart);

								if (imageValueEnd != std::string::npos) {
									std::string currentImage = finalContent.substr(imageValueStart, imageValueEnd - imageValueStart);

									// Check if this is the image we want to replace
									if (currentImage == originalImage) {
										finalContent.replace(imageValueStart, currentImage.length(), newImage);
										count++;
										modified = true;
										pos = imageValueStart + newImage.length();
									}
									else {
										pos = imageValueEnd;
									}
								}
								else {
									pos = imageStart + 9;
								}
							}
							else {
								pos += platformCondition.length();
							}
						}
						else {
							pos += platformCondition.length();
						}
					}
				}

				//if (count > 0) {
				//	Logger::TypedLog(CHN_LUA, "Replaced %zu instances of '%s' with '%s' for platform '%s' in %s \n",
				//		count, originalImage.c_str(), newImage.c_str(), platform.c_str(), filename);
				//}

				return count;
				};

			performReplacement("get_platform()", "vint_get_avg_processing_time(\"INPUT\")");
			performReplacement("ui_ctrl_PC_btn_enter", "ui_ctrl_PC_key_enter");
			performReplacement("SUI_PLATFORM ==", "vint_get_avg_processing_time(\"INPUT\") ==");
			replacePlatformImage("PC", "ui_ctrl_360_dpad_lr", "ui_ctrl_pc_dpad_lr_juiced");
			replacePlatformImage("PC", "ui_ctrl_360_dpad_up", "ui_ctrl_pc_dpad_up_juiced");
		}
		if (modified) {
			size_t newSize = finalContent.length();
			currentModifiedBuffer_general_lua = new char[newSize + 1]; // +1 for null terminator
			memcpy(currentModifiedBuffer_general_lua, finalContent.c_str(), newSize);
			currentModifiedBuffer_general_lua[newSize] = '\0';

			// Update the context
			ctx.eax = (DWORD)currentModifiedBuffer_general_lua;
			//ctx.edx = newSize;
			ctx.ecx = newSize;

		}
	}
	SafetyHookMid luaLoadBuffHook;
	void VINT_DOC_luaLoadBuff(safetyhook::Context32& ctx) {
		const char* buff = (const char*)ctx.ebp;
		const char* filename = (const char*)(ctx.esp + 0x14);
		size_t& sz = ctx.ecx;
	
		std::string convertedBuff(buff,sz);
#if !JLITE
		int* resX = (int*)(0xE8DF14);
		int* resY = (int*)(0xE8DF4C);

		patchedRes = std::to_string(resX[13]) + "x" + std::to_string(resY[13]);
		std::string searchAA = "adv_antiali_slider_values \t\t\t= { [0] = { label = \"CONTROL_NO\" }, [1] = { label = \"2x\" },\t\t\t\t[2] = { label = \"4x\" },\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\tnum_values = 3, cur_value = 0 }";
		std::string newAA = "adv_antiali_slider_values = { [0] = { label = \"CONTROL_NO\" }, [1] = { label = \"2x\" }, [2] = { label = \"4x\" }, [3] = { label = \"8x\" }, num_values = 4, cur_value = 0 }";
		// removed unnecessary tabs and spaces to make the extra label fit in without breaking the buffer
		std::string sLibSuperUI =
			"audio_play(\"SYS_RACE_FAIL\")\n\t"
			"local error_message = \"attempted to read undefined global variable '\"..k..\"'\"\n\t"
			"debug_print(error_message..\"\\n\")\n\t"
			"mission_help_table(\"[format][color:red]\"..tostring(error_message)..\"[/format]\")\n\t"
			"error(error_message)";

		std::string blankLib(sLibSuperUI.length(), ' ');

		if (buff) {
			for (int i = 0; i < 14; ++i) { // parses the hardcoded array to check if your current resolution exists in it
				if (userResX == resX[i] && userResY == resY[i]) {
					resFound = true;
					break;
				}
			}

			if (!resFound) {
				resX[13] = userResX;
				resY[13] = userResY;
			}

			replace_all(convertedBuff, searchAA, newAA);
			replace_all(convertedBuff, "Fullscreen_Antialiasing", "MSAA                   "); // extra spaces for padding otherwise it'll break the buffer
			replace_all(convertedBuff, "2048x1536", patchedRes); // easier to do it this way than to only patch if the user's res isn't found
			replace_all(convertedBuff, sLibSuperUI, blankLib); // fixes the error logger from SuperUI in system_lib.lua from crashing our executor, if nclok fixes it we'll get rid of this

			if (*(BYTE*)(0xE8C470) == 0) { // only patch these if the game's running in English
				replace_all(convertedBuff, "MENU_BLUR\",\t\t", "Pause Blur\",\t");
				replace_all(convertedBuff, "MENU_DEPTH_OF_FIELD", "Depth of Field     ");
				replace_all(convertedBuff, "ANISOTROPY_FILTERING\",\t\t", "Anisotropic Filtering\",\t");
				replace_all(convertedBuff, "CONTROLS_MINIMAP_VIEW", "Minimap View         ");
				replace_all(convertedBuff, "MENU_VSYNC\",\t\t\t\t\t\t", "Fullscreen VSync\",");
				replace_all(convertedBuff, "Shadow_Maps", "Shadows    ");
			}

			size_t& sz = ctx.edx;
			sz = convertedBuff.length();

			strncpy(const_cast<char*>(buff), convertedBuff.c_str(), sz);
			const_cast<char*>(buff)[sz] = '\0';
		}
#endif
		bool needBufferMod = Render2D::UltrawideFix
#if !JLITE
			|| Render2D::IVRadarScaling || allowJuicedAPI
#endif
			|| (strcmp(filename, "pause_menu.lua") == 0 && !InGameConfig::g_sliders.empty());
		{
			// Clean up previous buffer if it exists (regardless of which file it was for)
			if (needBufferMod) {
				// Clean up previous buffer
				if (currentModifiedBuffer != nullptr) {
					delete[] currentModifiedBuffer;
					currentModifiedBuffer = nullptr;
				}
				// Start with the current buffer state
				const char* currentBuff = buff;
				bool modified = false;


				std::string finalContent;
				finalContent = std::string(currentBuff, sz);
#if !RELOADED
				if (allowJuicedAPI) {
					bool externallyModified = false;

					for (auto& callback : g_VintluaHooksAPI) {
						if (callback) {
							if (callback(filename, finalContent, finalContent.length())) {
								externallyModified = true;
							}
						}
					}

						modified = externallyModified;
					
				}
#endif
				std::string customCode = "";
				// remove .lua
				std::string cached_str = filename;
				size_t dotPosition = cached_str.find(".lua");
				if (dotPosition != std::string::npos) {
					cached_str = cached_str.substr(0, dotPosition);
				}

				const char* lua_command = "vint_set_property(vint_object_find(\"%s\", 0, vint_document_find(\"%s\")), \"%s\", %f, %f)";
				char buffer[512];
#if !JLITE
				if (Render2D::IVRadarScaling) {
					if (cached_str == "hud") {
						snprintf(buffer, sizeof(buffer), lua_command, "map_grp", cached_str.c_str(), "scale",
							Render2D::RadarScale, Render2D::RadarScale);
						customCode += "\n";
						customCode += buffer;

						snprintf(buffer, sizeof(buffer), lua_command, "map_grp", cached_str.c_str(), "anchor",
							50.f, 710.f);
						customCode += "\n";
						customCode += buffer;
					}
					else if (cached_str == "hud_msg") {
						snprintf(buffer, sizeof(buffer), lua_command, "msg_diversion_anchor", cached_str.c_str(), "scale",
							Render2D::RadarScale, Render2D::RadarScale);
						customCode += "\n";
						customCode += buffer;

						snprintf(buffer, sizeof(buffer), lua_command, "msg_diversion_anchor", cached_str.c_str(), "anchor",
							75.f, 520.f);
						customCode += "\n";
						customCode += buffer;
					}
					else if (cached_str == "mp_hud") {
						snprintf(buffer, sizeof(buffer), lua_command, "perk_grp", cached_str.c_str(), "scale",
							Render2D::RadarScale, Render2D::RadarScale);
						customCode += "\n";
						customCode += buffer;

						snprintf(buffer, sizeof(buffer), lua_command, "perk_grp", cached_str.c_str(), "anchor",
							50.f, 710.f);
						customCode += "\n";
						customCode += buffer;
					}
					if (!customCode.empty()) {
						finalContent += customCode;
						modified = true;
					}
				}

#endif
				if (Render2D::UltrawideFix) {
					snprintf(buffer, sizeof(buffer), lua_command, "safe_frame", cached_str.c_str(), "anchor",
						(Render2D::get_vint_x_resolution() - 1280) / 2.f, 0.f);
					customCode += "\n";
					customCode += buffer;

					// Weird stuff on the screen you have to remove, also mayhem is re-stretched back.
					if (cached_str == "hud") {
						using namespace Render2D;
						char extraBuffer[512];

						snprintf(extraBuffer, sizeof(extraBuffer), lua_command, "extra_homie", "hud", "anchor",
							(get_vint_x_resolution() - 1280) / 2.f, -500.f);
						customCode += "\n";
						customCode += extraBuffer;

						snprintf(extraBuffer, sizeof(extraBuffer), lua_command, "mp_snatch_john", "hud", "anchor",
							(get_vint_x_resolution() - 1280) / 2.f, -500.f);
						customCode += "\n";
						customCode += extraBuffer;

						snprintf(extraBuffer, sizeof(extraBuffer), lua_command, "health_mini_grp", "hud", "anchor",
							(get_vint_x_resolution() - 1280) / 2.f, -500.f);
						customCode += "\n";
						customCode += extraBuffer;

						snprintf(extraBuffer, sizeof(extraBuffer), lua_command, "health_large_grp", "hud", "anchor",
							(get_vint_x_resolution() - 1280) / 2.f, -500.f);
						customCode += "\n";
						customCode += extraBuffer;

						snprintf(extraBuffer, sizeof(extraBuffer), lua_command, "mayhem_grp", "hud", "anchor",
							-((get_vint_x_resolution() - 1280) / 2.f), 0.f);
						customCode += "\n";
						customCode += extraBuffer;

						float weirdscale = 1.f / (Render2D::widescreenvalue / *Render2D::currentAR);
						snprintf(extraBuffer, sizeof(extraBuffer), lua_command, "mayhem_grp", "hud", "scale",
							weirdscale, 1.f);
						customCode += "\n";
						customCode += extraBuffer;

						snprintf(extraBuffer, sizeof(extraBuffer), lua_command, "vignettes", "hud", "scale",
							weirdscale, 1.f);
						customCode += "\n";
						customCode += extraBuffer;

						// This here is a LUA-less vint_document, we'll scale in hud's LUA file.
						snprintf(extraBuffer, sizeof(extraBuffer), lua_command, "safe_frame", "vignette", "scale",
							weirdscale, 1.f);
						customCode += "\n";
						customCode += extraBuffer;

						snprintf(extraBuffer, sizeof(extraBuffer), lua_command, "vignettes", "hud", "anchor",
							-((get_vint_x_resolution() - 1280) / 2.f), 0.f);
						customCode += "\n";
						customCode += extraBuffer;
					}
				}
				// If we have code to add
				if (!customCode.empty()) {
					finalContent += customCode;
					modified = true;
				}
				// Check if we need to apply slider patches
				if (strcmp(filename, "pause_menu.lua") == 0 && !InGameConfig::g_sliders.empty()) {
					if (!modified) {
						// If we haven't created finalContent yet, do it now
						finalContent = std::string(currentBuff, sz);
					}

					if (GameConfig::GetValue("Debug", "PopulateInGameOptions", 1)) {
						bool sliderModified = InGameConfig::PatchSliderContent(finalContent, filename);
						if (sliderModified) {
							modified = true;
						}
					}
				}

				// If any modifications were made, create a new buffer
				if (modified) {
					size_t newSize = finalContent.length();
					currentModifiedBuffer = new char[newSize + 1]; // +1 for null terminator
					memcpy(currentModifiedBuffer, finalContent.c_str(), newSize);
					currentModifiedBuffer[newSize] = '\0';

					// Update the context
					ctx.ebp = (DWORD)currentModifiedBuffer;
					//ctx.edx = newSize;
					ctx.ecx = newSize;

					//Logger::TypedLog(CHN_LUA, "Applied combined modifications to %s", filename);
				}
			}
		}
	}
	
	SafetyHookMid cleanupBufferHook;
	void CleanupModifiedScript() {
		if (currentModifiedBuffer != nullptr) {
			delete[] currentModifiedBuffer;
			currentModifiedBuffer = nullptr;
		}
	}
	void CleanupModifiedScript_general() {
		if (currentModifiedBuffer_general_lua != nullptr) {
			delete[] currentModifiedBuffer_general_lua;
			currentModifiedBuffer_general_lua = nullptr;
		}
	}
	bool __declspec(naked) VintGetGlobalBool(const char* Name)
	{
		_asm {
			push ebp
			mov ebp, esp
			sub esp, __LOCAL_SIZE


			mov ecx, ds: dword ptr[0x252A1B8] // Vint State
			mov eax, Name
			mov edx, 0xCDD760
			call edx

			mov esp, ebp
			pop ebp
			ret
		}
	}

	bool __declspec(naked) VintSetGlobalBool(const char* Name, bool Value)
	{
		_asm {
			push ebp
			mov ebp, esp
			sub esp, __LOCAL_SIZE


			mov eax, ds: dword ptr[0x252A1B8] // Vint State
			push eax
			push Value
			push Name
			mov edx, 0xCDD610
			call edx

			mov esp, ebp
			pop ebp
			ret
		}
	}

	int __declspec(naked) LuaExecute(const char* command)
	{
		_asm {
			push ebp
			mov ebp, esp
			sub esp, __LOCAL_SIZE


			mov esi, ds:0x0252983C // Lua State
			mov eax, command
			mov edx, 0xCDA000
			call edx

			mov esp, ebp
			pop ebp
			ret
		}
	}

	int __declspec(naked) VintExecute(const char* command)
	{
		_asm {
			push ebp
			mov ebp, esp
			sub esp, __LOCAL_SIZE

			mov esi, ds : 0x0252A1B8 // Vint State
			mov eax, command
			mov edx, 0xCDA000
			call edx


			mov esp, ebp
			pop ebp
			ret
		}
	}

	bool IsInSaveMenu() {
		VintExecute("is_in_save_menu = Menu_active and Menu_active.is_save_menu or false");
		return VintGetGlobalBool("is_in_save_menu");
	}

	void __declspec(naked) MSAA()
	{
		static int jmp_continue = 0x007737E4;
		__asm {
			mov ds : dword ptr[0x252A2DC], 0
			sub eax, 1
			jz MSAA8
			jmp jmp_continue

			MSAA8 :
			mov ds : dword ptr[0x252A2DC], 8
				jmp jmp_continue
		}
	}

	BOOL __declspec(naked) ValidCharFix()
	{
		static int jmp_continue = 0x0075C8D5;
		static int jmp_xor = 0x0075C8E7;

		__asm {
			mov ax, [esp + 4]
			cmp ax, 0x20
			jb short skip
			jmp jmp_continue

			skip :
			jmp jmp_xor
		}
	}

	void __declspec(naked) MouseFix()
	{
		static int jmp_continue = 0x00C1F4F2;
		__asm {
			mov ds : dword ptr[0x234F460], eax
			mov ds : dword ptr[0x0347B2F4], eax // reset the missing old delta to fix ghost scrolling when tabbing in and out of the game
			mov ds : dword ptr[0x0234F473], eax // reset left mouse button
			mov ds : dword ptr[0x0234F483], eax // reset right mouse button
			jmp jmp_continue
		}
	}

	static float X_divisor = 30.f;
	// Not a proper fix but final outcome should look like this lol, ideally find out why vehicles fuck up the sens when reading
	void WorkAroundHorizontalMouseSensitivity() {
		BYTE PlayerStatus = *(BYTE*)0x00E9A5BC;
		enum status {
			vehicle = 3,
			boat = 5,
			helicopter = 6,
			plane = 8,
		};
		switch (PlayerStatus) {
		case vehicle:
		case boat:
		case helicopter:
		case plane:
			X_divisor = 10.f;
			break;
		default:
			X_divisor = 30.f;
			break;
		}


	}
	void __declspec(naked) WorkAroundHorizontalMouseSensitivityASMHelper() {
		static int jmp_continue = 0x00C13720;

		__asm {
			pushad
			pushfd
		}
		__asm {
			call WorkAroundHorizontalMouseSensitivity
		}
		__asm {
			popfd
			popad
			fdiv ds : X_divisor
			jmp jmp_continue
		}

	}
	CPatch CFixHorizontalMouseSensitivity = CPatch::WriteRelJump(0x00C1371A, (uintptr_t)&WorkAroundHorizontalMouseSensitivityASMHelper);
	void __declspec(naked) StoreNPCPointer()
	{
		static int jmp_continue = 0x0098E498;
		__asm {
			mov		ecx, 0x9CFCE0
			call	ecx
			mov		CurrentNPC, eax
			jmp		jmp_continue
		}
	}

	void __declspec(naked) SpawningCheck()
	{
		static int jmp_skip = 0x0098EE3D;
		static int jmp_continue = 0x0098EE11;

		__asm {
			cmp		IsSpawning, 0
			jnz		skip
			mov		eax, [esi + 3132]
			jmp		jmp_continue

			skip :
			mov		edx, [esi + 68]
				jmp		jmp_skip
		}
	}

	int dirExists(const char* const path)
	{
		struct stat info;

		int statRC = stat(path, &info);
		if (statRC != 0)
		{
			if (errno == ENOENT) { return 0; } // something along the path does not exist
			if (errno == ENOTDIR) { return 0; } // something in path prefix is not a dir
			return -1;
		}

		return (info.st_mode & S_IFDIR) ? 1 : 0;
	}

	void __declspec(naked) RestoreFiltering()
	{
		static int jmp_continue = 0x00515974;
		__asm {
			fstp st(1)
			push esp
			add dword ptr[esp], 8
			fstp st
			jmp jmp_continue
		}
	}

	void __declspec(naked) AddVintLib()
	{
		static int Continue = 0x00B91553;
		static int VintLib = 0x00E4CEA8;
		static int LoadLib = 0xCDDF30;

		__asm {
			mov ecx, ds: 0x252A1B8
			push VintLib
			mov esi, ecx
			call LoadLib
			add esp, 4
			mov ecx, ds : 0x252A1B8
			push JVLib
			mov esi, ecx
			call LoadLib
			jmp Continue
		}
	}

	void __declspec(naked) AddStrings()
	{
		static int Continue = 0x007F46F0;
		static int StringsLoad = 0x7F4780;

		// loading our strings first which lets not only add new ones but "overwrite" original ones too

		__asm {
			push 0
			mov ecx, JStr
			call StringsLoad
			add esp, 4
			push 0
			mov ecx, dword ptr[0x00E26090]
			call StringsLoad
			jmp Continue
		}
	}

	__declspec(naked) wchar_t* RequestString(const wchar_t* Dest, const char* Label) { // first arg is used to format an existing wchar string, pass nullptr if you just want to get a string (use pushad and popad)
		__asm {
			push ebp
			mov ebp, esp
			sub esp, __LOCAL_SIZE

			mov		edi, Dest
			mov		esi, Label
			mov     ecx, 0x7F49E0
			call    ecx

			mov esp, ebp
			pop ebp
			ret
		}
	}
	CMultiPatch CMPatches_TervelTextureCrashWorkaround_be_as_pe = {

		[](CMultiPatch& mp) {
			mp.AddWriteRelCall(0x00C0900D,(uintptr_t)TextureCrashFix);
		},

		[](CMultiPatch& mp) {
			mp.AddWriteRelCall(0x00C08493,(uintptr_t)TextureCrashFix);
		},
	};
	std::function<void()> D3D9_create = nullptr;

	SAFETYHOOK_NOINLINE void CreateD3D9DeviceFunction(safetyhook::Context32& ctx) {
	
		Render3D::ChangeShaderOptions();
	}

	void LoadSaveSetPos(SafetyHookContext& ctx) {
		vector3 QuickSavePos = *(vector3*)(ctx.eax + 0x38C40);
		matrix QuickSaveOrient = *(matrix*)(ctx.eax + 0x38C52);
		if (!QuickSavePos.isNull()) {
			*(vector3*)(ctx.esp + 0x20) = QuickSavePos;
			memcpy((void*)ctx.ecx, (void*)&QuickSaveOrient, 0x24);
			ctx.eip = 0x006938F0;
		}
	}

	void SaveCurrentPos(SafetyHookContext& ctx) {
		if (IsQuickSaving) {
			vector3* QuickSavePos = (vector3*)(ctx.ebx + 0x38C40);
			matrix* QuickSaveOrient = (matrix*)(ctx.ebx + 0x38C52);
			UtilsGlobal::GetPlayerXYZ(QuickSavePos);
			UtilsGlobal::GetPlayerOrient(QuickSaveOrient);
		}
	}

	void NewSave() {
		if (*GameLoaded) {
			vint_message_struct SaveMessage;
			*(bool*)(0x252740E) = 1; // ins. fraud sound
			if (*InMission == 0 && !(*InCutscene) && !*LobbyCheck == 0 && *CurrentGamemode == -1) {
				Game::HUD::vint_message(L"JUICED: Quicksaving...", &SaveMessage);
				IsQuickSaving = true;
				((void(__cdecl*)(int*, bool, bool))0x695A60)((int*)0x1F7A9C0, false, false);
				IsQuickSaving = false;
			}
			else {
				Game::HUD::vint_message(L"JUICED: Unable to quicksave.", &SaveMessage);
			}
		}
	}
	// Fixes the Low Gravity cheat ignoring if you have the no fall damage unlockable, stripping it away due to the float being saved in the save file.
// Here we check if the reward is unlocked and thus setting it to 0.f, using ASM isn't required I just wanted a singular function handler for both apply/restore
	void __declspec(naked) LowGravity_cheat_fix_basejumping() {
		__asm {
			fstp dword ptr[eax + 0x18B4]
		}
		int player;
		__asm
		mov player, eax
			if (*(bool*)0x027DD27C)
				*(float*)(player + 0x18B4) = 0.f; // This is the damage multiplier or something.
		__asm {
			mov esp, ebp
			pop ebp
			ret
		}

	}
	void TopWinMain() {
		allowJuicedAPI = GameConfig::GetValue("API", "JuicedAPI", 1);
		bitmap_loader::Init();
		WriteRelJump(0x00685858, (UInt32)&LowGravity_cheat_fix_basejumping); // LowGravity_Apply()
		WriteRelJump(0x00685EE0, (UInt32)&LowGravity_cheat_fix_basejumping); // LowGravity_Restore()
		patchNop((BYTE*)0x004D6795, 5); // Fix for the sun flare disappearing upon reloading a save. Prevents the game from deallocating the flare.
		if(GameConfig::GetValue("Debug","AllowMultipleSR2Windows",1)) // in case this fucks up or something
		SafeWrite8(0x00BFA6B6, 0xEB);
		static SafetyHookMid LoadPosHook = safetyhook::create_mid(0x006938EB, &LoadSaveSetPos);
		static SafetyHookMid SavePosHook = safetyhook::create_mid(0x00695BBF, &SaveCurrentPos);
		static SafetyHookMid D3D9Create = safetyhook::create_mid(0x00D1F7B0, &CreateD3D9DeviceFunction);
		WriteRelJump(0x007F46E4, (UInt32)&AddStrings); // add custom string loading - the game automatically appends the string so it will load the right string file based on your language, eg - juiced_us.le_strings
#if !JLITE
		WriteRelJump(0x00B91541, (UInt32)&AddVintLib); // allows us to add our own side lib for vint to add new global variables without messing up mod support
		WriteRelJump(0x007787D0, (UInt32)&ChangeSOCallback); // replace the save overwrite callback with ours to avoid various warnings
		WriteRelJump(0x0077952F, (UInt32)&DeletionModeCheck); // avoid being able to "delete" when hovering over save new game
		WriteRelJump(0x007787FB, (UInt32)&ReplaceSOMessage); // replace the save overwrite warning message
		WriteRelJump(0x007788BE, (UInt32)&SkipSaving); // skip saving if deletion mode is enabled
		WriteRelJump(0x0068CAA0, (UInt32)&CutscenePauseWorkaround); // we need to make the cutscene process(?) function run even if the game's paused, original if check is dumb
		WriteRelJump(0x006D8E0A, (UInt32)&CutscenePauseCheck); // editing one of the ifs to prevent cutscenes from getting updated when paused
#endif
		if (GameConfig::GetValue("Misc", "DefaultToNativeRes", 1)) {
			SafeWrite32(0x774112 + 6, userResX);
			SafeWrite32(0x77411C + 6, userResY);
		}
		Logger::TypedLog(CHN_DLL, "SetProcessDPIAware result: %s\n", SetProcessDPIAware() ? "TRUE" : "FALSE");
#if !RELOADED
		/*if (FileExists("gotr.txt"))
			modpackread = 1;
			*/
#endif
		if (GameConfig::GetValue("Debug", "FixDefaultTexture", 1)) {
			Logger::TypedLog(CHN_DLL, "Fixing Default Texture\n");
			patchDWord((void*)0x00C08873, (int)"always_loaded"); // patches to correct the missing texture on PC
			patchDWord((void*)0x00C088F8, (int)"missing-grey.tga");
		}
		patchNop((BYTE*)0x0052598D, 6); // fix for black water in the distance with AA disabled
		patchNop((BYTE*)0x005267F0, 6); // fix for black water in the distance with AA enabled
		//patchDWord((void*)(0x007ECA66 + 1), (int)"PS3"); // patch get_platform to return ps3. Not ideal.
		//patchDWord((void*)(0x0051F62F + 1), (int)"PS3bitmap_sheets"); // patch get_platform to return ps3. Not ideal.
		SafeWrite32(0x004CBFEE + 2, (UInt32)0xE84380); // change the motion blur to directly read the current frametime (fix strength above 30 fps)
		patchBytesM((BYTE*)0x004CBFF4, (BYTE*)"\xEB\x13", 2); // jump over the stupid checks
		patchBytesM((BYTE*)0x0053818F, (BYTE*)"\xA1\x94\x89\xE9\x00", 5); // make shadow maps check shadows instead of shadow map type
		patchBytesM((BYTE*)0x00538194, (BYTE*)"\x83\xE8\x02", 3); // make it check if full shadows are enabled (so none = no shadows, simple = stencil and full = stencil & s. maps)
		WriteRelJump(0x0077377E, (UInt32)&ShadowsFix); // force full stencil shadows with the simple setting
		patchNop((BYTE*)0x006C5FE0, 10); // fix cutscenes resetting shadows
		patchNop((BYTE*)0x0073C01B, 6); // remove the command check from the level function
		patchCall((void*)0x00458646, (void*)IdleFix); // prevents you from being able to use the scroll wheel when idling
		patchCall((void*)0x009A3D8E, (void*)IdleFix);
		if (GameConfig::GetValue("Debug", "TervelTextureCrashWorkaround_be_as_pe", 1)) { // cause i want to disable it for reasons -- Clippy95, dont include in config?
			CMPatches_TervelTextureCrashWorkaround_be_as_pe.Apply(); // incorrect behaviour

		}
#if !JLITE
		if (GameConfig::GetValue("Debug", "Hook_lua_load_dynamic_script_buffer", 1)) {
#endif
			cleanupBufferHook = safetyhook::create_mid(0x00CDE388, [](safetyhook::Context32& ctx) {
				General::CleanupModifiedScript();
				},safetyhook::MidHook::Default);
			luaLoadBuffHook = safetyhook::create_mid(0x00CDE379, &VINT_DOC_luaLoadBuff
);
			if (Input::EnableDynamicPrompts) {
				static auto cleanupBufferHook_general = safetyhook::create_mid(0xCD9FF7, [](safetyhook::Context32& ctx) {
					General::CleanupModifiedScript_general();
					}, safetyhook::MidHook::Default);
				static auto luaLoadBuffHookGeneral = safetyhook::create_mid(0xCD9FD9, &generalluaLoadBuff);
			}
			if (GameConfig::GetValue("Graphics", "FixUltrawideHUD", 1) == 1) {
				Logger::TypedLog(CHN_MOD, "Patching Ultrawide HUD %d \n", 1);
				using namespace Render2D;
				//SR2Ultrawide_hook = safetyhook::create_inline(0xD1C910, &SR2Ultrawide_HUDScale);
				// Lets 3:2 act as 4:3 at least..
				patchDouble((void*)0x00E5C080, 1.55f);
				WriteRelCall(0x00D1EF2F, (UInt32)&SR2Ultrawide_HUDScale);
				WriteRelCall(0x00D1F944, (UInt32)&SR2Ultrawide_HUDScale);
			}
#if !JLITE
		}
#endif
		Render2D::vint_create_process_hook = safetyhook::create_mid(0x00B8BCC6, &Render2D::create_process_hook,safetyhook::MidHook::StartDisabled);
		if (GameConfig::GetValue("Graphics", "FixUltrawideHUD", 1) >= 2) {
			Logger::TypedLog(CHN_MOD, "Patching Ultrawide HUD %d \n", 2);
			using namespace Render2D;
			// Lets 3:2 act as 4:3 at least..
			patchDouble((void*)0x00E5C080, 1.55f);
			WriteRelCall(0x00D1EF2F, (UInt32)&SR2Ultrawide_HUDScale);
			WriteRelCall(0x00D1F944, (UInt32)&SR2Ultrawide_HUDScale);
			(void)vint_create_process_hook.enable();
		}
		// LUA EXECUTE
		patchBytesM((BYTE*)0x0075D5D6, (BYTE*)"\x68\x3A\x30\x7B\x02", 5);
		patchBytesM((BYTE*)0x0075D5B5, (BYTE*)"\x68\x3A\x30\x7B\x02", 5);

		WriteRelJump(0x007737DA, (UInt32)&MSAA); // 8x MSAA support; requires modded pause_menu.lua but won't cause issues without
		WriteRelJump(0x0075C8D0, (UInt32)&ValidCharFix); // add check for control keys to avoid pasting issues in the executor
		WriteRelJump(0x00C1F4ED, (UInt32)&MouseFix); // fix ghost mouse scroll inputs when tabbing in and out
		if (GameConfig::GetValue("Gameplay", "FixHorizontalMouseSensitivity", 1)) {
			//WriteRelJump(0x00C1371A, (UInt32)&WorkAroundHorizontalMouseSensitivityASMHelper); // attempt to fix Horizontal sens being 3x faster compared to vertical while on foot
			CFixHorizontalMouseSensitivity.Apply();
		}
		WriteRelJump(0x0098E493, (UInt32)&StoreNPCPointer);
		WriteRelJump(0x0098EE0B, (UInt32)&SpawningCheck);
		if (!dirExists("./shaderoverride")) {
			Logger::TypedLog(CHN_DLL, "We can patch 2x Filtering.\n");
			WriteRelJump(0x0051596E, (UInt32)&RestoreFiltering);
		}
		else {
			Logger::TypedLog(CHN_DLL, "Not patching 2x Filtering.\n");
		}
	}
	void BottomWinMain()
	{
		patchBytesM((BYTE*)0x009D3C70, (BYTE*)"\xD9\x05\x5A\x30\x7B\x02", 6); // TP X
		patchBytesM((BYTE*)0x009D3C83, (BYTE*)"\xD9\x05\x5E\x30\x7B\x02", 6); // TP Y
		patchBytesM((BYTE*)0x009D3CAE, (BYTE*)"\xD9\x05\x62\x30\x7B\x02", 6); // TP Z
		patchBytesM((BYTE*)0x00BE1B50, (BYTE*)"\xC3", 1); // return - avoid crashing from the unused broken debug console variable checker
		patchNop((BYTE*)0x009D3C65, 2); // nop out the command check so TP works without it


		patchBytesM((BYTE*)0x0068579B, (BYTE*)"\x6A\x06", 2);
		patchBytesM((BYTE*)0x006857CB, (BYTE*)"\x6A\x06", 2);
		patchBytesM((BYTE*)0x0068571F, (BYTE*)"\x6A\x06", 2);
		patchBytesM((BYTE*)0x0068574F, (BYTE*)"\x6A\x06", 2);
		patchBytesM((BYTE*)0x00685E12, (BYTE*)"\x6A\x06", 2);
		patchBytesM((BYTE*)0x00685E1E, (BYTE*)"\x6A\x06", 2);
		patchBytesM((BYTE*)0x00685DC7, (BYTE*)"\x6A\x06", 2);
		patchBytesM((BYTE*)0x00687C32, (BYTE*)"\x6A\x06", 2);
		patchBytesM((BYTE*)0x00687BC2, (BYTE*)"\x6A\x06", 2);
		patchBytesM((BYTE*)0x00687CAB, (BYTE*)"\x6A\x06", 2);
		// this should increase the stream priority for the character swap cheat - on 360, the loading times are much bigger so there are no issues there but here this might be needed
		patchNop((BYTE*)0x00684C84, 5); // get rid of the loading screen with the cheat, remove the nop if there are any issues but it should be fine?
	}
}