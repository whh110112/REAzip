ZENzip native candidate uses dynamically linked Qt 6.8.3 and the complete native 7-Zip 26.04 engine, accessed through bit7z 4.1.0. ZENzip's own source is MIT licensed; these dependencies have their own licenses.

Qt: Copyright The Qt Company Ltd. and other contributors. The Qt modules used here are distributed under their LGPL v3 option, with third-party notices and license texts in LICENSES/Qt and SBOM metadata where provided by the SDK. Qt source: https://download.qt.io/archive/qt/6.8/6.8.3/submodules/ and https://code.qt.io/cgit/qt/. Qt libraries are dynamically replaceable; no integrity check prevents interface-compatible replacements. Rebuild instructions and application source accompany the candidate.

bit7z: MPL 2.0, Copyright Riccardo Ostani and contributors. Pinned source is in sources/bit7z-source.tar.gz; no modifications to bit7z source are made.

7-Zip: Copyright Igor Pavlov, LGPL v2.1 or later, with BSD portions and the unRAR restriction. Read LICENSES/7-Zip for the exact terms; this is not an MIT-only package. The original source archive, build bootstrap and modified POSIX StringConvert.cpp accompany the candidate. Linux/macOS libraries have ZENzip's explicitly requested legacy ZIP codepage conversion patch; Windows uses the verified upstream DLL.

No executable from Bandizip or NanaZip is included. ZENzip is an independent implementation; proprietary recovery, antivirus and SFX features are not implied by use of the 7-Zip engine.

Signing: candidate Windows executables are unsigned. macOS candidates use ad-hoc signatures only, without Developer ID or notarization. Candidate artifacts have not been approved for public release. Consult acceptance.json for verification status and remaining limitations.

Replacing/rebuilding dependencies: extract the accompanying ZENzip native source, install the recorded compiler and Qt SDK, run bootstrap/dependencies.py, then cmake -S native -B build -DZENZIP_ENGINE_DIR=<absolute matching engine directory>, cmake --build build --config Release. Use the same CPU architecture and ABI. The source and bootstrap permit rebuilding with modified compatible Qt/7-Zip libraries; no DRM or anti-reverse-engineering condition is imposed.
