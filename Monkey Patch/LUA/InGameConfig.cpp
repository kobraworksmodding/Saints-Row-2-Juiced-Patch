#include "InGameConfig.h"
#include "..\Render\Render3D.h"
#include <unordered_set>
#include <regex>
#include "../FileLogger.h"
#include "../Player/Behavior.h"
namespace InGameConfig {
    static PatchEntry patch_registry[] = {
    { "VFXPlus", &Render3D::CMPatches_VFXPlus,nullptr ,"Graphics", "VanillaFXPlus" },
    { "BetterAO", nullptr,&Render3D::CBetterAO, "Graphics", "BetterAmbientOcclusion"},
    {"DisableBlueRefl",&Render3D::CMPatches_DisableSkyRefl,nullptr,"Graphics","DisableSkyRefl"},
    {"DisableCutSceneBlackBars",nullptr,&Render3D::CRemoveBlackBars,"Graphics","RemoveBlackBars"},
    {"BetterDriveByCam",nullptr,&Behavior::CBetterDBC,"Gameplay","BetterDriveByCam"}
    };
    void AddOptions() {
        InGameConfig::RegisterBoolSlider("VFXPlus", "VanillaFXPlus");
        InGameConfig::RegisterBoolSlider("BetterAO", "Better Ambient Occlusion");
        InGameConfig::RegisterBoolSlider("DisableBlueRefl", "Disable Sky Reflection on Windows");
        InGameConfig::RegisterBoolSlider("BetterDriveByCam", "Better Drive-by Cam", InGameConfig::MenuType::CONTROLS);
        //InGameConfig::RegisterSlider("BetterAO", "Better Ambient Occlusion", {"FUCK OFF ", "fucked off"}, 50);
        InGameConfig::RegisterSlider("SleepHack", "Sleep Hack", { "CONTROL_NO","QUALITY_LOW_TEXT","QUALITY_MEDIUM_TEXT","QUALITY_HIGH_TEXT" });
    }
    PatchEntry* FindPatchEntry(const char* name) {
        for (auto& entry : patch_registry) {
            if (strcmp(entry.name, name) == 0) {
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
                Logger::TypedLog(CHN_LUA, "Found menu ID: %d", id);
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

        return modified;
    }
}