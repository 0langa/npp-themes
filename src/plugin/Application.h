#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "HostAdapter.h"
#include "PluginInterface.h"
#include "SettingsStore.h"
#include "nppthemes/ThemeProfile.h"

namespace nppthemes::plugin {

class ThemeStudio;
class CommandPalette;

class Application {
  public:
    static Application& instance();

    void processAttach(HINSTANCE instance) noexcept;
    void processDetach() noexcept;
    void setInfo(NppData data);
    void setThemeStudioCommandId(int commandId) noexcept;
    void onNotification(const SCNotification& notification) noexcept;

    void showThemeStudio();
    void showCommandPalette();
    void applyNextTheme();
    void toggleFocusWorkspace();
    void toggleMinimalWorkspace();
    void togglePresentationWorkspace();
    void restoreNativeAppearance();
    void exportCurrentTheme();
    void showAbout() const;

    [[nodiscard]] HINSTANCE moduleInstance() const noexcept;
    [[nodiscard]] HostAdapter& host() noexcept;
    [[nodiscard]] const std::vector<ThemeProfile>& profiles() const noexcept;
    [[nodiscard]] ThemeProfile currentProfile() const;
    [[nodiscard]] std::size_t currentProfileIndex() const noexcept;
    void previewProfile(std::size_t index);
    void cancelPreview();
    void applyCustomProfile(ThemeProfile profile);
    void chooseProfile(std::size_t index);
    void importThemeFromDialog(HWND owner);
    void exportThemeFromDialog(HWND owner);
    void installCurrentThemeXml(HWND owner);

  private:
    Application();
    ~Application();
    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    void onReady();
    void reapplyCurrentProfile();
    void persist();
    void activateWorkspace(std::string mode);
    void restoreWorkspaceOnly();
    [[nodiscard]] std::optional<std::size_t> findProfile(std::string_view id) const;
    [[nodiscard]] std::filesystem::path chooseProfilePath(HWND owner, bool save) const;

    HINSTANCE moduleInstance_{};
    HostAdapter host_;
    SettingsStore store_;
    PersistedSettings settings_;
    std::vector<ThemeProfile> profiles_;
    std::size_t currentProfileIndex_{0};
    std::unique_ptr<ThemeStudio> studio_;
    std::unique_ptr<CommandPalette> commandPalette_;
    int themeStudioCommandId_{};
    bool ready_{false};
};

} // namespace nppthemes::plugin
