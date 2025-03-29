#pragma once
#include "../Patcher/CMultiPatch.h"
#include "../Patcher/CPatch.h"
#include <string>
namespace InGameConfig {
    struct PatchEntry {
        const char* name;
        CMultiPatch* multiPatch = nullptr;
        CPatch* singlePatch = nullptr;
        const char* configApp = nullptr;
        const char* configKey = nullptr;
    };
    extern PatchEntry* FindPatchEntry(const char* name);
    extern bool RegisterBoolSlider(const char* name, const char* display_name, int startingId);
    bool PatchSliderContent(std::string& buffer, const char* filename);

    // Structure to define a boolean slider
    struct BoolSlider {
        std::string name;         // Internal name used for the variable
        std::string display_name; // Display name shown in the menu
        int id;                   // Unique ID for the slider
    };

    extern std::vector<BoolSlider> g_boolSliders;
    extern bool RegisterBoolSlider(const char* name, const char* display_name, int startingId = -1);
}