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

void testCanonicalContract() {
    using namespace nppthemes;
    auto profile = migrateProfileToV2(builtInProfiles().front());
    profile.shell->colorOverrides["shell.caption.background"] = 0x010203;
    const auto palette = deriveShellPalette(profile);
    check(palette.captionBackground == 0x010203, "profile v2 override replaces derived role");
    const auto contract = serializeShellPalette(palette);
    check(contract.find("\"contractVersion\": 1") != std::string::npos, "canonical contract declares its version");
    check(contract.find("\"shell.caption.background\": \"#010203\"") != std::string::npos,
          "canonical contract contains normalized override");
    check(serializeShellPalette(palette) == contract, "canonical shell contract is deterministic");
    check(isShellColorRole("shell.tab.active"), "known shell role is accepted");
    check(!isShellColorRole("shell.tab.unknown"), "unknown shell role is rejected");
}

} // namespace

int main() {
    testBlendBoundaries();
    testBuiltInShellPalettes();
    testCanonicalContract();
    if (failures == 0) {
        std::cout << "All NppThemes shell palette tests passed\n";
        return 0;
    }
    std::cerr << failures << " test(s) failed\n";
    return 1;
}
