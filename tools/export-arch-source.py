#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-2.1-or-later
"""Export the bundled libfprint tree with stable tar/gzip metadata; no network."""
from pathlib import Path
import argparse
import gzip
import io
import tarfile

root = Path(__file__).resolve().parent.parent
source = root / 'libfprint'
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output', type=Path, default=root / 'packaging/arch/libfprint-elan-source.tar.gz')
args = parser.parse_args()
output = args.output
if output.exists():
    raise SystemExit(f'Output already exists: {output}; remove it explicitly before regeneration.')
output.parent.mkdir(parents=True, exist_ok=True)
excluded = {'.git', '__pycache__', 'build', 'builddir', 'build-audit', 'build-security'}
files = sorted(p for p in source.rglob('*') if p.is_file() and
               not excluded.intersection(p.relative_to(source).parts) and
               p.suffix != '.pyc')
if not files:
    raise SystemExit('No source files found.')
with output.open('wb') as raw:
    with gzip.GzipFile(filename='', mode='wb', fileobj=raw, mtime=0) as compressed:
        with tarfile.open(fileobj=compressed, mode='w', format=tarfile.PAX_FORMAT) as archive:
            for path in files:
                if path.is_symlink():
                    raise SystemExit(f'Unexpected source symlink: {path.relative_to(source)}')
                data = path.read_bytes()
                info = tarfile.TarInfo('libfprint-elan-source/' + path.relative_to(source).as_posix())
                info.size = len(data)
                info.mode = 0o755 if path.stat().st_mode & 0o111 else 0o644
                info.mtime = 0
                info.uid = info.gid = 0
                info.uname = info.gname = ''
                archive.addfile(info, io.BytesIO(data))
print(f'Exported {len(files)} source files to {output.name}')
