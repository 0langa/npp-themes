#include "nppthemes/ShellPalette.h"

#include <iostream>
#include <string>

namespace {

int failures = 0;

void check(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        ++failures;
    }
}

void testBlendBoundaries() {
    using namespace nppthemes;
    check(blendColors(0x102030, 0xA0B0C0, -1.0) == 0x102030, "blend clamps negative weight");
    check(blendColors(0x102030, 0xA0B0C0, 2.0) == 0xA0B0C0, "blend clamps excessive weight");
    check(blendColors(0x000000, 0xFFFFFF, 0.5) == 0x808080, "blend rounds channels deterministically");
}

void testBuiltInShellPalettes() {
    using namespace nppthemes;
    for (const auto& profile : builtInProfiles()) {
        const auto first = deriveShellPalette(profile);
        const auto second = deriveShellPalette(profile);
        check(first == second, profile.name + " shell derivation is deterministic");
        check(validateShellPalette(first).empty(), profile.name + " shell palette passes contrast validation");
        check(first.captionBackground == profile.palette.surface,
              profile.name + " caption derives from semantic surface");
        check(first.focusRing == profile.palette.accent, profile.name + " focus ring derives from accent");
    }
}

} // namespace

int main() {
    testBlendBoundaries();
    testBuiltInShellPalettes();
    if (failures == 0) {
        std::cout << "All NppThemes shell palette tests passed\n";
        return 0;
    }
    std::cerr << failures << " test(s) failed\n";
    return 1;
}
