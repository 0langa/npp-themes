#include "ShellBridge.h"

#include <algorithm>

namespace nppthemes::plugin {

ShellBridge::ShellBridge() noexcept
    : ShellBridge(::DwmGetWindowAttribute, ::DwmSetWindowAttribute, systemHighContrastActive) {}

ShellBridge::ShellBridge(GetWindowAttribute getAttribute, SetWindowAttribute setAttribute,
                         HighContrastActive highContrastActive) noexcept
    : getAttribute_(getAttribute), setAttribute_(setAttribute), highContrastActive_(highContrastActive) {
    resetSnapshots();
}

void ShellBridge::setHostWindow(HWND window) noexcept {
    if (window_ == window) {
        return;
    }
    restore();
    window_ = window;
}

bool ShellBridge::apply(const ShellPalette& palette) noexcept {
    if (window_ == nullptr || getAttribute_ == nullptr || setAttribute_ == nullptr || highContrastActive_ == nullptr) {
        return false;
    }
    if (!validateShellPalette(palette).empty()) {
        return false;
    }
    if (highContrastActive_()) {
        restore();
        return false;
    }
    if (!std::all_of(snapshots_.begin(), snapshots_.end(), [this](auto& snapshot) { return capture(snapshot); })) {
        resetSnapshots();
        return false;
    }

    const bool applied = set(DWMWA_BORDER_COLOR, toDwmColor(palette.border)) &&
                         set(DWMWA_CAPTION_COLOR, toDwmColor(palette.captionBackground)) &&
                         set(DWMWA_TEXT_COLOR, toDwmColor(palette.captionForeground));
    if (!applied) {
        restore();
        return false;
    }
    active_ = true;
    refreshFrame();
    return true;
}

void ShellBridge::restore() noexcept {
    if (window_ != nullptr && setAttribute_ != nullptr) {
        for (const auto& snapshot : snapshots_) {
            if (snapshot.captured) {
                static_cast<void>(set(snapshot.attribute, snapshot.originalValue));
            }
        }
        refreshFrame();
    }
    active_ = false;
    resetSnapshots();
}

bool ShellBridge::active() const noexcept {
    return active_;
}

bool ShellBridge::capture(AttributeSnapshot& snapshot) noexcept {
    if (snapshot.captured) {
        return true;
    }
    DWORD value{};
    const HRESULT result = getAttribute_(window_, snapshot.attribute, &value, sizeof(value));
    if (FAILED(result)) {
        return false;
    }
    snapshot.originalValue = value;
    snapshot.captured = true;
    return true;
}

bool ShellBridge::set(DWORD attribute, DWORD value) const noexcept {
    return SUCCEEDED(setAttribute_(window_, attribute, &value, sizeof(value)));
}

void ShellBridge::resetSnapshots() noexcept {
    snapshots_ = {
        AttributeSnapshot{DWMWA_BORDER_COLOR},
        AttributeSnapshot{DWMWA_CAPTION_COLOR},
        AttributeSnapshot{DWMWA_TEXT_COLOR},
    };
}

void ShellBridge::refreshFrame() const noexcept {
    if (window_ != nullptr) {
        ::SetWindowPos(window_, nullptr, 0, 0, 0, 0,
                       SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
    }
}

DWORD ShellBridge::toDwmColor(Color color) noexcept {
    const auto red = static_cast<DWORD>((color >> 16U) & 0xFFU);
    const auto green = static_cast<DWORD>((color >> 8U) & 0xFFU);
    const auto blue = static_cast<DWORD>(color & 0xFFU);
    return red | (green << 8U) | (blue << 16U);
}

bool ShellBridge::systemHighContrastActive() noexcept {
    HIGHCONTRASTW state{};
    state.cbSize = sizeof(state);
    return ::SystemParametersInfoW(SPI_GETHIGHCONTRAST, sizeof(state), &state, 0) != FALSE &&
           (state.dwFlags & HCF_HIGHCONTRASTON) != 0;
}

} // namespace nppthemes::plugin
