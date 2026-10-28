#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-2.1-or-later
from pathlib import Path
import subprocess
import sys
import tempfile

root=Path(__file__).resolve().parent.parent
checker=root/'tools/check-empty-storage.py'
with tempfile.TemporaryDirectory(prefix='elan-empty-tests-') as name:
    temp=Path(name)
    for case in ('empty','retained-info','unavailable-info','nonzero','short-count','wrong-ack','missing-ack','missing-count'):
        snapshot=temp/case
        snapshot.mkdir()
        if case!='missing-count':
            (snapshot/'count.bin').write_bytes(b'\x40\x01' if case=='nonzero' else
                                              b'\x40' if case=='short-count' else b'\x40\x00')
        ack=temp/(case+'.ack')
        if case!='missing-ack':
            ack.write_bytes(b'\x40\xff' if case=='wrong-ack' else b'\x40\x00')
        for slot in range(10):
            record=bytearray(b'\x40\x00'+b'\x00'*62)
            if case=='retained-info' and slot<2:
                record[6]=25
                record[7:32]=b'FP1-20261027-7-12345678-x'
            if case=='unavailable-info':
                record=b'\x40\xff'
            (snapshot/f'snapshot-slot-{slot}.bin').write_bytes(record)
        result=subprocess.run([sys.executable,str(checker),str(snapshot),'--clear-ack',str(ack)],
                              text=True,capture_output=True)
        expected=case in ('empty','retained-info','unavailable-info')
        assert (result.returncode==0)==expected,(case,result.stdout,result.stderr)
        if case=='retained-info':
            assert 'Retained inactive INFO metadata at slots: 0 1' in result.stdout
        print('PASS empty checker:',case)
