# Architecture

```text
KDE / command-line client
          |
        D-Bus
          |
       fprintd
          |
  libfprint-2.so.2
          |
  elanmoc driver (legacy 0c5e branch)
          |
      GUsb / libusb
          |
 ELAN MOC sensor + its own firmware/templates
```

This is a userspace library driver, not a newly written kernel module. KDE, fprintd's D-Bus interface and the sensor's internal recognition algorithm were not rewritten.

## Identity model

The sensor holds biometric templates. A libfprint print stores a Linux-generated identity, enrollment metadata and the device/driver association. Host and sensor identity must agree after a successful sensor-side match. A matching slot number alone is insufficient to authenticate a Linux reference.

Physical positions can be sparse. During the live test, deletion of slot 0 left the retained middle finger at slot 1 while COUNT was 1. Reenrollment later produced a match at slot 0. COUNT tracks active quantity, not a compact array boundary.

## Device capabilities

Modern ELAN behavior remains in the shared elanmoc driver. During probe, only PID 0c5e loses STORAGE_LIST. A direct list invocation on this branch returns NOT_SUPPORTED. STORAGE_DELETE and ordinary verify/identify/enroll remain available. There is no STORAGE_CLEAR implementation in this adaptation.

fprintd stores host references and lists the user's enrolled fingers from that storage. INFO is used in the matched-identity path, rather than as an authoritative cold device inventory.

## State machines

- Open: USB reset → two-second firmware wait → claim → READY → MODE → FW → DIM → COUNT.
- Verify/identify: COUNT → fail cleanly if zero → MODE → legacy IDENTIFY → matched INFO → exact Linux identity comparison → completion.
- Enroll: COUNT/capacity check → nine accepted capture stages → bounded identity commit → successful completion.
- Delete: COUNT before → one exact-identity DELETE → ACK → COUNT after; require a decrease of exactly one.
- Cancel: cancelled capture read → uncancellable abort OUT with no reply → complete cancellation.

A separate core guard skips physical sysfs wakeup configuration for JSON-emulated USB devices. It does not change the physical device's sysfs behavior.
