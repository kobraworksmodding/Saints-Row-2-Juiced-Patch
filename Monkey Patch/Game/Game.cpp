#include "../FileLogger.h"
#include "../Patcher/patch.h"
#include "../SafeWrite.h"
#include "../GameConfig.h"
#include "Game.h"
namespace Game
{
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
};