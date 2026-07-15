#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>

#include "nppthemes/ShellPalette.h"
#include "nppthemes/ThemeProfile.h"

namespace {

constexpr std::uintmax_t maxProfileBytes = 1024U * 1024U;

[[nodiscard]] std::string readProfile(const std::filesystem::path& path) {
    std::error_code error;
    const auto size = std::filesystem::file_size(path, error);
    if (error) {
        throw std::runtime_error("unable to inspect profile");
    }
    if (size > maxProfileBytes) {
        throw std::invalid_argument("profile exceeds 1 MiB limit");
    }
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        throw std::runtime_error("unable to open profile");
    }
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

} // namespace

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "Usage: NppThemesConformance <profile.npptheme.json>\n";
        return 2;
    }
    try {
        const auto profile = nppthemes::deserializeProfile(readProfile(argv[1]));
        const auto palette = nppthemes::deriveShellPalette(profile);
        const auto errors = nppthemes::validateShellPalette(palette);
        if (!errors.empty()) {
            throw std::invalid_argument(errors.front());
        }
        std::cout << nppthemes::serializeShellPalette(palette);
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Profile rejected: " << error.what() << '\n';
        return 1;
    }
}
