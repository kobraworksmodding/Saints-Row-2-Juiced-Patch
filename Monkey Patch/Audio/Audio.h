#pragma once
#include "..\Math\Math.h"
struct listener
{
	vector3 pos;
	vector3 vel;
	matrix orient;
	unsigned int district_in;
	unsigned int flags;
	int als;
};


namespace Audio
{

	extern void Init();
}