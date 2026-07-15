#include "Application.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <commdlg.h>
#include <windows.h>

#include "CommandPalette.h"
#include "ThemeStudio.h"
#include "nppthemes/ThemeXmlCompiler.h"

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

[[nodiscard]] std::wstring sanitizeFileName(std::string_view value) {
    auto name = widen(value);
    for (auto& character : name) {
        if (character == L'<' || character == L'>' || character == L':' || character == L'"' || character == L'/' ||
            character == L'\\' || character == L'|' || character == L'?' || character == L'*') {
            character = L'-';
        }
    }
    return name;
}

void showError(HWND owner, std::wstring_view text) {
    ::MessageBoxW(owner, std::wstring(text).c_str(), L"NppThemes", MB_OK | MB_ICONERROR);
}

void showInformation(HWND owner, std::wstring_view text) {
    ::MessageBoxW(owner, std::wstring(text).c_str(), L"NppThemes", MB_OK | MB_ICONINFORMATION);
}

} // namespace

Application& Application::instance() {
    static Application application;
    return application;
}

Application::Application() : profiles_(builtInProfiles()) {}

Application::~Application() = default;

void Application::processAttach(HINSTANCE instance) noexcept {
    moduleInstance_ = instance;
    ::DisableThreadLibraryCalls(instance);
}

void Application::processDetach() noexcept {
    ready_ = false;
}

void Application::setInfo(NppData data) {
    host_.setData(data);
}

void Application::setThemeStudioCommandId(int commandId) noexcept {
    themeStudioCommandId_ = commandId;
}

void Application::onNotification(const SCNotification& notification) noexcept {
    try {
        switch (notification.nmhdr.code) {
        case NPPN_READY:
            onReady();
            break;
        case NPPN_BUFFERACTIVATED:
        case NPPN_LANGCHANGED:
            if (ready_ && settings_.applyOnStartup) {
                reapplyCurrentProfile();
            }
            break;
        case NPPN_WORDSTYLESUPDATED:
            if (ready_ && settings_.applyOnStartup) {
                host_.invalidateSnapshotsForCurrentDocuments();
                reapplyCurrentProfile();
            }
            break;
        case NPPN_DARKMODECHANGED:
            if (studio_) {
                studio_->onDarkModeChanged();
            }
            if (commandPalette_) {
                commandPalette_->onDarkModeChanged();
            }
            if (ready_ && settings_.applyOnStartup) {
                host_.applyShellAppearance(currentProfile());
            }
            break;
        case NPPN_BEFORESHUTDOWN:
        case NPPN_SHUTDOWN:
            restoreWorkspaceOnly();
            break;
        default:
            break;
        }
    } catch (...) {
        store_.log(L"Unhandled plugin notification failure");
    }
}

void Application::onReady() {
    if (ready_) {
        return;
    }
    const auto startupBegan = std::chrono::steady_clock::now();
    const auto configDirectory = host_.pluginConfigDirectory();
    if (!store_.initialize(configDirectory)) {
        showError(host_.nppHandle(), L"Unable to create plugin configuration directory.");
        return;
    }
    store_.log(L"NppThemes starting with Notepad++ " + host_.versionString());
    settings_ = store_.load();
    host_.setShellAppearanceEnabled(settings_.themeWindowFrame);

    if (settings_.customProfile) {
        profiles_.push_back(*settings_.customProfile);
    }
    if (const auto selected = findProfile(settings_.activeProfileId)) {
        currentProfileIndex_ = *selected;
    }

    if (settings_.workspace.active) {
        host_.restoreWorkspace(settings_.workspace.previous);
        settings_.workspace = {};
        persist();
        store_.log(L"Recovered native UI after interrupted workspace profile");
    }

    ready_ = true;
    if (settings_.applyOnStartup) {
        reapplyCurrentProfile();
    }
    const auto startupElapsed =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - startupBegan);
    store_.log(L"NppThemes ready in " + std::to_wstring(startupElapsed.count()) + L" ms");
}

void Application::showThemeStudio() {
    if (!studio_) {
        studio_ = std::make_unique<ThemeStudio>(*this);
        if (!studio_->create(themeStudioCommandId_)) {
            studio_.reset();
            store_.log(L"Theme Studio creation failed");
            return;
        }
    }
    studio_->refresh();
    studio_->show();
}

void Application::showCommandPalette() {
    if (!commandPalette_) {
        commandPalette_ = std::make_unique<CommandPalette>(*this);
        if (!commandPalette_->create()) {
            commandPalette_.reset();
            store_.log(L"Command palette creation failed");
            return;
        }
    }
    commandPalette_->show();
}

