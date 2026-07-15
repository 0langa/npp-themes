# Contributing to NppThemes

## Prerequisites

- Windows 10 or 11
- Visual Studio 2022 Build Tools with Desktop development with C++
- CMake 3.24 or newer
- Git
- Notepad++ 8.5.4 or newer for manual testing

Dependencies are fetched at pinned commits during CMake configure. Runtime plugin has no non-system DLL dependency.

## Build

```powershell
cmake --preset x64
cmake --build --preset x64-debug --parallel
ctest --preset x64-debug
```

Other release presets:

```powershell
cmake --preset x86
cmake --build --preset x86-release --parallel

cmake --preset arm64
cmake --build --preset arm64-release --parallel --target NppThemes
```

ARM64 needs Visual Studio ARM64 C++ build tools. ARM64 tests must run on ARM64 Windows hardware.

## Real-host smoke test

`tools/test-portable-host.ps1` copies installed Notepad++ into ignored `out/test-host`, enables portable configuration, installs built plugin, starts a separate instance, waits for plugin ready log, then closes test instance.

```powershell
.\tools\test-portable-host.ps1 -Configuration Release
```

Script does not modify installed Notepad++.

## Architecture

- `NppThemesCore`: profile model, validation, semantic mapping, JSON, XML compiler.
- `HostAdapter`: typed boundary for documented Notepad++ and Scintilla messages.
- `SettingsStore`: bounded import, transactional persistence, backup, recovery log.
- `Application`: lifecycle, commands, profile selection, workspace recovery.
- `ThemeStudio`: native dockable UI and accessibility-compatible controls.

Read [ADR-0001](docs/adr/0001-supported-plugin-boundary.md) before changing host integration.

## Change requirements

- Build with `/W4 /WX` and no new warnings.
- Add focused tests for behavior, parsers, settings, recovery, or host-message changes.
- Run x64 Debug and Release tests.
- Run x86 tests when touching ABI, Win32, serialization, or packaging.
- Update user guide and changelog for user-visible behavior.
- Never add undocumented Notepad++ messages or private-window subclassing.
- Persistent writes must stay inside documented configuration roots and use transactional helper.
- Imported data must retain size and schema validation.

## Pull requests

PR description must include:

- User problem and behavior change.
- Tests run and results.
- Notepad++/architecture compatibility impact.
- Persistent-data or recovery impact.
- UI accessibility and DPI impact.
- Screenshots for visible Theme Studio changes.

## License

Contributions are accepted under GPL-3.0. By submitting code, contributor agrees contribution may be distributed under project license.
