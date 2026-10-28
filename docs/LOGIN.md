# Initial login and screen unlock

The tested machine has separate PAM services for initial SDDM login and KDE screen unlock.

- `/etc/pam.d/sddm` includes `system-login`, which includes `system-auth`. That path uses password authentication and contains no `pam_fprintd.so` entry.
- `/usr/lib/pam.d/kde-fingerprint` contains `pam_fprintd.so` for fingerprint screen unlock.

These files and the boot journal were inspected read-only on 2026-10-27. The user reports: reboot, log in with the password, lock the session, unlock successfully with the fingerprint. The settings explain that observation without requiring a driver recovery event. They do not establish an Android-style mandatory-first-password state inside this driver.

Other distributions/display managers may configure authentication differently. This project does not modify PAM, SDDM, disk encryption or wallet unlocking. Do not replace system PAM files with the example paths above.
