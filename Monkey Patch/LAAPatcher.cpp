#include <windows.h>
#include <direct.h>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <shellapi.h>
#include "FileLogger.h"

namespace LAAPatcher {
    std::wstring rootPath;
    bool GameIsLargeAddressAware()
    {
        static PBYTE module_base = reinterpret_cast<PBYTE>(GetModuleHandle(nullptr));

        PIMAGE_DOS_HEADER dos_header = reinterpret_cast<PIMAGE_DOS_HEADER>(module_base);
        PIMAGE_NT_HEADERS nt_headers = reinterpret_cast<PIMAGE_NT_HEADERS>(module_base + dos_header->e_lfanew);

        return (nt_headers->FileHeader.Characteristics & IMAGE_FILE_LARGE_ADDRESS_AWARE) == IMAGE_FILE_LARGE_ADDRESS_AWARE;
    }

    uint32_t calc_checksum(uint32_t checksum, void* data, int length) {
        if (length && data != nullptr) {
            uint32_t sum = 0;
            do {
                sum = *(uint16_t*)data + checksum;
                checksum = (uint16_t)sum + (sum >> 16);
                data = (char*)data + 2;
            } while (--length);
        }

        return checksum + (checksum >> 16);
    }

    void LAACheck()
    {
        Logger::TypedLog(CHN_LAA, "Checking if LAA needs to be patched.\n");

        static bool LAAChecked = false;
        if (LAAChecked) {
            Logger::TypedLog(CHN_LAA, "We've already checked for LAA. Skipping...\n");
            return; // user was prompted/EXE checked already, exit out
        }

        LAAChecked = true; // prevent us from checking more than once

        if (GameIsLargeAddressAware()) {
            Logger::TypedLog(CHN_LAA, "Game is already LAA. Skipping...\n");
            return; // Game is already 4GB/LAA patched, exit out
        }

            char module_path_array[4096];
            GetModuleFileNameA(GetModuleHandle(nullptr), module_path_array, 4096);

            std::string module_path = module_path_array;
            std::string module_path_new = module_path + ".new";
            std::string module_path_bak = module_path + ".bak";

            if (GetFileAttributesA(module_path_new.c_str()) != 0xFFFFFFFF)
                DeleteFileA(module_path_new.c_str());

            if (GetFileAttributesA(module_path_bak.c_str()) != 0xFFFFFFFF)
                DeleteFileA(module_path_bak.c_str());

            BOOL result_CopyFileA = CopyFileA(module_path.c_str(), module_path_new.c_str(), false);

            FILE* file;
            int LAA_ErrorNum = fopen_s(&file, module_path_new.c_str(), "rb+");
            if (LAA_ErrorNum == 0)
            {
                fseek(file, 0, SEEK_END);
                std::vector<uint8_t> exe_data(ftell(file));
                fseek(file, 0, SEEK_SET);

                if (fread(exe_data.data(), 1, exe_data.size(), file) != exe_data.size())
                {
                    fclose(file);
                    LAA_ErrorNum = 1;
                }
                else
                {
                    PIMAGE_DOS_HEADER dos_header = reinterpret_cast<PIMAGE_DOS_HEADER>(exe_data.data());
                    PIMAGE_NT_HEADERS nt_headers = reinterpret_cast<PIMAGE_NT_HEADERS>(exe_data.data() + dos_header->e_lfanew);

                    // Set LAA flag in PE headers
                    nt_headers->FileHeader.Characteristics |= IMAGE_FILE_LARGE_ADDRESS_AWARE;

                    // Fix up PE checksum
                    uint32_t header_size = (uintptr_t)nt_headers - (uintptr_t)exe_data.data() +
                        ((uintptr_t)&nt_headers->OptionalHeader.CheckSum - (uintptr_t)nt_headers);

                    // Skip over CheckSum field
                    uint32_t remain_size = (exe_data.size() - header_size - 4) / sizeof(uint16_t);
                    void* remain = &nt_headers->OptionalHeader.Subsystem;

                    uint32_t header_checksum = calc_checksum(0, exe_data.data(), header_size / sizeof(uint16_t));
                    uint32_t file_checksum = calc_checksum(header_checksum, remain, remain_size);
                    if (exe_data.size() & 1)
                        file_checksum += *((char*)exe_data.data() + exe_data.size() - 1);

                    nt_headers->OptionalHeader.CheckSum = file_checksum + exe_data.size();

                    fseek(file, dos_header->e_lfanew, SEEK_SET);
                    auto wrote = fwrite(nt_headers, sizeof(IMAGE_NT_HEADERS), 1, file);
                    fclose(file);

                    if (wrote != 1)
                    {
                        LAA_ErrorNum = 2;
                    }
                    else
                    {
                        BOOL result_moveFile1 = MoveFileExA(module_path.c_str(), module_path_bak.c_str(), MOVEFILE_REPLACE_EXISTING);
                        BOOL result_moveFile2 = MoveFileA(module_path_new.c_str(), module_path.c_str());
                        if (!result_moveFile1)
                            LAA_ErrorNum = 3;
                        else if (!result_moveFile2)
                            LAA_ErrorNum = 4;

                        if (result_moveFile1 && !result_moveFile2)
                        {
                            // Try restoring the original EXE if replacement failed
                            MoveFileA(module_path_bak.c_str(), module_path.c_str());
                        }
                    }
                }
            }

            if (LAA_ErrorNum == 0)
            {
                MessageBoxA(NULL, "Successfully LAA Patched SR2\n\nWhat does this mean? it means you will/should suffer far less crashes than someone normally would on Saints Row 2 without the LAA (Large Address Aware) patch.\n\nThis is a one time pop-up, do not be alarmed. The game will now close when you press OK so the patch can take effect. Feel free to re-launch.\n\nA backup copy of the original Executable has also been made." , "Saints Row 2 Juiced Patch", MB_ICONINFORMATION | MB_OK);
                    exit(0);
            }
            else
            {
                MessageBoxA(NULL, "Failed to perform the LAA Patch. Search up NTCore 4GB Patch on Google and try patching the EXE that way.", "Saints Row 2 Juiced" , MB_ICONEXCLAMATION);
            }
        }
    }