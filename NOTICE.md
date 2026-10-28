# License and provenance

This project is a derivative adaptation of libfprint, not a from-scratch implementation of the entire library or an official HP driver release.

- Base: [official libfprint](https://gitlab.freedesktop.org/libfprint/libfprint), commit `6f9479c3d55f847c1b3769f28ceb99227f9858cf`, source version `1.94.100`.
- Starting community adaptation: [HP ProBook 440 G8 Fingerprint Fix Module](https://github.com/prantopream420/HP-ProBook-440-G8-Fingerprint-Fix-Module-for-Ubuntu), commit `1ea6f30fce8c422ce7d629a3216dba1120bee75a`, libfprint `1.95.1+tod1` based source. Its embedded source archive SHA256 was `e120eeec31746191d3223e684e763b9b6b52deb502558c0f4de438e266d41363`.
- Additional protocol references consulted: [elan-0c00](https://github.com/jvikramsrd/elan-0c00), [Depau/libfprint](https://gitlab.freedesktop.org/Depau/libfprint), and [xerootg/libfprint](https://github.com/xerootg/libfprint). Their device observations are not automatically treated as facts for 0c5e.
- Vendor protocol analysis: HP SoftPaq [SP152998](https://ftp.hp.com/pub/softpaq/sp152501-153000/sp152998.html), version `3.2.12011.10056`. Package SHA256: `006361cff40906f2a0be511768e1e57dd539044aa074506623d1c4325a85363e`.

Original file-level copyright and license notices, including ELAN Microelectronics' notices, are preserved. Local driver modifications and local test/tool sources marked LGPL-2.1-or-later follow those terms. See [COPYING](COPYING). Third-party code already present in upstream libfprint retains its own file-level terms; this document does not relabel it.

The vendor package was inspected as data. Decompiled HP routines were not copied into the Linux implementation. Neither vendor DLLs nor decompiler output are included here. Wire-level observations are documented separately from the implementation and hardware results.

No HP endorsement, upstream acceptance, official certification or first-ever implementation claim is made. Repository references above identify prior work and protocol evidence.
