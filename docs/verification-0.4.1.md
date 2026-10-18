# ZENzip 0.4.1 verification

Stable maintenance release containing the localized settings fixes from PR #3 and archive-tab alignment fixes from PR #4.

## Release gate

The release workflow promotes artifacts only from a successful Native candidate run on main whose source commit matches the checked-out release sources. All six OS/architecture manifests must name version 0.4.1 and the same clean source commit. Every package is checked against its manifest SHA-256 before upload, and every uploaded asset is checked against GitHub's reported digest before publication. The release page links the exact candidate run and source commit.

## Published build evidence

- Source: `e9d7c405f44f95c62838db2e21b3f4f27fbf58c9`.
- [Native candidate 39143335032](https://github.com/whh110112/ZENzip/actions/runs/39143335032): six targets passed, 15/15 CTest per target, packaging and aggregate verification passed.
- The earlier [Native foundation run](https://github.com/whh110112/ZENzip/actions/runs/39140865402) passed all six targets on the identical native source tree; the later source checkpoint changes only CI collection/concurrency metadata.
- The paired README previews were copied byte-for-byte from this compiled Windows build and verified against its capture manifest. Thirty dedicated native tab captures were also retrieved; Chinese, Arabic Ink and Hindi Slate samples were manually reviewed.
- Documentation and release-tool follow-up leaves the published application/runtime sources unchanged.

## Regression and packaging scope

- Windows, macOS and Linux, each x64 and ARM64: 15 CTest suites, architecture checks and isolated application/worker startup.
- Per target: 648 settings layouts (six themes × eighteen languages × six pages) and 108 tab layouts (six themes × eighteen languages), with real mixed-script filenames, long-name ellipsis and tab selection/close clicks.
- Windows installer registration, installed shell-command execution and uninstall cleanup; Linux package upgrade/removal in an isolated dpkg root.
- Existing profile loading, encrypted Windows vault compatibility and interrupted-journal recovery retain explicit regression coverage.
- Branding guard: current tracked presentation text and filenames have no previous product name. A reviewed, count-bounded compatibility inventory covers only old identifiers required to read existing data, upgrade installations and test migration.

PR #4's native sources previously passed [candidate 39137222513](https://github.com/whh110112/ZENzip/actions/runs/39137222513), including all six targets. This is historical evidence for the tab fix; the final 0.4.1 packages require their own successful candidate run.

## Visual evidence and limits

The workflow captures the actual compiled Windows application in paired English/Chinese scenes and the theme/language/scale matrix. Dedicated tab captures are collected from CTest's working directory. Full matrices stay in build artifacts; the README retains one preview per language.

The earlier local Qt 6.8.3 probe passed 432 tab layouts at 100/125/150/200% scaling with a mock backend. It supplements native regression checks and does not certify every glyph or translation. Actual Windows English/Chinese tab captures were manually sampled.

Modern Windows 11 menus, comprehensive cross-application drag-out, physical-machine desktop coverage, screen-reader navigation, high-contrast support and native-language review remain outside this release gate. Windows packages are unsigned; macOS packages are ad-hoc signed and not notarized. macOS/Linux password vaults use file permissions and are not encrypted.
