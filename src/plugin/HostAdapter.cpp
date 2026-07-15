#include "HostAdapter.h"

#include <algorithm>
#include <array>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

#include "Scintilla.h"

namespace nppthemes::plugin {
namespace {

[[nodiscard]] std::filesystem::path queryPath(HWND host, UINT message) {
    const auto length = static_cast<std::size_t>(::SendMessageW(host, message, 0, 0));
    if (length == 0 || length > 32767) {
        return {};
    }
    std::vector<wchar_t> buffer(length + 1, L'\0');
    const auto copied =
        ::SendMessageW(host, message, static_cast<WPARAM>(buffer.size()), reinterpret_cast<LPARAM>(buffer.data()));
    if (copied == 0) {
        return {};
    }
    return std::filesystem::path(buffer.data());
}

[[nodiscard]] std::string utf8(const std::wstring& value) {
    if (value.empty()) {
        return {};
    }
    const int length = ::WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, value.c_str(),
                                             static_cast<int>(value.size()), nullptr, 0, nullptr, nullptr);
    if (length <= 0) {
        return {};
    }
    std::string result(static_cast<std::size_t>(length), '\0');
    ::WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, value.c_str(), static_cast<int>(value.size()), result.data(),
                          length, nullptr, nullptr);
    return result;
}

} // namespace

void HostAdapter::setData(NppData data) noexcept {
    data_ = data;
}

HWND HostAdapter::nppHandle() const noexcept {
    return data_._nppHandle;
}

HWND HostAdapter::scintillaHandle(int view) const noexcept {
    return view == MAIN_VIEW ? data_._scintillaMainHandle : data_._scintillaSecondHandle;
}

std::filesystem::path HostAdapter::pluginConfigDirectory() const {
    return queryPath(data_._nppHandle, NPPM_GETPLUGINSCONFIGDIR);
}

std::filesystem::path HostAdapter::settingsDirectory() const {
    return queryPath(data_._nppHandle, NPPM_GETNPPSETTINGSDIRPATH);
}

std::filesystem::path HostAdapter::nppDirectory() const {
    std::vector<wchar_t> path(32768, L'\0');
    const DWORD copied = ::GetModuleFileNameW(nullptr, path.data(), static_cast<DWORD>(path.size()));
    if (copied == 0 || copied >= path.size()) {
        return {};
    }
    return std::filesystem::path(path.data()).parent_path();
}

std::wstring HostAdapter::languageForView(int view) const {
    const auto documentIndex = static_cast<int>(::SendMessageW(data_._nppHandle, NPPM_GETCURRENTDOCINDEX, 0, view));
    if (documentIndex < 0) {
        return L"";
    }
    const auto bufferId =
        static_cast<UINT_PTR>(::SendMessageW(data_._nppHandle, NPPM_GETBUFFERIDFROMPOS, documentIndex, view));
    if (bufferId == 0) {
        return L"";
    }
    const auto language = static_cast<int>(::SendMessageW(data_._nppHandle, NPPM_GETBUFFERLANGTYPE, bufferId, 0));
    if (language < 0) {
        return L"";
    }
    const auto length = static_cast<std::size_t>(::SendMessageW(data_._nppHandle, NPPM_GETLANGUAGENAME, language, 0));
    if (length == 0 || length > 1024) {
        return L"";
    }
    std::vector<wchar_t> buffer(length + 1, L'\0');
    ::SendMessageW(data_._nppHandle, NPPM_GETLANGUAGENAME, language, reinterpret_cast<LPARAM>(buffer.data()));
    return std::wstring(buffer.data());
}

bool HostAdapter::darkModeEnabled() const noexcept {
    return ::SendMessageW(data_._nppHandle, NPPM_ISDARKMODEENABLED, 0, 0) != FALSE;
}

std::wstring HostAdapter::versionString() const {
    const auto version = static_cast<DWORD>(::SendMessageW(data_._nppHandle, NPPM_GETNPPVERSION, TRUE, 0));
    const int major = HIWORD(version);
    const int minor = LOWORD(version);
    std::wostringstream output;
    output << major;
    if (minor > 0) {
        const int first = minor / 100;
        const int second = (minor / 10) % 10;
        const int third = minor % 10;
        output << L'.' << first;
        if (second != 0 || third != 0) {
            output << L'.' << second;
        }
        if (third != 0) {
            output << L'.' << third;
        }
    }
    return output.str();
}

