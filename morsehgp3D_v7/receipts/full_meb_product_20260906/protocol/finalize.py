#!/usr/bin/env python3
"""Recheck the relocated published bytes without ELF, then seal this new packet."""
import hashlib
from pathlib import Path
import runpy

BASE = Path(__file__).resolve().parent
ROOT = Path('/workspaces/E-HGP')
PACKET = ROOT / 'morsehgp3D_v7/receipts/full_meb_product_20260906'

if __name__ == '__main__':
    helpers = runpy.run_path(str(BASE / 'publish.py'))
    checks = BASE / 'public_checks'
    checks.mkdir(exist_ok=False)
    for name, folder in (('cmake', 'core'), ('extra', 'extra'), ('mutations', 'mutations')):
        helpers['check'](name, PACKET / 'protocol' / ('verify_' + name + '.py'),
                         PACKET / folder, checks)
    helpers['tree'](checks, PACKET / 'public_replay')
    helpers['copy'](Path(__file__).resolve(), PACKET / 'protocol/finalize.py')
    lines = []
    for path in sorted(PACKET.rglob('*')):
        if path.is_symlink():
            raise ValueError('symlink in packet')
        if path.is_file():
            if path.name == 'SHA256SUMS':
                raise ValueError('packet already sealed')
            raw = path.read_bytes()
            if raw.startswith(b'\x7fELF'):
                raise ValueError('ELF unexpectedly distributed')
            lines.append(hashlib.sha256(raw).hexdigest() + '  ' + path.relative_to(PACKET).as_posix())
    helpers['put'](PACKET / 'SHA256SUMS', ('\n'.join(lines) + '\n').encode())
    print('sealed_files=' + str(len(lines)))
    print('manifest_sha256=' + helpers['sha'](PACKET / 'SHA256SUMS'))
