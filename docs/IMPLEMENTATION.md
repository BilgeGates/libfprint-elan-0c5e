# Implementation map

## Source files

| File | Local responsibility |
| --- | --- |
| `libfprint/drivers/elanmoc/elanmoc.c` | PID registration, legacy command routing, opening, matched identity parsing, capability restriction, UID deletion, cancellation and reply validation |
| `libfprint/drivers/elanmoc/elanmoc.h` | Legacy command descriptors and per-device state |
| `libfprint/drivers/elanmoc/elanmoc-legacy-storage.h` | Bounded 72-byte identity frame encoder |
| `libfprint/fprint-list-udev-hwdb.c` | Remove 0c5e from the known-unsupported list after registering support |
| `libfprint/fpi-device.c` | Skip physical sysfs configuration for synthetic GUsb devices |
| `tests/test-elanmoc-legacy-api.c` | Real public API + real driver with synthetic USB transfers |
| `tests/test-elanmoc-security.c` | Exact-size parser mutation and regression tests |
| `tests/meson.build` | Build/register legacy API tests when their dependencies are available |

Paths in this table are relative to the bundled `libfprint/` source root. The exact consolidated change is in `patches/0001-elan-0c5e-fw3014.patch` against the base in NOTICE.md.

## Important functions

- `elanmoc_probe`: reports the actual capabilities of the legacy device; no cold list support.
- `elanmoc_open` / `elanmoc_open_after_reset`: waits for firmware recovery before initialization.
- `elanmoc_get_enrolled_cb`: validates COUNT and handles an empty sensor for verification/identification.
- `create_print_from_0c5e_response`: validates 64/71-byte matched identity data, Linux UID bounds/format and metadata fields.
- `elanmoc_match_report_cb`: compares the actual matched Linux identity to the requested reference/gallery.
- `elan_legacy_build_identity`: generates the shared commit/delete identity body with strict opcode/size checks.
- `elanmoc_legacy_delete_*`: sends one identity-based deletion and verifies the count change; no enumeration/wipe fallback.
- `elanmoc_handle_cancel` / `elanmoc_cancel_done_cb`: wait for abort OUT before reporting capture cancellation.

## Why these decisions exist

Cold INFO returned `40 ff` after a fresh successful enrollment. Treating it as an inventory made a valid stored fingerprint appear unavailable. HP's reader uses the identification result to read SID. The local adaptation therefore uses host references and matched reads.

The sensor's count did not imply dense indices. Rejecting a matching slot merely because its number exceeded/equalled COUNT would reject a valid retained print.

The old test application kept a floating template in an autoptr while the API consumed its floating reference. A full API emulation reproduced a double unref at exit. Owning a sunk reference corrected the application; this was separate from the sensor protocol error.

Version 0.1.0 keeps the target wire commands and identity matching model, while adding the error/bounds/metadata hardening documented in SECURITY-AUDIT.md. `elanmoc_user_id_valid` validates Linux metadata before the core parser. Close errors and USB release errors have separate ownership. Completed initialization/close states are cleared. READY success depends on the reply, including at the retry boundary. Three previously dropped upstream ID entries are restored; those other devices are not hardware-tested here.
