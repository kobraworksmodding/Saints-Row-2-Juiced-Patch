#include "InGameConfig.h"
#include "..\Render\Render3D.h"
#include "..\Render\Render2D.h"
#include <unordered_set>
#include <regex>
#include "../FileLogger.h"
#include "../Player/Behavior.h"
#include "../GameConfig.h"
#include <iomanip>
#include <sstream>
int AddMessage(const wchar_t* Title, const wchar_t* Desc);
int AddMessageCustomized(const wchar_t* Title, const wchar_t* Desc, const wchar_t* Options[], int OptionCount);
namespace InGameConfig {
    typedef bool(*isCoopT)();
    isCoopT isCoop = (isCoopT)0x007F7AD0;
    static non_live_options restart_option[] = {
    { "Debug", "DisableXInput",MenuType::CONTROLS },
    { "Debug", "ForceDisableVibration",MenuType::CONTROLS },
    { "Graphics", "ShadowMapFiltering" },
    { "Graphics", "UHQScreenEffects" },
    { "Graphics", "UHQTreeShadows" },
    { "Graphics", "Borderless" },
#if !JLITE
    { "Graphics", "RemoveVignette" },
#endif
    { "Audio", "UseFixedXACT" },
    };
    static PatchEntry patch_registry[] = {
#if !JLITE
    { "VFXPlus", &Render3D::CMPatches_VFXPlus,nullptr ,"Graphics", "VanillaFXPlus" },
#endif
    { "BetterAO", nullptr,&Render3D::CBetterAO, "Graphics", "BetterAmbientOcclusion"},
    {"DisableBlueRefl",&Render3D::CMPatches_DisableSkyRefl,nullptr,"Graphics","DisableSkyRefl"},
    {"DisableCutSceneBlackBars",nullptr,&Render3D::CRemoveBlackBars,"Graphics","RemoveBlackBars"},
#if !JLITE
    {"BetterDriveByCam",nullptr,&Behavior::CBetterDBC,"Gameplay","BetterDriveByCam"},
    {"BetterHandbrakeCam",nullptr,&Behavior::CBetterHBC,"Gameplay","BetterHandbrakeCam"},
#endif
    {"UncapFPS",nullptr,&Render3D::CUncapFPS,"Debug","UncapFPS"},
#if !JLITE
    {"DisableFog",&Render3D::CMPatches_DisableFog,nullptr,"Graphics","DisableFog"},
    {"SR1Reloading",&Behavior::CMPatches_SR1Reloading,nullptr,"Gameplay","SR1Reloading"},
    {"SR1QuickSwitch",&Behavior::CMPatches_SR1QuickSwitch,nullptr,"Gameplay","SR1QuickSwitch"},
    {"BetterAnimBlend",nullptr,&Behavior::CAnimBlend,"Gameplay","BetterAnimBlend"},
    {"UseWeaponAfterEmpty",&Behavior::CMPatches_UseWeaponAfterEmpty,nullptr,"Gameplay","Keep Weapon After Empty"},
    {"TauntCancelling",&Behavior::CMPatches_UseWeaponAfterEmpty,nullptr,"Gameplay","TauntCancelling"}
#endif
    };
    void AddOptions() {
        InGameConfig::RegisterSlider("SleepHack", "Sleep Hack", { "CONTROL_NO","QUALITY_LOW_TEXT","QUALITY_MEDIUM_TEXT","QUALITY_HIGH_TEXT" });
        InGameConfig::RegisterBoolSlider("UncapFPS", "UncapFPS");
        InGameConfig::RegisterBoolSlider("X360Gamma", "Xbox 360 Gamma");
#if !JLITE
        InGameConfig::RegisterBoolSlider("VFXPlus", "VanillaFXPlus");
#endif
        InGameConfig::RegisterBoolSlider("BetterAO", "Better Ambient Occlusion");
#if !JLITE
        InGameConfig::RegisterBoolSlider("DisableFog", "Disable Fog");
#endif
        InGameConfig::RegisterBoolSlider("DisableBlueRefl", "Disable Sky Reflections");
#if !JLITE
        InGameConfig::RegisterBoolSlider("IVRadarScaling", "IV Radar Scaling");
#endif
        InGameConfig::RegisterSlider("DisableAimAssist", "Disable Aim Assist", { "CONTROL_NO","On Mouse only","Always"}, MenuType::CONTROLS);
#if !JLITE
        InGameConfig::RegisterSlider(
            "VehicleAutoCenterModifer",
            "Vehicle Auto Center Modifer",
            []() -> std::vector<std::string> {
                std::vector<std::string> result;
                std::ostringstream oss;
                for (double i = 0.0; i <= 10.0; i += 0.5) {
                    oss.str("");
                    oss.clear();
                    oss << std::fixed << std::setprecision(1) << i;
                    result.push_back(oss.str());
                }
                return result;
            }(),
                MenuType::CONTROLS
                );

        InGameConfig::RegisterBoolSlider("BetterDriveByCam", "Better Drive-by Cam", InGameConfig::MenuType::CONTROLS);
        InGameConfig::RegisterBoolSlider("BetterHandbrakeCam", "Better Handbrake Cam", InGameConfig::MenuType::CONTROLS);
        InGameConfig::RegisterBoolSlider("SR1Reloading", "SR1Reloading", InGameConfig::MenuType::CONTROLS);
        InGameConfig::RegisterBoolSlider("SR1QuickSwitch", "SR1QuickSwitch", InGameConfig::MenuType::CONTROLS);
        //InGameConfig::RegisterSlider("BetterAO", "Better Ambient Occlusion", {"FUCK OFF ", "fucked off"}, 50);
#endif
#if !JLITE
        InGameConfig::RegisterBoolSlider("BetterAnimBlend", "Better Anim Blend");
#endif
        for (const auto& opt : restart_option) {
            std::string label = std::string(opt.keyname) + " (R)";
            InGameConfig::RegisterBoolSlider(opt.keyname, label.c_str(),opt.type);
        }



    }
    void __cdecl UserUnderstands(int Unk, int SelectedOption, int Action) {

        if (Action == 2 && SelectedOption == 1) {
            GameConfig::SetValue("Debug", "ReadJuicedWarning", 1);
        }
    }
    void GLuaWrapper(const char* var, int* value, bool write) {
        if (!write && strcmp(var,"JuicedCall") == 0 && !GameConfig::GetValue("Debug", "ReadJuicedWarning", 0) && *value == 1) {
            const wchar_t* JuicedWelcome =
                L"[format][color:#B200FF]Juiced[/format] options and the options available in display & controls "
                L"is not representative of all the options that are available to change.\n\n"
                L"Refer to Juiced.ini for the full options list.\n"
                L"- [format][color:#B200FF]Juiced Team[/format]"
                L"[format][scale:1.0][image:ui_hud_inv_d_ginjuice][/format]";
            const wchar_t* Options[] = { L"OK", L"[format][color:#D41111]Do not repeat this pop-up[/format]\n\n" };
            int Result = AddMessageCustomized(L"Juiced", JuicedWelcome, Options, _countof(Options));
            *(void**)(Result + 0x930) = &UserUnderstands;
        }
#if !JLITE
        if (strcmp(var, "IVRadarScaling") == 0) {
            if (!write) {
                *value = Render2D::IVRadarScaling;
            }
            else if(write) {
                Render2D::IVRadarScaling = true;
                Render2D::RadarScaling();
                Render2D::VintScaleIV();
                GameConfig::SetValue("Graphics", "IVRadarScaling", *value);
               
                    const wchar_t* IVRadarScaleWarning =
                        L"Toggling IVRadarScale OFF while the game is running doesn't apply the changes in real time only when applying, restart to disable it if off.\n";
                    AddMessage(L"Juiced", IVRadarScaleWarning);
                
            }
        }

        else
#endif
            if (strcmp(var, "X360Gamma") == 0) {
            if (!write) {
                *value = (Render3D::ShaderOptions & (1 << 0)) ? 1 : 0;
            }
            else {
                Render3D::ShaderOptions = (*value) ? (Render3D::ShaderOptions | SHADER_X360_GAMMA) : (Render3D::ShaderOptions & ~SHADER_X360_GAMMA);
                Render3D::VFXBrightnesstoggle();
                Render3D::ChangeShaderOptions();
                GameConfig::SetValue("Graphics", "X360Gamma", *value);

            }
        } else if (strcmp(var, "VehicleAutoCenterModifer") == 0) {
            if(!write)
            *value = std::clamp(Behavior::sticky_cam_timer_add / 500, 0, 20);
            else {
                Behavior::sticky_cam_timer_add = (*value * 500);
                GameConfig::SetValue("Gameplay", "VehicleAutoCenterModifer", Behavior::sticky_cam_timer_add);
                if (Behavior::sticky_cam_timer_add != 0)
                    Behavior::cf_do_control_mode_sticky_MIDASMHOOK.enable();
                else Behavior::cf_do_control_mode_sticky_MIDASMHOOK.disable();
            }
        }
        else {
            non_live_options* require_restart = Find_option_restart(var);
            if (require_restart && !write) {
                *value = GameConfig::GetValue(require_restart->appname, require_restart->keyname,0);
            }
            else if (require_restart && write) {

                const wchar_t* Requires_Restart_message =
                    L"Options that have the (R) tag next to them require a restart to apply.\n";
                static bool read_require_message_bool = false;
                if (!read_require_message_bool) {
                    AddMessage(L"Juiced", Requires_Restart_message);
                    read_require_message_bool = true;
                }
                GameConfig::SetValue(require_restart->appname, require_restart->keyname, *value);
            }
        }
        if (!write) {
            *value = ClampSliderValue(var, *value);
        }
    }
    PatchEntry* FindPatchEntry(const char* name) {
        for (auto& entry : patch_registry) {
            if (strcmp(entry.name, name) == 0) {
                return &entry;
            }
        }
        return nullptr;
    }

