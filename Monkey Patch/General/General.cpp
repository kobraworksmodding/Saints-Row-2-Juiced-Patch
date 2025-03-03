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

namespace General {

	bool IsSpawning = false;
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

	typedef int __cdecl luaLoadBufferOrig_T(void* L, const char* buff, size_t sz, const char* name);
	luaLoadBufferOrig_T* luaLoadBufferOrig = (luaLoadBufferOrig_T*)(0xCDCFB0);

	int luaLoadBuff(void* L, const char* buff, size_t sz, const char* name) {

		__asm pushad

		std::string convertedBuff(buff);

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

			sz = convertedBuff.length();

			strncpy(const_cast<char*>(buff), convertedBuff.c_str(), sz);
			const_cast<char*>(buff)[sz] = '\0';

			__asm popad

			return luaLoadBufferOrig(L, buff, sz, name);
		}
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

	void TopWinMain() {
#if !JLITE
		WriteRelJump(0x0068CAA0, (UInt32)&CutscenePauseWorkaround); // we need to make the cutscene process(?) function run even if the game's paused, original if check is dumb
		WriteRelJump(0x006D8E0A, (UInt32)&CutscenePauseCheck); // editing one of the ifs to prevent cutscenes from getting updated when paused
#endif
	//patchCall((void*)0x00C080C0, (void*)TextureCrashFixDefinitive);
	//WriteRelJump(0x00C080E8, (UInt32)&TextureCrashFixRemasteredByGroveStreetGames);
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
		if (GameConfig::GetValue("Debug", "TextureCrashFix", 1)) { // cause i want to disable it for reasons -- Clippy95, dont include in config?
			patchCall((void*)0x00C0900D, (void*)TextureCrashFix); // WIP (unknown if it fixes it or not just yet)
			patchCall((void*)0x00C08493, (void*)TextureCrashFix);
		}
#if !JLITE
#if !RELOADED
		if (GameConfig::GetValue("Debug", "PatchPauseMenuLua", 1)) {
			patchCall((void*)0x00CD9FE8, (void*)luaLoadBuff); // used to intercept the pause menu lua before compiled, needed for full 8x MSAA support + custom res
		}
#endif
		// LUA EXECUTE
		patchBytesM((BYTE*)0x0075D5D6, (BYTE*)"\x68\x3A\x30\x7B\x02", 5);
		patchBytesM((BYTE*)0x0075D5B5, (BYTE*)"\x68\x3A\x30\x7B\x02", 5);

#endif 
		WriteRelJump(0x007737DA, (UInt32)&MSAA); // 8x MSAA support; requires modded pause_menu.lua but won't cause issues without
		WriteRelJump(0x0075C8D0, (UInt32)&ValidCharFix); // add check for control keys to avoid pasting issues in the executor
		WriteRelJump(0x00C1F4ED, (UInt32)&MouseFix); // fix ghost mouse scroll inputs when tabbing in and out
		if (GameConfig::GetValue("Gameplay", "FixHorizontalMouseSensitivity", 1)) {
			//WriteRelJump(0x00C1371A, (UInt32)&WorkAroundHorizontalMouseSensitivityASMHelper); // attempt to fix Horizontal sens being 3x faster compared to vertical while on foot
			CFixHorizontalMouseSensitivity.Apply();
		}
		//FixandImproveSlewMouseRuntimePatch();
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
		patchNop((BYTE*)0x004D6795, 5); // Fix for the sun flare disappearing upon reloading a save. Prevents the game from deallocating the flare.

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