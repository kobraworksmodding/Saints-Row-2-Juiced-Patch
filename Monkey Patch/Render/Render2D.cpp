// Render2D.cpp (uzis, Tervel, Clippy95)
// --------------------
// Created: 25/02/2025

#include "../FileLogger.h"
#include "../Patcher/patch.h"
#include "../GameConfig.h"
#include "../SafeWrite.h"
#include "../UtilsGlobal.h"
#include "Render2D.h"
#include <safetyhook.hpp>
#include "..\General\General.h"
#include "Render3D.h"
#include "..\Ext\Hooking.Patterns.h"
namespace Render2D
{
	float* currentAR = (float*)0x022FD8EC;
	const float widescreenvalue = 1.777777791f;
	bool BetterChatTest = 0;
	void RadarScaling() {
		 static float scale = 0.87272727272f;
		// vint scale
		auto pattern = hook::make_range_pattern(0x7A2C30, 0x007A37D2, "AC BF 2D 02");
		Logger::TypedLog(CHN_DEBUG,"Count of vint %d \n", pattern.size());
		pattern.for_each_result([](hook::pattern_match match) {
			void* addr = match.get<void*>();
			SafeWrite32((UInt32)addr, (UInt32)&scale);
			Logger::TypedLog(CHN_DEBUG,"Match found at address: %p \n", addr);
			});
		// Height scale
		pattern = hook::make_range_pattern(0x7A2C30, 0x007A37D2, "A8 BF 2D 02");
		pattern.for_each_result([](hook::pattern_match match) {
			void* addr = match.get<void*>();
			SafeWrite32((UInt32)addr, (UInt32)&scale);
			Logger::TypedLog(CHN_DEBUG, "Match found at address: %p \n", addr);
			});

		// vint scale
		pattern = hook::make_range_pattern(0x79E970, 0x0079EB0A, "AC BF 2D 02");
		Logger::TypedLog(CHN_DEBUG, "Count of vint %d \n", pattern.size());
		pattern.for_each_result([](hook::pattern_match match) {
			void* addr = match.get<void*>();
			SafeWrite32((UInt32)addr, (UInt32)&scale);
			Logger::TypedLog(CHN_DEBUG, "Match found at address: %p \n", addr);
			});

		// Height scale
		pattern = hook::make_range_pattern(0x79E970, 0x0079EB0A, "A8 BF 2D 02");
		Logger::TypedLog(CHN_DEBUG, "Count of vint %d \n", pattern.size());
		pattern.for_each_result([](hook::pattern_match match) {
			void* addr = match.get<void*>();
			SafeWrite32((UInt32)addr, (UInt32)&scale);
			Logger::TypedLog(CHN_DEBUG, "Match found at address: %p \n", addr);
			});
		// vint scale
		pattern = hook::make_range_pattern(0x79EB10, 0x0079EDE3, "AC BF 2D 02");
		Logger::TypedLog(CHN_DEBUG, "Count of vint %d \n", pattern.size());
		pattern.for_each_result([](hook::pattern_match match) {
			void* addr = match.get<void*>();
			SafeWrite32((UInt32)addr, (UInt32)&scale);
			Logger::TypedLog(CHN_DEBUG, "Match found at address: %p \n", addr);
			});

		// Height scale
		pattern = hook::make_range_pattern(0x79EB10, 0x0079EDE3, "A8 BF 2D 02");
		Logger::TypedLog(CHN_DEBUG, "Count of vint %d \n", pattern.size());
		pattern.for_each_result([](hook::pattern_match match) {
			void* addr = match.get<void*>();
			SafeWrite32((UInt32)addr, (UInt32)&scale);
			Logger::TypedLog(CHN_DEBUG, "Match found at address: %p \n", addr);
			});

		// vint scale
		pattern = hook::make_range_pattern(0x00B877B0, 0x00B877D7, "AC BF 2D 02");
		Logger::TypedLog(CHN_DEBUG, "Count of vint %d \n", pattern.size());
		pattern.for_each_result([](hook::pattern_match match) {
			void* addr = match.get<void*>();
			SafeWrite32((UInt32)addr, (UInt32)&scale);
			Logger::TypedLog(CHN_DEBUG, "Match found at address: %p \n", addr);
			});

		// height scale
		pattern = hook::make_range_pattern(0x00B877B0, 0x00B877D7, "A8 BF 2D 02");
		Logger::TypedLog(CHN_DEBUG, "Count of vint %d \n", pattern.size());
		pattern.for_each_result([](hook::pattern_match match) {
			void* addr = match.get<void*>();
			SafeWrite32((UInt32)addr, (UInt32)&scale);
			Logger::TypedLog(CHN_DEBUG, "Match found at address: %p \n", addr);
			});

		// vint scale
		pattern = hook::make_range_pattern(0x79EB10, 0x0079EDE3, "AC BF 2D 02");
		Logger::TypedLog(CHN_DEBUG, "Count of vint %d \n", pattern.size());
		pattern.for_each_result([](hook::pattern_match match) {
			void* addr = match.get<void*>();
			SafeWrite32((UInt32)addr, (UInt32)&scale);
			Logger::TypedLog(CHN_DEBUG, "Match found at address: %p \n", addr);
			});

		// height scale
		pattern = hook::make_range_pattern(0x79EB10, 0x0079EDE3, "A8 BF 2D 02");
		Logger::TypedLog(CHN_DEBUG, "Count of vint %d \n", pattern.size());
		pattern.for_each_result([](hook::pattern_match match) {
			void* addr = match.get<void*>();
			SafeWrite32((UInt32)addr, (UInt32)&scale);
			Logger::TypedLog(CHN_DEBUG, "Match found at address: %p \n", addr);
			});

		// vint scale
		pattern = hook::make_range_pattern(0x79FEB0, 0x0079FFE5, "AC BF 2D 02");
		Logger::TypedLog(CHN_DEBUG, "Count of vint %d \n", pattern.size());
		pattern.for_each_result([](hook::pattern_match match) {
			void* addr = match.get<void*>();
			SafeWrite32((UInt32)addr, (UInt32)&scale);
			Logger::TypedLog(CHN_DEBUG, "Match found at address: %p \n", addr);
			});

		// vint scale
		pattern = hook::make_range_pattern(0x79FEB0, 0x0079FFE5, "A8 BF 2D 02");
		Logger::TypedLog(CHN_DEBUG, "Count of vint %d \n", pattern.size());
		pattern.for_each_result([](hook::pattern_match match) {
			void* addr = match.get<void*>();
			SafeWrite32((UInt32)addr, (UInt32)&scale);
			Logger::TypedLog(CHN_DEBUG, "Match found at address: %p \n", addr);
			});

		pattern = hook::make_range_pattern(0x5489F0, 0x00548C5C, "A? BF 2D 02");
		Logger::TypedLog(CHN_DEBUG, "Count of vint %d \n", pattern.size());
		pattern.for_each_result([](hook::pattern_match match) {
			void* addr = match.get<void*>();
			SafeWrite32((UInt32)addr, (UInt32)&scale);
			Logger::TypedLog(CHN_DEBUG, "Match found at address: %p \n", addr);
			});

	}
	ChangeTextColorT ChangeTextColor = (ChangeTextColorT)0xD14840;
	// What I use in BlingMenu to clamp mouse cursor, maybe a bit too much for Ultrawide? who's going to have an ultrawide under 720p?
	void get_vint_res_limit(float* x, float* y) {
		if (!x || !y) return;

		float currentX = (float)(*(unsigned int*)0x022F63F8);
		float currentY = (float)(*(unsigned int*)0x022F63FC);

		if (currentX <= 0 || currentY <= 0) return;

		float aspect_ratio = currentX / currentY;
		float targetX = 1280.0f;
		float targetY = 720.0f;

		if (currentX > targetX || currentY > targetY) {
			targetX = currentY * aspect_ratio;
			if (targetX > currentX) {
				targetX = currentX;
				targetY = currentX / aspect_ratio;
			}
		}
		else {
			targetX = currentX;
			targetY = currentY;
		}
		if (targetX < 1280.0f) targetX = 1280.0f;
		if (targetY < 720.0f) targetY = 720.0f;

		if (currentX < 1280.0f || currentY < 720.0f) {
			targetX = currentX;
			targetY = currentY;
		}

		*x = targetX;
		*y = targetY;
	}

