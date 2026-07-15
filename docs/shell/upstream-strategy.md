# NppThemes Shell upstream strategy

## Repositories and authority

- Shared profile/core and stock plugin: <https://github.com/0langa/npp-themes>
- Full-window shell fork: <https://github.com/0langa/npp-themes-shell>
- Official upstream: <https://github.com/notepad-plus-plus/notepad-plus-plus>

The shell fork was created on 2026-07-16. Its initial product branch `shell/main` is pinned to official tag `v8.9.7`, commit `6634650414ff91220a4c353b7fe5ad741af0f9f9` dated 2026-07-14.

## Branches

- `master`: untouched mirror of official upstream `master`; never receives NppThemes product commits.
- `shell/main`: releasable NppThemes Shell integration branch. It starts at a verified official release commit.
- `sync/upstream-vX.Y.Z`: temporary branch for each upstream intake and conflict qualification.
- `feature/<area>`: focused shell work based on `shell/main`.

The GitHub default branch is `shell/main`. `upstream` points to the official repository and `origin` to the public fork.

## Shared-core delivery

`npp-themes` remains canonical for profile parsing, migration, validation, `ShellPalette`, the conformance executable, and fixtures. The shell imports reviewed source as a Git subtree under `PowerEditor/src/NppThemesCore`. Each subtree merge records the exact source commit. The shell does not load or bundle `NppThemes.dll`.

## Upstream intake

1. Fetch and verify the official release tag and resolved commit.
2. Fast-forward local `master` to `upstream/master` and push it without product changes.
3. Create `sync/upstream-vX.Y.Z` from the current `shell/main`.
4. Merge the verified upstream release commit. Do not squash away upstream history.
5. Resolve conflicts with the smallest theme-specific delta and document every behavioral conflict.
6. Build upstream configurations, run NppThemes conformance/unit/visual suites, and test settings/plugin compatibility.
7. Merge the qualified sync branch into `shell/main` and record the new upstream base in release notes.

Security fixes take precedence over visual work. A security-only upstream merge may bypass feature batching but not build, startup, provenance, or rollback checks.

## Branding and coexistence

Before distributing any shell binary, change product name, executable/installer identity, icons, config root, update channel, crash/report labels, and file-association prompts so users cannot confuse it with official Notepad++. The default install path must be side-by-side. Import of official settings is explicit and reversible.

GPL source, exact upstream base, fork patch history, third-party notices, checksums, SBOM, and provenance accompany every package. Preview/beta packages require these artifacts; Shell 1.0 additionally requires project-controlled Authenticode signing.
