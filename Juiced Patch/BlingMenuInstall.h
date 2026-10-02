
#pragma once
namespace BlingMenuInstall {

	extern void AddOptions();
}
#define BM_MakeCPatchFunction(funcName, patchVar, nameSpace) \
const char* BM_##funcName(void* userdata, int action) { \
    using namespace nameSpace; \
    return BM_GenericPatchFunction(patchVar, action); \
}

#define BM_MakeCPatchFunctionSaveConfig(funcName, patchVar, nameSpace, appName, keyName) \
const char* BM_##funcName(void* userdata, int action) { \
    using namespace nameSpace; \
    return BM_GenericPatchFunction(patchVar, action, appName, keyName); \
}


#define BM_MakeSafetyHookFunction(funcName, safetyhookVar, nameSpace) \
const char* BM_##funcName(void* userdata, int action) { \
    using namespace nameSpace; \
    return BM_GenericSafetyHookFunction(safetyhookVar, action); \
}

#define BM_MakeSafetyHookFunctionSaveConfig(funcName, safetyhookVar, nameSpace, appName, keyName) \
const char* BM_##funcName(void* userdata, int action) { \
    using namespace nameSpace; \
    return BM_GenericSafetyHookFunction(safetyhookVar, action, appName, keyName); \
}

