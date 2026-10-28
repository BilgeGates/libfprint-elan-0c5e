#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-2.1-or-later
"""Run full instrumentation and leak checks on synthetic ELAN fixtures only."""
from pathlib import Path
import json
import os
import shlex
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
build = root / 'build-security'
options = ['--prefix=/usr', '--libdir=lib', '-Ddrivers=elanmoc', '-Ddoc=false',
           '-Dgtk-examples=false', '-Dinstalled-tests=false', '-Dintrospection=false',
           '-Dudev_hwdb=disabled', '-Dudev_rules=disabled',
           '-Db_sanitize=address,undefined']
setup = ['meson', 'setup']
if (build / 'meson-private/coredata.dat').exists():
    setup.append('--reconfigure')
subprocess.run([*setup, str(build), str(root / 'libfprint'), *options], check=True)
# Build-time metadata generators also become instrumented. Leak detection is
# explicitly tested below, rather than accidentally depending on build wrappers.
subprocess.run(['meson', 'compile', '-C', str(build)], check=True,
               env={**os.environ, 'ASAN_OPTIONS': 'detect_leaks=0'})
env = {**os.environ, 'ASAN_OPTIONS': 'detect_leaks=1:halt_on_error=1',
       'LSAN_OPTIONS': 'exitcode=23', 'G_DEBUG': 'fatal-warnings'}
with tempfile.TemporaryDirectory(prefix='elan-leak-control-') as temporary:
    source = Path(temporary) / 'control.c'
    binary = Path(temporary) / 'control'
    source.write_text('''#include <stdlib.h>
#include <string.h>
static void leak(void) {
  void *p = malloc(37); memset(p, 1, 37);
  __asm__ volatile("" : : "r"(p) : "memory");
}
int main(int argc, char **argv) { (void)argv; if (argc > 1) leak(); return 0; }
''')
    subprocess.run([*shlex.split(os.environ.get('CC', 'cc')), '-O0', '-g',
                    '-fsanitize=address,undefined', str(source), '-o', str(binary)], check=True)
    control = subprocess.run([str(binary), 'leak'], env=env, text=True,
                             capture_output=True, timeout=15)
    if (control.returncode != 23 or 'detected memory leaks' not in control.stderr or
            '37 byte(s)' not in control.stderr):
        raise SystemExit('LeakSanitizer positive control did not work. Run outside '
                         'ptrace/debugger restrictions; no leak-free result is claimed.\n' + control.stderr)
    subprocess.run([str(binary)], check=True, env=env, timeout=15)
print('PASS leak detector controls: intentional 37-byte leak detected; clean case accepted.', flush=True)
tests = json.loads(subprocess.check_output(['meson', 'introspect', '--tests', str(build)], text=True))
selected = [test for test in tests if test['name'].startswith(('elanmoc-legacy-api-', 'elanmoc-security-'))]
if len(selected) < 29:
    raise SystemExit('Missing mandatory ELAN API/parser audit tests (GUsb >= 0.4, JSON-GLib and sanitizers required).')
allowed = {'test-elanmoc-legacy-api', 'test-elanmoc-security'}
for test in selected:
    command = test['cmd']
    executable = Path(command[0]).resolve()
    if executable.parent != (build / 'tests').resolve() or executable.name not in allowed:
        raise SystemExit('Unexpected audit command; refusing to run it.')
    subprocess.run(command, check=True, env=env, timeout=60)
    print('PASS ASan/UBSan/LSan:', test['name'], flush=True)
print(f'PASS {len(selected)} offline API/parser tests with leak detection; no physical sensor used.', flush=True)