std::vector<OpenDocument> HostAdapter::openDocuments() const {
    std::vector<OpenDocument> documents;
    for (const int view : {MAIN_VIEW, SUB_VIEW}) {
        const auto count = static_cast<int>(
            ::SendMessageW(data_._nppHandle, NPPM_GETNBOPENFILES, 0, view == MAIN_VIEW ? PRIMARY_VIEW : SECOND_VIEW));
        if (count <= 0 || count > 10000) {
            continue;
        }
        for (int index = 0; index < count; ++index) {
            const auto bufferId =
                static_cast<UINT_PTR>(::SendMessageW(data_._nppHandle, NPPM_GETBUFFERIDFROMPOS, index, view));
            if (bufferId == 0) {
                continue;
            }
            const auto length =
                static_cast<int>(::SendMessageW(data_._nppHandle, NPPM_GETFULLPATHFROMBUFFERID, bufferId, 0));
            if (length < 0 || length > 32767) {
                continue;
            }
            std::vector<wchar_t> path(static_cast<std::size_t>(length) + 1U, L'\0');
            const auto copied = static_cast<int>(::SendMessageW(data_._nppHandle, NPPM_GETFULLPATHFROMBUFFERID,
                                                                bufferId, reinterpret_cast<LPARAM>(path.data())));
            if (copied >= 0) {
                documents.push_back({view, index, std::filesystem::path(path.data())});
            }
        }
    }
    return documents;
}

bool HostAdapter::activateDocument(int view, int index) const noexcept {
    if ((view != MAIN_VIEW && view != SUB_VIEW) || index < 0) {
        return false;
    }
    return ::SendMessageW(data_._nppHandle, NPPM_ACTIVATEDOC, view, index) != FALSE;
}

std::string HostAdapter::snapshotKey(int view, const std::wstring& language) const {
    return std::to_string(view) + ':' + utf8(language);
}

HostAdapter::ViewSnapshot HostAdapter::captureView(HWND scintilla) const {
    ViewSnapshot snapshot;
    for (std::size_t index = 0; index < snapshot.styles.size(); ++index) {
        auto& style = snapshot.styles[index];
        style.foreground = static_cast<int>(::SendMessageW(scintilla, SCI_STYLEGETFORE, index, 0));
        style.background = static_cast<int>(::SendMessageW(scintilla, SCI_STYLEGETBACK, index, 0));
        style.bold = ::SendMessageW(scintilla, SCI_STYLEGETBOLD, index, 0) != FALSE;
        style.italic = ::SendMessageW(scintilla, SCI_STYLEGETITALIC, index, 0) != FALSE;
        style.underline = ::SendMessageW(scintilla, SCI_STYLEGETUNDERLINE, index, 0) != FALSE;
        style.size = static_cast<int>(::SendMessageW(scintilla, SCI_STYLEGETSIZE, index, 0));
        style.font.fill('\0');
        ::SendMessageW(scintilla, SCI_STYLEGETFONT, index, reinterpret_cast<LPARAM>(style.font.data()));
    }
    snapshot.caretForeground = static_cast<int>(::SendMessageW(scintilla, SCI_GETCARETFORE, 0, 0));
    snapshot.caretLineBackground = static_cast<int>(::SendMessageW(scintilla, SCI_GETCARETLINEBACK, 0, 0));
    snapshot.caretLineAlpha = static_cast<int>(::SendMessageW(scintilla, SCI_GETCARETLINEBACKALPHA, 0, 0));
    snapshot.fontQuality = static_cast<int>(::SendMessageW(scintilla, SCI_GETFONTQUALITY, 0, 0));

    const auto captureElement = [scintilla](int element) {
        ElementState state;
        state.set = ::SendMessageW(scintilla, SCI_GETELEMENTISSET, element, 0) != FALSE;
        if (state.set) {
            state.color = static_cast<int>(::SendMessageW(scintilla, SCI_GETELEMENTCOLOUR, element, 0));
        }
        return state;
    };
    snapshot.selectionBackground = captureElement(SC_ELEMENT_SELECTION_BACK);
    snapshot.caretLineElement = captureElement(SC_ELEMENT_CARET_LINE_BACK);
    snapshot.whitespace = captureElement(SC_ELEMENT_WHITE_SPACE);
    return snapshot;
}

