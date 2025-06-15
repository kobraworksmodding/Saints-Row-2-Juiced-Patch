#include "CModLoader.h"
#include "../GameConfig.h"
#include "../SafeWrite.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <regex>

namespace CModLoader {
    namespace Audio {

        struct Section {
            std::string name;
            std::vector<std::string> items;
        };

        // Static buffer for merged content - will be cleaned up on exit
        static char* merged_buffer = nullptr;
        static size_t merged_buffer_size = 0;

        // Cleanup function for the static buffer
        void cleanup_audio_buffer() {
            if (merged_buffer) {
                delete[] merged_buffer;
                merged_buffer = nullptr;
                merged_buffer_size = 0;
            }
        }

        // Parse existing audio_boot.idx_map content
        std::vector<Section> parse_existing_content(const char* content) {
            std::vector<Section> sections;
            std::istringstream stream(content);
            std::string line;

            // Skip the first line (total count)
            if (std::getline(stream, line)) {
                // Continue processing
            }

            Section current_section;
            bool in_section = false;

            while (std::getline(stream, line)) {
                // Remove leading/trailing whitespace
                line.erase(0, line.find_first_not_of(" \t\r\n"));
                line.erase(line.find_last_not_of(" \t\r\n") + 1);

                if (line.empty()) continue;

                // Check if this is a section header (e.g., "Animation"		309)
                std::regex section_regex("\"([^\"]+)\"\\s+(\\d+)");
                std::smatch matches;

                if (std::regex_match(line, matches, section_regex)) {
                    // Save previous section if exists
                    if (in_section && !current_section.name.empty()) {
                        sections.push_back(current_section);
                    }

                    // Start new section
                    current_section.name = matches[1].str();
                    current_section.items.clear();
                    in_section = true;
                }
                else if (in_section) {
                    // Check if this is an item line (e.g., 	0		"ANI_40OZ_STAND")
                    std::regex item_regex("\\s*\\d+\\s+\"([^\"]+)\"");
                    std::smatch item_matches;

                    if (std::regex_match(line, item_matches, item_regex)) {
                        current_section.items.push_back(item_matches[1].str());
                    }
                }
            }

            // Add the last section
            if (in_section && !current_section.name.empty()) {
                sections.push_back(current_section);
            }

            return sections;
        }

        // Parse mod file content with original format (like audio_boot.idx_map)
        std::vector<Section> parse_special_file(const std::string& filepath) {
            std::vector<Section> sections;
            std::ifstream file(filepath);
            if (!file.is_open()) return sections;

            std::string line;
            Section current_section;
            bool in_section = false;
            bool first_line = true;

            while (std::getline(file, line)) {
                // Remove leading/trailing whitespace
                line.erase(0, line.find_first_not_of(" \t\r\n"));
                line.erase(line.find_last_not_of(" \t\r\n") + 1);

                if (line.empty()) continue;

                // Skip the first line if it's just a number (total section count)
                if (first_line) {
                    first_line = false;
                    std::regex number_regex("^\\d+$");
                    if (std::regex_match(line, number_regex)) {
                        continue; // Skip this line
                    }
                    // If it's not just a number, process it normally below
                }

                // Check if this is a section header (e.g., "Animation"		309)
                std::regex section_regex("\"([^\"]+)\"\\s+(\\d+)");
                std::smatch matches;

                if (std::regex_match(line, matches, section_regex)) {
                    // Save previous section if exists
                    if (in_section && !current_section.name.empty()) {
                        sections.push_back(current_section);
                    }

                    // Start new section
                    current_section.name = matches[1].str();
                    current_section.items.clear();
                    in_section = true;
                }
                else if (in_section) {
                    // Check if this is an item line (e.g., 	0		"ANI_40OZ_STAND")
                    std::regex item_regex("\\s*\\d+\\s+\"([^\"]+)\"");
                    std::smatch item_matches;

                    if (std::regex_match(line, item_matches, item_regex)) {
                        current_section.items.push_back(item_matches[1].str());
                    }
                }
            }

            // Add the last section
            if (in_section && !current_section.name.empty()) {
                sections.push_back(current_section);
            }

            return sections;
        }

