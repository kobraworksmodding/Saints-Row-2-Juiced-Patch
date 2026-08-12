#pragma once
#include "../Game/Game.h"
#include "../Ext/Hooking.Patterns.h"
namespace Reloaded
{
	extern void PatchTables();
	extern void Init();

#define CLANTAG_MAX 5 // do we need a max name define?.
}
