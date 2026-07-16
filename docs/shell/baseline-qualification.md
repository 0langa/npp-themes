# NppThemes Shell baseline qualification

## 2026-07-16 — v8.9.7 x64 Release

- Repository: `0langa/npp-themes-shell`
- Branch: `shell/main`
- Official source tag: `v8.9.7`
- Resolved source commit: `6634650414ff91220a4c353b7fe5ad741af0f9f9`
- Product code delta at build: none; only bootstrap documentation/workflow files were uncommitted.
- Build engine: Visual Studio Build Tools 2022, MSBuild 17.14.40, v143 toolset.
- Windows SDK: 10.0.26100.0.
- Command: `MSBuild.exe PowerEditor\visual.net\notepadPlus.sln /m /p:Configuration=Release /p:Platform=x64 /v:minimal`
- Result: pass.
- Output: `PowerEditor\bin64\Notepad++.exe`, file/product version `8.9.7.0`, 8,485,376 bytes.
- Local qualification SHA-256: `CADA7DA86D61458F45E35F063B28742FAEE450309869AB1065C4903116749445`.
- Isolated startup smoke: pass with `-multiInst -noPlugin -nosession` and a temporary `-settingsDir`; process remained healthy for three seconds and closed normally through its main window.

The hash identifies this local compiler output only; it is not yet a reproducibility claim or distribution artifact. No shell binary is approved for publication until branding, coexistence, packaging, provenance, security, rollback, and compatibility gates pass.

## Shared-core integration follow-up

Canonical NppThemesCore split `0f272a5e82f272cf5c7bc57fc070befe5efcafea` is imported at `PowerEditor/src/NppThemesCore` and compiled directly by the upstream solution using its C++20, warnings-as-errors, nlohmann JSON, and pugixml setup. The first build identified upstream's `PUGIXML_NO_XPATH` configuration; the dependency was removed in canonical source, all plugin core tests passed, the subtree was updated, and Shell x64 Release build plus isolated startup smoke passed.

Win32 Release build and isolated startup smoke also pass with file version 8.9.7.0. ARM64 is blocked before compilation because this machine's VS 2022 v143 installation lacks the ARM64 compiler and libraries. Installing that workload can unblock cross-build; physical ARM64 Windows remains required for runtime qualification.

### Toolchain note

Visual Studio 18.7 MSBuild was detected first but lacked its native C++ `Microsoft.Cpp.Default.props`; that invocation failed before compilation. Retrying with the documented VS 2022/v143 toolchain passed. Future scripts must discover a VS 2022 instance requiring the VC tools component rather than accepting the newest MSBuild path blindly.
