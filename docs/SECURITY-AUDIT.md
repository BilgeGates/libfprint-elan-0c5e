# Local source security audit — 2026-10-28

Scope: the bundled ELAN driver, matched identity parsing, enrollment/deletion framing, async ownership/cancellation/close paths, synthetic test fixtures and source-delivery tools. Performed by the coding agent locally at the user's request. This is not an independent security assessment, penetration-test certificate, CVE assignment or a claim that every vulnerability is absent.

The real sensor was not exercised by this offline audit. The audited source builds Arch recipe `1:1.94.100-4`. Earlier instrumented physical lifecycle results used revision 2; the hardware tester later confirmed the updated driver works before the 0.1.0 stable release. That confirmation is separate from the offline audit.

## Trust model

- Biometric comparison takes place in the real sensor firmware; libfprint must compare the actual matched Linux identity with the requested host reference.
- USB replies and stored metadata are treated as untrusted for bounds/type/format handling.
- A compromised host/root account or malicious replacement sensor is outside the authentication guarantee. The legacy protocol does not provide cryptographic sensor attestation or make its FP1 identity a secret. This audit does not measure firmware liveness/spoof resistance.
- No vendor DLL, decompiled proprietary function, private print reference or actual biometric recording is included in the source release.

## Reproduced findings and fixes

| Finding | Evidence before fix | Change and result |
| --- | --- | --- |
| Close-error cleanup passed an already populated GError as an output parameter | Injected abort/close OUT failure caused a libgusb critical and exit 134 under fatal-warnings | Release into a separate owned GError; preserve the original transport error; clear task state. The same fault now returns the expected error without a critical or reported leak |
| Modern ELAN identity parser read fields before checking the supplied length | ASan reported a one-byte heap-buffer-overflow for an exact-sized one-byte `43` frame | Check minimum size before reads; check listing frame length too. This parser is the non-0c5e branch; the legacy parser already checked its 64/71-byte frame sizes |
| Malformed FP1 metadata could enter the core date parser | A synthetic invalid calendar date triggered a GLib critical under fatal-warnings in the modern path | Share bounded FP1 field validation across legacy and modern Linux identity parsing; reject invalid dates, finger/random fields, embedded NUL and invalid UTF-8 before metadata conversion |
| READY succeeded on the last permitted response but was rejected because retry count was zero | A synthetic sequence exhausted all busy replies and then returned READY; old open asserted failure | Decide success from the READY byte, not the remaining retry count. Last-attempt READY passes |
| Test fixture retained the synthetic GUsb object graph | LSan reported 925 bytes in 19 allocations on the original API fixture | Own the returned device array and explicitly dispose the synthetic context, matching upstream FpContext's cleanup pattern. This finding concerns the fixture that bypassed FpContext, not proof of a per-login production leak |

Normal matching still requires both a sensor match and exact host identity equality. Valid-but-different FP1 identities and foreign records were tested as negative results. No wipe-all, cold enumeration, authentication shortcut or recovery fallback was added.

## Validation

- The leak detector was checked with an intentional 37-byte leak (detected with the expected failing exit) and a clean control (exit 0). Its ptrace failure was distinguished from a real leak report. No leak suppressions were used.
- The entire audit library build used ASan/UBSan, in addition to instrumented real-driver test executables.
- 26 full public-API synthetic scenarios passed with ASan/UBSan/LSan and fatal GLib warnings. These include wrong firmware/dimensions, short mode replies, invalid counts, retry boundary, oversized host identity, different/foreign identities, malformed identity fields, repeated empty INFO, failed abort, truncated delete OUT, genuine GCancellable cancellation and failed close OUT.
- Stress includes 100 extra enroll/delete cycles and 1,000 matching operations in one process, followed by ordinary finalization and leak detection.
- Actual legacy and modern parser functions received 300,608 deterministic mutation cases with exact-sized allocations: length/fill patterns, every byte value at every position of accepted legacy frame lengths, and fixed-seed mutations around well-formed legacy/modern records. Two additional regressions cover the original short frame and invalid modern date.
- Clang 23.1.4 static analysis (core/unix/security checkers) of the driver translation unit reported no diagnostics. This does not imply whole-program coverage.
- Read-only inspection of the normal Arch package ELF found a non-executable stack, GNU_RELRO with BIND_NOW, stack-protector import and no RPATH/RUNPATH/TEXTREL. The normal runtime library does not depend on ASan/UBSan or TOD. These mitigations are not a security proof.
- Standard focused tests, three upstream core suites, source packaging and pinned-base patch reproduction are summarized in [TESTING.md](TESTING.md).

## Why there is a two-second wait

`ELAN_LEGACY_RESET_DELAY_MS` is a conservative initialization delay after USB reset, before claiming/initializing firmware 3014. It is asynchronous, using the libfprint timeout mechanism rather than a blocking sleep. It applies at each device open, not to every scan on an already open device.

It is not a security/password rule, not a documented minimum from the vendor, and not a measured optimum. Historical successful bring-up also used a 1.5-second wait. The later two-second value left a margin; it should not be described as hardware-mandated. No physical reset-timing sweep was performed during this offline audit, so it remains unchanged.

Synthetic ordinary opens measured approximately 2.05 seconds, dominated by that explicit wait. Those numbers are emulator/process timings, not real fingerprint-recognition latency. The existing READY retry count is bounded (500 retries) but is not an overall startup wall-clock deadline; normal USB command timeout is five seconds and capture reads wait for caller cancellation. Worst-case physical startup/retry/suspend timing remains unmeasured.

## Remaining limits

No physical operations were performed during this audit. Later hardware confirmation does not repeat every injected fault case. Suspend/resume, reset timing minimization, repeated real lifecycle stress, other hardware/firmware and non-Arch runtime remain unverified. This audit does not cover every libfprint driver, every third-party dependency/CVE, the operating system's PAM policy, malicious sensor firmware, biometric false-accept rates or liveness attacks. Sanitizers observe executed cases, and LSan does not prove the absence of every possible lifetime/resource leak.

## Reproduce

Run `./tools/test.sh` for the ordinary offline checks. Run `./tools/audit-security.py` in an environment where LSan works for full instrumentation, leak controls and all 29 API/parser cases. It uses synthetic USB only and blocks physical libusb initialization in the tests; it does not require root, install packages or change PAM/templates. If the leak control fails, the audit stops rather than disabling leak detection silently.

Source references: [GUsb context disposal](https://github.com/hughsie/libgusb/blob/main/gusb/gusb-context.c), [AddressSanitizer](https://clang.llvm.org/docs/AddressSanitizer.html), [LeakSanitizer](https://clang.llvm.org/docs/LeakSanitizer.html). The bundled libfprint `fp-context.c` already uses explicit GUsb context disposal; the fixture now follows it.
