#!/usr/bin/env bash
# SPDX-License-Identifier: LGPL-2.1-or-later
# Source-only local build. No package installation, network or sensor access.
set -euo pipefail
project_root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
options=(--prefix=/usr --libdir=lib -Ddrivers=elanmoc -Ddoc=false
  -Dgtk-examples=false -Dinstalled-tests=false -Dintrospection=false
  -Dudev_hwdb=disabled -Dudev_rules=disabled)
if [[ -f "$project_root/build/build.ninja" ]]; then
  meson setup --reconfigure "$project_root/build" "$project_root/libfprint" "${options[@]}"
else
  meson setup "$project_root/build" "$project_root/libfprint" "${options[@]}"
fi
meson compile -C "$project_root/build"
