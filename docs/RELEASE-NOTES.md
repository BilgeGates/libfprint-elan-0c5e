# 0.1.0 — first stable release

Released 2026-10-28 for the documented ELAN 04f3:0c5e firmware 3014 target, tested on HP ProBook 450 G8 / Arch Linux x86_64.

This community libfprint adaptation supports legacy initialization, nine-stage enrollment, sensor matching with exact Linux identity comparison, cancellation and identity-specific deletion. The original hardware lifecycle covered positive and negative verification, deletion while retaining another record, reenrollment and reopening. KDE integration and fingerprint screen unlock after reboot/password login were reported working. The hardware tester subsequently confirmed the updated driver works before authorizing this stable release.

Parser bounds/metadata checks, close-error ownership and READY boundary handling were hardened during the local audit. Offline validation includes 26 API scenarios, three parser tests including 300,608 mutations, 100 extra enroll/delete cycles, 1,000 verification operations, ASan/UBSan/LSan controls and Clang analysis. See [SECURITY-AUDIT.md](SECURITY-AUDIT.md).

The release includes full source, the pinned-base patch, a generated Arch source snapshot, English documentation and report templates. Arch recipe version: 1:1.94.100-4; project version: 0.1.0. No private prints, vendor binaries, decompiled routines or prebuilt package are distributed.

The conservative two-second reset recovery delay remains. Suspend/resume, other hardware/firmware and non-Arch runtime are unverified. This is a community stable release for the tested target, not an official HP/upstream release or independent security certification.
