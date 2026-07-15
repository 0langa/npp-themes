# Threat model

## Scope and assets

NppThemes is a native DLL loaded into Notepad++. Assets are editor availability, user configuration integrity, theme/profile files, and the confidentiality of filenames and document content.

## Trust boundaries

- Imported JSON profiles and host theme model XML are untrusted input.
- Notepad++ messages and host-provided paths cross the plugin/host boundary.
- GitHub Actions and pinned build dependencies cross the source/release supply-chain boundary.
- The plugin configuration and active settings directories are the only persistent write roots.

## Principal threats and controls

| Threat | Control |
|---|---|
| Malformed or oversized profile consumes resources | 1 MiB size cap, 64-level nesting cap, strict schema and field limits |
| Profile identifier becomes a path | Restricted identifier alphabet; generated filenames are sanitized |
| Malformed XML or entity content changes behavior | pugixml parsing without external-entity resolution, 16 MiB cap, required-root validation, escaped output, post-write parse |
| Theme generation corrupts active host files | New destination only, temporary write, validation, atomic replace; source and destination cannot match |
| Interrupted settings write strands hidden UI | Backup plus atomic replace, persisted workspace marker, startup chrome recovery, fixed restore shortcut |
| Plugin reads document content or leaks metadata | Theme logic sends style messages only; logs contain lifecycle messages, not document names or contents; no runtime network |
| Private API drift crashes host | Stable channel uses documented Notepad++ and Scintilla messages only |
| Compromised build dependency or workflow | Exact dependency commits, Dependabot for actions, CodeQL, clean tag builds, checksums, GitHub build provenance |

## Residual risk

Native memory-safety defects can affect the Notepad++ process. Compatibility testing, strict compiler warnings, CodeQL, narrow dependencies, bounded inputs, and exception containment reduce but cannot eliminate this risk. ARM64 runtime behavior remains provisional until hardware testing.
