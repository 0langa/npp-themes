# NppThemes User Guide

## Theme Studio

Open **Plugins > NppThemes > Theme Studio...** or press `Ctrl+Alt+T`.

Theme Studio starts as a floating dockable window because Notepad++ gives newly docked panels a narrow default width. Drag window onto Notepad++ docking guides to dock it after sizing it comfortably.

### Presets

1. Select profile in Profile list. Selection previews immediately.
2. Select **Use preset** to persist selection across restarts.
3. If preview is unwanted, choose current saved profile again or select **Restore native UI**.

Built-in profiles:

- Northern Lights
- Graphite
- Midnight Neon
- Paper
- Warm Sand
- Accessible Dark

### Custom profile

1. Choose starting preset.
2. Select semantic color swatches.
3. Enter installed font family and size from 6–48 points.
4. Review foreground/background contrast indicator.
5. Select **Apply custom profile**.

Built-in presets are immutable. Customization saves as `user.custom`.

### Window frame

Enable **Theme title bar + border** to apply the profile to supported Windows title-bar, caption-text, and border attributes. The adjacent status reads:

- **Off** — opt-in is disabled.
- **Active** — all supported attributes were captured and applied.
- **Unavailable** — Windows rejected or does not support an attribute, or forced High Contrast is active. NppThemes fails closed and keeps/restores the native frame.

Disabling the option or choosing **Restore native UI** restores the exact frame values captured in this Notepad++ process. The setting defaults off. It does not custom-paint menus, toolbars, tabs, status bars, or built-in dialogs.

### Import and export

- **Export** writes current applied profile as `.npptheme.json`.
- **Import** accepts validated profile up to 1 MiB and applies it as custom profile.
- Exported profiles contain no paths or machine identifiers.
- Schema-v1 profiles remain supported. Schema v2 adds shell density, shell typography preferences, and validated per-role shell colors; see [Profile schema v2](../profile-schema-v2.md).

### Install XML

Runtime styling is applied automatically by plugin. **Install XML** additionally generates complete theme file from current Notepad++ `stylers.model.xml` or `DarkModeDefault.xml`.

Generated file is placed in active settings `themes` folder. Select it through **Settings > Style Configurator** for native persistence outside plugin runtime.

Older Notepad++ versions without active-settings-path API keep runtime styling but cannot use one-click XML installation.

## Command palette and document navigator

Press `Ctrl+Alt+P`, then type part of a command, theme, filename, or folder. Press Enter to run the selected item. The list uses current non-deprecated Notepad++ buffer APIs and includes documents from both views.

Available actions include opening Theme Studio, switching a theme, toggling every workspace, restoring native appearance, and activating an open document.

## Workspace profiles

### Focus

Hides toolbar and status bar while keeping menu and document tabs.

Toggle: `Ctrl+Alt+F`.

### Minimal

Hides menu, toolbar, tab bar, and status bar.

Toggle: `Ctrl+Alt+M`.

### Restore

`Ctrl+Alt+R` restores the editor style snapshot, captured window-frame attributes, and exact menu/toolbar/tab/status visibility captured before workspace profile.

Plugin records workspace state before hiding controls. If Notepad++ terminates before restoration, next startup restores native controls before reapplying normal plugin state.

### Presentation

Keeps menu and tabs, hides toolbar and status bar, and temporarily raises editor type to at least 16 points. Toggle it from **Plugins > NppThemes** or the command palette. Exiting restores the saved theme and exact previous chrome state.

## Recovery

### UI controls hidden

Press `Ctrl+Alt+R`.

If shortcut was remapped, use **Plugins > NppThemes > Restore Native Appearance**. In Minimal mode menu is hidden; press `Alt` to expose menu temporarily, then choose command.

### Plugin fails during startup

1. Exit all Notepad++ instances.
2. Rename plugin folder from `plugins\NppThemes` to `plugins\NppThemes.disabled`.
3. Start Notepad++.
4. Settings and backups remain under `plugins\Config\NppThemes` for inspection.

### Restore settings backup

Plugin settings live at host-provided plugin config path:

```text
plugins/Config/NppThemes/settings.json
plugins/Config/NppThemes/settings.json.bak
```

With Notepad++ closed, replace corrupt `settings.json` with `.bak`, or remove settings file to return plugin defaults. Generated native theme XML is separate and can be deselected through Style Configurator.

## Known limitations

- The stock plugin can opt-in to documented title-bar, caption-text, and border colors. Native menu/toolbar/tab/status/dialog rendering remains host-owned and is not reskinned.
- Runtime semantic mapping is richest for common languages; unknown lexers keep existing readable foreground where contrast allows.
- Generated XML covers every lexer/style present in host model but semantic classification uses style-name heuristics.
- Font must already be installed. Missing font falls back through Scintilla/Windows.
- ARM64 build remains provisional until tested on ARM64 Windows hardware.
