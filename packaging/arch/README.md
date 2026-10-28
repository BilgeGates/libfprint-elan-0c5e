# Arch source recipe

This local-source PKGBUILD and `.SRCINFO` build `libfprint-elan-0c5e 1:1.94.100-4` with only the elanmoc driver. Generate the matching archive from the repository root:

```sh
./tools/export-arch-source.py
cd packaging/arch
makepkg
```

Alternatively, place the matching `libfprint-elan-source.tar.gz` release asset in this directory. Its SHA256 must match PKGBUILD. Run makepkg as an ordinary user with the declared dependencies installed.

`check()` runs three core suites, 26 synthetic API scenarios and three parser tests without opening a physical sensor. There are no PAM hooks, template operations or automatic installation steps. See [installation](../../docs/INSTALLATION.md).

This recipe is not an AUR submission. AUR publication requires a real hosted source URL, a maintainer identity, duplicate-package review and clean-chroot validation. Never commit source archives or built packages to an AUR Git repository.

After intentional source edits, regenerate the archive, update its checksum in PKGBUILD and run `makepkg --printsrcinfo > .SRCINFO`.
