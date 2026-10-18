# REAzip

[中文](README.zh-CN.md) | English

A cross-platform archive manager with a native 7-Zip core and a tabbed, themeable Qt Quick interface. REAzip pairs the proven 7-Zip 26.04 engine with the everyday workflow Bandizip popularized — tabs, queues, smart extraction and per-entry conflict control — while keeping the whole feature surface honest: nothing in the UI is a fake button.

> **Status: 0.3.0-dev pre-release.** Six native targets build and pass 14/14 tests each on the same commit; the [acceptance report](native/docs/acceptance-report.md) states exactly what is verified and what is not. v0.2.1 is the previous Electron build.

| Main window (light) | Create archive | Extract |
|---|---|---|
| ![Main window](native/design/native-reference/light-en-archive.png) | ![Create](native/design/native-reference/light-en-create.png) | ![Extract](native/design/native-reference/light-en-extract.png) |

| Password challenge | Name conflict | Settings |
|---|---|---|
| ![Password](native/design/native-reference/light-en-password.png) | ![Conflict](native/design/native-reference/light-en-conflict.png) | ![Settings](native/design/native-reference/light-de-settings.png) |

| Glass theme | macOS dark theme | Material theme | Sleep theme |
|---|---|---|---|
| ![Glass](native/design/native-reference/glass-en-archive.png) | ![macOS dark](native/design/native-reference/mac-dark-en-archive.png) | ![Material](native/design/native-reference/material-en-archive.png) | ![Sleep](native/design/native-reference/sleep-en-archive.png) |

RTL (Arabic) and Simplified Chinese previews: ![RTL](native/design/native-reference/light-ar-archive.png) ![zh-CN](native/design/native-reference/light-zh-CN-empty.png)

<sub>All screenshots are real captures of the 0.3.0-dev candidate running at 0.5 scale; each file is pinned by SHA-256 in [`native/design/native-reference/manifest.json`](native/design/native-reference/manifest.json). They were taken before the settings-layout fix in this change set, so the Settings/Appearance shots still show the old footer alignment and will be recaptured once the next build is green.</sub>

## Highlights

- **Native engine, no runtime baggage** — C++17 + Qt Quick 6.8.3 + bit7z 4.1.0 + the complete 7-Zip 26.04 library. No Electron, no Node, no bundled browser. The x64 installer is ~26 MB versus ~121 MB for v0.2.1.
- **Tabs, queues, cancellation** — multi-tab browsing, a background task queue with real cancellation, and a worker process that keeps the GUI alive when an archive misbehaves.
- **Format coverage** — create/list/test/extract for ZIP, 7Z, TAR and TAR.GZ; read/extract for RAR, RAR5, ISO and WIM; ZIP AES-256 and legacy ZipCrypto; 7z header encryption; split volumes; ZIP64 above 4 GiB.
- **Legacy filename encodings** — explicit GBK / Big5 / Shift-JIS decoding for old ZIP files, with an automatic policy.
- **Safe by construction** — path-escape, symlink and reparse-point attacks are blocked during extraction; every write lands through a committed temporary file with journal recovery, so a crash or a full disk never corrupts the original archive.
- **Six themes, 18 languages, full RTL** — with conservative migration of old custom color tokens.
- **Password vault** — save passwords once and pick them while creating or extracting. On Windows the vault is encrypted with DPAPI (tied to your Windows account); on macOS/Linux it is protected by owner-only file permissions and the UI says exactly that instead of implying encryption. A damaged or foreign vault is preserved, never silently replaced.
- **Desktop integration where the OS allows it** — Windows: per-user ProgID, Default Programs entry, "Open with" list and classic context-menu verbs (extract here / extract to / add to archive) with icons and multi-select over 22 extensions. macOS: UTI + `CFBundleTypeExtensions` declarations in the bundle. Linux: desktop entry with 15 MIME types, desktop actions, `update-desktop-database` and `xdg-mime default`. The system default application always stays under your control, and the settings page shows the actual registration state.

## Feature comparison

Comparison is limited to publicly documented capabilities and REAzip's own acceptance scope; “–” means not implemented in this version.

