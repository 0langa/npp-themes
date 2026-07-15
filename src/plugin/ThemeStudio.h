#pragma once

#include <string>

#include <windows.h>

#include "nppthemes/ThemeProfile.h"

namespace nppthemes::plugin {

class Application;

class ThemeStudio {
  public:
    explicit ThemeStudio(Application& application);
    ~ThemeStudio();

    bool create(int commandId);
    void show();
    void refresh();
    void onDarkModeChanged();

  private:
    static INT_PTR CALLBACK dialogProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam);
    INT_PTR handleMessage(UINT message, WPARAM wParam, LPARAM lParam);
    void populateProfiles();
    void loadProfileToControls(const ThemeProfile& profile);
    [[nodiscard]] ThemeProfile profileFromControls() const;
    void chooseColor(int controlId, Color& color);
    void updateContrastLabel();
    void updateShellStatus();
    void drawColorButton(const DRAWITEMSTRUCT& item) const;

    Application& application_;
    HWND window_{};
    int commandId_{};
    std::wstring panelName_{L"NppThemes Theme Studio"};
    std::wstring moduleName_;
    ThemeProfile editedProfile_;
};

} // namespace nppthemes::plugin
