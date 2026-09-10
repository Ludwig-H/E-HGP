#!/usr/bin/env python3
"""Causal mutants on the normalized combined snapshot, never active sources."""
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import sys
import time

ROOT = Path('/workspaces/E-HGP')
BASE = ROOT / 'build/v7_combined_resolver_20260910'
kind = sys.argv[1]
specs = {
    'identity': ('if (entry.token == absent || entry.sites != key) return absent;',
                 'if (entry.token == absent) return absent;', 'facet_resolver_cache_gate'),
    'normalization': ('if (cached != absent) return root(cached, prior_count);',
                      'if (cached != absent) return cached;', 'full_ball_tower_gate'),
    'seed': ('resolver_cache.store(key, target, true);', '(void)target;', 'facet_resolver_cache_gate'),
    'release': ('    resolver_cache.release();', '    // mutant: retain dead cache', 'facet_resolver_cache_gate'),
    'growth_grouped': ('if (action.parents.size() != 1 || !action.contributions.empty()) batch.actions.push_back(std::move(action));',
                       'if (action.parents.size() != 1) batch.actions.push_back(std::move(action));', 'full_ball_tower_gate'),
    'inert_grouped': ('for (size_t b = 0; b < blocks.size(); ++b) {\n      require(anchors[blocks[b].ball]',
                      'for (size_t b = 0; b < blocks.size(); ++b) {\n      if (blocks[b].roots.size() == 1 && !blocks[b].contribution && !blocks[b].interior) continue;\n      require(anchors[blocks[b].ball]', 'full_ball_tower_gate'),
}
old, new, gate = specs[kind]
run = BASE / ('run_mutant_' + kind)
run.mkdir(exist_ok=False)
tree = run / 'source'
shutil.copytree(BASE / 'combined', tree)
header = tree / 'src/forest/full_ball_tower.hpp'
source = header.read_text()
if source.count(old) != 1:
    raise RuntimeError('mutant site mismatch')
header.write_text(source.replace(old, new))
shutil.copy2(Path(__file__), run / 'mutant.py')

def sources():
    return {str(p.relative_to(tree)): hashlib.sha256(p.read_bytes()).hexdigest()
            for p in sorted(tree.rglob('*')) if p.is_file() and p.suffix in ['.cpp', '.hpp', '.cu', '.cuh']}

receipt = {'schema': 'combined-resolver-causal-mutant-v1', 'gcp_used': False,
           'sources_before': sources(), 'commands': []}
commands = [('compile', ['g++', '-std=c++20', '-pthread', '-Wall', '-Wextra', '-Wpedantic', '-Werror',
    '-O2', '-isystem', str(ROOT / 'build/v7_boost_gate/extracted/usr/include'),
    str(tree / 'tests' / (gate + '.cpp')), '-o', str(run / 'gate')], 0),
    ('selftest', [str(run / 'gate'), '--selftest'], 1)]
for name, argv, expected in commands:
    start = time.time_ns()
    with (run / (name + '.stdout')).open('wb') as out, (run / (name + '.stderr')).open('wb') as err:
        result = subprocess.run(argv, cwd=ROOT, stdout=out, stderr=err, check=False)
    receipt['commands'].append({'argv': argv, 'returncode': result.returncode, 'expected': expected,
        'start_ns': start, 'end_ns': time.time_ns(),
        'stdout_sha256': hashlib.sha256((run / (name + '.stdout')).read_bytes()).hexdigest(),
        'stderr_sha256': hashlib.sha256((run / (name + '.stderr')).read_bytes()).hexdigest()})
    (run / 'receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')
    print(kind, name, result.returncode, flush=True)
    if result.returncode != expected:
        raise RuntimeError('unexpected causal mutant outcome')
receipt['sources_after'] = sources()
receipt['antidrift'] = receipt['sources_before'] == receipt['sources_after']
(run / 'receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')
if not receipt['antidrift']:
    raise RuntimeError('mutant source drift')
