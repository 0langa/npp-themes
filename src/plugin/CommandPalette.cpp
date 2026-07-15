#include "CommandPalette.h"

#include <algorithm>
#include <array>
#include <cwctype>
#include <filesystem>
#include <string>

#include "Application.h"
#include "Notepad_plus_msgs.h"
#include "resource.h"

namespace nppthemes::plugin {
namespace {

[[nodiscard]] std::wstring lower(std::wstring value) {
    std::transform(value.begin(), value.end(), value.begin(),
                   [](wchar_t character) { return static_cast<wchar_t>(std::towlower(character)); });
    return value;
}

[[nodiscard]] std::wstring widen(std::string_view value) {
    if (value.empty()) {
        return {};
    }
    const int length =
        ::MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.data(), static_cast<int>(value.size()), nullptr, 0);
    if (length <= 0) {
        return {};
    }
    std::wstring result(static_cast<std::size_t>(length), L'\0');
    ::MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.data(), static_cast<int>(value.size()), result.data(),
                          length);
    return result;
}

} // namespace

CommandPalette::CommandPalette(Application& application) : application_(application) {}

CommandPalette::~CommandPalette() = default;

bool CommandPalette::create() {
    if (window_ != nullptr) {
        return true;
    }
    window_ = ::CreateDialogParamW(application_.moduleInstance(), MAKEINTRESOURCEW(IDD_COMMAND_PALETTE),
                                   application_.host().nppHandle(), dialogProc, reinterpret_cast<LPARAM>(this));
    return window_ != nullptr;
}

void CommandPalette::show() {
    if (window_ == nullptr) {
        return;
    }
    rebuildItems();
    ::SetDlgItemTextW(window_, IDC_PALETTE_FILTER, L"");
    applyFilter();

    RECT owner{};
    RECT palette{};
    ::GetWindowRect(application_.host().nppHandle(), &owner);
    ::GetWindowRect(window_, &palette);
    const int width = palette.right - palette.left;
    const int height = palette.bottom - palette.top;
    const int x = static_cast<int>(owner.left) + std::max(0, (static_cast<int>(owner.right - owner.left) - width) / 2);
    const int y = static_cast<int>(owner.top) + std::max(0, (static_cast<int>(owner.bottom - owner.top) - height) / 4);
    ::SetWindowPos(window_, HWND_TOP, x, y, 0, 0, SWP_NOSIZE | SWP_SHOWWINDOW);
    ::SetForegroundWindow(window_);
    const HWND filter = ::GetDlgItem(window_, IDC_PALETTE_FILTER);
    ::SetFocus(filter);
    ::SendMessageW(filter, EM_SETSEL, 0, -1);
}

void CommandPalette::onDarkModeChanged() {
    if (window_ == nullptr) {
        return;
    }
    ::SendMessageW(application_.host().nppHandle(), NPPM_DARKMODESUBCLASSANDTHEME, NppDarkMode::dmfHandleChange,
                   reinterpret_cast<LPARAM>(window_));
    ::InvalidateRect(window_, nullptr, TRUE);
}

INT_PTR CALLBACK CommandPalette::dialogProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    if (message == WM_INITDIALOG) {
        auto* palette = reinterpret_cast<CommandPalette*>(lParam);
        palette->window_ = window;
        ::SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(palette));
        return palette->handleMessage(message, wParam, lParam);
    }
    auto* palette = reinterpret_cast<CommandPalette*>(::GetWindowLongPtrW(window, GWLP_USERDATA));
    return palette == nullptr ? FALSE : palette->handleMessage(message, wParam, lParam);
}

INT_PTR CommandPalette::handleMessage(UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_INITDIALOG:
        ::SendMessageW(application_.host().nppHandle(), NPPM_MODELESSDIALOG, MODELESSDIALOGADD,
                       reinterpret_cast<LPARAM>(window_));
        ::SendMessageW(application_.host().nppHandle(), NPPM_DARKMODESUBCLASSANDTHEME, NppDarkMode::dmfInit,
                       reinterpret_cast<LPARAM>(window_));
        return TRUE;

    case WM_SIZE:
        layout(LOWORD(lParam), HIWORD(lParam));
        return TRUE;

    case WM_COMMAND: {
        const int control = LOWORD(wParam);
        const int notification = HIWORD(wParam);
        if (control == IDC_PALETTE_FILTER && notification == EN_CHANGE) {
            applyFilter();
            return TRUE;
        }
        if ((control == IDC_PALETTE_RESULTS && notification == LBN_DBLCLK) || control == IDOK) {
            runSelected();
            return TRUE;
        }
        if (control == IDCANCEL) {
            ::ShowWindow(window_, SW_HIDE);
            return TRUE;
        }
        return FALSE;
    }

    case WM_CLOSE:
        ::ShowWindow(window_, SW_HIDE);
        return TRUE;

    case WM_DESTROY:
        ::SendMessageW(application_.host().nppHandle(), NPPM_MODELESSDIALOG, MODELESSDIALOGREMOVE,
                       reinterpret_cast<LPARAM>(window_));
        window_ = nullptr;
        return TRUE;

    default:
        return FALSE;
    }
}

