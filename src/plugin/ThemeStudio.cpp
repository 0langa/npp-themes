#include "ThemeStudio.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <string>

#include <commdlg.h>

#include "Application.h"
#include "Docking.h"
#include "Notepad_plus_msgs.h"
#include "resource.h"

namespace nppthemes::plugin {
namespace {

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

[[nodiscard]] std::string narrow(std::wstring_view value) {
    if (value.empty()) {
        return {};
    }
    const int length = ::WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, value.data(),
                                             static_cast<int>(value.size()), nullptr, 0, nullptr, nullptr);
    if (length <= 0) {
        return {};
    }
    std::string result(static_cast<std::size_t>(length), '\0');
    ::WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, value.data(), static_cast<int>(value.size()), result.data(),
                          length, nullptr, nullptr);
    return result;
}

[[nodiscard]] COLORREF toColorRef(Color color) {
    return RGB((color >> 16U) & 0xFFU, (color >> 8U) & 0xFFU, color & 0xFFU);
}

[[nodiscard]] Color fromColorRef(COLORREF color) {
    return (static_cast<Color>(GetRValue(color)) << 16U) | (static_cast<Color>(GetGValue(color)) << 8U) |
           static_cast<Color>(GetBValue(color));
}

} // namespace

ThemeStudio::ThemeStudio(Application& application)
    : application_(application), editedProfile_(application.currentProfile()) {}

ThemeStudio::~ThemeStudio() = default;

bool ThemeStudio::create(int commandId) {
    if (window_ != nullptr) {
        return true;
    }
    commandId_ = commandId;
    window_ = ::CreateDialogParamW(application_.moduleInstance(), MAKEINTRESOURCEW(IDD_THEME_STUDIO),
                                   application_.host().nppHandle(), dialogProc, reinterpret_cast<LPARAM>(this));
    if (window_ == nullptr) {
        return false;
    }

    std::array<wchar_t, 32768> modulePath{};
    const DWORD copied =
        ::GetModuleFileNameW(application_.moduleInstance(), modulePath.data(), static_cast<DWORD>(modulePath.size()));
    if (copied > 0 && copied < modulePath.size()) {
        moduleName_ = std::filesystem::path(modulePath.data()).filename().wstring();
    } else {
        moduleName_ = L"NppThemes.dll";
    }

    DockedWidgetData docking{};
    docking.hClient = window_;
    docking.pszName = panelName_.c_str();
    docking.dlgID = commandId_;
    docking.uMask = DWS_DF_FLOATING;
    docking.pszModuleName = moduleName_.c_str();
    const auto registered =
        ::SendMessageW(application_.host().nppHandle(), NPPM_DMMREGASDCKDLG, 0, reinterpret_cast<LPARAM>(&docking));
    return registered != FALSE;
}

void ThemeStudio::show() {
    if (window_ != nullptr) {
        ::SendMessageW(application_.host().nppHandle(), NPPM_DMMSHOW, 0, reinterpret_cast<LPARAM>(window_));
    }
}

void ThemeStudio::refresh() {
    if (window_ == nullptr) {
        return;
    }
    populateProfiles();
    const auto index = application_.currentProfileIndex();
    ::SendDlgItemMessageW(window_, IDC_PROFILE_COMBO, CB_SETCURSEL, index, 0);
    editedProfile_ = application_.currentProfile();
    loadProfileToControls(editedProfile_);
}

void ThemeStudio::onDarkModeChanged() {
    if (window_ == nullptr) {
        return;
    }
    ::SendMessageW(application_.host().nppHandle(), NPPM_DARKMODESUBCLASSANDTHEME, NppDarkMode::dmfHandleChange,
                   reinterpret_cast<LPARAM>(window_));
    ::SetWindowPos(window_, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
    ::InvalidateRect(window_, nullptr, TRUE);
}

INT_PTR CALLBACK ThemeStudio::dialogProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    if (message == WM_INITDIALOG) {
        auto* studio = reinterpret_cast<ThemeStudio*>(lParam);
        studio->window_ = window;
        ::SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(studio));
        return studio->handleMessage(message, wParam, lParam);
    }
    auto* studio = reinterpret_cast<ThemeStudio*>(::GetWindowLongPtrW(window, GWLP_USERDATA));
    return studio == nullptr ? FALSE : studio->handleMessage(message, wParam, lParam);
}

