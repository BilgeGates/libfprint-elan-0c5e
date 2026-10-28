#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-2.1-or-later
"""Verify the checked-in manifest and export deterministic source release assets."""
from pathlib import Path
import argparse
import gzip
import hashlib
import io
import re
import json
import subprocess
import tarfile

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('output_directory', type=Path)
args = parser.parse_args()
root = Path(__file__).resolve().parent.parent
version = (root / 'VERSION').read_text().strip()
if not re.fullmatch(r'[0-9]+\.[0-9]+\.[0-9]+(?:-[a-z0-9]+)?', version):
    raise SystemExit('Invalid release version.')
manifest = root / 'checksums.sha256'
if not manifest.is_file():
    raise SystemExit('Missing checksums.sha256; finalize/review the source snapshot first.')
files = []
for line in manifest.read_text().splitlines():
    if not re.fullmatch(r'[0-9a-f]{64}  [^\r\n]+', line):
        raise SystemExit('Malformed manifest line.')
    digest, name = line.split('  ', 1)
    relative = Path(name)
    if relative.is_absolute() or '..' in relative.parts or relative == Path('.'):
        raise SystemExit(f'Unsafe manifest path: {name}')
    if any(part in {'.git', 'build', 'build-audit', 'build-security', 'private', 'sessions', 'test-output', '__pycache__'}
           for part in relative.parts):
        raise SystemExit(f'Excluded release path: {name}')
    path = root / relative
    if path.is_symlink() or not path.is_file() or not path.resolve().is_relative_to(root):
        raise SystemExit(f'Unsafe or missing source file: {name}')
    if hashlib.sha256(path.read_bytes()).hexdigest() != digest:
        raise SystemExit(f'Manifest mismatch: {name}')
    files.append(path)
if len(files) != len(set(files)):
    raise SystemExit('Duplicate manifest entries.')
files.append(manifest)
output = args.output_directory.resolve()
if output.is_relative_to(root):
    raise SystemExit('Use an output directory outside the source tree.')
output.mkdir(parents=True, exist_ok=True)
stem = f'libfprint-elan-0c5e-{version}'
target = output / f'{stem}.tar.gz'
patch = output / f'elan-0c5e-fw3014-{version}.patch'
arch_source = output / 'libfprint-elan-source.tar.gz'
checksum = output / 'SHA256SUMS'
if any(p.exists() for p in [target, patch, arch_source, checksum]):
    raise SystemExit('Release outputs already exist; choose a fresh directory.')
with target.open('xb') as raw:
    with gzip.GzipFile(filename='', mode='wb', fileobj=raw, mtime=0) as compressed:
        with tarfile.open(fileobj=compressed, mode='w', format=tarfile.PAX_FORMAT) as archive:
            for path in sorted(files):
                data = path.read_bytes()
                info = tarfile.TarInfo(stem + '/' + path.relative_to(root).as_posix())
                info.size = len(data)
                info.mode = 0o755 if path.stat().st_mode & 0o111 else 0o644
                info.uid = info.gid = 0
                info.uname = info.gname = ''
                info.mtime = 0
                archive.addfile(info, io.BytesIO(data))
patch.write_bytes((root / 'patches/0001-elan-0c5e-fw3014.patch').read_bytes())
subprocess.run([str(root / 'tools/export-arch-source.py'), '--output', str(arch_source)], check=True)
expected = json.loads((root / 'PROVENANCE.json').read_text())['artifacts']['libfprint-elan-source.tar.gz']['sha256']
if hashlib.sha256(arch_source.read_bytes()).hexdigest() != expected:
    raise SystemExit('Arch source hash does not match reviewed provenance.')
checksum.write_text(''.join(f'{hashlib.sha256(p.read_bytes()).hexdigest()}  {p.name}\n'
                            for p in [target, patch, arch_source]))
print(f'Created {target.name}, {patch.name} plus pinned Arch source and SHA256SUMS; {len(files)} source files.')