void Application::applyNextTheme() {
    if (profiles_.empty()) {
        return;
    }
    chooseProfile((currentProfileIndex_ + 1) % profiles_.size());
}

void Application::toggleFocusWorkspace() {
    if (settings_.workspace.active && settings_.workspace.mode == "focus") {
        restoreWorkspaceOnly();
    } else {
        activateWorkspace("focus");
    }
}

void Application::toggleMinimalWorkspace() {
    if (settings_.workspace.active && settings_.workspace.mode == "minimal") {
        restoreWorkspaceOnly();
    } else {
        activateWorkspace("minimal");
    }
}

void Application::togglePresentationWorkspace() {
    if (settings_.workspace.active && settings_.workspace.mode == "presentation") {
        restoreWorkspaceOnly();
        reapplyCurrentProfile();
        return;
    }
    activateWorkspace("presentation");
    auto presentation = currentProfile();
    presentation.fontSizePt = std::min(48, std::max(16, presentation.fontSizePt + 5));
    host_.applyProfile(presentation);
}

void Application::restoreNativeAppearance() {
    host_.restoreEditorAppearance();
    restoreWorkspaceOnly();
    settings_.applyOnStartup = false;
    settings_.themeWindowFrame = false;
    host_.setShellAppearanceEnabled(false);
    persist();
    if (studio_) {
        studio_->refresh();
    }
    store_.log(L"Native editor appearance restored");
}

void Application::exportCurrentTheme() {
    exportThemeFromDialog(host_.nppHandle());
}

void Application::showAbout() const {
    std::wstring message = L"NppThemes " + widen(NPP_THEMES_VERSION) +
                           L"\n\nModern, reversible appearance profiles for Notepad++."
                           L"\n\nRuntime styling uses documented Notepad++ and Scintilla APIs only."
                           L"\nNo telemetry. No network access.";
    ::MessageBoxW(host_.nppHandle(), message.c_str(), L"About NppThemes", MB_OK | MB_ICONINFORMATION);
}

HINSTANCE Application::moduleInstance() const noexcept {
    return moduleInstance_;
}

HostAdapter& Application::host() noexcept {
    return host_;
}

const std::vector<ThemeProfile>& Application::profiles() const noexcept {
    return profiles_;
}

ThemeProfile Application::currentProfile() const {
    if (profiles_.empty()) {
        throw std::runtime_error("No profiles available");
    }
    return profiles_.at(currentProfileIndex_);
}

std::size_t Application::currentProfileIndex() const noexcept {
    return currentProfileIndex_;
}

bool Application::windowFrameThemingEnabled() const noexcept {
    return settings_.themeWindowFrame;
}

void Application::setWindowFrameThemingEnabled(bool enabled) {
    settings_.themeWindowFrame = enabled;
    host_.setShellAppearanceEnabled(enabled);
    if (enabled && ready_) {
        host_.applyShellAppearance(currentProfile());
    }
    persist();
    if (studio_) {
        studio_->refresh();
    }
}

void Application::previewProfile(std::size_t index) {
    if (index < profiles_.size()) {
        host_.applyProfile(profiles_[index]);
    }
}

void Application::cancelPreview() {
    reapplyCurrentProfile();
    if (studio_) {
        studio_->refresh();
    }
}

void Application::applyCustomProfile(ThemeProfile profile) {
    profile.id = "user.custom";
    if (profile.name.empty()) {
        profile.name = "Custom";
    } else if (profile.name.find("Custom") == std::string::npos) {
        profile.name += " Custom";
    }
    const auto errors = validateProfile(profile);
    if (!errors.empty()) {
        showError(host_.nppHandle(), widen(errors.front()));
        return;
    }

    settings_.customProfile = profile;
    if (const auto existing = findProfile(profile.id)) {
        profiles_[*existing] = profile;
        currentProfileIndex_ = *existing;
    } else {
        profiles_.push_back(profile);
        currentProfileIndex_ = profiles_.size() - 1;
    }
    settings_.activeProfileId = profile.id;
    settings_.applyOnStartup = true;
    host_.applyProfile(profile);
    persist();
    if (studio_) {
        studio_->refresh();
    }
}

void Application::chooseProfile(std::size_t index) {
    if (index >= profiles_.size()) {
        return;
    }
    currentProfileIndex_ = index;
    settings_.activeProfileId = profiles_[index].id;
    settings_.applyOnStartup = true;
    host_.applyProfile(profiles_[index]);
    persist();
    if (studio_) {
        studio_->refresh();
    }
}

