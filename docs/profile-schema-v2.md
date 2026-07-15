# NppThemes profile schema v2

Profile v2 extends the existing editor profile with host-independent shell preferences. Version 1 remains accepted and produces the same editor visuals. Migration to v2 adds default shell settings and does not alter any derived color.

## Contract

Required top-level fields remain:

- `schemaVersion`: `1` or `2`.
- `id`: 1–128 ASCII letters, digits, dots, underscores, or hyphens.
- `name`: 1–128 characters without control characters.
- `mode`: `light` or `dark`.
- `palette`: the 16 required editor colors in canonical `#RRGGBB` form.
- `typography`: installed editor `fontFamily` and `fontSizePt` from 6–48.

Version 2 additionally requires `shell`:

```json
{
  "schemaVersion": 2,
  "shell": {
    "density": "comfortable",
    "typography": {
      "fontFamily": "Segoe UI",
      "fontSizePt": 9
    },
    "colors": {
      "shell.caption.background": "#20242C",
      "shell.caption.foreground": "#F4F7FB"
    }
  }
}
```

`density` is `compact`, `comfortable`, or `touch`. Shell fonts must already be installed; consumers use their audited system fallback if unavailable.

`colors` is an override map. Omitted roles use deterministic values derived from the editor palette. The allowed groups are:

- `shell.window.*`, `shell.surface.*`, `shell.border`, `shell.divider`, `shell.focusRing`
- `shell.caption.*`, `shell.menu.*`, `shell.toolbar.*`
- `shell.tab.*`, `shell.status.*`, `shell.scrollbar.*`
- `shell.control.*`, `shell.dialog.*`, `shell.icon.*`

The exact 45 role names and their canonical order are emitted by `NppThemesConformance`. Unknown roles are rejected. Profiles cannot embed fonts, icons, scripts, executables, paths, DLLs, or arbitrary binary assets.

## Validation and safety

- Maximum serialized input size is 1 MiB and maximum JSON nesting is 64 levels.
- Caption, menu, active-tab, status, control, and dialog text require at least 4.5:1 contrast.
- Focus indication requires at least 3:1 contrast.
- Validation finishes before any host window state changes.
- Windows forced High Contrast overrides profile shell rendering.

## Migration

`migrateProfileToV2` validates the source, sets `schemaVersion` to `2`, and adds comfortable density with Segoe UI 9 pt and no explicit shell colors. The resolved `ShellPalette` before and after migration must be identical.

Consumers must not silently downgrade v2 to v1. Stock NppThemes uses the applicable frame subset; NppThemes Shell consumes all roles.

## Conformance

Build and run:

```powershell
cmake --build --preset x64-release --target NppThemesConformance
.\out\build\x64\Release\NppThemesConformance.exe .\tests\fixtures\profile-v2.json
```

The executable validates the input and emits canonical JSON containing `contractVersion` and every resolved shell token. The future shell fork must produce a byte-identical token document for the same fixture before a shared-core update can merge.
