#pragma once

#include <array>
#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include "PluginInterface.h"
#include "ShellBridge.h"
#include "WorkspaceState.h"
#include "nppthemes/ThemeProfile.h"

namespace nppthemes::plugin {

struct OpenDocument {
    int view{};
    int index{};
    std::filesystem::path path;
};

class HostAdapter {
  public:
    void setData(NppData data) noexcept;

    [[nodiscard]] HWND nppHandle() const noexcept;
    [[nodiscard]] HWND scintillaHandle(int view) const noexcept;
    [[nodiscard]] std::filesystem::path pluginConfigDirectory() const;
    [[nodiscard]] std::filesystem::path settingsDirectory() const;
    [[nodiscard]] std::filesystem::path nppDirectory() const;
    [[nodiscard]] std::wstring languageForView(int view) const;
    [[nodiscard]] bool darkModeEnabled() const noexcept;
    [[nodiscard]] std::wstring versionString() const;
    [[nodiscard]] std::vector<OpenDocument> openDocuments() const;
    [[nodiscard]] bool activateDocument(int view, int index) const noexcept;

    void applyProfile(const ThemeProfile& profile);
    void setShellAppearanceEnabled(bool enabled) noexcept;
    void applyShellAppearance(const ThemeProfile& profile);
    [[nodiscard]] bool shellAppearanceActive() const noexcept;
    void restoreEditorAppearance();
    void invalidateSnapshotsForCurrentDocuments();

    [[nodiscard]] WorkspaceSnapshot captureWorkspace() const noexcept;
    void applyWorkspace(std::string_view mode) const noexcept;
    void restoreWorkspace(const WorkspaceSnapshot& snapshot) const noexcept;

  private:
    struct StyleState {
        int foreground{};
        int background{};
        bool bold{};
        bool italic{};
        bool underline{};
        int size{};
        std::array<char, 128> font{};
    };

    struct ElementState {
        bool set{};
        int color{};
    };

    struct ViewSnapshot {
        std::array<StyleState, 256> styles{};
        int caretForeground{};
        int caretLineBackground{};
        int caretLineAlpha{};
        int fontQuality{};
        ElementState selectionBackground{};
        ElementState caretLineElement{};
        ElementState whitespace{};
    };

    [[nodiscard]] std::string snapshotKey(int view, const std::wstring& language) const;
    [[nodiscard]] ViewSnapshot captureView(HWND scintilla) const;
    void restoreView(HWND scintilla, const ViewSnapshot& snapshot) const;
    void applyToView(HWND scintilla, const std::wstring& language, const ThemeProfile& profile) const;
    [[nodiscard]] static int toScintillaColor(Color color) noexcept;
    [[nodiscard]] static Color fromScintillaColor(int color) noexcept;

    NppData data_{};
    ShellBridge shellBridge_;
    bool shellAppearanceEnabled_{false};
    std::map<std::string, ViewSnapshot> snapshots_;
};

} // namespace nppthemes::plugin