INT_PTR ThemeStudio::handleMessage(UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_INITDIALOG:
        ::SendMessageW(application_.host().nppHandle(), NPPM_MODELESSDIALOG, MODELESSDIALOGADD,
                       reinterpret_cast<LPARAM>(window_));
        ::SendMessageW(application_.host().nppHandle(), NPPM_DARKMODESUBCLASSANDTHEME, NppDarkMode::dmfInit,
                       reinterpret_cast<LPARAM>(window_));
        refresh();
        return TRUE;

    case WM_COMMAND: {
        const int controlId = LOWORD(wParam);
        const int notification = HIWORD(wParam);
        if (controlId == IDC_PROFILE_COMBO && notification == CBN_SELCHANGE) {
            const auto index =
                static_cast<std::size_t>(::SendDlgItemMessageW(window_, IDC_PROFILE_COMBO, CB_GETCURSEL, 0, 0));
            if (index < application_.profiles().size()) {
                editedProfile_ = application_.profiles()[index];
                loadProfileToControls(editedProfile_);
                application_.previewProfile(index);
            }
            return TRUE;
        }
        if (notification != BN_CLICKED) {
            return FALSE;
        }
        switch (controlId) {
        case IDC_USE_PRESET: {
            const auto index =
                static_cast<std::size_t>(::SendDlgItemMessageW(window_, IDC_PROFILE_COMBO, CB_GETCURSEL, 0, 0));
            application_.chooseProfile(index);
            return TRUE;
        }
        case IDC_CANCEL_PREVIEW:
            application_.cancelPreview();
            return TRUE;
        case IDC_BACKGROUND_COLOR:
            chooseColor(controlId, editedProfile_.palette.background);
            break;
        case IDC_FOREGROUND_COLOR:
            chooseColor(controlId, editedProfile_.palette.foreground);
            break;
        case IDC_ACCENT_COLOR:
            chooseColor(controlId, editedProfile_.palette.accent);
            break;
        case IDC_KEYWORD_COLOR:
            chooseColor(controlId, editedProfile_.palette.keyword);
            break;
        case IDC_STRING_COLOR:
            chooseColor(controlId, editedProfile_.palette.string);
            break;
        case IDC_COMMENT_COLOR:
            chooseColor(controlId, editedProfile_.palette.comment);
            break;
        case IDC_APPLY_CUSTOM:
            application_.applyCustomProfile(profileFromControls());
            return TRUE;
        case IDC_IMPORT_THEME:
            application_.importThemeFromDialog(window_);
            return TRUE;
        case IDC_EXPORT_THEME:
            application_.exportThemeFromDialog(window_);
            return TRUE;
        case IDC_INSTALL_XML:
            application_.installCurrentThemeXml(window_);
            return TRUE;
        case IDC_FOCUS_MODE:
            application_.toggleFocusWorkspace();
            return TRUE;
        case IDC_MINIMAL_MODE:
            application_.toggleMinimalWorkspace();
            return TRUE;
        case IDC_PRESENTATION_MODE:
            application_.togglePresentationWorkspace();
            return TRUE;
        case IDC_RESTORE_NATIVE:
            application_.restoreNativeAppearance();
            return TRUE;
        default:
            return FALSE;
        }
        application_.host().applyProfile(editedProfile_);
        updateContrastLabel();
        return TRUE;
    }

    case WM_DRAWITEM:
        drawColorButton(*reinterpret_cast<DRAWITEMSTRUCT*>(lParam));
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

void ThemeStudio::populateProfiles() {
    const auto combo = ::GetDlgItem(window_, IDC_PROFILE_COMBO);
    ::SendMessageW(combo, CB_RESETCONTENT, 0, 0);
    for (const auto& profile : application_.profiles()) {
        const auto name = widen(profile.name);
        ::SendMessageW(combo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(name.c_str()));
    }
}

void ThemeStudio::loadProfileToControls(const ThemeProfile& profile) {
    editedProfile_ = profile;
    ::SetDlgItemTextW(window_, IDC_FONT_NAME, widen(profile.fontFamily).c_str());
    ::SetDlgItemInt(window_, IDC_FONT_SIZE, static_cast<UINT>(profile.fontSizePt), FALSE);
    for (const int control : {IDC_BACKGROUND_COLOR, IDC_FOREGROUND_COLOR, IDC_ACCENT_COLOR, IDC_KEYWORD_COLOR,
                              IDC_STRING_COLOR, IDC_COMMENT_COLOR}) {
        ::InvalidateRect(::GetDlgItem(window_, control), nullptr, TRUE);
    }
    updateContrastLabel();
}

ThemeProfile ThemeStudio::profileFromControls() const {
    ThemeProfile profile = editedProfile_;
    std::array<wchar_t, 256> font{};
    ::GetDlgItemTextW(window_, IDC_FONT_NAME, font.data(), static_cast<int>(font.size()));
    profile.fontFamily = narrow(font.data());
    BOOL translated = FALSE;
    const UINT size = ::GetDlgItemInt(window_, IDC_FONT_SIZE, &translated, FALSE);
    if (translated) {
        profile.fontSizePt = static_cast<int>(size);
    }
    return profile;
}

void ThemeStudio::chooseColor(int controlId, Color& color) {
    static std::array<COLORREF, 16> customColors{};
    CHOOSECOLORW dialog{};
    dialog.lStructSize = sizeof(dialog);
    dialog.hwndOwner = window_;
    dialog.rgbResult = toColorRef(color);
    dialog.lpCustColors = customColors.data();
    dialog.Flags = CC_FULLOPEN | CC_RGBINIT;
    if (::ChooseColorW(&dialog)) {
        color = fromColorRef(dialog.rgbResult);
        ::InvalidateRect(::GetDlgItem(window_, controlId), nullptr, TRUE);
    }
}

void ThemeStudio::updateContrastLabel() {
    const double ratio = contrastRatio(editedProfile_.palette.foreground, editedProfile_.palette.background);
    std::wostringstream text;
    text << std::fixed << std::setprecision(2) << ratio << L":1 — "
         << (ratio >= 7.0   ? L"AAA"
             : ratio >= 4.5 ? L"AA"
             : ratio >= 3.0 ? L"Large text only"
                            : L"Low");
    ::SetDlgItemTextW(window_, IDC_CONTRAST, text.str().c_str());
}

void ThemeStudio::drawColorButton(const DRAWITEMSTRUCT& item) const {
    Color color = editedProfile_.palette.background;
    switch (item.CtlID) {
    case IDC_FOREGROUND_COLOR:
        color = editedProfile_.palette.foreground;
        break;
    case IDC_ACCENT_COLOR:
        color = editedProfile_.palette.accent;
        break;
    case IDC_KEYWORD_COLOR:
        color = editedProfile_.palette.keyword;
        break;
    case IDC_STRING_COLOR:
        color = editedProfile_.palette.string;
        break;
    case IDC_COMMENT_COLOR:
        color = editedProfile_.palette.comment;
        break;
    default:
        break;
    }

    const HBRUSH brush = ::CreateSolidBrush(toColorRef(color));
    ::FillRect(item.hDC, &item.rcItem, brush);
    ::DeleteObject(brush);
    ::FrameRect(item.hDC, &item.rcItem, static_cast<HBRUSH>(::GetStockObject(GRAY_BRUSH)));

    const auto label = widen(formatHexColor(color));
    ::SetBkMode(item.hDC, TRANSPARENT);
    ::SetTextColor(item.hDC, contrastRatio(0xFFFFFF, color) >= contrastRatio(0x000000, color) ? RGB(255, 255, 255)
                                                                                              : RGB(0, 0, 0));
    RECT textRect = item.rcItem;
    ::DrawTextW(item.hDC, label.c_str(), static_cast<int>(label.size()), &textRect,
                DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    if ((item.itemState & ODS_FOCUS) != 0) {
        RECT focus = item.rcItem;
        ::InflateRect(&focus, -2, -2);
        ::DrawFocusRect(item.hDC, &focus);
    }
}

} // namespace nppthemes::plugin
