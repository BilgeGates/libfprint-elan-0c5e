# Building

The bundled source is self-contained; no clone/download step is needed. `tools/build.sh` configures a separate `build/` directory and compiles only the elanmoc driver with docs, introspection, GTK examples, installed tests and udev installation disabled. It does not install anything system-wide.

Required tools: C and C++ compilers, Python 3, Meson >= 0.62, Ninja, pkg-config, GLib/GIO/GObject/GModule >= 2.68, GUsb development files and libusb headers. Complete API emulation additionally needs GUsb >= 0.4.0, JSON-GLib and compiler/linker support for ASan/UBSan. Cairo is used by an existing core unit suite when available.

```sh
./tools/build.sh
./tools/test.sh
```

The runtime library is `build/libfprint/libfprint-2.so.2`. The command does not use TOD. Runtime libraries are not linked to the test executable's sanitizer runtime.

## Arch package reproduction

`packaging/arch/PKGBUILD` builds a deterministic source snapshot generated from this repository:

```sh
./tools/export-arch-source.py
cd packaging/arch
makepkg
```

Base-devel is assumed by Arch makepkg; additional build dependencies are declared in PKGBUILD. No package installation or network dependency installation is automatic. `check()` runs the three core suites, 26 API scenarios and three parser tests.

The pinned archive is generated from the same bundled libfprint source used by tools/test.sh. It includes the three restored upstream ID entries and dependency-checked API-test registration. The original hardware lifecycle used revision 2; the audited recipe is revision 4, later reported working by the hardware tester. See RELEASE-NOTES.md. tools/export-arch-source.py regenerates the archive deterministically; after source changes, update the PKGBUILD hash and .SRCINFO before building.

## Other distributions

Use the equivalent development packages and the same Meson source build. No Debian binary/package is supplied and no Debian runtime result is claimed. Do not install the supplied Arch package on Debian/Ubuntu.

## Release exports

`tools/export-arch-source.py` regenerates the pinned Arch source archive from the maintained source files. `tools/make-source-release.py OUTPUT_DIRECTORY` creates a deterministic source tarball, standalone patch, pinned Arch source archive and SHA256SUMS from the checksummed manifest. It rejects changed/missing files and never invokes git, a network operation, privileged installation or hardware access. No signing key is created or used.

## Full instrumentation and leak audit

Run `./tools/audit-security.py` outside ptrace/debugger restrictions. It builds the whole audit library with ASan/UBSan and runs LeakSanitizer controls plus all 29 synthetic API/parser tests. It fails if leak detection is unavailable and does not install packages or access real USB. See SECURITY-AUDIT.md for scope and limits.
