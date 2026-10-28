#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-2.1-or-later
from pathlib import Path
import os
import subprocess
import tempfile
root=Path(__file__).resolve().parent
driver=root.parent/'libfprint/libfprint/drivers/elanmoc'
source=(driver/'elanmoc.c').read_text()
header=(driver/'elanmoc.h').read_text()
start=source.index('static uint8_t *\nelanmoc_compose_cmd (')
end=source.index('\nstatic void\nelanmoc_cmd_ack_cb',start)
compose=source[start:end]
start=source.index('static void\nelanmoc_cancel_done_cb (')
end=source.index('\nenum enroll_states {',start)
callbacks=source[start:end]
start=header.index('static const struct elanmoc_cmd elanmoc_above_cmd =')
end=header.index('\n};',start)+3
command=header[start:end]
with tempfile.TemporaryDirectory(prefix='elan-cancel-test-') as temp:
    temp=Path(temp)
    (temp/'driver-cancel-functions.inc').write_text(command+'\n'+compose+'\n'+callbacks)
    binary=temp/'cancel-test'
    flags=subprocess.check_output(['pkg-config','--cflags','--libs','gio-2.0'],text=True).split()
    subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror','-O1','-g',
                    '-fsanitize=address,undefined','-I'+str(temp),str(root/'cancel-state-test.c'),
                    '-o',str(binary),*flags],check=True)
    subprocess.run([str(binary)],check=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})
