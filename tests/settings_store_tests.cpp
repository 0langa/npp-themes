#include "SettingsStore.h"

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

void testSettingsRoundTrip(const std::filesystem::path& root) {
    using namespace nppthemes;
    using namespace nppthemes::plugin;

    SettingsStore store;
    check(store.initialize(root), "settings store initializes");
    check(store.load().activeProfileId == "builtin.northern-lights", "missing settings produce defaults");

    PersistedSettings expected;
    expected.activeProfileId = "user.custom";
    expected.customProfile = builtInProfiles().at(2);
    expected.customProfile->id = "user.custom";
    expected.applyOnStartup = false;
    expected.workspace.active = true;
    expected.workspace.mode = "minimal";
    expected.workspace.previous = {false, true, false, true};

    check(store.save(expected), "settings save succeeds");
    const auto actual = store.load();
    check(actual.activeProfileId == expected.activeProfileId, "active profile round trips");
    check(actual.customProfile == expected.customProfile, "custom profile round trips");
    check(actual.applyOnStartup == expected.applyOnStartup, "startup flag round trips");
    check(actual.workspace.active && actual.workspace.mode == "minimal", "workspace session round trips");
    check(actual.workspace.previous.toolbarHidden && actual.workspace.previous.statusbarHidden,
          "workspace prior state round trips");

    expected.applyOnStartup = true;
    check(store.save(expected), "second settings save succeeds");
    auto backup = store.settingsPath();
    backup += L".bak";
    check(std::filesystem::exists(backup), "second save creates recovery backup");
}

void testProfileImportExport(const std::filesystem::path& root) {
    using namespace nppthemes;
    using namespace nppthemes::plugin;

    SettingsStore store;
    check(store.initialize(root), "import store initializes");
    const auto profile = builtInProfiles().at(4);
    const auto output = root / L"exported.npptheme.json";
    check(store.exportProfile(output, profile), "profile export succeeds");
    check(store.importProfile(output) == profile, "exported profile imports identically");

    const auto oversized = root / L"oversized.json";
    {
        std::ofstream file(oversized, std::ios::binary | std::ios::trunc);
        const std::string block(1024, 'x');
        for (int index = 0; index < 1025; ++index) {
            file << block;
        }
    }
    bool rejected = false;
    try {
        static_cast<void>(store.importProfile(oversized));
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    check(rejected, "oversized import rejected before parsing");
}

void testCorruptSettingsFallback(const std::filesystem::path& root) {
    using namespace nppthemes::plugin;
    SettingsStore store;
    check(store.initialize(root), "corrupt-settings store initializes");
    {
        std::ofstream output(store.settingsPath(), std::ios::binary | std::ios::trunc);
        output << "{not-json";
    }
    const auto settings = store.load();
    check(settings.activeProfileId == "builtin.northern-lights", "corrupt settings fail closed to defaults");
    check(std::filesystem::exists(store.root() / L"NppThemes.log"), "corrupt settings produce local diagnostic");
}

void testCorruptSettingsBackupRecovery(const std::filesystem::path& root) {
    using namespace nppthemes::plugin;
    SettingsStore store;
    check(store.initialize(root), "backup-recovery store initializes");
    PersistedSettings original;
    original.activeProfileId = "builtin.paper";
    check(store.save(original), "initial recovery settings save succeeds");
    original.activeProfileId = "builtin.graphite";
    check(store.save(original), "second recovery settings save creates backup");
    {
        std::ofstream output(store.settingsPath(), std::ios::binary | std::ios::trunc);
        output << "{broken";
    }
    check(store.load().activeProfileId == "builtin.paper", "corrupt primary recovers valid backup");
}

} // namespace

int main() {
    const auto root = std::filesystem::temp_directory_path() / "nppthemes-settings-tests";
    std::error_code ignored;
    std::filesystem::remove_all(root, ignored);
    std::filesystem::create_directories(root);

    testSettingsRoundTrip(root / L"roundtrip");
    testProfileImportExport(root / L"profiles");
    testCorruptSettingsFallback(root / L"corrupt");
    testCorruptSettingsBackupRecovery(root / L"recovery");

    std::filesystem::remove_all(root, ignored);
    if (failures == 0) {
        std::cout << "All NppThemes settings tests passed\n";
        return 0;
    }
    std::cerr << failures << " test(s) failed\n";
    return 1;
}
