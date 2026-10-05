#!/usr/bin/env python3
"""Relecture locale du plan propose ; aucun build, test natif ni appel cloud."""
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import types

HERE = Path(__file__).resolve().parent
REPO = Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else HERE.parents[2]
PIN = 'c97776ea8b8730bc41c8da4b137879cb8b544c2c'


def need(ok, message):
    if not ok:
        raise RuntimeError(message)


def git(*args):
    return subprocess.check_output(['git', '-C', str(REPO), *args])


if (HERE / 'SHA256SUMS').exists():
    for line in (HERE / 'SHA256SUMS').read_text().splitlines():
        sha, name = line.split('  ', 1)
        need(hashlib.sha256((HERE / name).read_bytes()).hexdigest() == sha, 'empreinte : ' + name)

plan = json.loads((HERE / 'points_differential_plan.json').read_text())
source = git('show', PIN + ':gcp-migration/v11_session.py')
session = types.ModuleType('audit_v11_session')
session.__file__ = str(REPO / 'gcp-migration/v11_session.py')
exec(compile(source, session.__file__, 'exec'), session.__dict__)
tracked = set(git('ls-tree', '-r', '--name-only', PIN).decode().splitlines())
validated = session.validate_plan(plan, tracked, set())
need(validated['python_packages'] == 'pinned', 'dependances Python')
need(validated['default_build'], 'construction attendue')
need(set(validated['build_targets']) == {'mhgp11_points_probe', 'mhgp11_points_export'}, 'cibles')
expected = ['mhgp11_points_vs_python'] + [
    'mhgp11_points_vs_python_lidar_' + frame + '_k5' for frame in ('ng00', 'ng01', 'ng02')]
selected = []
for command in validated['commands']:
    argv = command['argv']
    need(argv[:2] == ['ctest', '--no-tests=error'], 'refus de selection vide')
    need('-E' not in argv and '-LE' not in argv, 'aucun filtre excluant les differentiels')
    need(argv[argv.index('-j') + 1] == '1', 'commandes sequentielles')
    pattern = argv[argv.index('-R') + 1]
    need(pattern.startswith('^') and pattern.endswith('$'), 'selection ancree')
    selected.append(pattern[1:-1])
need(selected == expected, 'quatre portes distinctes')

capsule = REPO / 'morsehgp3D_v11/receipts/audit_s9_sort_fix_20261005/native'
manifest = json.loads((capsule / 'source_manifest.json').read_text())
for name, entry in manifest['files'].items():
    need(hashlib.sha256(git('show', PIN + ':' + name)).hexdigest() == entry['sha256'],
         'correctif publie different : ' + name)
print(json.dumps(dict(pin=PIN, plan_schema='conforme', commands=selected,
                      python_packages='pinned', sort_fix_files_identical=4,
                      controller_sha256=hashlib.sha256(source).hexdigest(),
                      scope='validation_locale_uniquement_sans_preflight_cloud_ni_execution'),
                 sort_keys=True, separators=(',', ':')))