	int processtextwidth(int width) {
		return width * (*currentAR / 1.77777777778f);

	}

	float get_vint_x_resolution() {
		
		return *currentAR * 720;
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
			Sleep(1);
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

	SafetyHookMid vint_create_process_hook;
	void create_process_hook(safetyhook::Context32& ctx) {
		const char* target_str = "safe_frame";
		uintptr_t* vint_document = (uintptr_t*)ctx.esi;
		const char* possible_str = reinterpret_cast<const char*>(&vint_document[10]);
		char cached_str[64];
		strncpy(cached_str, possible_str, sizeof(cached_str) - 1);
		cached_str[sizeof(cached_str) - 1] = '\0';
		char buffer[256];
		const char* lua_command = "vint_set_property(vint_object_find(\"%s\", 0, vint_document_find(\"%s\")), \"%s\", %f, %f)";
		snprintf(buffer, sizeof(buffer), lua_command,"safe_frame", cached_str,"anchor", (get_vint_x_resolution() - 1280) / 2.f,0.f);
		//Logger::TypedLog(CHN_DEBUG, "%s \n", possible_str);
		General::VintExecute(buffer);

		if (strcmp(cached_str, "hud") == 0) {

			snprintf(buffer, sizeof(buffer), lua_command, "extra_homie", "hud", "anchor", (get_vint_x_resolution() - 1280) / 2.f, -500.f);
			General::VintExecute(buffer);

			snprintf(buffer, sizeof(buffer), lua_command, "mp_snatch_john", "hud", "anchor", (get_vint_x_resolution() - 1280) / 2.f, -500.f);
			General::VintExecute(buffer);

			snprintf(buffer, sizeof(buffer), lua_command, "health_mini_grp", "hud", "anchor", (get_vint_x_resolution() - 1280) / 2.f, -500.f);
			General::VintExecute(buffer);


			snprintf(buffer, sizeof(buffer), lua_command, "health_large_grp", "hud", "anchor", (get_vint_x_resolution() - 1280) / 2.f, -500.f);
			General::VintExecute(buffer);
			snprintf(buffer, sizeof(buffer), lua_command, "mayhem_grp", "hud", "anchor", -((get_vint_x_resolution() - 1280) / 2.f), 0.f);
			General::VintExecute(buffer);

			float weirdscale = 1.f / (widescreenvalue / *currentAR);
			snprintf(buffer, sizeof(buffer), lua_command, "mayhem_grp", "hud", "scale", weirdscale, 1.f);
			General::VintExecute(buffer);
		}
	}

bool UltrawideFix = false;
// Clippy TODO, maybe handle 16:10?
std::thread RefreshHUD_thread;
void RefreshHUD_loop() {
	Logger::TypedLog(CHN_DEBUG, "SR2Ultrawide Refreshing HUD %d\n",2);
	vint_create_process_hook.enable();
	std::this_thread::sleep_for(std::chrono::seconds(4));
	vint_create_process_hook.disable();
}

char SR2Ultrawide_HUDScale() {
	Logger::TypedLog(CHN_DEBUG, "SR2Ultrawide Refreshing HUD %d\n", 1);
	float currentX = (float)(*(unsigned int*)0x022f63f8);
	float currentY = (float)(*(unsigned int*)0x022f63fc);
	char result;

	float aspectRatio = currentX / currentY;

	// Fucking tagging system cause yeah lets hard code the anchor for it?

	int var = (int)(aspectRatio * 720.f);
	static int var2;
	 var2 = (int)(aspectRatio * 360.f);
	SafeWrite32(0x00622571 + 1, var);
	SafeWrite32(0x00625A2B + 2, var);
	//SafeWrite32(0x00625F70 + 1, var);
	//SafeWrite32(0x00755A21 + 1, var);
	//SafeWrite32(0x00755C49 + 1, var);
	//SafeWrite32(0x00B87313 + 1, var2);
	//SafeWrite32(0x00B87313 + 1, var2);
	SafeWrite32(0x00625D09 + 2, (UInt32)&var2);
	SafeWrite32(0x0062597F + 2, (UInt32)&var2);
	Render3D::AspectRatioFix(true);


	if (aspectRatio >= 1.77) {
		Logger::TypedLog(CHN_DEBUG, "SR2Ultrawide Refreshing HUD %d\n", 3);
		RefreshHUD_thread = std::thread(RefreshHUD_loop);
		RefreshHUD_thread.detach();
	}
	if ((GameConfig::GetValue("Graphics", "FixUltrawideHUD", 1) == 1)) {
		if (aspectRatio <= 1.79777777778f) {
#if JLITE
			General::luaLoadBuffHook.disable();
#endif
			UltrawideFix = false;
			General::cleanupBufferHook.disable();
			General::CleanupModifiedScript();
			return ((char(*)())0xD1C910)(); // Original HUD scale function.
			
		}
		else {
#if JLITE
			General::luaLoadBuffHook.enable();
#endif
			Logger::TypedLog(CHN_DEBUG, "SR2Ultrawide Refreshing HUD %d\n", 4);
			General::cleanupBufferHook.enable();
			UltrawideFix = true;
		}
	}

	float correctionFactor = 1.777777777777778f / aspectRatio;

	float stretchedX = currentX / 1280.0f;
	float adjustedX = stretchedX * correctionFactor;

	if (aspectRatio <= 1.45f) {
		result = 0;
		*(uint8_t*)0x0213c383 = 0;
		*(uint8_t*)0x025272dd = 0;
		*(float*)0x022fdcc0 = adjustedX;
		*(float*)0x022fdcbc = currentY / 480.0f;
	}
	else {
		result = 1;
		*(uint8_t*)0x0213c383 = 1;
		*(uint8_t*)0x025272dd = 1;
		*(float*)0x022fdcc0 = adjustedX;
		*(float*)0x022fdcbc = currentY / 720.0f;
	}
	Logger::TypedLog(CHN_MOD, "SR2Ultrawide patched HUD scale X: %f Y: %f bool: %d \n", adjustedX, currentY / 720.0f, UltrawideFix);
	return result;
}
	void Init() {
		RadarScaling();
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