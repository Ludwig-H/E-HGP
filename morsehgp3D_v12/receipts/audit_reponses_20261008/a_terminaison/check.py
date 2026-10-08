#!/usr/bin/env python3
"""Verifier les cinq pins, appliquer/inverser la proposition a part, rejouer le modele SC."""
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tempfile

sys.dont_write_bytecode = True
import model


def main():
    here, repo = Path(__file__).resolve().parent, Path(sys.argv[1])
    c = json.loads((here / 'capture.json').read_text())
    source = {}
    for name, digest in c['sources'].items():
        raw = subprocess.check_output(['git', '-C', str(repo), 'show', c['pin'] + ':morsehgp3D_v12/' + name])
        model.need(hashlib.sha256(raw).hexdigest() == digest, name)
        source[name] = raw
    patch = here / 'proposition.patch'
    model.need(hashlib.sha256(patch.read_bytes()).hexdigest() == c['patch_sha256'], 'patch')
    with tempfile.TemporaryDirectory(prefix='audit-a-termination-') as folder:
        root = Path(folder)
        name = 'src/tower/pipeline_run.cpp'
        target = root / 'morsehgp3D_v12' / name
        target.parent.mkdir(parents=True)
        target.write_bytes(source[name])
        for flags in (['--check'], []):
            subprocess.run(['git', 'apply', *flags, str(patch)], cwd=root, check=True, capture_output=True)
        model.need(hashlib.sha256(target.read_bytes()).hexdigest() == c['proposed_sha256'], 'postimage')
        subprocess.run(['git', 'apply', '--reverse', str(patch)], cwd=root, check=True, capture_output=True)
        model.need(target.read_bytes() == source[name], 'inverse')
    model.main()


if __name__ == '__main__':
    main()
