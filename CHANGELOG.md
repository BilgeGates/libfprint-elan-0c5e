# Changelog

## 0.1.0 — 2026-10-28

First stable project release for ELAN 04f3:0c5e firmware 3014 on the documented HP ProBook 450 G8 / Arch Linux configuration.

- Adapt legacy initialization, enrollment, identification and exact-identity deletion to libfprint 1.94.100.
- Use host references and matched identity reads instead of unreliable cold enumeration.
- Handle cancellation with an explicit sensor abort and validate transfer lengths, identity bounds, acknowledgments and count changes.
- Harden close-error ownership, modern identity parsing, malformed FP1 metadata and the final READY retry boundary.
- Add 26 synthetic public-API scenarios, three parser tests, mutation/stress checks and reproducible sanitizer/leak auditing.
- Provide English documentation, upstream provenance, a standalone patch, deterministic source exports and an Arch source recipe.

Hardware lifecycle testing and later confirmation of the updated driver are documented in docs/TESTING.md. Other hardware, firmware and distribution runtime support remain unverified.