    non_live_options* Find_option_restart(const char* name) {
        for (auto& entry : restart_option) {
            if (strcmp(entry.keyname, name) == 0) {
                return &entry;
            }
        }
        return nullptr;
    }



    static std::unordered_map<std::string, int> g_juicedVars;
    static std::unordered_set<int> g_usedIds;

    int FindNextAvailableId(const char* original_buffer, size_t original_size) {
        // Parse the lua buffer to find used IDs in the menu
        std::string buffer(original_buffer, original_size);

        // Keep track of all found IDs to avoid duplicates
        std::unordered_set<int> foundIds;

        // First, find the display menu section
        std::string menuStart = "Pause_display_menu_PC = {";
        size_t menuStartPos = buffer.find(menuStart);
        if (menuStartPos == std::string::npos) {
            // Menu not found, use default starting ID
            return 17;
        }

        // Find where the menu entries start
        size_t menuEntryPos = buffer.find("[", menuStartPos);
        if (menuEntryPos == std::string::npos) {
            return 17;
        }

        // Now scan through all menu entries looking for id = X patterns
        size_t currentPos = menuEntryPos;
        size_t btnTipsPos = buffer.find("btn_tips = ", menuStartPos);

        if (btnTipsPos == std::string::npos) {
            // Can't find the end of the menu, use a different marker
            btnTipsPos = buffer.find("num_items = ", menuStartPos);
            if (btnTipsPos == std::string::npos) {
                return 17;
            }
        }

        // Extract the menu section
        std::string menuSection = buffer.substr(menuStartPos, btnTipsPos - menuStartPos);

        // Now use regex to find all IDs
        const std::regex idPattern("id\\s*=\\s*(\\d+)");

        // Find all occurrences
        std::sregex_iterator it(menuSection.begin(), menuSection.end(), idPattern);
        std::sregex_iterator end;

        while (it != end) {
            std::smatch match = *it;
            if (match.size() > 1) {
                // Extract the ID number
                int id = std::stoi(match[1].str());
                foundIds.insert(id);
                //Logger::TypedLog(CHN_LUA, "Found menu ID: %d \n", id);
            }
            ++it;
        }
        
        // Combine with our already tracked IDs
        std::unordered_set<int> allUsedIds = foundIds;
        allUsedIds.insert(g_usedIds.begin(), g_usedIds.end());

        // Find the first unused ID starting from a reasonable minimum
        // Based on your menu, it looks like IDs start from 1
        int nextId = 1;

        // Find the highest used ID first
        int highestId = 0;
        for (int id : allUsedIds) {
            if (id > highestId) {
                highestId = id;
            }
        }

        // Simply use the next ID after the highest one
        nextId = highestId + 1;

        // Add this ID to our used set
        g_usedIds.insert(nextId);

        Logger::TypedLog(CHN_LUA, "Assigned new menu ID: %d", nextId);

        return nextId;
    }
    std::vector<Slider> g_sliders;
    bool RegisterSlider(const char* name, const char* display_name, const std::vector<std::string>& labels, MenuType menuType, int startingId) {
        // If a starting ID is provided, try to use it first
        int id = startingId;

        // If no ID provided or the provided ID is already used, find one automatically
        if (id == -1 || g_usedIds.find(id) != g_usedIds.end()) {
            // Need to find a free ID when we have access to the buffer
            // For now, just mark that we need to assign an ID later
            id = -1;
        }
        else {
            g_usedIds.insert(id);
        }

        // Store the slider information with custom labels
        g_sliders.push_back({ name, display_name, id, labels, menuType });

        // Initialize the variable if it doesn't exist yet
        if (g_juicedVars.find(name) == g_juicedVars.end()) {
            g_juicedVars[name] = 0;
        }

        return true;
    }
    bool RegisterBoolSlider(const char* name, const char* display_name, MenuType type ,int startingId) {
        // Create a bool slider with default Yes/No labels
        return RegisterSlider(name, display_name, { "CONTROL_NO", "CONTROL_YES" }, type, startingId);
    }
    int ClampSliderValue(const std::string& sliderName, int currentValue)
    {
        auto it = std::find_if(
            g_sliders.begin(),
            g_sliders.end(),
            [&](const Slider& s) {
                return s.name == sliderName;
            }
        );
        if (it != g_sliders.end())
        {
            const int maxIndex = static_cast<int>(it->labels.size()) - 1;
            if (currentValue < 0) currentValue = 0;
            if (currentValue > maxIndex) currentValue = maxIndex;
        }

        return currentValue;
    }
    static char* g_sliderModifiedBuffer = nullptr;
    bool PatchSliderContent(std::string& buffer, const char* filename) {
        // Only process pause_menu.lua
        if (strcmp(filename, "pause_menu.lua") != 0 || g_sliders.empty()) {
            return false; // Nothing to patch
        }

        // For any sliders that don't have an ID yet, assign one now
        bool needsIdAssignment = false;
        for (auto& slider : g_sliders) {
            if (slider.id == -1) {
                needsIdAssignment = true;
                break;
            }
        }

        if (needsIdAssignment) {
            // Find the next available ID
            int nextId = FindNextAvailableId(buffer.c_str(), buffer.length());

            // Assign IDs to any sliders that need them
            for (auto& slider : g_sliders) {
                if (slider.id == -1) {
                    slider.id = nextId++;
                    g_usedIds.insert(slider.id);
                }
            }
        }

        bool modified = false;

        // 1. Find and add slider values definitions
        std::string sliderSection = "----[ Sliders for the Menus ]----";
        size_t sliderPos = buffer.find(sliderSection);
        if (sliderPos != std::string::npos) {
            // Move past the section header to find insertion point
            sliderPos = buffer.find("\n", sliderPos) + 1;

            std::string sliderAdditions;
            for (const auto& slider : g_sliders) {
                // Create slider definition with custom labels
                std::string sliderValuesStr = slider.name + "_slider_values = { ";

                // Add each label
                for (size_t i = 0; i < slider.labels.size(); i++) {
                    sliderValuesStr += "[" + std::to_string(i) + "] = { label = \"" + slider.labels[i] + "\" }";
                    if (i < slider.labels.size() - 1) {
                        sliderValuesStr += ", ";
                    }
                }

                // Add num_values and cur_value
                sliderValuesStr += ", num_values = " + std::to_string(slider.labels.size()) + ", cur_value = 0 }\n";

                sliderAdditions += sliderValuesStr;
            }

            buffer.insert(sliderPos, sliderAdditions);
            modified = true;
        }

        // Count sliders for each menu type
        std::vector<Slider> displaySliders;
        std::vector<Slider> controlSliders;

        for (const auto& slider : g_sliders) {
            if (slider.menuType == MenuType::DISPLAY) {
                displaySliders.push_back(slider);
            }
            else if (slider.menuType == MenuType::CONTROLS) {
                controlSliders.push_back(slider);
            }
        }

        // 2. Find and update the display menu array (if we have display sliders)
        if (!displaySliders.empty()) {
            std::string menuArrayStart = "Pause_display_menu_PC = {";
            std::string numItemsStr = "num_items = ";

            size_t menuPos = buffer.find(menuArrayStart);
            if (menuPos != std::string::npos) {
                // Find num_items line
                size_t numItemsPos = buffer.find(numItemsStr, menuPos);
                if (numItemsPos != std::string::npos) {
                    // Extract current num_items value
                    size_t numValuePos = numItemsPos + numItemsStr.length();
                    size_t numValueEnd = buffer.find(",", numValuePos);
                    std::string currentNumStr = buffer.substr(numValuePos, numValueEnd - numValuePos);
                    int currentNumItems = std::stoi(currentNumStr);

                    // Update num_items to account for our new sliders plus the header
                    int additionalItems = displaySliders.size() + 1; // +1 for the header
                    buffer.replace(numValuePos, numValueEnd - numValuePos,
                        std::to_string(currentNumItems + additionalItems));

                    // Find the end of the array entries
                    std::string btnTipsStr = "btn_tips = Pause_options_btn_tips,";
                    size_t btnTipsPos = buffer.find(btnTipsStr, menuPos);
                    if (btnTipsPos != std::string::npos) {
                        // Find the last entry bracket to insert after
                        size_t lastBracketPos = buffer.rfind("},", btnTipsPos);
                        if (lastBracketPos != std::string::npos) {
                            // Move to the next line after the last entry
                            lastBracketPos = buffer.find("\n", lastBracketPos) + 1;

                            std::string menuEntries;

                            // First add the "Juiced Options" header
                            int headerIndex = currentNumItems;
                            menuEntries += "\t[" + std::to_string(headerIndex) +
                                "] = { label = \"Juiced Options\", type = MENU_ITEM_TYPE_SELECTABLE, on_select = nil, disabled = true, it_is_caption_label = true, dimm_disabled = true },\n";

                            // Then add all display sliders
                            for (size_t i = 0; i < displaySliders.size(); i++) {
                                const auto& slider = displaySliders[i];
                                // Create new menu entry with the same format as existing entries
                                menuEntries += "\t[" + std::to_string(headerIndex + 1 + i) +
                                    "] = { label = \"" + slider.display_name +
                                    "\",\t\t\ttype = MENU_ITEM_TYPE_TEXT_SLIDER, text_slider_values = " +
                                    slider.name + "_slider_values,\t\t\ton_value_update = pause_menu_display_options_update_value,\tid =" +
                                    std::to_string(slider.id) + ",\t\ton_select = pause_menu_options_submenu_exit_confirm },\n";
                            }

                            buffer.insert(lastBracketPos, menuEntries);
                            modified = true;
                        }
                    }
                }
            }

            // 3. Update the display value initialization function
            std::string initFunction = "function pause_menu_populate_display(";
            size_t initFuncPos = buffer.find(initFunction);
            if (initFuncPos != std::string::npos) {
                // Find the end of the function parameters
                size_t initFuncEnd = buffer.find(")", initFuncPos);
                if (initFuncEnd != std::string::npos) {
                    // Find the function body start
                    size_t functionBodyStart = buffer.find("\n", initFuncEnd) + 1;

                    std::string initLines;
                    initLines += "vint_get_avg_processing_time(\"JuicedCall\",1)";
                    for (const auto& slider : displaySliders) {
                        // Create initialization lines
                        initLines += "\t" + slider.name + "_slider_values.cur_value = vint_get_avg_processing_time(\"ReadJuiced\",\"" +
                            slider.name + "\")\n";
                    }

                    buffer.insert(functionBodyStart, initLines);
                    modified = true;
                }
            }

            // 4. Update the display options update function to write values
            std::string updateFunction = "function pause_menu_display_options_update_value(menu_label, menu_data)";
            size_t updateFuncPos = buffer.find(updateFunction);
            if (updateFuncPos != std::string::npos) {
                // Find the local idx line
                std::string idxLine = "\tlocal idx = menu_data.id";
                size_t idxLinePos = buffer.find(idxLine, updateFuncPos);
                if (idxLinePos != std::string::npos) {
                    // Find the point to insert our condition
                    size_t insertPos = buffer.find("\n", idxLinePos) + 1;

                    std::string conditions;
                    for (const auto& slider : displaySliders) {
                        // Create condition for each slider
                        conditions += "\tif idx == " + std::to_string(slider.id) + " then\n" +
                            "\t\tvint_get_avg_processing_time(\"WriteJuiced\",\"" + slider.name +
                            "\", menu_data.text_slider_values.cur_value)\n" +
                            "\tend\n";
                    }

                    buffer.insert(insertPos, conditions);
                    modified = true;
                }
            }
        }

        // 5. Find and update the controls menu array (if we have control sliders)
        if (!controlSliders.empty()) {
            std::string controlMenuStart = "Pause_control_menu_PC = {";
            std::string controlHeaderStr = "header_label_str	= \"MENU_OPTIONS_CONTROLS\",";
            std::string controlNumItemsStr = "num_items = ";

            size_t controlMenuPos = buffer.find(controlMenuStart);
            if (controlMenuPos != std::string::npos) {
                // Verify we found the right menu
                size_t controlHeaderPos = buffer.find(controlHeaderStr, controlMenuPos);
                if (controlHeaderPos != std::string::npos && controlHeaderPos < controlMenuPos + 200) { // Check if header is close to the start
                    // Find num_items line
                    size_t numItemsPos = buffer.find(controlNumItemsStr, controlMenuPos);
                    if (numItemsPos != std::string::npos) {
                        // Extract current num_items value
                        size_t numValuePos = numItemsPos + controlNumItemsStr.length();
                        size_t numValueEnd = buffer.find(",", numValuePos);
                        std::string currentNumStr = buffer.substr(numValuePos, numValueEnd - numValuePos);
                        int currentNumItems = std::stoi(currentNumStr);

                        // Update num_items to account for our new control sliders plus the header
                        int additionalItems = controlSliders.size() + 1; // +1 for the header
                        buffer.replace(numValuePos, numValueEnd - numValuePos,
                            std::to_string(currentNumItems + additionalItems));

                        // Find the end of the array entries
                        std::string btnTipsStr = "btn_tips = Pause_options_btn_tips,";
                        size_t btnTipsPos = buffer.find(btnTipsStr, controlMenuPos);
                        if (btnTipsPos != std::string::npos) {
                            // Find the last entry bracket to insert after
                            size_t lastBracketPos = buffer.rfind("},", btnTipsPos);
                            if (lastBracketPos != std::string::npos) {
                                // Move to the next line after the last entry
                                lastBracketPos = buffer.find("\n", lastBracketPos) + 1;

                                std::string menuEntries;

                                // First add the "Juiced Options" header
                                int headerIndex = currentNumItems;
                                menuEntries += "\t[" + std::to_string(headerIndex) +
                                    "] = { label = \"Juiced Options\", type = MENU_ITEM_TYPE_SELECTABLE, on_select = nil, disabled = true, it_is_caption_label = true, dimm_disabled = true },\n";

                                // Then add all control sliders
                                for (size_t i = 0; i < controlSliders.size(); i++) {
                                    const auto& slider = controlSliders[i];
                                    // Create new menu entry with the same format as existing entries
                                    menuEntries += "\t[" + std::to_string(headerIndex + 1 + i) +
                                        "] = { label = \"" + slider.display_name +
                                        "\",\t\t\ttype = MENU_ITEM_TYPE_TEXT_SLIDER, text_slider_values = " +
                                        slider.name + "_slider_values,\t\t\ton_value_update = pause_menu_control_options_update_value,\tid =" +
                                        std::to_string(slider.id) + ",\t\ton_select = pause_menu_option_accept },\n";
                                }

                                buffer.insert(lastBracketPos, menuEntries);
                                modified = true;
                            }
                        }
                    }
                }
            }

            // 6. Update the control value initialization function
            std::string controlInitFunction = "function pause_menu_populate_control_options(";
            size_t controlInitFuncPos = buffer.find(controlInitFunction);
            if (controlInitFuncPos != std::string::npos) {
                // Find the end of the function parameters
                size_t initFuncEnd = buffer.find(")", controlInitFuncPos);
                if (initFuncEnd != std::string::npos) {
                    // Find the function body start
                    size_t functionBodyStart = buffer.find("\n", initFuncEnd) + 1;

                    std::string initLines;
                    initLines += "vint_get_avg_processing_time(\"JuicedCall\",1)";
                    for (const auto& slider : controlSliders) {
                        // Create initialization lines
                        initLines += "\t" + slider.name + "_slider_values.cur_value = vint_get_avg_processing_time(\"ReadJuiced\",\"" +
                            slider.name + "\")\n";
                    }

                    buffer.insert(functionBodyStart, initLines);
                    modified = true;
                }
            }

            // 7. Update the control options update function to write values
            std::string controlUpdateFunction = "function pause_menu_control_options_update_value(menu_label, menu_data)";
            size_t controlUpdateFuncPos = buffer.find(controlUpdateFunction);
            if (controlUpdateFuncPos != std::string::npos) {
                // Find the local idx line
                std::string idxLine = "\tlocal idx = menu_data.id";
                size_t idxLinePos = buffer.find(idxLine, controlUpdateFuncPos);
                if (idxLinePos != std::string::npos) {
                    // Find the point to insert our condition
                    size_t insertPos = buffer.find("\n", idxLinePos) + 1;

                    std::string conditions;
                    for (const auto& slider : controlSliders) {
                        // Create condition for each slider
                        conditions += "\tif idx == " + std::to_string(slider.id) + " then\n" +
                            "\t\tvint_get_avg_processing_time(\"WriteJuiced\",\"" + slider.name +
                            "\", menu_data.text_slider_values.cur_value)\n" +
                            "\tend\n";
                    }

                    buffer.insert(insertPos, conditions);
                    modified = true;
                }
            }
        }
        int MP_game_mode = *(int*)0x00E8B210;
        if (isCoop() || MP_game_mode == -1) {
            // Find a good insertion point - look for Pause_display_menu_PC
            std::string displayMenuStr = "Pause_display_menu_PC = {";
            size_t displayMenuPos = buffer.find(displayMenuStr);
            if (displayMenuPos != std::string::npos) {
                // First add the supporting functions before the menu
                std::string juicedFunctions = "function juiced_menu_build_display_options_menu_PC(menu_data)\n";
                juicedFunctions += "vint_get_avg_processing_time(\"JuicedCall\",1)";
                // Initialize all sliders (both display and control)
                for (const auto& slider : g_sliders) {
                    juicedFunctions += "\t" + slider.name + "_slider_values.cur_value = vint_get_avg_processing_time(\"ReadJuiced\",\"" +
                        slider.name + "\")\n";
                }

                juicedFunctions += "end\n\n";

                // Add update function for juiced menu
                juicedFunctions += "function juiced_menu_display_options_update_value(menu_label, menu_data)\n";
                juicedFunctions += "\tlocal idx = menu_data.id\n";
                juicedFunctions += "if menu_data.type == MENU_ITEM_TYPE_NUM_SLIDER then\n";
                juicedFunctions += "local h = vint_object_find(\"value_text\", menu_label.control.grp_h)\n";
                juicedFunctions += "vint_set_property(h, \"text_tag\", floor(menu_data.cur_value * 100) .. \" % %\")\n";
                juicedFunctions += "end\n\n";
                // Add conditions for all sliders

                juicedFunctions += "if menu_data.type == MENU_ITEM_TYPE_TEXT_SLIDER then\n";

                for (const auto& slider : g_sliders) {
                    juicedFunctions += "\tif idx == " + std::to_string(slider.id) + " then\n" +
                        "\t\tvint_get_avg_processing_time(\"WriteJuiced\",\"" + slider.name +
                        "\", menu_data.text_slider_values.cur_value)\n" +
                        "\tend\n";
                }
                juicedFunctions += "end\n\n";
                juicedFunctions += "end\n\n";

                // Add back function
                juicedFunctions += "function juiced_back_workaround()\n";
                juicedFunctions += "\t\tmenu_show(Pause_options_menu, MENU_TRANSITION_SWEEP_RIGHT)\n";
                juicedFunctions += "\t\taudio_play(Menu_sound_back)\n";
                juicedFunctions += "end\n\n";

                // Now add the Juiced_options menu after the functions
                std::string juicedOptionsMenu = juicedFunctions + "Juiced_options = {\n";
                juicedOptionsMenu += "\theader_label_str\t= \"Juiced Options\",\n";
                juicedOptionsMenu += "\tmax_height = 450,\t-- Default: 375 [nclok1405]\n";
                juicedOptionsMenu += "\ton_show \t\t\t= juiced_menu_build_display_options_menu_PC,\n";
                juicedOptionsMenu += "\ton_alt_select \t\t= pause_menu_options_restore_defaults,\n";
                juicedOptionsMenu += "\ton_back \t\t\t= juiced_back_workaround,\n";
                juicedOptionsMenu += "\ton_pause\t\t\t= pause_menu_options_exit_confirm,\n";
                juicedOptionsMenu += "\ton_map \t\t\t\t= pause_menu_options_swap_confirm,\n";
                juicedOptionsMenu += "\ton_nav\t\t\t\t= pause_menu_display_options_nav,\n";
                juicedOptionsMenu += "\ton_horz_show \t\t= pause_menu_option_accept_horz,\n";

                // Calculate num_items based on all sliders (both display and control)
                int totalItems = displaySliders.size() + controlSliders.size();

                // If we have both types, we need two headers
                if (!displaySliders.empty() && !controlSliders.empty()) {
                    totalItems += 2; // Two headers (one for display, one for controls)
                }
                // If we have only one type, we need just one header
                else if (!displaySliders.empty() || !controlSliders.empty()) {
                    totalItems += 1; // One header
                }

                juicedOptionsMenu += "\tnum_items = " + std::to_string(totalItems) + ",\n\n";

                // Add entries for all sliders
                int entryIndex = 0;

                // Add display sliders if we have any
                if (!displaySliders.empty()) {
                    // Add display header
                    juicedOptionsMenu += "\t[" + std::to_string(entryIndex++) +
                        "] = { label = \"Display & Audio Options\", type = MENU_ITEM_TYPE_SELECTABLE, on_select = nil, disabled = true, it_is_caption_label = true, dimm_disabled = true },\n";

                    // Add display sliders
                    for (const auto& slider : displaySliders) {
                        juicedOptionsMenu += "\t[" + std::to_string(entryIndex++) +
                            "] = { label = \"" + slider.display_name +
                            "\",\t\t\ttype = MENU_ITEM_TYPE_TEXT_SLIDER, text_slider_values = " +
                            slider.name + "_slider_values,\t\t\ton_value_update = juiced_menu_display_options_update_value,\tid =" +
                            std::to_string(slider.id) + ",\t\ton_select = pause_menu_options_submenu_exit_confirm },\n";
                    }
                }

                // Add control sliders if we have any
                if (!controlSliders.empty()) {
                    // Add control header
                    juicedOptionsMenu += "\t[" + std::to_string(entryIndex++) +
                        "] = { label = \"Control Options\", type = MENU_ITEM_TYPE_SELECTABLE, on_select = nil, disabled = true, it_is_caption_label = true, dimm_disabled = true },\n";

                    // Add control sliders
                    for (const auto& slider : controlSliders) {
                        juicedOptionsMenu += "\t[" + std::to_string(entryIndex++) +
                            "] = { label = \"" + slider.display_name +
                            "\",\t\t\ttype = MENU_ITEM_TYPE_TEXT_SLIDER, text_slider_values = " +
                            slider.name + "_slider_values,\t\t\ton_value_update = juiced_menu_display_options_update_value,\tid =" +
                            std::to_string(slider.id) + ",\t\ton_select = pause_menu_options_submenu_exit_confirm },\n";
                    }
                }

                juicedOptionsMenu += "\tbtn_tips = Pause_options_btn_tips,\n";
                juicedOptionsMenu += "}\n\n";
                // No need to add functions here as they were already added before the menu definition

                buffer.insert(displayMenuPos, juicedOptionsMenu);
                modified = true;
            }

            // 9. Update Pause_options_menu to include Juiced_options
            std::string pauseOptionsMenu = "Pause_options_menu = {";
            size_t pauseOptionsPos = buffer.find(pauseOptionsMenu);
            if (pauseOptionsPos != std::string::npos) {
                // Find the num_items line
                std::string numItemsStr = "num_items = ";
                size_t numItemsPos = buffer.find(numItemsStr, pauseOptionsPos);
                if (numItemsPos != std::string::npos) {
                    // Extract current num_items value
                    size_t numValuePos = numItemsPos + numItemsStr.length();
                    size_t numValueEnd = buffer.find(",", numValuePos);
                    std::string currentNumStr = buffer.substr(numValuePos, numValueEnd - numValuePos);
                    int currentNumItems = std::stoi(currentNumStr);

                    // Update num_items to account for our new Juiced_options menu
                    buffer.replace(numValuePos, numValueEnd - numValuePos, std::to_string(currentNumItems + 1));

                    // Find MENU_OPTIONS_AUDIO entry to insert after
                    std::string audioStr = "MENU_OPTIONS_AUDIO";
                    size_t audioPos = buffer.find(audioStr, pauseOptionsPos);
                    if (audioPos != std::string::npos) {
                        // Find the end of this entry
                        size_t entryEnd = buffer.find("},", audioPos);
                        if (entryEnd != std::string::npos) {
                            // Move to the next line after this entry
                            entryEnd += 2;

                            // Find the index of the AUDIO menu item
                            std::string audioIndexStr = "[";
                            size_t audioIndexStart = buffer.rfind(audioIndexStr, audioPos);
                            if (audioIndexStart != std::string::npos) {
                                // Extract the audio index number
                                size_t indexNumStart = audioIndexStart + 1;
                                size_t indexNumEnd = buffer.find("]", indexNumStart);
                                if (indexNumEnd != std::string::npos) {
                                    std::string audioIndexNum = buffer.substr(indexNumStart, indexNumEnd - indexNumStart);
                                    int audioIndex = std::stoi(audioIndexNum);

                                    // The next index for Juiced Options will be audioIndex + 1
                                    int juicedIndex = audioIndex + 1;

                                    // Find the item that comes after AUDIO (should be QUIT_GAME)
                                    std::string quitGameStr = "MENU_OPTIONS_QUIT_GAME";
                                    size_t quitGamePos = buffer.find(quitGameStr, entryEnd);
                                    if (quitGamePos != std::string::npos) {
                                        // Find the index of QUIT_GAME
                                        std::string quitIndexStr = "[";
                                        size_t quitIndexStart = buffer.rfind(quitIndexStr, quitGamePos);
                                        if (quitIndexStart != std::string::npos) {
                                            // Update all subsequent indices
                                            for (int i = currentNumItems - 1; i > audioIndex; i--) {
                                                std::string oldIndex = "[" + std::to_string(i) + "]";
                                                std::string newIndex = "[" + std::to_string(i + 1) + "]";

                                                // Find this index after the audioPos
                                                size_t oldIndexPos = buffer.find(oldIndex, audioPos);
                                                if (oldIndexPos != std::string::npos) {
                                                    buffer.replace(oldIndexPos, oldIndex.length(), newIndex);
                                                }
                                            }

                                            // Insert Juiced_options entry
                                            std::string juicedEntry = "\t[" + std::to_string(juicedIndex) +
                                                "] = { label = \"Juiced Options\",\ttype = MENU_ITEM_TYPE_SUB_MENU, \tsub_menu = Juiced_options, \t\t},\n";
                                            buffer.insert(entryEnd, juicedEntry);
                                            modified = true;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // 10. Update Pause_options_menu_no_difficulty to include Juiced_options
            std::string pauseOptionsNoDifficulty = "Pause_options_menu_no_difficulty = {";
            size_t pauseOptionsNoDiffPos = buffer.find(pauseOptionsNoDifficulty);
            if (pauseOptionsNoDiffPos != std::string::npos) {
                // Find the num_items line
                std::string numItemsStr = "num_items = ";
                size_t numItemsPos = buffer.find(numItemsStr, pauseOptionsNoDiffPos);
                if (numItemsPos != std::string::npos) {
                    // Extract current num_items value
                    size_t numValuePos = numItemsPos + numItemsStr.length();
                    size_t numValueEnd = buffer.find(",", numValuePos);
                    std::string currentNumStr = buffer.substr(numValuePos, numValueEnd - numValuePos);
                    int currentNumItems = std::stoi(currentNumStr);

                    // Update num_items to account for our new Juiced_options menu
                    buffer.replace(numValuePos, numValueEnd - numValuePos, std::to_string(currentNumItems + 1));

                    // Find MENU_OPTIONS_AUDIO entry to insert after
                    std::string audioStr = "MENU_OPTIONS_AUDIO";
                    size_t audioPos = buffer.find(audioStr, pauseOptionsNoDiffPos);
                    if (audioPos != std::string::npos) {
                        // Find the end of this entry
                        size_t entryEnd = buffer.find("},", audioPos);
                        if (entryEnd != std::string::npos) {
                            // Move to the next line after this entry
                            entryEnd += 2;

                            // Find the index of the AUDIO menu item
                            std::string audioIndexStr = "[";
                            size_t audioIndexStart = buffer.rfind(audioIndexStr, audioPos);
                            if (audioIndexStart != std::string::npos) {
                                // Extract the audio index number
                                size_t indexNumStart = audioIndexStart + 1;
                                size_t indexNumEnd = buffer.find("]", indexNumStart);
                                if (indexNumEnd != std::string::npos) {
                                    std::string audioIndexNum = buffer.substr(indexNumStart, indexNumEnd - indexNumStart);
                                    int audioIndex = std::stoi(audioIndexNum);

                                    // The next index for Juiced Options will be audioIndex + 1
                                    int juicedIndex = audioIndex + 1;

                                    // Find the item that comes after AUDIO (should be QUIT_GAME)
                                    std::string quitGameStr = "MENU_OPTIONS_QUIT_GAME";
                                    size_t quitGamePos = buffer.find(quitGameStr, entryEnd);
                                    if (quitGamePos != std::string::npos) {
                                        // Find the index of QUIT_GAME
                                        std::string quitIndexStr = "[";
                                        size_t quitIndexStart = buffer.rfind(quitIndexStr, quitGamePos);
                                        if (quitIndexStart != std::string::npos) {
                                            // Update all subsequent indices
                                            for (int i = currentNumItems - 1; i > audioIndex; i--) {
                                                std::string oldIndex = "[" + std::to_string(i) + "]";
                                                std::string newIndex = "[" + std::to_string(i + 1) + "]";

                                                // Find this index after the audioPos
                                                size_t oldIndexPos = buffer.find(oldIndex, audioPos);
                                                if (oldIndexPos != std::string::npos) {
                                                    buffer.replace(oldIndexPos, oldIndex.length(), newIndex);
                                                }
                                            }

                                            // Insert Juiced_options entry
                                            std::string juicedEntry = "\t[" + std::to_string(juicedIndex) +
                                                "] = { label = \"Juiced Options\",\ttype = MENU_ITEM_TYPE_SUB_MENU, \tsub_menu = Juiced_options, \t\t},\n";
                                            buffer.insert(entryEnd, juicedEntry);
                                            modified = true;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // 11. Update pause_menu_option_menu_init to account for additional menu item
            std::string initFunc = "function pause_menu_option_menu_init(menu_format)";
            size_t initFuncPos = buffer.find(initFunc);
            if (initFuncPos != std::string::npos) {
                // Find the menu_format checks
                std::string format0Check = "if menu_format == 0 then";
                std::string format1Check = "elseif menu_format == 1 then";
                std::string format2Check = "elseif menu_format == 2 then";

                // For standard format (0) and multiplayer (2), just increase num_items
                std::vector<std::pair<std::string, std::string>> formatChecks = {
                    {format0Check, "Pause_options_menu.num_items = 5"},
                    {format2Check, "Pause_options_menu.num_items = 4"}
                };

                for (const auto& check : formatChecks) {
                    size_t checkPos = buffer.find(check.first, initFuncPos);
                    if (checkPos != std::string::npos) {
                        // Find the num_items assignment
                        size_t assignPos = buffer.find(check.second, checkPos);
                        if (assignPos != std::string::npos) {
                            // Extract current value
                            size_t valuePos = assignPos + std::string("Pause_options_menu.num_items = ").length();
                            std::string currentValue = buffer.substr(valuePos, 1); // Should be a single digit
                            int newValue = std::stoi(currentValue) + 1;

                            // Replace with new value
                            buffer.replace(valuePos, 1, std::to_string(newValue));
                            modified = true;
                        }
                    }
                }

                size_t format1Pos = buffer.find(format1Check, initFuncPos);
                if (format1Pos != std::string::npos) {
                    // Find the num_items assignment for format 1
                    std::string format1NumItems = "Pause_options_menu.num_items = 3";
                    size_t assignPos = buffer.find(format1NumItems, format1Pos);
                    if (assignPos != std::string::npos) {
                        // Increase num_items by 1 (from 3 to 4)
                        size_t valuePos = assignPos + std::string("Pause_options_menu.num_items = ").length();
                        buffer.replace(valuePos, 1, "4");
                        modified = true;

                        // We need to insert additional code to properly arrange the main menu options
                        size_t endOfFormat1 = buffer.find("end", format1Pos);
                        if (endOfFormat1 != std::string::npos) {
                            // Insert code to set up the menu items correctly for format 1
                            std::string mainMenuFix = "\n\t\t-- Set up Juiced Options in position 2 (after Display, before Audio)\n";
                            mainMenuFix += "\t\tPause_options_menu[2] = { label = \"Juiced Options\", type = MENU_ITEM_TYPE_SUB_MENU, sub_menu = Juiced_options }\n";

                            buffer.insert(endOfFormat1, mainMenuFix);
                            modified = true;
                        }
                    }
                }

                // Find Pause_options_menu[2] = Pause_options_menu[7] and add the new line below it
                std::string targetLine = "Pause_options_menu[2] = Pause_options_menu[7]";
                size_t targetLinePos = buffer.find(targetLine, initFuncPos);
                if (targetLinePos != std::string::npos) {
                    size_t lineEndPos = buffer.find('\n', targetLinePos);
                    if (lineEndPos != std::string::npos) {
                        size_t indentStart = targetLinePos;
                        while (indentStart > 0 && (buffer[indentStart - 1] == ' ' || buffer[indentStart - 1] == '\t')) {
                            indentStart--;
                        }
                        std::string indentation = buffer.substr(indentStart, targetLinePos - indentStart);
                        std::string newLine = "\n" + indentation + "Pause_options_menu[3] = Pause_options_menu[8]";

                        buffer.insert(lineEndPos, newLine);
                        modified = true;
                    }
                }

                // Also need to update the circular swapping logic if it exists
                // Find lines that swap menu items
                std::vector<std::string> swapPatterns = {
                    "Pause_options_menu[0] = Pause_options_menu[1]",
                    "Pause_options_menu[1] = Pause_options_menu[2]",
                    "Pause_options_menu[2] = Pause_options_menu[3]",
                    "Pause_options_menu[3] = Pause_options_menu[4]",
                    "Pause_options_menu[4] = Pause_options_menu[0]"
                };

                // If there's a 5th line to add (for the new item)
                bool foundSwap = false;
                for (const auto& pattern : swapPatterns) {
                    if (buffer.find(pattern, initFuncPos) != std::string::npos) {
                        foundSwap = true;
                        break;
                    }
                }

                if (foundSwap) {
                    // Find the last swap line
                    size_t lastSwapPos = buffer.find("Pause_options_menu[4] = Pause_options_menu[0]", initFuncPos);
                    if (lastSwapPos != std::string::npos) {
                        // Find the end of this line
                        size_t lineEnd = buffer.find("\n", lastSwapPos);
                        if (lineEnd != std::string::npos) {
                            // Add a new swap line for index 5
                            std::string newSwapLine = "\n\tPause_options_menu[5] = Pause_options_menu[1]";
                            buffer.insert(lineEnd, newSwapLine);
                            modified = true;
                        }
                    }
                }
            }
        }
        return modified;
    }
}