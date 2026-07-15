# ADR 0001: Supported plugin boundary

Status: Accepted, amended 2026-07-16

Date: 2026-07-15

## Context

NppThemes aims for significant Notepad++ appearance modernization. Notepad++ plugin API exposes editor styles, dark-mode-aware dialogs, docking, documented UI visibility, and menu commands. It does not provide stable replacement hooks for title bar, menu rendering, toolbar layout, or native tab strip.

Internal messages and child-window subclassing could reach more UI but can change without notice and can crash host.

## Decision

Stable releases use only:

- Documented `NPPM_*` messages.
- Documented `NPPN_*` notifications.
- Public Scintilla messages.
- Public docking structures from official template.
- Win32 APIs for plugin-owned windows.
- Documented DWM attributes on the Notepad++ top-level window only when capability-checked, opt-in, transactionally applied, and exactly restorable during the current process.
- Documented theme/configuration file formats and host-provided paths.

Stable releases prohibit:

- `NPPM_INTERNAL_*` messages.
- Private child-window class discovery or subclassing.
- Binary patching, DLL injection, or executable modification.
- Writing live Notepad++ `config.xml` to force theme selection.

## Consequences

- Plugin can modernize editor, plugin surfaces, workflow, and chrome visibility safely.
- Native application frame keeps Notepad++/Windows visual ownership except for documented caption, caption-text, and border color attributes. No custom non-client painting or subclassing is permitted.
- Windows forced High Contrast overrides NppThemes frame colors.
- Full frame redesign requires separate maintained Notepad++ fork and separate distribution.
- Unsupported capabilities appear disabled/explained rather than guessed.
