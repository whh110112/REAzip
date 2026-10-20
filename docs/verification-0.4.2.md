# ZENzip 0.4.2 verification

This release adopts the A open-circle identity from PR #10. Window logos automatically consume theme colors and background artwork; a retained archive-size ratio of 60% produces a 40% opening. OS and file-association assets keep the fixed default mark. Theme authoring and measurement semantics are documented in [English](../native/docs/themes.md) and [Chinese](../native/docs/themes.zh-CN.md).

## Release gate

Publication requires a successful Native candidate run on main for exactly the checked-out source commit and version 0.4.2. All six OS/architecture manifests must identify that same clean source. Every package's size and SHA256 must match its manifest, and every uploaded asset's digest and size must match before the release becomes public. The release page records the final candidate run and source commit.

## Test and packaging scope

- Windows, macOS and Linux on x64 and ARM64: 15 CTest suites per target, isolated application/worker startup, package architecture and dependency checks.
- Rendered annulus area at 60%, unknown/empty/overhead boundaries, low-contrast and alpha fallback, all six palettes, and a real custom texture loaded from the skin directory.
- Compression ratios calculated from actual worker listings, independent of filtering or selection; real split-volume totals and sizes exceeding 32-bit limits.
- Existing 648 settings layouts and 108 archive-tab layouts per target, covering six themes and 18 interface languages; Windows capture matrix includes paired English/Chinese scenes and multiple scales.
- Installer registration/uninstall checks and isolated Linux package upgrade/removal. Existing profile, Windows vault and interrupted-journal compatibility coverage remains enabled.
- Branding guard and release-promotion rejection tests. AppImage runtime inputs use versioned official releases with verified digests.

## Evidence and limits

The final release page links the exact main candidate used for published binaries. Native captures remain CI artifacts; README previews are updated from the published build's paired capture manifest. Earlier failed development runs remain historical records and are not eligible for publication.

Visual sampling supplements automated layout and rendering checks. It does not certify every glyph or translation. Windows packages are unsigned; macOS packages are ad-hoc signed and not notarized. Modern Windows 11 menus, comprehensive cross-application drag-out improvements and physical-machine desktop coverage remain outside this release. macOS/Linux vault files use permissions and are not encrypted.
