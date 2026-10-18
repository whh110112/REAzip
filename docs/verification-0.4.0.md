# ZENzip 0.4.0 verification

This report separates automated acceptance from interactive desktop testing. Version 0.4.0 removes the development suffix; it does not imply that every optional feature or every desktop environment has been tested.

## Release changes

- REAzip is now ZENzip: executable names, QML module, package names, desktop identifiers and visible branding are updated.
- Paper / Ink / Sand / Slate / Mist / Bamboo use restrained neutral colors and readable controls. Saved theme IDs remain compatible.
- Existing native profiles are reused in place. Windows DPAPI entropy and installer upgrade identity are deliberately preserved.
- Legacy recovery journals are accepted. Owned legacy Windows archive handlers are redirected to ZENzip without changing the user's default-app choice.

## Verified release candidate

Source checkpoint: `30726b4d67dfdb74422a44221fd379ab7bb22320`.

- [Native candidate 39125212344](https://github.com/whh110112/ZENzip/actions/runs/39125212344): Windows, macOS and Linux, each x64/ARM64, all successful; **15/15 CTest per target**, isolated packaged application/worker smoke checks and package-format aggregation.
- [Native foundation 39125212364](https://github.com/whh110112/ZENzip/actions/runs/39125212364): six native engine targets successful.
- Windows x64 and ARM64: actual installer registration under HKCU, ProgID/open-with/icons/classic verbs, opening an archive and extracting via the registered command, followed by uninstall registration cleanup. The test discovered and fixed an existing `extract_here` request that omitted `entryIds` and therefore opened the archive without extracting it.
- Linux x64: install the published REAzip 0.3.1-dev deb into an isolated dpkg root, replace it with ZENzip 0.4.0, confirm old executable/desktop entry/payload removal, run the installed application, then uninstall. Dependency checks are intentionally disabled in this isolated metadata/transition test; ordinary package smoke checks run separately.
- Legacy Windows DPAPI vaults, owned Windows handler aliases, and old interrupted archive journals have explicit regression checks. The journal check uses a real interrupted transaction before converting its name/header to the legacy representation.

The final public packages are rebuilt on `main`; their manifests identify the exact release source commit and per-file hashes. This candidate checkpoint and the README capture checkpoint are retained as traceable review evidence.

## Visual review

The capture manifest contains **86 distinct real Qt window captures**, with zero QML diagnostics, covering six palettes and paired English/Chinese archive, settings, appearance, create, extract and vault scenes; the settings matrix also covers English, Chinese, German and Arabic at 100%, 125%, 150% and 200%. A pinned Noto CJK font is loaded only by the development capture hook; it is not shipped as an application dependency.

Visually inspected: the matching English/Chinese archive pair, Chinese general settings, Ink English settings, Sand appearance, Slate Chinese create dialog, Mist Chinese extraction, Bamboo vault, Chinese 125% settings, German 150% settings, and English/Arabic 200% settings. The reviewed samples show readable Chinese, localized encryption status, inset dialog actions, the six renamed palettes, and correctly mirrored Arabic layout. A long German sidebar label uses intentional ellipsis. This is sample-based visual acceptance, not a claim that all 18 translations or every accessibility interaction received manual review.

Only the two homepage previews and their [source/hash manifest](screenshots/manifest.json) are kept in `docs/screenshots/`. The complete matrix stays in Actions artifacts and the release verification archive.

## Scope boundaries

Actual HKCU registration and registered command execution are stronger than fixture-only checks, but they are not a visual Explorer context-menu inspection. Finder/Linux desktop interactions, user-selected default handlers, cross-application drag-out, full screen-reader navigation and fresh physical-machine installs remain outside the automated gate. These are disclosed limitations rather than claimed passes.

Windows packages are unsigned. macOS packages are ad-hoc signed and not notarized. macOS/Linux password vaults use owner-only file permissions and are not encrypted.