void CommandPalette::rebuildItems() {
    items_ = {
        {ItemKind::OpenStudio, L"Command: Open Theme Studio"},
        {ItemKind::NextTheme, L"Command: Apply next theme"},
        {ItemKind::Focus, L"Workspace: Toggle Focus"},
        {ItemKind::Minimal, L"Workspace: Toggle Minimal"},
        {ItemKind::Presentation, L"Workspace: Toggle Presentation"},
        {ItemKind::Restore, L"Command: Restore native appearance"},
    };
    const auto& profiles = application_.profiles();
    for (std::size_t index = 0; index < profiles.size(); ++index) {
        Item item{ItemKind::Profile, L"Theme: " + widen(profiles[index].name)};
        item.profileIndex = index;
        items_.push_back(std::move(item));
    }
    for (const auto& document : application_.host().openDocuments()) {
        const auto filename = document.path.filename().wstring();
        const auto parent = document.path.parent_path().wstring();
        Item item{ItemKind::Document, L"Document: " + filename};
        if (!parent.empty()) {
            item.label += L" — " + parent;
        }
        item.view = document.view;
        item.documentIndex = document.index;
        items_.push_back(std::move(item));
    }
}

void CommandPalette::applyFilter() {
    if (window_ == nullptr) {
        return;
    }
    std::array<wchar_t, 512> buffer{};
    ::GetDlgItemTextW(window_, IDC_PALETTE_FILTER, buffer.data(), static_cast<int>(buffer.size()));
    const auto filter = lower(buffer.data());
    const HWND results = ::GetDlgItem(window_, IDC_PALETTE_RESULTS);
    ::SendMessageW(results, LB_RESETCONTENT, 0, 0);
    visibleItems_.clear();
    for (std::size_t index = 0; index < items_.size(); ++index) {
        if (!filter.empty() && lower(items_[index].label).find(filter) == std::wstring::npos) {
            continue;
        }
        visibleItems_.push_back(index);
        ::SendMessageW(results, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(items_[index].label.c_str()));
    }
    if (!visibleItems_.empty()) {
        ::SendMessageW(results, LB_SETCURSEL, 0, 0);
    }
}

void CommandPalette::runSelected() {
    const auto selection = static_cast<int>(::SendDlgItemMessageW(window_, IDC_PALETTE_RESULTS, LB_GETCURSEL, 0, 0));
    if (selection < 0 || static_cast<std::size_t>(selection) >= visibleItems_.size()) {
        return;
    }
    const Item item = items_[visibleItems_[static_cast<std::size_t>(selection)]];
    ::ShowWindow(window_, SW_HIDE);
    switch (item.kind) {
    case ItemKind::OpenStudio:
        application_.showThemeStudio();
        break;
    case ItemKind::NextTheme:
        application_.applyNextTheme();
        break;
    case ItemKind::Focus:
        application_.toggleFocusWorkspace();
        break;
    case ItemKind::Minimal:
        application_.toggleMinimalWorkspace();
        break;
    case ItemKind::Presentation:
        application_.togglePresentationWorkspace();
        break;
    case ItemKind::Restore:
        application_.restoreNativeAppearance();
        break;
    case ItemKind::Profile:
        application_.chooseProfile(item.profileIndex);
        break;
    case ItemKind::Document:
        static_cast<void>(application_.host().activateDocument(item.view, item.documentIndex));
        break;
    }
}

void CommandPalette::layout(int width, int height) const {
    constexpr int margin = 10;
    constexpr int editHeight = 25;
    constexpr int buttonHeight = 25;
    constexpr int buttonWidth = 72;
    const int contentWidth = std::max(80, width - 2 * margin);
    const int listTop = margin + editHeight + 8;
    const int listHeight = std::max(40, height - listTop - buttonHeight - 2 * margin);
    ::MoveWindow(::GetDlgItem(window_, IDC_PALETTE_FILTER), margin, margin, contentWidth, editHeight, TRUE);
    ::MoveWindow(::GetDlgItem(window_, IDC_PALETTE_RESULTS), margin, listTop, contentWidth, listHeight, TRUE);
    ::MoveWindow(::GetDlgItem(window_, IDOK), width - margin - 2 * buttonWidth - 8, height - margin - buttonHeight,
                 buttonWidth, buttonHeight, TRUE);
    ::MoveWindow(::GetDlgItem(window_, IDCANCEL), width - margin - buttonWidth, height - margin - buttonHeight,
                 buttonWidth, buttonHeight, TRUE);
}

} // namespace nppthemes::plugin
