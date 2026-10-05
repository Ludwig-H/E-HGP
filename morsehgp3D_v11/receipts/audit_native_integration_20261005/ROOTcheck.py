#!/usr/bin/env python3
"""Rejeu autonome borné ; ne lance aucun exécutable HGP."""
import hashlib
import json
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
    actual = {p.relative_to(folder).as_posix() for p in folder.rglob('*')
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
        command = [sys.executable] + (['-O'] if optimized else [])
        command += ['-S', '-B', str(folder / script)]
        done = subprocess.run(command, cwd=folder, capture_output=True,
                              timeout=60, check=False)
        require(done.returncode == 0 and not done.stderr, 'replay failed ' + name)
        require(done.stdout == (folder / stored[optimized]).read_bytes(),
                'replay differs ' + name)
        outputs.append(done.stdout)
    require(outputs[0] == outputs[1], 'normal/O mismatch ' + name)
    return json.loads(outputs[0])


files = inventory(HERE)
for name in ('tower', 'api', 'qb', 'root'):
    inventory(HERE / name)
results = {
    'tower': replay('tower', 'check_journal.py', ('normal.json', 'optimized.json')),
    'api': replay('api', 'check_s5_contract.py', ('stdout_normal.json', 'stdout_optimized.json')),
    'qb': replay('qb', 'check_high_k.py', ('normal.json', 'optimized.json')),
    'root': replay('root', 'check.py', ('normal.stdout', 'optimized.stdout')),
}
counts = {name: value['checks'] for name, value in results.items()}
require(counts == {'tower': 15283, 'api': 168, 'qb': 4135, 'root': 127}, 'guard floors')
sphere = results['qb']['sphere5']
require(sphere['N12'] == 2704040 and sphere['separable12'] == 116, 'high closure')
require(sphere['independent_arrangement_chambers'] == 116, 'independent chambers')
require(sphere['K12_counts'] == {'kparties_reliees': 2704156, 'compressed_parts': 2704156,
        'strict_traces': 116, 'cofaces': 2496144, 'gabriel_cofaces': 2496144}, 'K12 counts')
require(sphere['incidences_K12'] == 149954688, 'incidences distinct from cofaces')
require(results['root']['full_mask'] == 16379 and
        results['root']['single_order_mask'] == 7035, 'fair FULL reference')
summary = json.loads((HERE / 'RESULTS.json').read_text())
require(summary['guard_counts'] == counts and
        summary['total_guards'] == sum(counts.values()), 'summary counts')
require(summary['native_hgp_executed'] is False and summary['gcp_used'] is False
        and summary['fit_used'] is False, 'scope')
context = json.loads((HERE / 'SOURCE_CONTEXT.json').read_text())
require(hashlib.sha256((HERE / 'developer_note_F.md').read_bytes()).hexdigest() ==
        context['developer_reply_sha256'], 'developer note')
print(json.dumps({'status': 'conforme', 'files': files, 'guard_counts': counts,
                  'total_guards': sum(counts.values()), 'K12_strict_traces': 116,
                  'native_hgp_executed': False, 'gcp_used': False}, sort_keys=True))
