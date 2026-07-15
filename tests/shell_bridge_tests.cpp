#include "ShellBridge.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <string>

namespace {

std::array<DWORD, 64> attributes{};
int getCalls = 0;
int setCalls = 0;
bool attributesSupported = true;
DWORD rejectedSetAttribute = 0;
bool highContrastEnabled = false;

int failures = 0;

void check(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        ++failures;
    }
}

HRESULT WINAPI fakeGet(HWND, DWORD attribute, PVOID value, DWORD size) {
    ++getCalls;
    if (!attributesSupported || attribute == rejectedSetAttribute || value == nullptr || size != sizeof(DWORD) ||
        attribute >= attributes.size()) {
        return E_NOTIMPL;
    }
    *static_cast<DWORD*>(value) = attributes[attribute];
    return S_OK;
}

HRESULT WINAPI fakeSet(HWND, DWORD attribute, LPCVOID value, DWORD size) {
    ++setCalls;
    if (!attributesSupported || value == nullptr || size != sizeof(DWORD) || attribute >= attributes.size()) {
        return E_NOTIMPL;
    }
    attributes[attribute] = *static_cast<const DWORD*>(value);
    return S_OK;
}

bool normalContrast() noexcept {
    return false;
}

bool highContrast() noexcept {
    return highContrastEnabled;
}

[[nodiscard]] DWORD colorRef(nppthemes::Color color) {
    return static_cast<DWORD>((color >> 16U) & 0xFFU) | static_cast<DWORD>(color & 0x00FF00U) |
           static_cast<DWORD>((color & 0xFFU) << 16U);
}

void resetFakeState() {
    attributes.fill(0);
    attributes[DWMWA_BORDER_COLOR] = 0x00112233;
    attributes[DWMWA_CAPTION_COLOR] = 0x00445566;
    attributes[DWMWA_TEXT_COLOR] = 0x00778899;
    getCalls = 0;
    setCalls = 0;
    attributesSupported = true;
    rejectedSetAttribute = 0;
    highContrastEnabled = false;
}

void testApplyUpdateAndRestore() {
    using namespace nppthemes;
    using namespace nppthemes::plugin;
    resetFakeState();
    const DWORD originalBorder = attributes[DWMWA_BORDER_COLOR];
    const DWORD originalCaption = attributes[DWMWA_CAPTION_COLOR];
    const DWORD originalText = attributes[DWMWA_TEXT_COLOR];
    ShellBridge bridge(fakeGet, fakeSet, normalContrast);
    bridge.setHostWindow(reinterpret_cast<HWND>(static_cast<std::uintptr_t>(1)));

    const auto first = deriveShellPalette(builtInProfiles().at(0));
    check(bridge.apply(first), "documented DWM attributes apply when supported");
    check(bridge.active(), "bridge reports active after complete apply");
    check(getCalls == 3 && setCalls == 3, "first apply snapshots and sets three frame attributes");
    check(attributes[DWMWA_BORDER_COLOR] == colorRef(first.border), "border receives shell token");
    check(attributes[DWMWA_CAPTION_COLOR] == colorRef(first.captionBackground), "caption receives shell token");
    check(attributes[DWMWA_TEXT_COLOR] == colorRef(first.captionForeground), "caption text receives shell token");

    const auto second = deriveShellPalette(builtInProfiles().at(3));
    check(bridge.apply(second), "second profile updates active frame");
    check(getCalls == 3 && setCalls == 6, "second apply retains original snapshot");

    bridge.restore();
    check(!bridge.active(), "restore clears active state");
    check(attributes[DWMWA_BORDER_COLOR] == originalBorder, "restore returns original border");
    check(attributes[DWMWA_CAPTION_COLOR] == originalCaption, "restore returns original caption");
    check(attributes[DWMWA_TEXT_COLOR] == originalText, "restore returns original caption text");
}

void testUnsupportedAndHighContrastFailClosed() {
    using namespace nppthemes;
    using namespace nppthemes::plugin;
    resetFakeState();
    attributesSupported = false;
    ShellBridge unsupported(fakeGet, fakeSet, normalContrast);
    unsupported.setHostWindow(reinterpret_cast<HWND>(static_cast<std::uintptr_t>(1)));
    check(!unsupported.apply(deriveShellPalette(builtInProfiles().front())),
          "unsupported DWM attributes leave shell unchanged");
    check(setCalls == 0, "unsupported capture prevents partial writes");

    resetFakeState();
    highContrastEnabled = true;
    ShellBridge accessible(fakeGet, fakeSet, highContrast);
    accessible.setHostWindow(reinterpret_cast<HWND>(static_cast<std::uintptr_t>(1)));
    check(!accessible.apply(deriveShellPalette(builtInProfiles().front())),
          "Windows high contrast disables custom frame colors");
    check(getCalls == 0 && setCalls == 0, "high contrast exits before DWM access");
}

void testPartialFailureAndHighContrastTransitionRestore() {
    using namespace nppthemes;
    using namespace nppthemes::plugin;
    resetFakeState();
    const auto originals = attributes;
    rejectedSetAttribute = DWMWA_CAPTION_COLOR;
    ShellBridge partial(fakeGet, fakeSet, highContrast);
    partial.setHostWindow(reinterpret_cast<HWND>(static_cast<std::uintptr_t>(1)));
    check(!partial.apply(deriveShellPalette(builtInProfiles().front())), "partial DWM write failure rejects apply");
    check(!partial.active(), "partial DWM write failure does not report active");
    check(attributes == originals, "partial DWM write failure restores every captured original");

    resetFakeState();
    ShellBridge transition(fakeGet, fakeSet, highContrast);
    transition.setHostWindow(reinterpret_cast<HWND>(static_cast<std::uintptr_t>(1)));
    check(transition.apply(deriveShellPalette(builtInProfiles().front())), "frame applies before high contrast");
    highContrastEnabled = true;
    check(!transition.apply(deriveShellPalette(builtInProfiles().back())), "high contrast transition rejects reapply");
    check(attributes[DWMWA_BORDER_COLOR] == 0x00112233 && attributes[DWMWA_CAPTION_COLOR] == 0x00445566 &&
              attributes[DWMWA_TEXT_COLOR] == 0x00778899,
          "high contrast transition restores captured native frame");
}

} // namespace

int main() {
    testApplyUpdateAndRestore();
    testUnsupportedAndHighContrastFailClosed();
    testPartialFailureAndHighContrastTransitionRestore();
    if (failures == 0) {
        std::cout << "All NppThemes shell bridge tests passed\n";
        return 0;
    }
    std::cerr << failures << " test(s) failed\n";
    return 1;
}
