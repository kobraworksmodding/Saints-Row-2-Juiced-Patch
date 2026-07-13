module;

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <wininet.h>
#include <string>

#include "UtilsGlobal.h"
#pragma comment(lib, "wininet.lib")

export module PatchNotifier;
#if !JLITE && !RELOADED
export namespace PatchNotifier {

    struct VersionInfo {
        int major, minor, patch;

        VersionInfo(int maj = 0, int min = 0, int pat = 0) : major(maj), minor(min), patch(pat) {}

        static VersionInfo fromString(const std::string& versionStr) {
            VersionInfo version;
            std::string clean = versionStr;

            if (!clean.empty() && clean[0] == 'v') clean = clean.substr(1);

            if (sscanf_s(clean.c_str(), "%d.%d.%d", &version.major, &version.minor, &version.patch) < 3) {
                version.patch = 0;
                sscanf_s(clean.c_str(), "%d.%d", &version.major, &version.minor);
            }

            return version;
        }

        std::string toString() const {
            return std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(patch);
        }

        bool isNewerThan(const VersionInfo& other) const {
            if (major != other.major) return major > other.major;
            if (minor != other.minor) return minor > other.minor;
            return patch > other.patch;
        }
    };

    struct UpdateResult {
        bool updateAvailable = false;
        bool checkSuccessful = false;
        std::string currentVersion;
        std::string latestVersion;
        std::string downloadUrl;
        std::string releaseNotes;
        std::string errorMessage;
    };

    class GitHubReleaseChecker {
    private:
        static std::string httpGet(const std::string& url) {
            std::string result;

            HINTERNET hInternet = InternetOpenA("SR2JuicedPatch/1.0",
                INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
            if (!hInternet) return "";

            HINTERNET hUrl = InternetOpenUrlA(hInternet, url.c_str(), NULL, 0,
                INTERNET_FLAG_SECURE | INTERNET_FLAG_RELOAD, 0);
            if (!hUrl) {
                InternetCloseHandle(hInternet);
                return "";
            }

            char buffer[4096]{};
            DWORD bytesRead;
            while (InternetReadFile(hUrl, buffer, sizeof(buffer) - 1, &bytesRead) && bytesRead > 0) {
                buffer[bytesRead] = '\0';
                result += buffer;
            }

            InternetCloseHandle(hUrl);
            InternetCloseHandle(hInternet);
            return result;
        }

        static std::string extractJsonString(const std::string& json, const std::string& key) {
            std::string searchKey = "\"" + key + "\":\"";
            size_t pos = json.find(searchKey);
            if (pos == std::string::npos) return "";

            pos += searchKey.length();
            size_t endPos = json.find("\"", pos);
            if (endPos == std::string::npos) return "";

            return json.substr(pos, endPos - pos);
        }

        static std::string unescapeJson(const std::string& str) {
            std::string result = str;
            size_t pos = 0;
            while ((pos = result.find("\\n", pos)) != std::string::npos) {
                result.replace(pos, 2, "\n");
                pos += 1;
            }
            while ((pos = result.find("\\r", pos)) != std::string::npos) {
                result.replace(pos, 2, "\r");
                pos += 1;
            }
            while ((pos = result.find("\\\"", pos)) != std::string::npos) {
                result.replace(pos, 2, "\"");
                pos += 1;
            }
            return result;
        }

    public:
        static UpdateResult checkForUpdate(const std::string& currentVersion,
            const std::string& repoOwner = "kobraworksmodding",
            const std::string& repoName = "Saints-Row-2-Juiced-Patch") {
            UpdateResult result;
            result.currentVersion = currentVersion;

            try {
                std::string apiUrl = "https://api.github.com/repos/" + repoOwner + "/" + repoName + "/releases/latest";

                std::string response = httpGet(apiUrl);
                if (response.empty()) {
                    result.errorMessage = "Failed to fetch release data from GitHub";
                    return result;
                }
                if (response.find("\"message\":") != std::string::npos) {
                    result.errorMessage = "GitHub API error: " + extractJsonString(response, "message");
                    return result;
                }

                // Extract version from tag_name
                std::string tagName = extractJsonString(response, "tag_name");
                if (tagName.empty()) {
                    result.errorMessage = "Could not find tag_name in API response";
                    return result;
                }

                result.latestVersion = tagName;
                result.downloadUrl = extractJsonString(response, "html_url");
                result.releaseNotes = unescapeJson(extractJsonString(response, "body"));
                result.checkSuccessful = true;

                VersionInfo current = VersionInfo::fromString(currentVersion);
                VersionInfo latest = VersionInfo::fromString(tagName);
                result.updateAvailable = latest.isNewerThan(current);

            }
            catch (const std::exception& e) {
                result.errorMessage = "Exception during update check: " + std::string(e.what());
            }
            catch (...) {
                result.errorMessage = "Unknown error during update check";
            }

            return result;
        }
    };

    UpdateResult checkForPatchUpdate() {
        return GitHubReleaseChecker::checkForUpdate(UtilsGlobal::juicedversion);
    }

} // namespace PatchNotifier
#endif