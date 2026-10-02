#pragma once
#include <windows.h>
#include <array>
#include <filesystem>
#include <string>

namespace ExecutableProfile
{
    struct Selection
    {
        std::string executable;
        std::string loose_list;
        bool configured = false;
        std::string error;
    };

    inline Selection Select(const std::filesystem::path& executable_path)
    {
        Selection result;
        result.executable = executable_path.filename().string();
        const auto directory = executable_path.parent_path();
        const auto config = (directory / "juiced_executables.ini").string();
        std::array<char, MAX_PATH> value{};
        // A sentinel distinguishes an absent mapping from an invalid empty one.
        const DWORD count = GetPrivateProfileStringA("LooseFiles", result.executable.c_str(),
            "\1", value.data(), static_cast<DWORD>(value.size()), config.c_str());
        result.configured = !(count == 1 && value[0] == '\1');
        std::string name = result.configured ? value.data() : "loose.txt";
        if (count == value.size() - 1 || name.empty() || name == "." || name == ".." ||
            name.find_first_of("<>:\"/\\|?*") != std::string::npos ||
            name.back() == '.' || name.back() == ' ')
        {
            result.error = "Invalid loose-list filename in " + config + " for " + result.executable;
            return result;
        }
        result.loose_list = (directory / name).string();
        if (result.configured)
        {
            const auto attributes = GetFileAttributesA(result.loose_list.c_str());
            if (attributes == INVALID_FILE_ATTRIBUTES || (attributes & FILE_ATTRIBUTE_DIRECTORY))
                result.error = "Configured loose list is missing or unreadable: " + result.loose_list;
        }
        return result;
    }

    inline Selection ForCurrentProcess()
    {
        std::array<char, 32768> path{};
        const DWORD count = GetModuleFileNameA(nullptr, path.data(), static_cast<DWORD>(path.size()));
        if (count == 0 || count >= path.size())
        {
            Selection result;
            result.error = "Unable to identify the running executable for loose-file selection.";
            return result;
        }
        return Select(std::filesystem::path(path.data()));
    }
}
