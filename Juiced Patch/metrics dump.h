#pragma once
#include "Patcher\patch.h"
#include "FileLogger.h"

void patch_metrics();
void hook_metrics_dump();
char* hook_metrics_get_name(int index);
void hook_metrics_get_frame_time(int index, double* value_out);
