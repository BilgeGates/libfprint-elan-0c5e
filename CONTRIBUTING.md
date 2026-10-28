# Contributing

Read [libfprint/HACKING.md](libfprint/HACKING.md) and the [implementation map](docs/IMPLEMENTATION.md) before changing the driver.

Follow GLib ownership/error conventions, the existing two-space GNU indentation and [uncrustify configuration](libfprint/scripts/uncrustify.cfg). Keep functional changes focused. Document protocol observations separately from assumptions and hardware results. This adaptation adds no public application API.

## Review requirements

- Require sensor matching and exact host identity equality for authentication.
- Treat unavailable INFO as an error, never as a successful empty inventory.
- Treat COUNT as a quantity, not the maximum slot number.
- Keep deletion identity-specific; do not add clear-all recovery.
- Preserve ownership across async operations and floating-reference boundaries.
- Document packet sizes, endpoints, cancellation behavior and bounds before changing a command.
- Run `./tools/test.sh`; run `./tools/audit-security.py` for parser/ownership changes.
- Report physical tests separately from synthetic tests and identify the USB ID and firmware.

New device IDs require independent hardware evidence. A shared laptop family does not establish protocol compatibility.

## Licensing and reports

Preserve upstream notices and the pinned base. Do not include proprietary decompiled routines, vendor binaries or personal biometric data. Submit sanitized summaries using the issue/PR templates; read [SECURITY.md](SECURITY.md) for sensitive reports.

After source changes, regenerate the pinned Arch archive with `./tools/export-arch-source.py`, update its PKGBUILD/.SRCINFO hash, and review the consolidated upstream patch. Release maintainers must regenerate the root checksum manifest after reviewing the exact source tree.
