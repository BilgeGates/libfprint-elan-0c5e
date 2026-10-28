# Security and privacy

Biometric comparison runs inside the sensor. libfprint validates command replies and maps the returned Linux identity to an exact host reference. Host `.fprint` files contain identity metadata, not restorable biometric templates.

The legacy branch checks firmware/dimensions, reply framing, UID bounds/format, sensor-match results and exact host identity equality. Deletion requires a strict acknowledgment and count change. Cancellation explicitly aborts capture. No clear-all fallback is exposed.

## Sensitive data

Do not submit private `.fprint`/variant files, on-chip metadata, real enrollment USB recordings, passwords, tokens, vendor binaries, decompiler projects or core dumps. Synthetic local fixtures and pre-existing upstream public fixtures are included; personal test artifacts are excluded.

`checksums.sha256` and release `SHA256SUMS` detect changes but are not digital signatures or certifications. Source releases contain no prebuilt driver package.

## Reporting

Use issue templates for ordinary bugs after removing usernames and FP1 identity strings. For authentication bypass or other sensitive vulnerabilities, use GitHub private vulnerability reporting if enabled. If unavailable, request a private reporting channel without disclosing exploit details or personal data in a public issue. Do not erase sensor records to collect a report.

## Audit scope

See [SECURITY-AUDIT.md](docs/SECURITY-AUDIT.md) for reproduced defects, fixes and reproducible ASan/UBSan/LSan tests. This was a local, agent-assisted source review, not an independent security assessment. Passing tests do not establish absence of all vulnerabilities, leaks or timing defects.

Malicious sensor firmware, biometric liveness/spoof resistance, compromised root accounts, every third-party dependency and operating-system authentication policy are outside this review. Suspend/resume and other hardware/firmware/distribution runtime remain unverified.
