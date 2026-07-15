cmake_minimum_required(VERSION 3.24)

project(NppThemes VERSION 0.1.0 LANGUAGES CXX RC)

include(CTest)
include(FetchContent)

if(POLICY CMP0169)
    cmake_policy(SET CMP0169 OLD)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/generated")
configure_file(
    src/plugin/NppThemes.rc
    "${CMAKE_CURRENT_BINARY_DIR}/generated/NppThemes.rc"
    @ONLY
)

FetchContent_Declare(
    npp_sdk
    GIT_REPOSITORY https://github.com/npp-plugins/plugintemplate.git
    GIT_TAG 987553015dc12caf2ca0e49617ba1f0e2d17faae
    GIT_SHALLOW FALSE
)
FetchContent_GetProperties(npp_sdk)
if(NOT npp_sdk_POPULATED)
    FetchContent_Populate(npp_sdk)
endif()

FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG 65ee68451d8eb2b5f3a30b410476ab83deb3289b
    GIT_SHALLOW FALSE
)
set(JSON_BuildTests OFF CACHE INTERNAL "")
set(JSON_Install OFF CACHE INTERNAL "")
FetchContent_MakeAvailable(nlohmann_json)

FetchContent_Declare(
    pugixml
    GIT_REPOSITORY https://github.com/zeux/pugixml.git
    GIT_TAG ee86beb30e4973f5feffe3ce63bfa4fbadf72f38
    GIT_SHALLOW FALSE
)
set(PUGIXML_BUILD_TESTS OFF CACHE INTERNAL "")
set(PUGIXML_BUILD_SAMPLES OFF CACHE INTERNAL "")
FetchContent_MakeAvailable(pugixml)

add_library(NppThemesCore STATIC
    src/core/ShellPalette.cpp
    src/core/ThemeProfile.cpp
    src/core/ThemeXmlCompiler.cpp
)
target_include_directories(NppThemesCore PUBLIC include)
target_link_libraries(NppThemesCore PUBLIC nlohmann_json::nlohmann_json pugixml::pugixml)
target_compile_features(NppThemesCore PUBLIC cxx_std_20)

add_library(NppThemes SHARED
    src/plugin/Application.cpp
    src/plugin/CommandPalette.cpp
    src/plugin/HostAdapter.cpp
    src/plugin/PluginMain.cpp
    src/plugin/SettingsStore.cpp
    src/plugin/ShellBridge.cpp
    src/plugin/ThemeStudio.cpp
    "${CMAKE_CURRENT_BINARY_DIR}/generated/NppThemes.rc"
)
target_include_directories(NppThemes PRIVATE
    include
    src/plugin
    "${npp_sdk_SOURCE_DIR}/src"
    "${npp_sdk_SOURCE_DIR}/src/DockingFeature"
)
target_link_libraries(NppThemes PRIVATE
    NppThemesCore
    Comctl32
    Comdlg32
    Dwmapi
    Shlwapi
)
target_compile_definitions(NppThemes PRIVATE
    UNICODE
    _UNICODE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    NPP_THEMES_VERSION="${PROJECT_VERSION}"
)
set_target_properties(NppThemes PROPERTIES
    PREFIX ""
    OUTPUT_NAME "NppThemes"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

if(MSVC)
    foreach(target NppThemesCore NppThemes)
        target_compile_options(${target} PRIVATE /W4 /WX /permissive- /utf-8 /EHsc)
    endforeach()
    target_compile_options(NppThemes PRIVATE /guard:cf)
    target_link_options(NppThemes PRIVATE /DYNAMICBASE /NXCOMPAT /guard:cf)
endif()

if(BUILD_TESTING)
    add_executable(NppThemesCoreTests tests/core_tests.cpp)
    target_link_libraries(NppThemesCoreTests PRIVATE NppThemesCore)
    add_executable(NppThemesHostTests
        tests/host_adapter_tests.cpp
        src/plugin/HostAdapter.cpp
        src/plugin/ShellBridge.cpp
    )
    add_executable(NppThemesSettingsTests
        tests/settings_store_tests.cpp
        src/plugin/SettingsStore.cpp
    )
    add_executable(NppThemesShellPaletteTests tests/shell_palette_tests.cpp)
    target_link_libraries(NppThemesShellPaletteTests PRIVATE NppThemesCore)
    add_executable(NppThemesShellBridgeTests
        tests/shell_bridge_tests.cpp
        src/plugin/ShellBridge.cpp
    )
    target_include_directories(NppThemesShellBridgeTests PRIVATE include src/plugin)
    target_link_libraries(NppThemesShellBridgeTests PRIVATE NppThemesCore Dwmapi User32)
    target_compile_definitions(NppThemesShellBridgeTests PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
    target_include_directories(NppThemesHostTests PRIVATE
        include
        src/plugin
        "${npp_sdk_SOURCE_DIR}/src"
        "${npp_sdk_SOURCE_DIR}/src/DockingFeature"
    )
    target_link_libraries(NppThemesHostTests PRIVATE NppThemesCore Dwmapi User32)
    target_compile_definitions(NppThemesHostTests PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
    target_include_directories(NppThemesSettingsTests PRIVATE include src/plugin)
    target_link_libraries(NppThemesSettingsTests PRIVATE NppThemesCore)
    target_compile_definitions(NppThemesSettingsTests PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
    if(MSVC)
        target_compile_options(NppThemesCoreTests PRIVATE /W4 /WX /permissive- /utf-8 /EHsc)
        target_compile_options(NppThemesHostTests PRIVATE /W4 /WX /permissive- /utf-8 /EHsc)
        target_compile_options(NppThemesSettingsTests PRIVATE /W4 /WX /permissive- /utf-8 /EHsc)
        target_compile_options(NppThemesShellPaletteTests PRIVATE /W4 /WX /permissive- /utf-8 /EHsc)
        target_compile_options(NppThemesShellBridgeTests PRIVATE /W4 /WX /permissive- /utf-8 /EHsc)
    endif()
    add_test(NAME core COMMAND NppThemesCoreTests)
    add_test(NAME host-adapter COMMAND NppThemesHostTests)
    add_test(NAME settings-store COMMAND NppThemesSettingsTests)
    add_test(NAME shell-palette COMMAND NppThemesShellPaletteTests)
    add_test(NAME shell-bridge COMMAND NppThemesShellBridgeTests)
endif()

install(TARGETS NppThemes RUNTIME DESTINATION .)
install(FILES README.md LICENSE DESTINATION doc)
