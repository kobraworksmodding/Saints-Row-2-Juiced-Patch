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
    struct non_live_options {
        const char* appname;
        const char* keyname;
        MenuType type = MenuType::DISPLAY;
    };
    extern non_live_options* Find_option_restart(const char* name);
    struct Slider {
        std::string name;                     // Variable name
        std::string display_name;             // Display name shown in menu
        int id;                               // Menu ID
        std::vector<std::string> labels;      // Custom labels for each value (TEXT_SLIDER only)
        MenuType menuType;                    // Which menu this slider belongs to

        // ---- NUM_SLIDER (float/double) extras ----
        bool        isNumSlider = false;      // true => MENU_ITEM_TYPE_NUM_SLIDER
        std::string suffix      = "%%";       // Suffix appended to displayed value (lua/printf string)
        int         scale       = 100;        // Display multiplier: floor(cur_value * scale) .. suffix
        // Indexes inside the menu tables, populated during PatchSliderContent.
        int         idxOriginal = -1;         // Index inside Pause_display_menu_PC / Pause_control_menu_PC
        int         idxJuiced   = -1;         // Index inside Juiced_options
    };

    extern int ClampSliderValue(const std::string& sliderName, int currentValue);
    extern std::vector<Slider> g_sliders;
    extern bool RegisterSlider(const char* name, const char* display_name, const std::vector<std::string>& labels, MenuType menuType = MenuType::DISPLAY, int startingId = -1);
    extern bool RegisterBoolSlider(const char* name, const char* display_name, MenuType type = MenuType::DISPLAY, int startingId = -1);
    // NUM_SLIDER (float/double 0..1). Display text is `floor(cur_value * scale) .. suffix`.
    // suffix is emitted verbatim into a lua string literal (so "%%" -> printf '%').
    extern bool RegisterNumSlider(const char* name, const char* display_name,
                                  const char* suffix = "%%", int scale = 100,
                                  MenuType menuType = MenuType::DISPLAY, int startingId = -1);
    extern void GLuaWrapper(const char* var, int* value, bool write);
    extern void GLuaWrapperF(const char* var, double* value, bool write);
    void AddOptions();

    extern void DebugDumpLua(const std::string& buffer, const std::string& stage);
}