#pragma once
namespace DLC {
	void Init();
}

struct VehiclePadding {
	unsigned char Padding[2000];
};

extern bool DLCInstalled;
extern VehiclePadding* VehArr;