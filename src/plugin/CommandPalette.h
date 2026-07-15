#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include <windows.h>

namespace nppthemes::plugin {

class Application;

class CommandPalette {
  public:
    explicit CommandPalette(Application& application);
    ~CommandPalette();

    [[nodiscard]] bool create();
    void show();
    void onDarkModeChanged();

  private:
    enum class ItemKind {
        OpenStudio,
        NextTheme,
        Focus,
        Minimal,
        Presentation,
        Restore,
        Profile,
        Document,
    };

    struct Item {
        ItemKind kind{};
        std::wstring label;
        std::size_t profileIndex{};
        int view{};
        int documentIndex{};
    };

    static INT_PTR CALLBACK dialogProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam);
    INT_PTR handleMessage(UINT message, WPARAM wParam, LPARAM lParam);
    void rebuildItems();
    void applyFilter();
    void runSelected();
    void layout(int width, int height) const;

    Application& application_;
    HWND window_{};
    std::vector<Item> items_;
    std::vector<std::size_t> visibleItems_;
};

} // namespace nppthemes::plugin
