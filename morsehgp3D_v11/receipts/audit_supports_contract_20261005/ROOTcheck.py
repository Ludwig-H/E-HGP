#!/usr/bin/env python3
"""Replayer autonome des preuves bornees de cet audit ; aucun moteur HGP."""
import hashlib
import json
from math import comb
from pathlib import Path
import subprocess
import sys

HERE = Path(__file__).resolve().parent

def require(value, message):
    if not value:
        raise RuntimeError(message)

def inventory(folder):
    expected = {}
    for line in (folder / 'SHA256SUMS').read_text().splitlines():
        digest, relative = line.split('  ', 1)
        require(relative not in expected, 'inventory duplicate ' + relative)
        require(not Path(relative).is_absolute() and '..' not in Path(relative).parts,
                'inventory unsafe path ' + relative)
        expected[relative] = digest
    actual = {str(p.relative_to(folder)) for p in folder.rglob('*')
              if p.is_file() and p != folder / 'SHA256SUMS'}
    require(actual == set(expected), 'inventory coverage ' + folder.name)
    for relative, digest in expected.items():
        require(hashlib.sha256((folder / relative).read_bytes()).hexdigest() == digest,
                'inventory hash ' + relative)
    return len(expected)

def replay(name, script, stored):
    folder = HERE / name
    outputs = []
    for optimized in (False, True):
        command = [sys.executable] + (['-O'] if optimized else []) + ['-S', '-B', str(folder / script)]
        done = subprocess.run(command, cwd=folder, capture_output=True, timeout=60, check=False)
        require(done.returncode == 0 and not done.stderr, 'replay failed ' + name)
        require(done.stdout == (folder / stored[optimized]).read_bytes(), 'replay differs ' + name)
        outputs.append(done.stdout)
    require(outputs[0] == outputs[1], 'normal/O mismatch ' + name)
    return json.loads(outputs[0])

files = inventory(HERE)
for name in ('tower', 'qb', 'evidence'):
    inventory(HERE / name)
results = {
    'tower': replay('tower', 'check_published.py', ('normal.json', 'optimized.json')),
    'qb': replay('qb', 'check_followup.py', ('normal.json', 'optimized.json')),
    'evidence': replay('evidence', 'check_l0_s1.py', ('stdout_normal.json', 'stdout_opt.json')),
    'api': replay('api', 'check.py', ('normal.stdout', 'optimized.stdout')),
}
counts = {name: value['checks'] for name, value in results.items()}
counts['tower'] += results['tower']['independent_guards']
require(counts == {'tower': 1447, 'qb': 13491, 'evidence': 208, 'api': 83}, 'guard floors')
sphere = results['qb']['sphere5']
require(sphere['Q_by_arity'] == {'2': 12, '3': 24, '4': 792}, 'mixed arities')
require(sphere['N2_N3_N4'] == {'2': 12, '3': 288, '4': 3906}, 'unique closures')
incidences = sum(sphere['Q_by_arity'][str(a)] * comb(24 - a, 4 - a) for a in (2, 3, 4))
require(incidences == 4068 and incidences > sphere['N2_N3_N4']['4'], 'incidences versus unique cofaces')
summary = json.loads((HERE / 'RESULTS.json').read_text())
require(summary['guard_counts'] == counts and summary['total_guards'] == sum(counts.values()), 'summary counts')
require(summary['native_hgp_executed'] is False and summary['gcp_used'] is False, 'scope')
print(json.dumps({'status': 'conforme', 'files': files, 'guard_counts': counts,
                  'total_guards': sum(counts.values()), 'sphere5_supports': 828,
                  'K3_unique_cofaces': 3906, 'K3_support_incidences': incidences,
                  'native_hgp_executed': False, 'gcp_used': False}, sort_keys=True))
