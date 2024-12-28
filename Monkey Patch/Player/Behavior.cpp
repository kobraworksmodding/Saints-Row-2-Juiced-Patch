// Player/Behavior.cpp (uzis, Tervel)
// --------------------
// Created: 13/12/2024

#include "../FileLogger.h"
#include "../Patcher/patch.h"
#include "../SafeWrite.h"

namespace Behavior
{
	void BetterMovement() 
	{
		// Majority of the SR2 movement sluggishness is due to the fact that certain walking anims add an
		// increased latency to walking generally and 180 anims tend to play constantly when trying to strafe, 180 anims didnt exist in SR1.
		Logger::TypedLog(CHN_MOD, "Patching In Better Movement Behavior...\n");
		patchNop((BYTE*)0x00E9216C, 3); // Jog180
		patchNop((BYTE*)0x00E9213C, 3); // Stand180
		patchNop((BYTE*)0x00E92130, 3); // WalkStop
		patchNop((BYTE*)0x00E9237C, 3); // StandToRun
		patchNop((BYTE*)0x00E92370, 3); // RunToStop
		patchNop((BYTE*)0x00E92364, 3); // WalkToStop
		patchNop((BYTE*)0x00E92394, 3); // WalkToStand
		patchNop((BYTE*)0x00E92388, 3); // StandToWalk
	}

	void SR1Reloading()
	{
		//This fixes the functionality to be able to reload while sprinting.
		Logger::TypedLog(CHN_MOD, "Patching In ReloadDuringSprint...\n");
		patchNop((BYTE*)0x009F1A9C, 5);
		patchNop((BYTE*)0x009F1ACE, 5);
		patchByte((BYTE*)0x009F1A4C, 0x75);
	}

	void SR1QuickSwitch()
	{
		// Fixes broken weapon wheel implementation and brings back quick switching.

		Logger::TypedLog(CHN_MOD, "Patching in Weapon Quick Switching...\n");
		patchNop((BYTE*)0x0079266D, 6);
		patchNop((BYTE*)0x004F943E, 14);
		patchNop((BYTE*)0x00797003, 5);
	}

	void __declspec(naked) TauntLeft()
	{
		static int jmp_continue = 0x004F8323;
		__asm {
			push 1
			push 1
			push 0
			push - 1
			push - 1
			push 1
			push 1
			push 1
			jmp jmp_continue
		}
	}

	void __declspec(naked) TauntRight()
	{
		static int jmp_continue = 0x004F833F;
		__asm {
			push 1
			push 1
			push 0
			push - 1
			push - 1
			push 1
			push 1
			push 0
			jmp jmp_continue
		}
	}

	void TauntCancelling()
	{
		// Makes it so you can cancel out Taunts. 
		Logger::TypedLog(CHN_MOD, "Patching In TauntCancelling...\n");
		WriteRelJump(0x004F8315, (UInt32)&TauntLeft);
		WriteRelJump(0x004F8332, (UInt32)&TauntRight);
		patchBytesM((BYTE*)0x00964F77 + 1, (BYTE*)"\x00\x00", 2);
	}

	void WeaponJam()
	{
		Logger::TypedLog(CHN_MOD, "Patching In UseWeaponAfterEmpty...\n");
		patchByte((BYTE*)0x9D95F0, 0xC3);
		patchNop((BYTE*)0x0055B496, 2);
	}
}