        std::vector<Section> parse_mod_file(const std::string& filepath) {
            std::vector<Section> sections;
            std::ifstream file(filepath);
            if (!file.is_open()) return sections;

            std::string line;
            Section current_section;
            bool in_section = false;

            while (std::getline(file, line)) {
                // Remove leading/trailing whitespace
                line.erase(0, line.find_first_not_of(" \t\r\n"));
                line.erase(line.find_last_not_of(" \t\r\n") + 1);

                if (line.empty()) continue;

                // Check if this is a section header [SECTION_NAME]
                std::regex section_regex("\\[([^\\]]+)\\]");
                std::smatch matches;

                if (std::regex_match(line, matches, section_regex)) {
                    // Save previous section if exists
                    if (in_section && !current_section.name.empty()) {
                        sections.push_back(current_section);
                    }

                    // Start new section
                    current_section.name = matches[1].str();
                    current_section.items.clear();
                    in_section = true;
                }
                else if (in_section) {
                    // Check if this is an item line "ITEM_NAME"
                    std::regex item_regex("\"([^\"]+)\"");
                    std::smatch item_matches;

                    if (std::regex_match(line, item_matches, item_regex)) {
                        current_section.items.push_back(item_matches[1].str());
                    }
                }
            }

            // Add the last section
            if (in_section && !current_section.name.empty()) {
                sections.push_back(current_section);
            }

            return sections;
        }

        // Generate merged content string
        std::string generate_merged_content(const std::vector<Section>& all_sections) {
            std::ostringstream result;

            // Write total section count
            result << all_sections.size() << "\n";

            // Write each section
            for (const auto& section : all_sections) {
                // Section header: "SectionName"		ItemCount
                result << "\"" << section.name << "\"\t\t" << section.items.size() << "\n";

                // Section items: 	Index		"ItemName"
                for (size_t i = 0; i < section.items.size(); ++i) {
                    result << "\t" << i << "\t\t\"" << section.items[i] << "\"\n";
                }
            }

            return result.str();
        }

        SAFETYHOOK_NOINLINE void audio_idx_map_hook_start(SafetyHookContext& ctx) {
            SR_FILE_READER* current = (SR_FILE_READER*)ctx.esi;
            if (!(strcmp(current->FILENAME, "audio_boot.idx_map") == 0))
                return;

            try {
                // Parse existing content
                std::vector<Section> all_sections = parse_existing_content(current->TXT);

                // Read mod files from mods/CModLoader/Audio/
                std::string mod_audio_path = "mods/CModLoader/Audio/";

                if (std::filesystem::exists(mod_audio_path)) {
                    for (const auto& entry : std::filesystem::directory_iterator(mod_audio_path)) {
                        if (entry.is_regular_file()) {
                            std::string filename = entry.path().filename().string();
                            std::string extension = entry.path().extension().string();

                            // Skip the main audio files
                            if (filename == "audio_boot.idx_map" || filename == "audio_level.idx_map") {
                                continue;
                            }

                            std::vector<Section> mod_sections;

                            if (extension == ".idx_map") {
                                mod_sections = parse_mod_file(entry.path().string());
                            }
                            else if (filename.ends_with(".idx_map_iformat")) {
                                mod_sections = parse_special_file(entry.path().string());
                            }
                            else {
                                continue; // Skip other file types
                            }

                            // Add all sections from this mod file
                            for (const auto& mod_section : mod_sections) {
                                all_sections.push_back(mod_section);
                            }
                        }
                    }
                }

                // Generate merged content
                std::string merged_content = generate_merged_content(all_sections);

                // Clean up previous buffer if exists
                cleanup_audio_buffer();

                // Create new static buffer
                merged_buffer_size = merged_content.size() + 1; // +1 for null terminator
                merged_buffer = new char[merged_buffer_size];
                strcpy_s(merged_buffer, merged_buffer_size, merged_content.c_str());

                // Update the file reader structure
                current->TXT = merged_buffer;
                current->TXT_READ = merged_buffer;
                current->size = static_cast<int>(merged_buffer_size - 1); // Exclude null terminator

            }
            catch (const std::exception& e) {
                // Log error or handle gracefully
                // For now, just continue with original content
                return;
            }
        }

        SafetyHookInline sub_46FA20T{};
        void* __cdecl sub_46FA20(char a1) {
            auto result = sub_46FA20T.ccall<void*>(a1);
            cleanup_audio_buffer();
            return result;
        }

    }

    void Init() {
        if (GameConfig::GetValue("UGC", "CModLoader", 1)) {
            static auto idx_map_hook = safetyhook::create_mid(0x46FA76, &Audio::audio_idx_map_hook_start);
            Audio::sub_46FA20T = safetyhook::create_inline(0x46FA20, &Audio::sub_46FA20);
        }
    }
}