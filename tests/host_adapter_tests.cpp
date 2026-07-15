#include "HostAdapter.h"

#include <filesystem>
#include <iostream>
#include <string>

#include <windows.h>

namespace {

struct FakeHostState {
    bool menuHidden{false};
    bool toolbarHidden{false};
    bool tabbarHidden{false};
    bool statusbarHidden{false};
};

FakeHostState hostState;
int activatedView = -1;
int activatedIndex = -1;
int failures = 0;

void check(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        ++failures;
    }
}

LRESULT copyPath(LPARAM destination, WPARAM capacity, const wchar_t* value) {
    const auto length = ::lstrlenW(value);
    if (destination == 0) {
        return length;
    }
    if (capacity <= static_cast<WPARAM>(length)) {
        return FALSE;
    }
    ::lstrcpyW(reinterpret_cast<wchar_t*>(destination), value);
    return TRUE;
}

LRESULT CALLBACK fakeHostProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case NPPM_ISMENUHIDDEN:
        return hostState.menuHidden;
    case NPPM_ISTOOLBARHIDDEN:
        return hostState.toolbarHidden;
    case NPPM_ISTABBARHIDDEN:
        return hostState.tabbarHidden;
    case NPPM_ISSTATUSBARHIDDEN:
        return hostState.statusbarHidden;
    case NPPM_HIDEMENU: {
        const bool previous = hostState.menuHidden;
        hostState.menuHidden = lParam != FALSE;
        return previous;
    }
    case NPPM_HIDETOOLBAR: {
        const bool previous = hostState.toolbarHidden;
        hostState.toolbarHidden = lParam != FALSE;
        return previous;
    }
    case NPPM_HIDETABBAR: {
        const bool previous = hostState.tabbarHidden;
        hostState.tabbarHidden = lParam != FALSE;
        return previous;
    }
    case NPPM_HIDESTATUSBAR: {
        const bool previous = hostState.statusbarHidden;
        hostState.statusbarHidden = lParam != FALSE;
        return previous;
    }
    case NPPM_GETPLUGINSCONFIGDIR:
        return copyPath(lParam, wParam, L"C:\\test\\plugins\\Config");
    case NPPM_GETNPPSETTINGSDIRPATH:
        return copyPath(lParam, wParam, L"C:\\test\\settings");
    case NPPM_GETNPPVERSION:
        return MAKELONG(970, 8);
    case NPPM_ISDARKMODEENABLED:
        return TRUE;
    case NPPM_GETCURRENTDOCINDEX:
        return -1;
    case NPPM_GETNBOPENFILES:
        return lParam == PRIMARY_VIEW ? 2 : lParam == SECOND_VIEW ? 1 : 3;
    case NPPM_GETBUFFERIDFROMPOS:
        if (lParam == MAIN_VIEW && wParam < 2) {
            return 100 + static_cast<LRESULT>(wParam);
        }
        if (lParam == SUB_VIEW && wParam == 0) {
            return 200;
        }
        return 0;
    case NPPM_GETFULLPATHFROMBUFFERID: {
        const wchar_t* path = wParam == 100   ? L"C:\\work\\alpha.cpp"
                              : wParam == 101 ? L"C:\\work\\beta.py"
                                              : L"D:\\notes\\readme.md";
        if (lParam == 0) {
            return ::lstrlenW(path);
        }
        ::lstrcpyW(reinterpret_cast<wchar_t*>(lParam), path);
        return ::lstrlenW(path);
    }
    case NPPM_ACTIVATEDOC:
        activatedView = static_cast<int>(wParam);
        activatedIndex = static_cast<int>(lParam);
        return TRUE;
    default:
        return ::DefWindowProcW(window, message, wParam, lParam);
    }
}

HWND createFakeHost() {
    WNDCLASSW windowClass{};
    windowClass.lpfnWndProc = fakeHostProc;
    windowClass.hInstance = ::GetModuleHandleW(nullptr);
    windowClass.lpszClassName = L"NppThemesFakeHost";
    ::RegisterClassW(&windowClass);
    return ::CreateWindowExW(0, windowClass.lpszClassName, L"fake", WS_OVERLAPPED, 0, 0, 100, 100, nullptr, nullptr,
                             windowClass.hInstance, nullptr);
}

void testPathsAndCapabilities(nppthemes::plugin::HostAdapter& host) {
    check(host.pluginConfigDirectory() == std::filesystem::path(L"C:\\test\\plugins\\Config"),
          "plugin config path uses public query");
    check(host.settingsDirectory() == std::filesystem::path(L"C:\\test\\settings"),
          "active settings path uses public query");
    check(host.versionString() == L"8.9.7", "padded Notepad++ version formats correctly");
    check(host.darkModeEnabled(), "dark mode capability query forwarded");
}

void testWorkspaceRoundTrip(nppthemes::plugin::HostAdapter& host) {
    hostState = {};
    const auto original = host.captureWorkspace();
    host.applyWorkspace("focus");
    check(!hostState.menuHidden, "focus keeps menu visible");
    check(hostState.toolbarHidden, "focus hides toolbar");
    check(!hostState.tabbarHidden, "focus keeps tabs visible");
    check(hostState.statusbarHidden, "focus hides status bar");
    host.restoreWorkspace(original);
    check(!hostState.menuHidden && !hostState.toolbarHidden && !hostState.tabbarHidden && !hostState.statusbarHidden,
          "focus restores exact original state");

    hostState = {false, true, false, true};
    const auto mixed = host.captureWorkspace();
    host.applyWorkspace("minimal");
    check(hostState.menuHidden && hostState.toolbarHidden && hostState.tabbarHidden && hostState.statusbarHidden,
          "minimal hides documented chrome");
    host.restoreWorkspace(mixed);
    check(!hostState.menuHidden && hostState.toolbarHidden && !hostState.tabbarHidden && hostState.statusbarHidden,
          "minimal restores mixed prior state without defaults");
}

void testDocumentNavigation(nppthemes::plugin::HostAdapter& host) {
    const auto documents = host.openDocuments();
    check(documents.size() == 3, "document enumeration covers both views");
    check(documents.at(0).path.filename() == L"alpha.cpp", "document path resolved from buffer id");
    check(documents.at(2).view == SUB_VIEW && documents.at(2).index == 0,
          "secondary-view document retains activation coordinates");
    check(host.activateDocument(SUB_VIEW, 0), "document activation uses public host message");
    check(activatedView == SUB_VIEW && activatedIndex == 0, "document activation coordinates forwarded");
    check(!host.activateDocument(7, 0), "invalid view rejected before host call");
}

} // namespace

int main() {
    const HWND window = createFakeHost();
    if (window == nullptr) {
        std::cerr << "Unable to create fake host window\n";
        return 1;
    }
    nppthemes::plugin::HostAdapter host;
    host.setData({window, nullptr, nullptr});
    testPathsAndCapabilities(host);
    testWorkspaceRoundTrip(host);
    testDocumentNavigation(host);
    ::DestroyWindow(window);

    if (failures == 0) {
        std::cout << "All NppThemes host adapter tests passed\n";
        return 0;
    }
    std::cerr << failures << " test(s) failed\n";
    return 1;
}
