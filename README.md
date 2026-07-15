# NppThemes

[![CI](https://github.com/0langa/npp-themes/actions/workflows/ci.yml/badge.svg)](https://github.com/0langa/npp-themes/actions/workflows/ci.yml)

NppThemes is a native appearance and workspace modernization plugin for Notepad++. It adds live semantic themes, a dockable Theme Studio, safe workspace profiles, portable JSON profiles, and generated native theme XML without using private Notepad++ APIs.

## Features

- Six curated light, dark, high-contrast, and low-distraction profiles.
- Live editor styling across both Notepad++ views.
- Semantic colors for C-family languages, Python, JSON, HTML/XML/PHP, SQL, PowerShell, and shell languages.
- Contrast-aware palette editing for background, foreground, accent, keyword, string, and comment roles.
- Font family and size customization.
- Versioned JSON import/export with input limits and validation.
- Native Notepad++ theme XML generation that preserves unknown lexers and style nodes.
- Reversible Focus and Minimal workspace profiles using documented host messages.
- Searchable command palette for commands, themes, and open documents in both views.
- Presentation workspace with temporary larger editor typography.
- Opt-in title-bar, caption-text, and window-border colors through documented DWM attributes, with exact in-process restoration and High Contrast fallback.
- Exact workspace-state restoration and startup recovery after interrupted sessions.
- Transactional settings writes with recovery backups.
- No telemetry, accounts, background network access, or self-updater.

## Requirements

- Windows 10 or Windows 11.
- Notepad++ 8.5.4 or newer.
- x64 and x86 builds are tested. ARM64 is built in CI and remains provisional until hardware testing.

## Install

### Release package

1. Download archive matching Notepad++ architecture from [Releases](https://github.com/0langa/npp-themes/releases).
2. Create `NppThemes` folder under Notepad++ `plugins` directory.
3. Place `NppThemes.dll` inside that folder.
4. Restart Notepad++.
5. Open **Plugins > NppThemes > Theme Studio...**.

Expected installed layout:

```text
Notepad++/
└─ plugins/
   └─ NppThemes/
      └─ NppThemes.dll
```

Plugins Admin distribution follows after public beta compatibility data is sufficient.

## Quick start

- Open Theme Studio: `Ctrl+Alt+T`.
- Open command palette and document navigator: `Ctrl+Alt+P`.
- Choose a preset to preview it, then select **Use preset** to persist it.
- Adjust semantic colors or typography, then select **Apply custom profile**.
- Toggle Focus workspace: `Ctrl+Alt+F`.
- Toggle Minimal workspace: `Ctrl+Alt+M`.
- Toggle Presentation workspace from command palette or Plugins menu.
- Restore native editor and UI appearance: `Ctrl+Alt+R`.
- Select **Install XML** to create a native theme for Notepad++ Style Configurator.
- Enable **Theme title bar + border** for the supported whole-window frame subset. Theme Studio reports whether it is active.

See [User Guide](docs/user-guide/README.md), including recovery instructions and known limitations.
Dedicated references: [Recovery and rollback](docs/recovery.md) and [Threat model](docs/threat-model.md).

## Build and test

```powershell
cmake --preset x64
cmake --build --preset x64-release --parallel
ctest --preset x64-release
```

Run isolated host smoke test without changing installed Notepad++:

```powershell
.\tools\test-portable-host.ps1 -Architecture x64 -Configuration Release
```

See [CONTRIBUTING.md](CONTRIBUTING.md) for prerequisites, architecture, tests, and pull-request gates.

## Design boundary

Stable builds use documented `NPPM_*`, `NPPN_*`, Scintilla, DWM, Win32, and theme/configuration contracts only. NppThemes does not subclass private Notepad++ chrome, patch binaries, or send `NPPM_INTERNAL_*` messages. The stock plugin can color supported native frame attributes, but full menu/toolbar/tab/dialog replacement requires the separate maintained shell described in the [full-window roadmap](FULL_WINDOW_THEMING_PLAN.md).

Stock-plugin roadmap and quality gates: [DEVELOPMENT_PLAN.md](DEVELOPMENT_PLAN.md). Complete-window delivery: [FULL_WINDOW_THEMING_PLAN.md](FULL_WINDOW_THEMING_PLAN.md).

## License

GNU General Public License v3.0. See [LICENSE](LICENSE).
