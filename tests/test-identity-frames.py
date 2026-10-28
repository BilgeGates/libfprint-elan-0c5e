#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-2.1-or-later
from pathlib import Path
import os
import subprocess
import tempfile
root=Path(__file__).resolve().parent
driver=root.parent/'libfprint/libfprint/drivers/elanmoc'
with tempfile.TemporaryDirectory(prefix='elan-identity-tests-') as temp:
    binary=Path(temp)/'frames'
    flags=subprocess.check_output(['pkg-config','--cflags','--libs','glib-2.0'],text=True).split()
    subprocess.run(['cc','-std=c11','-O1','-g','-Wall','-Wextra','-Werror',
                    '-fsanitize=address,undefined','-I'+str(driver),str(root/'identity-frames.c'),
                    '-o',str(binary),*flags],check=True)
    subprocess.run([str(binary)],check=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})
