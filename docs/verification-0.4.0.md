# ZENzip 0.4.0 verification

This report separates automated acceptance from interactive desktop testing. Version 0.4.0 removes the development suffix; it does not imply that every optional feature or every desktop environment has been tested.

## Release changes

- REAzip is now ZENzip: executable names, QML module, package names, desktop identifiers and visible branding are updated.
- Paper / Ink / Sand / Slate / Mist / Bamboo use restrained neutral colors and readable controls. Saved theme IDs remain compatible.
- Existing native profiles are reused in place. Windows DPAPI entropy and installer upgrade identity are deliberately preserved.
- Legacy recovery journals are accepted. Owned legacy Windows archive handlers are redirected to ZENzip without changing the user's default-app choice.

## Evidence being collected

Six-target CTest and isolated package smoke checks, paired English/Chinese archive captures, theme/settings/scale captures, actual Windows installed registry and shell-command roundtrips, uninstall cleanup, and legacy vault/journal compatibility tests. Final run IDs and reviewed screenshot details will be recorded before publishing.

## Scope boundaries

Actual HKCU registration and registered command execution are stronger than fixture-only checks, but they are not a visual Explorer context-menu inspection. Finder/Linux desktop interactions, user-selected default handlers, cross-application drag-out, full screen-reader navigation and fresh physical-machine installs remain outside the automated gate. These are disclosed limitations rather than claimed passes.

Windows packages are unsigned. macOS packages are ad-hoc signed and not notarized. macOS/Linux password vaults use owner-only file permissions and are not encrypted.
