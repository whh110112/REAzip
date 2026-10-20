# ZENzip 0.4.4 verification

Extraction defaults to the folder containing the active archive. Settings → General offers Archive folder (the default) or Custom folder, with directory entry and a folder picker. The archive's internal navigation and the process working directory do not change this base directory. The existing smart-subfolder option still controls whether an additional archive-named subfolder is created.

The Extract to dialog refreshes its destination whenever it opens, including after switching archive tabs or after an earlier one-off destination edit. One-off dialog edits do not change the saved default. Switching back to Archive folder preserves the remembered custom path.

## Migration and fallback

- New profiles use Archive folder.
- Older profiles with the automatically supplied Downloads default use Archive folder after upgrade.
- Older profiles with a different explicit directory keep Custom folder. Explicit new-mode choices, including a custom Downloads directory, survive restarts.
- A missing custom directory falls back to the active archive's folder; the remembered path is retained for correction in Settings.

## Acceptance scope

- Settings validation, old-profile migration, invalid-mode rejection and persisted mode/path.
- Real GUI with archives in two different directories: initial default, tab switching, reopening after a one-off edit, custom override, restart and restoring archive-relative behavior.
- Existing six-theme / 18-language settings layouts, conflict and tab layouts, worker/core suites and isolated packages.

Publication requires successful acceptance on the exact main source across Windows, macOS and Linux, x64 and ARM64, followed by manifest, package size and SHA256 validation. Final source/run/release evidence is recorded after completion.
