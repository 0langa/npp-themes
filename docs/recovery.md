# Recovery and rollback

NppThemes treats appearance changes as reversible. It does not edit active Notepad++ configuration XML.

## Emergency UI restore

Press `Ctrl+Alt+R`. This restores the captured editor appearance and the exact menu, toolbar, tabs, and status-bar visibility that existed before a workspace profile was activated.

If Notepad++ terminates while Focus, Minimal, or Presentation is active, the persisted recovery marker is detected on next startup. Native chrome is restored before the normal theme is reapplied.

## Disable plugin without loading it

1. Exit every Notepad++ process.
2. Rename `plugins\NppThemes` to `plugins\NppThemes.disabled`.
3. Start Notepad++.

This preserves settings and backups for inspection.

## Settings rollback

Settings are written through a temporary file and atomic replace. Before replacing an existing file, plugin writes `settings.json.bak`. A corrupt primary settings file is automatically recovered from a valid backup. If both are invalid, safe defaults are used.

With Notepad++ closed, settings may also be reset manually by removing the plugin-owned `NppThemes` directory below the host-provided plugin configuration directory.

## Generated theme rollback

Generated theme files are new files in the active settings `themes` directory. Choose another theme in **Settings > Style Configurator**, close Notepad++, then remove the `NppThemes - *.xml` file if desired. Source model files are never overwritten.

## Uninstall

Restore native appearance first, exit Notepad++, then remove `plugins\NppThemes`. Plugin-owned configuration may be retained for reinstall or removed separately. No runtime files exist outside the Notepad++ plugin and active settings roots.
