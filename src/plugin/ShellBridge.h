#pragma once

#include <array>
#include <cstddef>

#include <dwmapi.h>
#include <windows.h>

#include "nppthemes/ShellPalette.h"

namespace nppthemes::plugin {

class ShellBridge {
  public:
    using GetWindowAttribute = HRESULT(WINAPI*)(HWND, DWORD, PVOID, DWORD);
    using SetWindowAttribute = HRESULT(WINAPI*)(HWND, DWORD, LPCVOID, DWORD);
    using HighContrastActive = bool (*)() noexcept;

    ShellBridge() noexcept;
    ShellBridge(GetWindowAttribute getAttribute, SetWindowAttribute setAttribute,
                HighContrastActive highContrastActive) noexcept;

    void setHostWindow(HWND window) noexcept;
    [[nodiscard]] bool apply(const ShellPalette& palette) noexcept;
    void restore() noexcept;
    [[nodiscard]] bool active() const noexcept;

  private:
    struct AttributeSnapshot {
        DWORD attribute{};
        DWORD originalValue{};
        bool captured{false};
    };

    [[nodiscard]] bool capture(AttributeSnapshot& snapshot) noexcept;
    [[nodiscard]] bool set(DWORD attribute, DWORD value) const noexcept;
    void resetSnapshots() noexcept;
    void refreshFrame() const noexcept;
    [[nodiscard]] static DWORD toDwmColor(Color color) noexcept;
    [[nodiscard]] static bool systemHighContrastActive() noexcept;

    HWND window_{};
    GetWindowAttribute getAttribute_{};
    SetWindowAttribute setAttribute_{};
    HighContrastActive highContrastActive_{};
    std::array<AttributeSnapshot, 3> snapshots_{};
    bool active_{false};
};

} // namespace nppthemes::plugin
