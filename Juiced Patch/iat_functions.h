#pragma once
#include "stdafx.h"

HRESULT PatchIat(HMODULE Module,PSTR ImportedModuleName,PSTR ImportedProcName,
				 PVOID AlternateProc,PVOID *OldProc);