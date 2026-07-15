# Security Policy

## Supported versions

Until 1.0, only latest preview release receives security fixes. After 1.0, latest minor release receives fixes; older releases may receive critical fixes when practical.

## Reporting vulnerability

Do not open public issue for suspected vulnerability involving arbitrary code execution, path escape, unsafe configuration writes, or data loss.

Use GitHub private vulnerability reporting for this repository. Include:

- Affected NppThemes and Notepad++ versions.
- Architecture and Windows version.
- Reproduction steps or proof of concept.
- Expected impact.
- Suggested mitigation, if known.

No credentials, private documents, or unrelated log content should be included.

## Security posture

- Plugin executes inside Notepad++ process and is high trust.
- Stable code uses documented Notepad++ and Scintilla APIs only.
- Core operation is local-only; no telemetry or runtime network access.
- Profile imports are size- and nesting-limited and schema-validated.
- XML output escapes content through pugixml.
- Configuration writes use temp file, backup, and atomic replacement.
- Settings and generated themes are restricted to host-provided configuration roots.
- Releases are built in GitHub Actions and include SHA-256 checksums and build provenance attestations.

Detailed trust boundaries and controls: [docs/threat-model.md](docs/threat-model.md).
