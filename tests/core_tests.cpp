#include "nppthemes/ShellPalette.h"
#include "nppthemes/ThemeProfile.h"
#include "nppthemes/ThemeXmlCompiler.h"

#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

int failures = 0;

void check(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        ++failures;
    }
}

void testColorParsing() {
    using namespace nppthemes;
    check(parseHexColor("#12AbEF") == Color{0x12ABEF}, "hex colors parse case-insensitively");
    check(parseHexColor("12ABEF") == Color{0x12ABEF}, "hex colors parse without hash");
    check(!parseHexColor("#12345"), "short hex color rejected");
    check(!parseHexColor("#GG0000"), "invalid hex color rejected");
    check(formatHexColor(0x00A1BC) == "#00A1BC", "colors serialize canonically");
}

void testBuiltInProfiles() {
    using namespace nppthemes;
    const auto profiles = builtInProfiles();
    check(profiles.size() == 6, "six built-in profiles available");
    for (const auto& profile : profiles) {
        check(validateProfile(profile).empty(), profile.name + " validates");
        check(contrastRatio(profile.palette.foreground, profile.palette.background) >= 3.0,
              profile.name + " has usable text contrast");
        const auto serialized = serializeProfile(profile);
        check(deserializeProfile(serialized) == profile, profile.name + " JSON round trip");
    }
}

void testValidationAndImportLimits() {
    using namespace nppthemes;
    auto profile = builtInProfiles().front();
    profile.fontSizePt = 100;
    check(!validateProfile(profile).empty(), "invalid font size rejected");

    bool invalidModeRejected = false;
    auto serialized = serializeProfile(builtInProfiles().front());
    const auto offset = serialized.find("\"dark\"");
    serialized.replace(offset, 6, "\"sepia\"");
    try {
        static_cast<void>(deserializeProfile(serialized));
    } catch (const std::invalid_argument&) {
        invalidModeRejected = true;
    }
    check(invalidModeRejected, "unknown mode rejected");

    bool deeplyNestedRejected = false;
    try {
        static_cast<void>(deserializeProfile(std::string(65, '[') + std::string(65, ']')));
    } catch (const std::invalid_argument&) {
        deeplyNestedRejected = true;
    }
    check(deeplyNestedRejected, "deeply nested profile rejected before parsing");

    profile = builtInProfiles().front();
    profile.id = "../escape";
    check(!validateProfile(profile).empty(), "path-like profile identifier rejected");

    bool oversizedRejected = false;
    try {
        static_cast<void>(deserializeProfile(std::string(1024U * 1024U + 1U, 'x')));
    } catch (const std::invalid_argument&) {
        oversizedRejected = true;
    }
    check(oversizedRejected, "oversized profile rejected before parsing");
}

void testProfileV2MigrationAndOverrides() {
    using namespace nppthemes;
    const auto source = builtInProfiles().front();
    const auto migrated = migrateProfileToV2(source);
    check(migrated.schemaVersion == 2 && migrated.shell.has_value(), "v1 profile migrates to canonical v2 shell");
    check(deriveShellPalette(migrated) == deriveShellPalette(source), "v1-to-v2 migration preserves derived visuals");
    check(deserializeProfile(serializeProfile(migrated)) == migrated, "v2 profile JSON round trips");

    auto customized = migrated;
    customized.shell->density = ShellDensity::Compact;
    customized.shell->fontFamily = "Segoe UI Variable";
    customized.shell->fontSizePt = 10;
    customized.shell->colorOverrides["shell.caption.background"] = 0x202020;
    const auto roundTripped = deserializeProfile(serializeProfile(customized));
    check(roundTripped == customized, "v2 shell preferences and overrides round trip");
    check(deriveShellPalette(roundTripped).captionBackground == 0x202020,
          "v2 shell color override reaches resolved palette");

    customized.shell->colorOverrides["shell.asset.executable"] = 0xFFFFFF;
    check(!validateProfile(customized).empty(), "unknown or asset-like shell roles are rejected");
}

void testSemanticMapping() {
    using namespace nppthemes;
    const auto profile = builtInProfiles().front();
    check(semanticColorForStyle(profile, "C++", 5, 0) == profile.palette.keyword,
          "C++ keyword receives semantic keyword color");
    check(semanticColorForStyle(profile, "Python", 1, 0) == profile.palette.comment,
          "Python comment receives semantic comment color");
    check(semanticColorForStyle(profile, "JSON", 2, 0) == profile.palette.string,
          "JSON string receives semantic string color");
    check(semanticColorForStyle(profile, "unknown", 0, 0) == profile.palette.foreground,
          "default style receives foreground color");
}

void testThemeCompiler() {
    using namespace nppthemes;
    const auto root = std::filesystem::temp_directory_path() / "nppthemes-core-test";
    std::filesystem::create_directories(root);
    const auto source = root / "source.xml";
    const auto destination = root / "output.xml";

    {
        std::ofstream output(source, std::ios::binary | std::ios::trunc);
        output << R"(<?xml version="1.0" encoding="UTF-8" ?>
<NotepadPlus>
  <LexerStyles>
    <LexerType name="cpp" desc="C++" ext="">
      <WordsStyle name="DEFAULT" styleID="0" fgColor="000000" bgColor="FFFFFF" />
      <WordsStyle name="INSTRUCTION WORD" styleID="5" fgColor="0000FF" bgColor="FFFFFF" />
      <WordsStyle name="COMMENT LINE" styleID="2" fgColor="008000" bgColor="FFFFFF" />
    </LexerType>
  </LexerStyles>
  <GlobalStyles>
    <WidgetStyle name="Default Style" styleID="32" fgColor="000000" bgColor="FFFFFF" />
  </GlobalStyles>
</NotepadPlus>)";
    }

    const auto profile = builtInProfiles().front();
    const auto result = compileThemeXml(source, destination, profile);
    check(result.success, "theme compiler succeeds");
    check(result.stylesUpdated == 4, "theme compiler updates all fixture styles");
    check(std::filesystem::exists(destination), "theme compiler creates destination");

    std::ifstream input(destination, std::ios::binary);
    const std::string generated((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
    check(generated.find(formatHexColor(profile.palette.background).substr(1)) != std::string::npos,
          "generated theme contains profile background");
    check(generated.find("fontName=\"Cascadia Mono\"") != std::string::npos, "generated theme contains profile font");
    check(generated.find("LexerType name=\"cpp\"") != std::string::npos,
          "generated theme preserves lexer node and attributes");
    check(generated.find("name=\"INSTRUCTION WORD\"") != std::string::npos,
          "generated theme preserves unknown style metadata");

    std::error_code ignored;
    std::filesystem::remove_all(root, ignored);
}

} // namespace

int main() {
    testColorParsing();
    testBuiltInProfiles();
    testValidationAndImportLimits();
    testProfileV2MigrationAndOverrides();
    testSemanticMapping();
    testThemeCompiler();

    if (failures == 0) {
        std::cout << "All NppThemes core tests passed\n";
        return 0;
    }
    std::cerr << failures << " test(s) failed\n";
    return 1;
}
