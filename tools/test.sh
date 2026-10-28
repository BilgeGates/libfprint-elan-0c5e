#!/usr/bin/env bash
# SPDX-License-Identifier: LGPL-2.1-or-later
# All tests are offline: synthetic data/device only, no physical USB.
set -euo pipefail
project_root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
"$project_root/tools/build.sh"
if [[ ! -x "$project_root/build/tests/test-elanmoc-legacy-api" ]]; then
  echo 'Complete validation requires GUsb >= 0.4, JSON-GLib and ASan/UBSan.' >&2
  exit 1
fi
python3 "$project_root/tests/test-count-state.py"
python3 "$project_root/tests/test-identity-frames.py"
python3 "$project_root/tests/test-cancel-state.py"
python3 "$project_root/tests/test-empty-checker.py"
meson test -C "$project_root/build" fpi-device fpi-ssm fpi-assembling --print-errorlogs
meson test -C "$project_root/build" --suite elanmoc-legacy --print-errorlogs
meson test -C "$project_root/build" --suite elanmoc-security --print-errorlogs
bash -n "$project_root/tools/build.sh" "$project_root/tools/test.sh"
