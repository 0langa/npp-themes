#pragma once

#include <filesystem>
#include <optional>
#include <string>

#include "WorkspaceState.h"
#include "nppthemes/ThemeProfile.h"

namespace nppthemes::plugin {

struct PersistedSettings {
    int schemaVersion{1};
    std::string activeProfileId{"builtin.northern-lights"};
    std::optional<ThemeProfile> customProfile;
    bool applyOnStartup{true};
    WorkspaceSession workspace;
};

class SettingsStore {
  public:
    bool initialize(const std::filesystem::path& pluginConfigRoot);
    [[nodiscard]] PersistedSettings load() const;
    [[nodiscard]] bool save(const PersistedSettings& settings) const;
    [[nodiscard]] bool exportProfile(const std::filesystem::path& destination, const ThemeProfile& profile) const;
    [[nodiscard]] ThemeProfile importProfile(const std::filesystem::path& source) const;
    void log(std::wstring_view message) const noexcept;

    [[nodiscard]] const std::filesystem::path& root() const noexcept;
    [[nodiscard]] const std::filesystem::path& settingsPath() const noexcept;

  private:
    [[nodiscard]] static bool atomicWrite(const std::filesystem::path& destination, std::string_view content);

    std::filesystem::path root_;
    std::filesystem::path settingsPath_;
    std::filesystem::path logPath_;
};

} // namespace nppthemes::plugin
