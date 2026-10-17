# Native platform/toolchain lock

Qt 6.8.3 open-source dynamically deployed; C++17. Windows local: MinGW 13.1.0; Windows CI: native MSVC; Linux: Ubuntu 24.04 gcc; macOS: macOS 15 clang. Qt package architectures verified via aqtinstall 3.3.0: Windows win64_mingw / win64_msvc2022_64 / win64_msvc2022_arm64_cross_compiled, linux linux_gcc_64, linux_arm64 linux_gcc_arm64, mac clang_64 (universal SDK).

7-Zip 26.04 full shared libraries: official Windows architecture-specific 7z.dll extracted from hash-verified official installers; Linux/macOS 7z.so compiled from hash-verified official source `CPP/7zip/Bundles/Format7zF/makefile.gcc`. macOS uses clang/clang++, output is Mach-O with .so suffix; suffix does not determine architecture.

bit7z 4.1.0 commit c81c6c1cbf44e148cd4b06f4bb69d7ea1e299742 compiles against those same 26.04 headers, using native strings and automatic format detection. Full licenses/source locations must accompany packaging in T09.

Foundation runner labels: windows-2022, windows-11-arm, ubuntu-24.04, ubuntu-24.04-arm, macos-15-intel, macos-15. CI executes each architecture natively; it does not just cross-compile. This does not prove GUI, installation or system integration support. Minimum end-user OS versions remain pending deployed Qt/library dependency checks in T09; runner OS is not a minimum-OS claim.
