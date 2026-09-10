#!/usr/bin/env python3
"""Causal mutants: exact identity, component normalization and order namespace."""
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import sys
import time

ROOT = Path('/workspaces/E-HGP')
BASE = ROOT / 'build/v7_ball_resolver_opt_20260910_r2'
MODE = sys.argv[1]
specs = {
    'identity': ('if (entry.token == absent || entry.sites != key) return absent;',
                 'if (entry.token == absent) return absent;', 'resolver_cache_gate.cpp'),
    'normalization': ('if (cached != absent) return root(cached, prior_count);',
                      'if (cached != absent) return cached;', 'full_ball_tower_gate.cpp'),
    'order': ('if (k > 1) resolver_cache.reset();',
              'if (k == 2) resolver_cache.reset();', 'full_ball_tower_gate.cpp'),
}
old, new, gate = specs[MODE]
run = BASE / ('run_final_mutant_' + MODE)
run.mkdir(exist_ok=False)
tree = run / 'candidate'
shutil.copytree(BASE / 'cache_final', tree)
header = tree / 'src/forest/full_ball_tower.hpp'
source = header.read_text()
if source.count(old) != 1:
    raise RuntimeError('mutant site mismatch')
header.write_text(source.replace(old, new))
shutil.copy2(Path(__file__), run / 'cache_mutants.py')

def sources():
    return {str(p.relative_to(tree)): hashlib.sha256(p.read_bytes()).hexdigest()
            for p in sorted(tree.rglob('*')) if p.is_file() and p.suffix in ['.cpp', '.hpp', '.cu', '.cuh']}

receipt = {'mutant': MODE, 'commands': [], 'sources_before': sources(), 'gcp_used': False}
commands = [
    ('compile', ['g++', '-std=c++20', '-pthread', '-Wall', '-Wextra', '-Wpedantic', '-Werror',
     '-O2', '-DMHGP7_PRIVATE_RESOLVER_SEED=1', '-isystem',
     str(ROOT / 'build/v7_boost_gate/extracted/usr/include'), str(tree / 'tests' / gate),
     '-o', str(run / 'gate')], 0),
    ('selftest', [str(run / 'gate'), '--selftest'], 1),
]
for name, argv, expected in commands:
    start = time.time_ns()
    with (run / (name + '.stdout')).open('wb') as out, (run / (name + '.stderr')).open('wb') as err:
        result = subprocess.run(argv, cwd=ROOT, stdout=out, stderr=err, check=False)
    receipt['commands'].append({'argv': argv, 'expected': expected, 'returncode': result.returncode,
        'start_ns': start, 'end_ns': time.time_ns(),
        'stdout_sha256': hashlib.sha256((run / (name + '.stdout')).read_bytes()).hexdigest(),
        'stderr_sha256': hashlib.sha256((run / (name + '.stderr')).read_bytes()).hexdigest()})
    (run / 'receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')
    print(MODE, name, result.returncode, flush=True)
    if result.returncode != expected:
        raise RuntimeError('mutant outcome differs; preserve raw outcome')
receipt['sources_after'] = sources()
receipt['antidrift'] = receipt['sources_before'] == receipt['sources_after']
(run / 'receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')
if not receipt['antidrift']:
    raise RuntimeError('source drift')
