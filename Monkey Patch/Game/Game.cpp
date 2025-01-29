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