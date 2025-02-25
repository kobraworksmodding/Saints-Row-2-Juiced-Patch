// Render2D.cpp (uzis, Tervel, Clippy95)
// --------------------
// Created: 25/02/2025

#include "../FileLogger.h"
#include "../Patcher/patch.h"
#include "../GameConfig.h"
#include "../SafeWrite.h"
#include "../UtilsGlobal.h"
#include "Render2D.h"

namespace Render2D
{

	bool BetterChatTest = 0;

	ChangeTextColorT ChangeTextColor = (ChangeTextColorT)0xD14840;

	int processtextwidth(int width) {
		float currentAR = *(float*)0x022FD8EC;
		return width * (currentAR / 1.77777777778f);

	}

	void __declspec(naked) InGamePrintASM(const char* Text, int x, int y, int font) {
		__asm {
			push ebp
			mov ebp, esp
			sub esp, __LOCAL_SIZE

			push edi
			push esi
			push eax

			mov edi, font
			mov esi, Text
			push x
			push y

			mov eax, 0xD15DC0
			call eax

			pop eax
			pop esi
			pop edi

			mov esp, ebp
			pop ebp
			ret
		}
	}

	void InGamePrint(const char* Text, int x, int y, int font) {
		int game_setting_language = *(int*)0x00E98AF8;
		switch (game_setting_language) {
		case LANG_JAPANESE:
		case LANG_CHINESE:

			font = 0; // Fixes crash for Japanese and Chinese SR2 and our InGamePrint
			break;
		}
		InGamePrintASM(Text, x, y, font);

	}

	typedef void SomeMMFunc_Native();
	SomeMMFunc_Native* UpdateSomeMMFunc = (SomeMMFunc_Native*)(0x0075B270);

	typedef void SomePMFunc_Native();
	SomePMFunc_Native* UpdateSomePMFunc = (SomePMFunc_Native*)(0x00B99DB0);
	void SomeMMFunc_Hacked()
	{
#if JLITE
		if (*(BYTE*)0x02527B75 == 1 && *(BYTE*)0xE8D56B == 1) {
			ChangeTextColor(160, 160, 160, 128);
			__asm pushad
			InGamePrint(("JUICED LITE " + std::string(UtilsGlobal::juicedversion)).c_str(), 680, processtextwidth(1070), 2);
			__asm popad
		}
#else
		if (*(BYTE*)0x02527B75 == 1 && *(BYTE*)0xE8D56B == 1) {
			ChangeTextColor(160, 160, 160, 128);
			__asm pushad
			InGamePrint(("JUICED " + std::string(UtilsGlobal::juicedversion)).c_str(), 680, processtextwidth(1120), 2);
			__asm popad
		}
#endif

		// Call original func
		return UpdateSomeMMFunc();
	}
	/*
	void SomePMFunc_Hacked()
	{

		if (menustatus(menustatus::pausemenu) || menustatus(menustatus::pausemenuphone) || menustatus(menustatus::pausemenuscroll2) || menustatus(menustatus::pausemenescroll1) || menustatus(menustatus::pausemenuphonebook)) {
			ChangeTextColor(160, 160, 160, 128);
			__asm pushad
			InGamePrint(("JUICED " + std::string(juicedversion)).c_str(), 680, 160, 6);
			__asm popad
		}

		// Call original func
		return UpdateSomePMFunc();
	}
	*/

	void InitMenVerNum() {
		if (GameConfig::GetValue("Debug", "MenuVersionNumber", 1))
		{
			Logger::TypedLog(CHN_MOD, "Patching MenuVersionNumber...\n");
			//patchCall((void*)0x0052050C, (void*)SomeMMFunc_Hacked);
			patchCall((void*)0x0073CE0D, (void*)SomeMMFunc_Hacked);
			//patchCall((void*)0x00B995D5, (void*)SomePMFunc_Hacked);
		}
	}

	void SetupBorderless()
	{
		int l_IsBorderless = GameConfig::GetValue("Graphics", "Borderless", 0);
		uint32_t window_style = l_IsBorderless ? (WS_POPUP) : (WS_CAPTION | WS_BORDER);
		patchDWord((void*)(0x00BFA35A + 4), window_style);
		patchBytesM((BYTE*)0x00BFA494, (BYTE*)"\x6A\x03", 2); //Maximise Borderless so it fits perfectly.
	}

	void SetupBetterWindowed()
	{
		int l_IsBetterWindowed = GameConfig::GetValue("Graphics", "BetterWindowed", 0);
		uint32_t windowed_style = l_IsBetterWindowed ? (WS_CAPTION | WS_OVERLAPPED | WS_SYSMENU | WS_DLGFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SIZEBOX) : (WS_DLGFRAME);
		patchDWord((void*)(0x00BFA35A + 4), windowed_style);
	}

	void Init() {
		if (GameConfig::GetValue("Graphics", "Borderless", 0))
		{
			SetupBorderless();
			Logger::TypedLog(CHN_DEBUG, "Enabling Borderless Windowed.\n");
		}
		else
		{
			SetupBetterWindowed();
			Logger::TypedLog(CHN_DEBUG, "Fixing Windowed Mode.\n");
		}

		if (GameConfig::GetValue("Gameplay", "BetterChat", 1)) // changes char limit from 64 to 128 and formats the input after the 64th character
		{
			BetterChatTest = 1;
			patchBytesM((BYTE*)0x0075C91E, (BYTE*)"\xC7\x05\x1C\x69\xF7\x01\x80\x00\00\x00", 10); // change chat char limit from 64 to 128
			patchBytesM((BYTE*)0x0075CCF7, (BYTE*)"\x6A\x82", 2);  // change chat print limit from 64 to 130 (extra 2 characters to account for formatted input with - and newline)
			patchBytesM((BYTE*)0x0075CDEA, (BYTE*)"\x68\xFF\x92\x20\x02", 5); // new chat read address for entered message
			Logger::TypedLog(CHN_DEBUG, "Enabling better chat...\n");
		}
	}
}