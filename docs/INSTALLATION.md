# Installation and use

This is a source release for one tested sensor/firmware. Verify `lsusb -d 04f3:0c5e` and read [compatibility](../README.md) first. A shared laptop model name is not enough. The library is built with only the `elanmoc` driver, so replacing the normal package also removes other compiled driver families.

## Arch Linux

Review `packaging/arch/PKGBUILD`. With its dependencies installed, build as an ordinary user:

```sh
./tools/export-arch-source.py
cd packaging/arch
makepkg
```

Building does not install the package. After a successful build/check, close fingerprint settings and any active fingerprint operations. If you decide to install, use an interactive terminal:

```sh
sudo systemctl stop fprintd.service
sudo pacman -U './libfprint-elan-0c5e-1:1.94.100-4-x86_64.pkg.tar.zst'
sudo systemctl start fprintd.service
```

Restart fprintd even if pacman reports an error, so the existing installed library can be used. Pacman resolves the library conflict interactively. The revision-4 source includes the audited hardening fixes and was subsequently reported working by the hardware tester. The detailed original lifecycle used revision 2; see [TESTING.md](TESTING.md).

No PAM hook, sensor erase, host-reference import or automatic enrollment is included. This is a local source recipe, not an already-published AUR package.

## Enroll and verify

Use KDE's fingerprint settings or run as your account:

```sh
fprintd-enroll -f right-index-finger
fprintd-list "$(id -un)"
fprintd-verify -f right-index-finger "$(id -un)"
```

A successful verification ends with `verify-match (done)`. Lift and reposition the same finger between enrollment stages. Keep the account password available.

Existing Windows records or Linux records whose host references were lost are not automatically imported. The legacy driver cannot enumerate those records reliably. No wipe or recovery fallback is provided.

## Initial login versus lock screen

Enrolling a finger does not enable every authentication service. The tested machine's initial SDDM login is password-based; KDE screen unlock uses its separate fingerprint service. See [LOGIN.md](LOGIN.md). Authentication policy is outside this driver's installation.

## Rollback and error handling

To return to Arch's normal library, reinstall the distribution's `libfprint` package interactively with pacman and restart fprintd after closing fingerprint clients. Pacman should resolve the conflicting local package. This does not restore deleted sensor templates.

If an enrollment/delete fails, stop at the error and report sanitized details. Do not erase all on-chip records as a retry strategy. Host identity files are not restorable biometric templates.

## Other distributions

Source compilation is documented in [BUILDING.md](BUILDING.md), but Debian/Ubuntu/Fedora package installation and runtime behavior have not been validated. Do not install an Arch package on those distributions or copy a newly built shared library over managed system files.
