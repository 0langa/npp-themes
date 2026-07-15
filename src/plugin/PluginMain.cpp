#include <array>
#include <cwchar>
#include <functional>

#include <windows.h>

#include "Application.h"
#include "PluginInterface.h"

namespace {

constexpr wchar_t pluginName[] = L"NppThemes";
constexpr int commandCount = 9;
std::array<FuncItem, commandCount> commands{};
std::array<ShortcutKey, 5> shortcuts{};
bool commandsInitialized = false;
HINSTANCE pluginModule = nullptr;

void invoke(const std::function<void()>& action) noexcept {
    try {
        action();
    } catch (...) {
        ::MessageBoxW(nullptr, L"NppThemes command failed. See plugin log for details.", L"NppThemes",
                      MB_OK | MB_ICONERROR);
    }
}

void showCommandPalette() {
    invoke([] { nppthemes::plugin::Application::instance().showCommandPalette(); });
}

void showStudio() {
    invoke([] { nppthemes::plugin::Application::instance().showThemeStudio(); });
}

void nextTheme() {
    invoke([] { nppthemes::plugin::Application::instance().applyNextTheme(); });
}

void focusWorkspace() {
    invoke([] { nppthemes::plugin::Application::instance().toggleFocusWorkspace(); });
}

void minimalWorkspace() {
    invoke([] { nppthemes::plugin::Application::instance().toggleMinimalWorkspace(); });
}

void presentationWorkspace() {
    invoke([] { nppthemes::plugin::Application::instance().togglePresentationWorkspace(); });
}

void restoreAppearance() {
    invoke([] { nppthemes::plugin::Application::instance().restoreNativeAppearance(); });
}

void exportTheme() {
    invoke([] { nppthemes::plugin::Application::instance().exportCurrentTheme(); });
}

void about() {
    invoke([] { nppthemes::plugin::Application::instance().showAbout(); });
}

void setCommand(int index, const wchar_t* name, PFUNCPLUGINCMD function, ShortcutKey* shortcut = nullptr) {
    ::lstrcpynW(commands[static_cast<std::size_t>(index)]._itemName, name, menuItemSize);
    commands[static_cast<std::size_t>(index)]._pFunc = function;
    commands[static_cast<std::size_t>(index)]._pShKey = shortcut;
}

void initializeCommands() {
    if (commandsInitialized) {
        return;
    }
    shortcuts[0] = {true, true, false, 'P'};
    shortcuts[1] = {true, true, false, 'T'};
    shortcuts[2] = {true, true, false, 'F'};
    shortcuts[3] = {true, true, false, 'M'};
    shortcuts[4] = {true, true, false, 'R'};

    setCommand(0, L"Command Palette...", showCommandPalette, &shortcuts[0]);
    setCommand(1, L"Theme Studio...", showStudio, &shortcuts[1]);
    setCommand(2, L"Apply Next Theme", nextTheme);
    setCommand(3, L"Toggle Focus Workspace", focusWorkspace, &shortcuts[2]);
    setCommand(4, L"Toggle Minimal Workspace", minimalWorkspace, &shortcuts[3]);
    setCommand(5, L"Toggle Presentation Workspace", presentationWorkspace);
    setCommand(6, L"Restore Native Appearance", restoreAppearance, &shortcuts[4]);
    setCommand(7, L"Export Current Theme...", exportTheme);
    setCommand(8, L"About NppThemes", about);
    commandsInitialized = true;
}

} // namespace

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        pluginModule = module;
        ::DisableThreadLibraryCalls(module);
    }
    return TRUE;
}

extern "C" __declspec(dllexport) void setInfo(NppData data) {
    initializeCommands();
    nppthemes::plugin::Application::instance().processAttach(pluginModule);
    nppthemes::plugin::Application::instance().setInfo(data);
}

extern "C" __declspec(dllexport) const wchar_t* getName() {
    return pluginName;
}

extern "C" __declspec(dllexport) FuncItem* getFuncsArray(int* count) {
    initializeCommands();
    if (count != nullptr) {
        *count = commandCount;
    }
    return commands.data();
}

extern "C" __declspec(dllexport) void beNotified(SCNotification* notification) {
    if (notification == nullptr) {
        return;
    }
    if (notification->nmhdr.code == NPPN_READY) {
        nppthemes::plugin::Application::instance().setThemeStudioCommandId(commands[1]._cmdID);
    }
    nppthemes::plugin::Application::instance().onNotification(*notification);
}

extern "C" __declspec(dllexport) LRESULT messageProc(UINT, WPARAM, LPARAM) {
    return TRUE;
}

extern "C" __declspec(dllexport) BOOL isUnicode() {
    return TRUE;
}
