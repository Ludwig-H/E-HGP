#!/usr/bin/env python3
"""Fresh strict qualification of the combined active-byte snapshot."""
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import time

ROOT = Path('/workspaces/E-HGP')
BASE = ROOT / 'build/v7_combined_resolver_20260910'
MODE = sys.argv[1]
RUN = BASE / ('run_' + MODE)
RUN.mkdir(exist_ok=False)
shutil.copy2(Path(__file__), RUN / 'record.py')

def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def sources(tree):
    return {str(p.relative_to(BASE)): sha(p) for p in sorted(tree.rglob('*'))
            if p.is_file() and p.suffix in ['.cpp', '.hpp', '.cu', '.cuh']}

def active_pins():
    return {name: sha(ROOT / 'morsehgp3D_v7' / name) for name in [
        'src/forest/full_ball_tower.hpp', 'src/forest/full_coverage_certificate.hpp',
        'tests/facet_resolver_cache_gate.cpp']}

manifest = {'schema': 'combined-resolver-residence-v1', 'gcp_used': False, 'commands': [],
            'active_before': active_pins()}

def run(name, argv, expected=0, env=None):
    start = time.time_ns()
    with (RUN / (name + '.stdout')).open('wb') as out, (RUN / (name + '.stderr')).open('wb') as err:
        result = subprocess.run(argv, cwd=ROOT, stdout=out, stderr=err, env=env, check=False)
    manifest['commands'].append({'name': name, 'argv': argv, 'returncode': result.returncode,
        'expected': expected, 'start_ns': start, 'end_ns': time.time_ns(),
        'stdout_sha256': sha(RUN / (name + '.stdout')), 'stderr_sha256': sha(RUN / (name + '.stderr'))})
    (RUN / 'receipt.json').write_text(json.dumps(manifest, indent=2) + '\n')
    print(name, result.returncode, flush=True)
    if result.returncode != expected:
        print((RUN / (name + '.stderr')).read_text(), flush=True)
        raise RuntimeError(name)

common = ['g++', '-std=c++20', '-pthread', '-Wall', '-Wextra', '-Wpedantic', '-Werror',
          '-isystem', str(ROOT / 'build/v7_boost_gate/extracted/usr/include')]
if MODE in ['o2', 'san']:
    tree = BASE / 'combined'
    manifest['sources_before'] = sources(tree)
    flags = ['-O2'] if MODE == 'o2' else ['-O1', '-g', '-fsanitize=address,undefined',
        '-fno-omit-frame-pointer', '-fno-pie', '-no-pie']
    env = os.environ.copy()
    if MODE == 'san':
        env.update(ASAN_OPTIONS='detect_leaks=1:halt_on_error=1', UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1')
        manifest['sanitizer_env'] = {name: env[name] for name in ['ASAN_OPTIONS', 'UBSAN_OPTIONS']}
    for gate in ['full_ball_tower_gate', 'full_ball_work_gate', 'full_coverage_certificate_gate', 'facet_resolver_cache_gate']:
        executable = RUN / gate
        run(gate + '_compile', common + flags + ['-MMD', '-MF', str(RUN / (gate + '.d')),
            str(tree / 'tests' / (gate + '.cpp')), '-o', str(executable)])
        run(gate + '_selftest', [str(executable), '--selftest'], env=env)
        run(gate + '_unknown', [str(executable), '--unknown'], 2, env)
    manifest['sources_after'] = sources(tree)
elif MODE == 'probe_build':
    manifest['sources_before'] = {**sources(BASE / 'baseline'), **sources(BASE / 'combined')}
    for kind in ['baseline', 'combined']:
        run(kind + '_compile', common + ['-O2', str(BASE / kind / 'bench/full_ball_tower_probe.cpp'),
            '-o', str(RUN / kind)])
    manifest['sources_after'] = {**sources(BASE / 'baseline'), **sources(BASE / 'combined')}
elif MODE == 'n1000':
    manifest['sources_before'] = {**sources(BASE / 'baseline'), **sources(BASE / 'combined')}
    results = []
    run('process_context_before', ['ps', '-eo', 'pid,ppid,pcpu,pmem,etime,comm', '--sort=-pcpu'])
    for kind in ['baseline', 'combined']:
        run(kind, ['/usr/bin/time', '-v', str(BASE / 'run_probe_build' / kind), '--n=1000', '--s=8', '--kmax=10', '--threads=1'])
        row = json.loads((RUN / (kind + '.stdout')).read_text())
        row['variant'] = kind
        match = re.search(r'Maximum resident set size \(kbytes\): (\d+)', (RUN / (kind + '.stderr')).read_text())
        if not match:
            raise RuntimeError('RSS missing')
        row['max_rss_kib'] = int(match.group(1))
        results.append(row)
        (RUN / 'results.json').write_text(json.dumps(results, indent=2) + '\n')
    a, b = results
    for field in ['input_digest', 'payload_digest', 'nodes', 'parent_refs', 'contributions', 'vertical_refs']:
        if a[field] != b[field]:
            raise RuntimeError('paired output mismatch: ' + field)
    if not b['resolver_meb_calls'] < a['resolver_meb_calls'] or not b['resolver_supports'] < a['resolver_supports']:
        raise RuntimeError('cache work reduction is vacuous')
    if b['cache_released_slots'] != b['cache_slots']:
        raise RuntimeError('cache allocation not released before final result')
    if not b['output_arenas_with_vertical_capacity_bytes'] < a['output_arenas_with_vertical_capacity_bytes']:
        raise RuntimeError('exact reservation memory saving is vacuous')
    run('process_context_after', ['ps', '-eo', 'pid,ppid,pcpu,pmem,etime,comm', '--sort=-pcpu'])
    manifest['sources_after'] = {**sources(BASE / 'baseline'), **sources(BASE / 'combined')}
else:
    raise ValueError(MODE)
manifest['active_after'] = active_pins()
manifest['antidrift'] = manifest['sources_before'] == manifest['sources_after'] and manifest['active_before'] == manifest['active_after']
if not manifest['antidrift']:
    raise RuntimeError('source drift')
(RUN / 'receipt.json').write_text(json.dumps(manifest, indent=2) + '\n')
print('completed', MODE, flush=True)
