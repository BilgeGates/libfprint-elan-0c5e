# Firmware 3014 protocol notes

Target: USB `04f3:0c5e`, interface 0, firmware raw bytes `30 14`, sensor dimensions `4f 00 4f 00` (79×79).

## Endpoints and commands

| Operation | OUT bytes / size | IN endpoint / result |
| --- | --- | --- |
| READY | `40 ff 00`, 3 bytes | 83, `40 03` |
| Mode | `40 ff 14 03`, 4 bytes | 83, `40 00` |
| Firmware | `40 19`, 2 bytes | 83, raw `30 14` (no 40 prefix) |
| Dimensions | `00 0c`, 2 bytes | 83, four dimension bytes |
| Active count | `40 ff 04`, 3 bytes | 83, `40 NN` |
| Identify | `40 ff 03`, 3 bytes | 84, `40 slot` or a scan/status code |
| Capture/enroll | `40 ff 01 …`, 7 bytes | 84, two-byte stage result |
| Matched identity | `40 ff 12 slot`, 4 bytes | 83, request 71 bytes; accepted identity frame sizes 64 or 71 |
| Commit | `40 ff 11` + 69-byte identity body | 83, two-byte ACK |
| Delete | `40 ff 13` + the same identity body | 83, two-byte ACK, then separate COUNT verification |
| Abort | `40 ff 02`, 3 bytes | No IN request |

All endpoints above are hexadecimal. Ordinary replies are bounded and validated. Capture reads are cancellable; the abort write is not tied to the already-cancelled read token.

## Linux identity representation

The Linux-generated `FP1-…` string carries date, finger, random identifier and username metadata. It is not a biometric template or a secret password.

The current implementation's 69-byte identity body is:

```text
body[0..3] = 00 00 00 00
body[4]    = UID byte length
body[5..]  = UID bytes
remainder  = zero padding
```

The maximum accepted UID is 57 bytes, fitting the shortest accepted 64-byte INFO reply (2-byte reply header + 5-byte body header + 57 UID bytes). Longer generated identities fail before a persistent commit. This layout is this Linux implementation's representation; it is not a generic parser for Windows SID records.

## Evidence and limits

- HP SP152998 analysis: matched SID read requests 71 bytes; full-identity non-dual-OS commit/delete uses a 72-byte frame with a supplied 69-byte body.
- Local hardware: matching with the earlier 128-byte zero-padded commit reference, new 72-byte commits, identity deletion, reenrollment, negative matching, cancellation recovery and both identities after reopening passed.
- COUNT was observed as 0, 1 and 2 in the clean/re-enrollment lifecycle. Indices remained sparse after deletion.
- An explicit historical HP clear received ACK `40 00` and a fresh COUNT 0, while INFO retained old metadata. Therefore metadata non-emptiness does not prove an active enrollment or failure of logical deletion.

The clear command is not part of the distributed driver's delete/enroll fallback or provided installation tool. No claim is made about physical secure erasure of flash. No proprietary decompiled functions are included here.
