# ZENzip 0.4.4 verification

Extraction defaults to the folder containing the active archive. Settings → General offers Archive folder (the default) or Custom folder, with directory entry and a folder picker. The archive's internal navigation and the process working directory do not change this base directory. The existing smart-subfolder option still controls whether an additional archive-named subfolder is created.

The Extract to dialog refreshes its destination whenever it opens, including after switching archive tabs or after an earlier one-off destination edit. One-off dialog edits do not change the saved default. Switching back to Archive folder preserves the remembered custom path.

## Migration and fallback

- New profiles use Archive folder.
- Older profiles with the automatically supplied Downloads default use Archive folder after upgrade.
- Older profiles with a different explicit directory keep Custom folder. Explicit new-mode choices, including a custom Downloads directory, survive restarts.
- A missing custom directory falls back to the active archive's folder; the remembered path is retained for correction in Settings.

## Published build evidence

- Source: `3490a7b0197cdd54887b2b792e590fd8b44e9927`.
- [Main candidate 39384876297](https://github.com/whh110112/ZENzip/actions/runs/39384876297): all six targets and aggregate passed, including all 16 CTest suites per target and isolated offline packages.
- [Stable publication 39385944337](https://github.com/whh110112/ZENzip/actions/runs/39385944337): [ZENzip 0.4.4](https://github.com/whh110112/ZENzip/releases/tag/v0.4.4), with 21 assets verified against package manifests and SHA256SUMS.
- README previews are the matched English/Chinese captures from Windows artifact `12247546517`; their hashes are recorded in `docs/screenshots/manifest.json`.
- Both the development candidate `39383487190` and main acceptance passed on their first attempts. The exact main Windows x64 portable package opened an archive from a separate directory and displayed that directory in Extract to, with exit 0 and no diagnostics.
- After publication, the previous 0.4.3 Windows portable package successfully discovered 0.4.4 through its real HTTPS update check and displayed the new-version/download controls. No installer was downloaded or launched by the check.
- The follow-up changes documentation only; native sources remain identical to the published tag.

## Acceptance scope

- Settings validation, old-profile migration, invalid-mode rejection and persisted mode/path.
- Real GUI with archives in two different directories: initial default, tab switching, reopening after a one-off edit, custom override, restart and restoring archive-relative behavior.
- Existing six-theme / 18-language settings layouts, conflict and tab layouts, worker/core suites and isolated packages.

Publication requires successful acceptance on the exact main source across Windows, macOS and Linux, x64 and ARM64, followed by manifest, package size and SHA256 validation. Final source/run/release evidence is recorded after completion.
