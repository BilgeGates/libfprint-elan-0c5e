# Verification record

## Automated checks

| Check | Result on the original development system |
| --- | --- |
| Full libfprint/ELAN build | Passed |
| Real driver + public API | Baseline 8; Audited source 26 scenarios with ASan/UBSan/LSan |
| Audited source actual parser mutation tests | 300608 cases + 2 explicit regressions |
| Audited source repeated API allocation stress | 100 enroll/delete cycles + 1000 verifies, no reported leak |
| Audited source leak-detector controls | Intentional 37-byte leak detected; clean control passed |
| Audited source Clang driver static analysis | No diagnostics with core/unix/security checkers |
| COUNT/FW/DIM callback boundaries | 78,609 cases passed |
| Identity-frame bounds/opcode allowlist | 798 cases passed |
| Cancellation/abort helper | 6 scenarios passed |
| Saved empty-state validator | 8 scenarios passed |
| Existing fpi-device / fpi-ssm / fpi-assembling | 3 suites passed |
| Arch makepkg build/check/package | Revision 4 build/check/package passed |

Baseline API scenarios: normal 71-byte INFO, 64-byte INFO, rejected INFO, short INFO, refused commit, short commit OUT, refused delete, and acknowledged deletion with unchanged COUNT. These run the real driver and public API. A synthetic GUsb context is constructed without physical initialization; a libusb initialization attempt is a hard test failure. No personal fingerprint input is used. Ordinary tools/test.sh keeps leak detection disabled for ptrace-restricted wrappers. Audited source tools/audit-security.py separately runs full instrumentation and LSan outside those restrictions, after positive/negative detector controls. See SECURITY-AUDIT.md.

Focused callback tests extract the actual callback definitions from the maintained source before compiling their fixtures. They complement, rather than replace, full API and hardware tests.

## User-run hardware lifecycle

HP ProBook 450 G8, 04f3:0c5e, firmware 3014, 79×79, Arch x86_64:

1. Existing right-index sensor match and exact host UID match.
2. Unregistered left-little finger → NO MATCH.
3. No-finger verification cancelled after three seconds.
4. Right-index matching immediately after cancellation.
5. Nine-stage right-middle enrollment and matching.
6. Exact first/right-index deletion; active COUNT 2 → 1.
7. Retained middle finger still matches at slot 1.
8. Deleted right-index finger → NO MATCH.
9. Right-index reenrollment; matching at slot 0.
10. Three close/open cycles.
11. Both exact identities match after reopen; independent final COUNT 2.

The cycle finished successfully without an exit crash. Detailed personal logs are not included in this bundle.

## System integration

The user installed `libfprint-elan-0c5e 1:1.94.100-2`. fprintd `1.94.5-2` linked `/usr/lib/libfprint-2.so.2`, listed right-index and right-middle, and returned `verify-match (done)` for right-index.

## Later desktop observations (user-reported)

On 2026-10-27 the user reported successful deletion and fresh enrollment through KDE, followed by working fingerprint authentication. The user also reported a reboot, first login with a password, then a successful fingerprint screen unlock. These are user observations rather than a new instrumented hardware test by this source-preparation step.

Read-only inspection found no pam_fprintd entry on the SDDM login chain, while KDE's separate kde-fingerprint PAM service loads it. See LOGIN.md. The driver does not implement an Android-style mandatory password on the first unlock.

## Release source and hardware confirmation

Version 0.1.0 includes the hardening fixes in SECURITY-AUDIT.md and recipe revision 4. Earlier instrumented physical lifecycle results used revision 2. On 2026-10-28, after receiving the revision-4 installation and reboot-test instructions, the hardware tester confirmed the driver works and authorized a stable release. No new instrumented transcript of that confirmation was supplied. It does not establish that every earlier lifecycle/fault-injection test was repeated on revision 4. The two-second reset delay is not a measured minimum.

## Package validation

The earlier revision-3 recipe completed on the development host with a source-path warning. Audited source's revision-4 makepkg build/check/package passed: three core suites, 26 API cases and three parser tests. The source-build-path warning remains; no prebuilt binary is distributed. No clean-chroot or namcap result is claimed and no prebuilt package is distributed.

## Not established

Suspend/resume, repeated reboot stress testing, other machines, other firmware and Debian builds are not established. Neither a clean-chroot package build nor an independent security review has been completed; the local agent-performed source audit is documented separately. Existing upstream USB replay tests requiring introspection/umockdev were not counted as passed. The single-driver hwdb comparison test was skipped; its generator ran.

Run `./tools/test.sh` to reproduce the portable offline checks. The source export tools verify the reviewed manifest before creating release artifacts.