void Application::importThemeFromDialog(HWND owner) {
    const auto path = chooseProfilePath(owner, false);
    if (path.empty()) {
        return;
    }
    try {
        auto profile = store_.importProfile(path);
        profile.id = "user.custom";
        applyCustomProfile(std::move(profile));
        showInformation(owner, L"Theme imported and applied.");
    } catch (const std::exception& error) {
        showError(owner, L"Theme import failed: " + widen(error.what()));
    }
}

void Application::exportThemeFromDialog(HWND owner) {
    const auto path = chooseProfilePath(owner, true);
    if (path.empty()) {
        return;
    }
    if (store_.exportProfile(path, currentProfile())) {
        showInformation(owner, L"Theme exported.");
    } else {
        showError(owner, L"Theme export failed.");
    }
}

void Application::installCurrentThemeXml(HWND owner) {
    const auto settingsDirectory = host_.settingsDirectory();
    if (settingsDirectory.empty()) {
        showError(
            owner,
            L"Current Notepad++ version does not expose active settings directory. Runtime theme remains active.");
        return;
    }
    const auto profile = currentProfile();
    auto source = host_.nppDirectory();
    source /= profile.dark ? L"themes\\DarkModeDefault.xml" : L"stylers.model.xml";
    if (!std::filesystem::exists(source)) {
        showError(owner, L"Notepad++ source theme model was not found.");
        return;
    }

    const auto destination =
        settingsDirectory / L"themes" / (L"NppThemes - " + sanitizeFileName(profile.name) + L".xml");
    const auto result = compileThemeXml(source, destination, profile);
    if (!result.success) {
        showError(owner, L"Theme installation failed: " + widen(result.error));
        return;
    }
    showInformation(owner,
                    L"Theme XML installed. Select it in Settings > Style Configurator for native persistent styling."
                    L"\n\nNppThemes runtime profile is already active.");
}

void Application::reapplyCurrentProfile() {
    if (currentProfileIndex_ < profiles_.size()) {
        host_.applyProfile(profiles_[currentProfileIndex_]);
    }
}

void Application::persist() {
    if (!store_.save(settings_)) {
        store_.log(L"Settings save failed");
    }
}

void Application::activateWorkspace(std::string mode) {
    if (settings_.workspace.active) {
        host_.restoreWorkspace(settings_.workspace.previous);
    } else {
        settings_.workspace.previous = host_.captureWorkspace();
    }
    settings_.workspace.active = true;
    settings_.workspace.mode = std::move(mode);
    persist();
    host_.applyWorkspace(settings_.workspace.mode);
    store_.log(L"Workspace profile activated");
}

void Application::restoreWorkspaceOnly() {
    if (!settings_.workspace.active) {
        return;
    }
    host_.restoreWorkspace(settings_.workspace.previous);
    settings_.workspace = {};
    persist();
    store_.log(L"Workspace profile restored");
}

std::optional<std::size_t> Application::findProfile(std::string_view id) const {
    for (std::size_t index = 0; index < profiles_.size(); ++index) {
        if (profiles_[index].id == id) {
            return index;
        }
    }
    return std::nullopt;
}

std::filesystem::path Application::chooseProfilePath(HWND owner, bool save) const {
    std::array<wchar_t, 32768> buffer{};
    if (save) {
        const auto suggested = sanitizeFileName(currentProfile().name) + L".npptheme.json";
        std::copy_n(suggested.c_str(), std::min(suggested.size(), buffer.size() - 1), buffer.data());
    }

    constexpr wchar_t filter[] =
        L"NppThemes profiles (*.npptheme.json)\0*.npptheme.json\0JSON files (*.json)\0*.json\0All files (*.*)\0*.*\0\0";
    OPENFILENAMEW dialog{};
    dialog.lStructSize = sizeof(dialog);
    dialog.hwndOwner = owner;
    dialog.hInstance = moduleInstance_;
    dialog.lpstrFilter = filter;
    dialog.lpstrFile = buffer.data();
    dialog.nMaxFile = static_cast<DWORD>(buffer.size());
    dialog.lpstrDefExt = L"json";
    dialog.Flags =
        OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR | (save ? OFN_OVERWRITEPROMPT : OFN_FILEMUSTEXIST);

    const BOOL accepted = save ? ::GetSaveFileNameW(&dialog) : ::GetOpenFileNameW(&dialog);
    return accepted ? std::filesystem::path(buffer.data()) : std::filesystem::path{};
}

} // namespace nppthemes::plugin
