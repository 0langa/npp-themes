#include "SettingsStore.h"

#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>

#include <nlohmann/json.hpp>
#include <windows.h>

namespace nppthemes::plugin {
namespace {

using nlohmann::json;
constexpr std::uintmax_t maxProfileBytes = 1024U * 1024U;

[[nodiscard]] std::string readBoundedFile(const std::filesystem::path& path) {
    std::error_code error;
    const auto size = std::filesystem::file_size(path, error);
    if (error) {
        throw std::runtime_error("Unable to inspect file");
    }
    if (size > maxProfileBytes) {
        throw std::invalid_argument("File exceeds 1 MiB limit");
    }
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        throw std::runtime_error("Unable to open file");
    }
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

[[nodiscard]] PersistedSettings parseSettingsFile(const std::filesystem::path& path) {
    PersistedSettings settings;
    const auto document = json::parse(readBoundedFile(path));
    settings.schemaVersion = document.value("schemaVersion", 1);
    if (settings.schemaVersion != 1) {
        throw std::invalid_argument("Unsupported settings schema");
    }
    settings.activeProfileId = document.value("activeProfileId", settings.activeProfileId);
    settings.applyOnStartup = document.value("applyOnStartup", true);
    settings.themeWindowFrame = document.value("themeWindowFrame", false);
    if (document.contains("customProfile") && !document.at("customProfile").is_null()) {
        settings.customProfile = deserializeProfile(document.at("customProfile").dump());
    }
    if (const auto workspace = document.find("workspace"); workspace != document.end()) {
        settings.workspace.active = workspace->value("active", false);
        settings.workspace.mode = workspace->value("mode", "");
        if (const auto previous = workspace->find("previous"); previous != workspace->end()) {
            settings.workspace.previous.menuHidden = previous->value("menuHidden", false);
            settings.workspace.previous.toolbarHidden = previous->value("toolbarHidden", false);
            settings.workspace.previous.tabbarHidden = previous->value("tabbarHidden", false);
            settings.workspace.previous.statusbarHidden = previous->value("statusbarHidden", false);
        }
    }
    return settings;
}

} // namespace

bool SettingsStore::initialize(const std::filesystem::path& pluginConfigRoot) {
    if (pluginConfigRoot.empty()) {
        return false;
    }
    root_ = pluginConfigRoot / L"NppThemes";
    settingsPath_ = root_ / L"settings.json";
    logPath_ = root_ / L"NppThemes.log";

    std::error_code error;
    std::filesystem::create_directories(root_, error);
    return !error;
}

PersistedSettings SettingsStore::load() const {
    if (settingsPath_.empty() || !std::filesystem::exists(settingsPath_)) {
        return {};
    }

    try {
        return parseSettingsFile(settingsPath_);
    } catch (const std::exception&) {
        auto backup = settingsPath_;
        backup += L".bak";
        if (std::filesystem::exists(backup)) {
            try {
                const auto recovered = parseSettingsFile(backup);
                log(L"Settings load failed; valid backup recovered");
                return recovered;
            } catch (const std::exception&) {
            }
        }
        log(L"Settings and backup load failed; defaults restored");
        return {};
    }
}

bool SettingsStore::save(const PersistedSettings& settings) const {
    if (settingsPath_.empty()) {
        return false;
    }
    json customProfile = nullptr;
    if (settings.customProfile) {
        customProfile = json::parse(serializeProfile(*settings.customProfile));
    }
    const json document = {
        {"schemaVersion", 1},
        {"activeProfileId", settings.activeProfileId},
        {"applyOnStartup", settings.applyOnStartup},
        {"themeWindowFrame", settings.themeWindowFrame},
        {"customProfile", customProfile},
        {"workspace",
         {
             {"active", settings.workspace.active},
             {"mode", settings.workspace.mode},
             {"previous",
              {
                  {"menuHidden", settings.workspace.previous.menuHidden},
                  {"toolbarHidden", settings.workspace.previous.toolbarHidden},
                  {"tabbarHidden", settings.workspace.previous.tabbarHidden},
                  {"statusbarHidden", settings.workspace.previous.statusbarHidden},
              }},
         }},
    };
    return atomicWrite(settingsPath_, document.dump(2) + '\n');
}

bool SettingsStore::exportProfile(const std::filesystem::path& destination, const ThemeProfile& profile) const {
    if (!validateProfile(profile).empty()) {
        return false;
    }
    return atomicWrite(destination, serializeProfile(profile));
}

ThemeProfile SettingsStore::importProfile(const std::filesystem::path& source) const {
    return deserializeProfile(readBoundedFile(source));
}

void SettingsStore::log(std::wstring_view message) const noexcept {
    if (logPath_.empty()) {
        return;
    }
    try {
        SYSTEMTIME time{};
        ::GetLocalTime(&time);
        std::wofstream output(logPath_, std::ios::app);
        output << L'[' << time.wYear << L'-';
        output.width(2);
        output.fill(L'0');
        output << time.wMonth << L'-';
        output.width(2);
        output << time.wDay << L' ';
        output.width(2);
        output << time.wHour << L':';
        output.width(2);
        output << time.wMinute << L':';
        output.width(2);
        output << time.wSecond << L"] " << message << L'\n';
    } catch (...) {
    }
}

const std::filesystem::path& SettingsStore::root() const noexcept {
    return root_;
}

const std::filesystem::path& SettingsStore::settingsPath() const noexcept {
    return settingsPath_;
}

bool SettingsStore::atomicWrite(const std::filesystem::path& destination, std::string_view content) {
    if (destination.empty()) {
        return false;
    }
    std::error_code error;
    std::filesystem::create_directories(destination.parent_path(), error);
    if (error) {
        return false;
    }

    auto temporary = destination;
    temporary += L".tmp";
    {
        std::ofstream output(temporary, std::ios::binary | std::ios::trunc);
        if (!output) {
            return false;
        }
        output.write(content.data(), static_cast<std::streamsize>(content.size()));
        output.flush();
        if (!output) {
            return false;
        }
    }

    if (std::filesystem::exists(destination)) {
        auto backup = destination;
        backup += L".bak";
        std::filesystem::copy_file(destination, backup, std::filesystem::copy_options::overwrite_existing, error);
        if (error) {
            std::filesystem::remove(temporary, error);
            return false;
        }
    }

    if (!::MoveFileExW(temporary.c_str(), destination.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        std::filesystem::remove(temporary, error);
        return false;
    }
    return true;
}

} // namespace nppthemes::plugin
