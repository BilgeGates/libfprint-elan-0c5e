#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-2.1-or-later
from pathlib import Path
import os
import subprocess
import tempfile
root=Path(__file__).resolve().parent
source=(root.parent/'libfprint/libfprint/drivers/elanmoc/elanmoc.c').read_text()
start=source.index('static void\nelanmoc_get_enrolled_cb (')
end=source.index('\nstatic void\nelanmoc_reenroll_cb (',start)
with tempfile.TemporaryDirectory(prefix='elan-count-test-') as temp:
    temp=Path(temp)
    sections=[source[start:end]]
    start=source.index('static void\nelanmoc_cmd_ver_cb (')
    end=source.index('\nstatic void\nelanmoc_get_status_cb (',start)
    sections.append(source[start:end])
    (temp/'driver-count-function.inc').write_text('\n'.join(sections))
    binary=temp/'count-test'
    flags=subprocess.check_output(['pkg-config','--cflags','--libs','glib-2.0'],text=True).split()
    subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror','-O1','-g',
                    '-fsanitize=address,undefined','-I'+str(temp),str(root/'count-state-test.c'),
                    '-o',str(binary),*flags],check=True)
    subprocess.run([str(binary)],check=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})
