# NppThemes Shell surface inventory

Baseline: official Notepad++ v8.9.7 at `6634650414ff91220a4c353b7fe5ad741af0f9f9`. Paths are relative to the shell repository.

| Surface family | Primary upstream owner | Current rendering path | Planned integration |
|---|---|---|---|
| Application window and startup background | `PowerEditor/src/Notepad_plus_Window.*`, `Notepad_plus.*` | Win32 main window, DWM, dark-mode coordinator | ThemeService lifetime, frame and fallback owner |
| Existing dark palette/control renderer | `PowerEditor/src/NppDarkMode.*`, `DarkMode/*` | Global palette, UxTheme, owner/subclass painting | Adapter first; replace global reads with resolved tokens incrementally |
| Editor, margins, selection | `PowerEditor/src/ScintillaComponent/*`, Scintilla | Scintilla messages and lexer XML | Shared editor profile compiler/runtime |
| Main and popup menus | `PowerEditor/src/DarkMode/UAHMenuBar.h`, main command dispatch | Native/UAH menu painting | Central menu token/state renderer |
| Toolbar | `PowerEditor/src/WinControls/ToolBar/*` | Win32 toolbar plus icon sets and dark accent options | Background/state tokens and audited icon pipeline |
| Document and panel tabs | `PowerEditor/src/WinControls/TabBar/*`, `NppDarkMode.cpp` tab subclass | Native tab control with custom dark painting | Active/inactive/hover/dirty/focus tokens |
| Status bar | `PowerEditor/src/WinControls/StatusBar/*` | Win32 status bar | Segment, separator, text, hover tokens |
| Docking chrome and splitters | `PowerEditor/src/WinControls/DockingWnd/*` | Custom docking manager, captions, bitmaps, splitters | Dock surface/caption/focus tokens; replace fixed bitmap assumptions |
| Find/Replace and search results | `PowerEditor/src/ScintillaComponent/FindReplaceDlg.*` | Resource dialogs, common controls, NppDarkMode handlers | Reusable themed-control wrappers and explicit surface ownership |
| Preferences | `PowerEditor/src/WinControls/Preference/*` | Resource dialog and tab/common controls | Theme Studio entry point plus standard dialog tokens |
| Plugins Admin | `PowerEditor/src/WinControls/PluginsAdmin/*` | Resource dialog, list/common controls | Standard control renderer; third-party plugin content remains excluded |
| Shortcut Mapper | `PowerEditor/src/WinControls/Grid/ShortcutMapper.*` | Custom grid/resource dialog | Grid header/cell/selection/focus tokens |
| File switcher | `PowerEditor/src/WinControls/VerticalFileSwitcher/*` | List view/resource dialog | List/state/preview tokens |
| Function List, Project Panel, Document Map | respective `PowerEditor/src/WinControls/*` folders | Docked panels, toolbars, trees, Scintilla snapshots | Shared dock/control tokens; preserve panel-specific content rendering |
| Message boxes, tooltips, progress, buttons, edits, lists, trees, combos | primarily `NppDarkMode.*` | UxTheme, subclass and custom drawing | ThemeService-backed reusable control adapter |
| OS file pickers, IME/accessibility overlays | Windows/external owner | OS rendering | Explicitly excluded; system High Contrast/light/dark wins |
| Third-party plugin windows | plugin owner | External plugin rendering | Explicitly excluded; expose theme metadata later without forced hooks |

## Inventory gate

Every application-owned top-level dialog and panel must be added before its phase begins, with its source owner, rendering mechanism, theme consumer, keyboard/DPI checks, and fallback behavior. A release cannot call a surface complete while unexplained stock-light islands remain in an NppThemes dark profile.
