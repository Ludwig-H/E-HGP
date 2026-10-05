#!/usr/bin/env python3
"""Reconstitue la reference WIP epinglee et rejoue les deux controles Python bornes.

Requiert le commit de base dans le depot Git ; aucun worktree du developpeur,
compilateur, binaire natif ou service distant n'est utilise.
"""
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile

HERE = Path(__file__).resolve().parent
SOURCE = json.loads((HERE / 'source_manifest.json').read_text())
python = [sys.executable, '-B'] + (['-O'] if sys.flags.optimize else [])
env = dict(os.environ, PYTHONDONTWRITEBYTECODE='1')


def run(argv, cwd, environment=env):
    result = subprocess.run(argv, cwd=cwd, env=environment, capture_output=True, check=False, timeout=60)
    if result.returncode != 0:
        raise RuntimeError('%r: code %d\n%s\n%s' %
                           (argv, result.returncode, result.stdout.decode(), result.stderr.decode()))
    return result.stdout


with tempfile.TemporaryDirectory(prefix='mhgp11-audit-l1-') as temporary:
    tree = Path(temporary)
    references = {p: digest for p, digest in SOURCE['files'].items()
                  if '/reference/' in p and p.endswith('.py')}
    for path in references:
        target = tree / path
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(run(['git', 'show', SOURCE['head'] + ':' + path], HERE))
    run(['git', 'apply', str(HERE / 'reference.patch')], tree)
    for path, digest in references.items():
        if hashlib.sha256((tree / path).read_bytes()).hexdigest() != digest:
            raise RuntimeError('source differente : ' + path)
    reference = tree / 'morsehgp3D_v11/reference'
    primitive = run(python + ['test_supports.py', '--suite=primitives'], reference)
    k12 = run(python + [str(HERE / 'check_k12.py')], tree,
              dict(env, MHGP11_L1_REFERENCE=str(reference)))
    if primitive != (HERE / 'primitives.normal.txt').read_bytes():
        raise RuntimeError('sortie primitives differente')
    if k12 != (HERE / 'k12.normal.json').read_bytes():
        raise RuntimeError('sortie K12 differente')
print('audit_l1_replay_ok primitives=1 temoin_K12=1 refus_budget=2 natif=0')