void HostAdapter::restoreView(HWND scintilla, const ViewSnapshot& snapshot) const {
    for (std::size_t index = 0; index < snapshot.styles.size(); ++index) {
        const auto& style = snapshot.styles[index];
        ::SendMessageW(scintilla, SCI_STYLESETFORE, index, style.foreground);
        ::SendMessageW(scintilla, SCI_STYLESETBACK, index, style.background);
        ::SendMessageW(scintilla, SCI_STYLESETBOLD, index, style.bold);
        ::SendMessageW(scintilla, SCI_STYLESETITALIC, index, style.italic);
        ::SendMessageW(scintilla, SCI_STYLESETUNDERLINE, index, style.underline);
        ::SendMessageW(scintilla, SCI_STYLESETSIZE, index, style.size);
        ::SendMessageW(scintilla, SCI_STYLESETFONT, index, reinterpret_cast<LPARAM>(style.font.data()));
    }
    ::SendMessageW(scintilla, SCI_SETCARETFORE, snapshot.caretForeground, 0);
    ::SendMessageW(scintilla, SCI_SETCARETLINEBACK, snapshot.caretLineBackground, 0);
    ::SendMessageW(scintilla, SCI_SETCARETLINEBACKALPHA, snapshot.caretLineAlpha, 0);
    ::SendMessageW(scintilla, SCI_SETFONTQUALITY, snapshot.fontQuality, 0);

    const auto restoreElement = [scintilla](int element, const ElementState& state) {
        if (state.set) {
            ::SendMessageW(scintilla, SCI_SETELEMENTCOLOUR, element, state.color);
        } else {
            ::SendMessageW(scintilla, SCI_RESETELEMENTCOLOUR, element, 0);
        }
    };
    restoreElement(SC_ELEMENT_SELECTION_BACK, snapshot.selectionBackground);
    restoreElement(SC_ELEMENT_CARET_LINE_BACK, snapshot.caretLineElement);
    restoreElement(SC_ELEMENT_WHITE_SPACE, snapshot.whitespace);
    ::InvalidateRect(scintilla, nullptr, TRUE);
}

void HostAdapter::applyToView(HWND scintilla, const std::wstring& language, const ThemeProfile& profile) const {
    if (scintilla == nullptr) {
        return;
    }
    const auto languageUtf8 = utf8(language);
    for (int style = 0; style <= 255; ++style) {
        const auto existing = static_cast<int>(::SendMessageW(scintilla, SCI_STYLEGETFORE, style, 0));
        const Color semantic = semanticColorForStyle(profile, languageUtf8, style, fromScintillaColor(existing));
        ::SendMessageW(scintilla, SCI_STYLESETFORE, style, toScintillaColor(semantic));
        ::SendMessageW(scintilla, SCI_STYLESETBACK, style, toScintillaColor(profile.palette.background));
        ::SendMessageW(scintilla, SCI_STYLESETFONT, style, reinterpret_cast<LPARAM>(profile.fontFamily.c_str()));
        ::SendMessageW(scintilla, SCI_STYLESETSIZE, style, profile.fontSizePt);
    }

    const int selection = toScintillaColor(profile.palette.selection) | static_cast<int>(0xC0000000U);
    const int currentLine = toScintillaColor(profile.palette.currentLine) | static_cast<int>(0x70000000U);
    const int whitespace = toScintillaColor(profile.palette.whitespace) | static_cast<int>(0xFF000000U);
    ::SendMessageW(scintilla, SCI_SETELEMENTCOLOUR, SC_ELEMENT_SELECTION_BACK, selection);
    ::SendMessageW(scintilla, SCI_SETELEMENTCOLOUR, SC_ELEMENT_CARET_LINE_BACK, currentLine);
    ::SendMessageW(scintilla, SCI_SETELEMENTCOLOUR, SC_ELEMENT_WHITE_SPACE, whitespace);
    ::SendMessageW(scintilla, SCI_SETCARETFORE, toScintillaColor(profile.palette.caret), 0);
    ::SendMessageW(scintilla, SCI_SETCARETLINEVISIBLE, TRUE, 0);
    ::SendMessageW(scintilla, SCI_SETCARETLINEVISIBLEALWAYS, TRUE, 0);
    ::SendMessageW(scintilla, SCI_SETFONTQUALITY, SC_EFF_QUALITY_LCD_OPTIMIZED, 0);
    ::InvalidateRect(scintilla, nullptr, TRUE);
}

