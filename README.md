# libfprint-elan-0c5e

Community libfprint support for **ELAN USB `04f3:0c5e`, firmware `3014`**, tested on an **HP ProBook 450 G8 running Arch Linux**.

**Version 0.1.0 — first stable project release for the documented target.** This is a userspace driver adaptation built on libfprint, not an official HP or upstream libfprint release. Biometric matching runs in the sensor's firmware. No Windows driver binary is required at runtime.

[Build](docs/BUILDING.md) · [Install](docs/INSTALLATION.md) · [Test results](docs/TESTING.md) · [Protocol](docs/PROTOCOL.md) · [Security audit](docs/SECURITY-AUDIT.md)

## Compatibility

| Component | Tested configuration |
| --- | --- |
| Laptop | HP ProBook 450 G8 |
| Sensor | USB `04f3:0c5e`, firmware `3014`, 79×79 |
| System | Arch Linux x86_64 |
| Integration | libfprint 1.94.100, fprintd 1.94.5-2, KDE Plasma |
| Hardware behavior | Enrollment, positive/negative verification, cancellation recovery, exact deletion, reenrollment and reopening |
| Desktop behavior | KDE enrollment/deletion and fingerprint screen unlock after reboot/password login, reported by the hardware tester |

Check `lsusb -d 04f3:0c5e` before installing. Laptop model names alone do not establish compatibility. Other hardware/firmware, suspend/resume and Debian/Ubuntu/Fedora runtime are unverified. The source is portable Linux userspace code; the supplied package recipe is for Arch Linux only.

The default build selects only `elanmoc` and replaces the distribution's libfprint library. Other driver families are not included in that build.

## Driver behavior

- Initializes the legacy firmware after USB reset, with firmware/dimension checks.
- Reads the identity returned after a sensor match and compares the exact Linux identity with the host reference.
- Uses fprintd's host references rather than unreliable cold storage enumeration.
- Bounds commit/delete frames and validates deletion acknowledgment and active-count change.
- Aborts capture before completing cancellation and rejects malformed replies.
- Provides offline tests using the real driver and public API with synthetic USB data.

See the [implementation map](docs/IMPLEMENTATION.md) and the complete [patch against the pinned upstream base](patches/0001-elan-0c5e-fw3014.patch).

## Build and test

Install the development dependencies listed in [BUILDING.md](docs/BUILDING.md), then run:

```sh
sha256sum --check checksums.sha256
./tools/build.sh
./tools/test.sh
```

These commands use synthetic test data and do not install packages, access the physical sensor or change authentication policy. Complete tests require the documented GUsb/JSON-GLib and sanitizer dependencies.

## Arch Linux package

Generate the pinned source archive, then build as an ordinary user:

```sh
./tools/export-arch-source.py
cd packaging/arch
makepkg
```

The archive is generated locally to keep duplicate compressed source out of Git. A matching archive is also available as a release asset. The recipe builds `libfprint-elan-0c5e 1:1.94.100-4`; the project release version is `0.1.0`.

Installation and enrollment are separate steps in [INSTALLATION.md](docs/INSTALLATION.md). No prebuilt binary, PAM modification, sensor-clear operation or automatic enrollment is included.

## Validation and security

The offline audit passed 26 API scenarios, three parser tests including 300,608 deterministic mutation cases, sanitizer/leak checks with detector controls, and a Clang driver analysis pass. Reproduce full instrumentation outside a ptrace/debugger sandbox with:

```sh
./tools/audit-security.py
```

The original hardware lifecycle and later user confirmation of the updated driver are recorded separately in [TESTING.md](docs/TESTING.md). Stable denotes this project's release status for the tested target; it does not establish universal compatibility or independent security certification. The [audit report](docs/SECURITY-AUDIT.md) explains coverage and remaining limits.

## Enrollment and login

Enroll through KDE's user settings or `fprintd-enroll -f right-index-finger`. Retain your account password. The driver does not install authentication policies or clear existing sensor records.

On the tested system, initial SDDM login uses the password while KDE screen unlock uses a separate fingerprint PAM service. This is system policy, not a driver-enforced first-unlock rule. See [LOGIN.md](docs/LOGIN.md).

## Contributing

Use the issue templates for sanitized bug reports and hardware results. Do not attach private fingerprint references, identity strings or real USB recordings. See [CONTRIBUTING.md](CONTRIBUTING.md) and [SECURITY.md](SECURITY.md).

## License and attribution

Based on [libfprint](https://gitlab.freedesktop.org/libfprint/libfprint), commit `6f9479c3d55f847c1b3769f28ceb99227f9858cf`, with prior community work credited in [NOTICE.md](NOTICE.md). Original notices are preserved. Local LGPL-2.1-or-later modifications follow those terms; upstream files retain their file-level licenses. See [COPYING](COPYING).
