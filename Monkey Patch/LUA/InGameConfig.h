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
    bool PatchSliderContent(std::string& buffer, const char* filename);
    enum class MenuType {
        DISPLAY,
        CONTROLS
    };
    struct Slider {
        std::string name;                     // Variable name
        std::string display_name;             // Display name shown in menu
        int id;                               // Menu ID
        std::vector<std::string> labels;      // Custom labels for each value
        MenuType menuType;                    // Which menu this slider belongs to
    };


    extern std::vector<Slider> g_sliders;
    extern bool RegisterSlider(const char* name, const char* display_name, const std::vector<std::string>& labels, MenuType menuType = MenuType::DISPLAY, int startingId = -1);
    extern bool RegisterBoolSlider(const char* name, const char* display_name, MenuType type = MenuType::DISPLAY, int startingId = -1);
    void AddOptions();
}