void HostAdapter::applyProfile(const ThemeProfile& profile) {
    for (int view : {MAIN_VIEW, SUB_VIEW}) {
        const HWND scintilla = scintillaHandle(view);
        if (scintilla == nullptr) {
            continue;
        }
        const auto language = languageForView(view);
        const auto key = snapshotKey(view, language);
        if (!snapshots_.contains(key)) {
            snapshots_.emplace(key, captureView(scintilla));
        }
        applyToView(scintilla, language, profile);
    }
}

void HostAdapter::restoreEditorAppearance() {
    for (int view : {MAIN_VIEW, SUB_VIEW}) {
        const auto key = snapshotKey(view, languageForView(view));
        const auto snapshot = snapshots_.find(key);
        if (snapshot != snapshots_.end()) {
            restoreView(scintillaHandle(view), snapshot->second);
        }
    }
    snapshots_.clear();
}

void HostAdapter::invalidateSnapshotsForCurrentDocuments() {
    for (int view : {MAIN_VIEW, SUB_VIEW}) {
        snapshots_.erase(snapshotKey(view, languageForView(view)));
    }
}

WorkspaceSnapshot HostAdapter::captureWorkspace() const noexcept {
    return {
        ::SendMessageW(data_._nppHandle, NPPM_ISMENUHIDDEN, 0, 0) != FALSE,
        ::SendMessageW(data_._nppHandle, NPPM_ISTOOLBARHIDDEN, 0, 0) != FALSE,
        ::SendMessageW(data_._nppHandle, NPPM_ISTABBARHIDDEN, 0, 0) != FALSE,
        ::SendMessageW(data_._nppHandle, NPPM_ISSTATUSBARHIDDEN, 0, 0) != FALSE,
    };
}

void HostAdapter::applyWorkspace(std::string_view mode) const noexcept {
    const bool minimal = mode == "minimal";
    ::SendMessageW(data_._nppHandle, NPPM_HIDEMENU, 0, minimal);
    ::SendMessageW(data_._nppHandle, NPPM_HIDETOOLBAR, 0, TRUE);
    ::SendMessageW(data_._nppHandle, NPPM_HIDETABBAR, 0, minimal);
    ::SendMessageW(data_._nppHandle, NPPM_HIDESTATUSBAR, 0, TRUE);
}

void HostAdapter::restoreWorkspace(const WorkspaceSnapshot& snapshot) const noexcept {
    ::SendMessageW(data_._nppHandle, NPPM_HIDEMENU, 0, snapshot.menuHidden);
    ::SendMessageW(data_._nppHandle, NPPM_HIDETOOLBAR, 0, snapshot.toolbarHidden);
    ::SendMessageW(data_._nppHandle, NPPM_HIDETABBAR, 0, snapshot.tabbarHidden);
    ::SendMessageW(data_._nppHandle, NPPM_HIDESTATUSBAR, 0, snapshot.statusbarHidden);
}

int HostAdapter::toScintillaColor(Color color) noexcept {
    const auto red = static_cast<int>((color >> 16U) & 0xFFU);
    const auto green = static_cast<int>((color >> 8U) & 0xFFU);
    const auto blue = static_cast<int>(color & 0xFFU);
    return red | (green << 8) | (blue << 16);
}

Color HostAdapter::fromScintillaColor(int color) noexcept {
    const auto red = static_cast<Color>(color & 0xFF);
    const auto green = static_cast<Color>((color >> 8) & 0xFF);
    const auto blue = static_cast<Color>((color >> 16) & 0xFF);
    return (red << 16U) | (green << 8U) | blue;
}

} // namespace nppthemes::plugin