| Capability | REAzip 0.3.1-dev | 7-Zip | NanaZip | Bandizip |
|---|---|---|---|---|
| ZIP / 7Z / TAR / GZ create + extract | ✓ | ✓ | ✓ | ✓ |
| RAR / RAR5 / ISO / WIM extract | ✓ | ✓ | ✓ | ✓ |
| ZIP AES-256 write, 7z header encryption | ✓ | ✓ | ✓ | ✓ |
| Split volumes, ZIP64 > 4 GiB | ✓ | ✓ | ✓ | ✓ |
| Legacy ZIP code pages (GBK/Big5/Shift-JIS) | ✓ | ✓ | ✓ | ✓ |
| Tabbed browsing | ✓ | – | – | ✓ |
| Task queue with cancellation | ✓ | – | – | ✓ |
| Saved password vault (DPAPI on Windows) | ✓ | – | – | ✓ |
| Classic context menu: extract here / add | ✓ | ✓ | ✓ | ✓ |
| Theme / 18 languages / RTL | ✓ | – | partial | partial |
| Per-user install without admin | ✓ | ✓ | ✓ (MSIX) | ✓ |
| Windows 11 modern context menu | – | – | ✓ | ✓ |
| SFX creation, password recovery, archive repair | – | – | – | ✓ |

## Downloads

The pre-release [`v0.3.1-dev`](https://github.com/whh110112/REAzip/releases/tag/v0.3.1-dev) carries the six-target builds: Windows x64/ARM64 installer + portable ZIP, macOS x64/ARM64 DMG + ZIP, Linux x64/ARM64 AppImage + deb + tar.gz. Every package ships with its version, source SHA, architecture, per-binary SHA-256, signature status and license notices; `SHA256SUMS.txt` covers the release assets.

It is flagged **pre-release** on purpose: the automated six-target gate builds and tests every target, but the manual acceptance items listed in the [acceptance report](native/docs/acceptance-report.md) (side-by-side visual review, real desktop association and menus, drag-out, large-archive stress) have not been run yet.

- Windows binaries are **unsigned**; macOS builds are ad-hoc signed and **not notarized**.
- Linux `deb` requires glibc ≥ 2.39 (built on Ubuntu 24.04); macOS deployment target is 13.0.
- Installation, portable mode and old-setting migration are described in the [installation guide](native/docs/installation-guide.md).

## Building from source

Requirements: Python 3.12+, a C++17 toolchain, CMake and Ninja. Dependency versions and hashes are pinned in `native/deps.lock.json`; the CI workflows are the authoritative recipe.

```sh
python -m pip install aqtinstall==3.3.0 ninja==1.11.1.4 cmake==3.31.6
python native/bootstrap/dependencies.py --arch x64
python native/bootstrap/qt-sdk.py --arch x64 --output .native-build/Qt
# add the generated Qt SDK to PATH / CMAKE_PREFIX_PATH, then:
cmake -S native -B .native-build/release -DREAZIP_ENGINE_DIR=/absolute/path/to/native/.deps/engine/x64 -DREAZIP_HEAVY_TESTS=ON
cmake --build .native-build/release --config Release --parallel 3
ctest --test-dir .native-build/release -C Release --output-on-failure
```

Replace `x64` with `arm64` for ARM targets (Windows ARM additionally needs `QT_HOST_PATH`). First setup downloads Qt and compiles the full 7-Zip core; the released packages run fully offline.

## What is deliberately not in this version

Win11 modern context menus, Finder/Nautilus extensions, SFX creation, password recovery, complex archive repair, a password manager, image preview and an auto-updater are **not implemented** and are never shown as clickable stubs. RAR writing, TAR/TGZ encryption and TGZ volumes are explicitly rejected with an explanation rather than silently ignored. The full boundary is documented in [known limitations](native/docs/known-limitations.md) and the [feature matrix](native/docs/feature-parity.md).

## License

REAzip's own code is MIT. Distributed packages also contain Qt (LGPL), bit7z (MPL) and 7-Zip (LGPL, unRAR restriction) — the bundles as a whole are **not** “just MIT”. Third-party notices and corresponding source links ship with every package; see [packaging NOTICE](native/packaging/NOTICE.md). Bandizip is a trademark of its respective owner; this project is not affiliated with it.
