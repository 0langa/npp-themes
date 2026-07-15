# Compatibility matrix

## Current evidence

| Target | Build | Automated tests | Real host smoke | Status |
|---|---:|---:|---:|---|
| Windows x64 / Notepad++ 8.9.7 | Pass | Pass | Pass | Supported preview |
| Windows x86 / Notepad++ 8.9.7 | Pass | Pass | Pass | Supported preview |
| Windows ARM64 | CI target | Cannot run on x64 host | Requires ARM64 hardware | Provisional |

## Minimum host behavior

- Runtime theme engine and dark-aware plugin UI target Notepad++ 8.5.4+.
- `NPPM_GETNPPSETTINGSDIRPATH` is capability-checked; missing support disables one-click XML installation only.
- Theme Studio docking, modeless keyboard registration, Scintilla styles, and documented chrome visibility use public API.

## Release qualification matrix

Before 1.0:

- Minimum supported and latest stable Notepad++.
- x64 and x86 portable builds.
- ARM64 portable build on ARM64 Windows or ARM64 removed from supported release list.
- Standard, portable, cloud settings, and `-settingsDir` paths.
- Windows 10 22H2 and current maintained Windows 11.
- Light, dark, Windows high contrast.
- 100%, 125%, 150%, 175%, 200%, mixed-DPI monitors.
