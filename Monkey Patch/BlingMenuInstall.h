#if !RELOADED
#pragma once
namespace BlingMenuInstall {

	extern void AddOptions();
}
#define BM_MakeCPatchFunction(funcName, patchVar, nameSpace) \
const char* BM_##funcName(void* userdata, int action) { \
    using namespace nameSpace; \
    if (action != -1) { \
        if (patchVar.IsApplied()) \
            patchVar.Restore(); \
        else \
            patchVar.Apply(); \
    } \
    switch (patchVar.IsApplied()) { \
    case false: return "OFF"; \
        break; \
    case true: return "ON "; \
        break; \
    } \
    return ERROR_MESSAGE; \
}

#define BM_MakeCPatchFunctionSaveConfig(funcName, patchVar, nameSpace, appName, keyName) \
const char* BM_##funcName(void* userdata, int action) { \
    using namespace nameSpace; \
    if (action != -1) { \
        if (patchVar.IsApplied()) \
            patchVar.Restore(); \
        else \
            patchVar.Apply(); \
        GameConfig::SetValue(appName, keyName, (uint32_t)patchVar.IsApplied()); \
    } \
    switch (patchVar.IsApplied()) { \
    case false: return "OFF"; \
        break; \
    case true: return "ON "; \
        break; \
    } \
    return ERROR_MESSAGE; \
}
#endif
