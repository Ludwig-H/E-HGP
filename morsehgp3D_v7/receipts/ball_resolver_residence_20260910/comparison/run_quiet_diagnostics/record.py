#!/usr/bin/env python3
"""Private candidate qualification and sequential paired diagnostics."""
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import statistics
import subprocess
import sys
import time

ROOT = Path('/workspaces/E-HGP')
BASE = ROOT / 'build/v7_ball_resolver_opt_20260910_r2'
MODE = sys.argv[1]
RUN = BASE / ('run_quiet_' + MODE)
RUN.mkdir(exist_ok=False)
manifest = {'schema': 'private-full-resolver-cache-v1', 'commands': [], 'gcp_used': False}


def sources(tree):
    return {str(path.relative_to(BASE)): hashlib.sha256(path.read_bytes()).hexdigest()
            for path in sorted(tree.rglob('*')) if path.is_file() and path.suffix in ['.cpp', '.hpp', '.cu', '.cuh']}


def run(name, argv, expected=0, env=None):
    started = time.time_ns()
    with (RUN / (name + '.stdout')).open('wb') as out, (RUN / (name + '.stderr')).open('wb') as err:
        result = subprocess.run(argv, cwd=ROOT, stdout=out, stderr=err, env=env, check=False)
    command = {'name': name, 'argv': argv, 'returncode': result.returncode, 'expected': expected,
               'start_ns': started, 'end_ns': time.time_ns()}
    for stream in ['stdout', 'stderr']:
        command[stream + '_sha256'] = hashlib.sha256((RUN / (name + '.' + stream)).read_bytes()).hexdigest()
    manifest['commands'].append(command)
    (RUN / 'receipt.json').write_text(json.dumps(manifest, indent=2) + '\n')
    print(name, result.returncode, flush=True)
    if result.returncode != expected:
        print((RUN / (name + '.stderr')).read_text(), flush=True)
        raise RuntimeError(name)


shutil.copy2(Path(__file__), RUN / 'record.py')
common = ['g++', '-std=c++20', '-pthread', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-isystem',
          str(ROOT / 'build/v7_boost_gate/extracted/usr/include')]
if MODE.endswith('_o2') or MODE.endswith('_san'):
    kind, build = MODE.rsplit('_', 1)
    tree = BASE / ('baseline_final' if kind == 'baseline' else 'cache_final')
    flags = ['-O2'] if build == 'o2' else ['-O1', '-g', '-fsanitize=address,undefined', '-fno-omit-frame-pointer', '-fno-pie', '-no-pie']
    if kind != 'baseline': flags += ['-DMHGP7_PRIVATE_RESOLVER_SEED=' + ('1' if kind == 'seed' else '0')]
    manifest['sources_before'] = sources(tree)
    env = os.environ.copy()
    if build == 'san':
        env['ASAN_OPTIONS'] = 'detect_leaks=1:halt_on_error=1'
        env['UBSAN_OPTIONS'] = 'halt_on_error=1:print_stacktrace=1'
        manifest['sanitizer_env'] = {key: env[key] for key in ['ASAN_OPTIONS', 'UBSAN_OPTIONS']}
    gates = ['full_ball_tower_gate', 'full_ball_work_gate']
    if kind != 'baseline': gates += ['resolver_cache_gate']
    for gate in gates:
        executable = RUN / gate
        run(gate + '_compile', common + flags + ['-MMD', '-MF', str(RUN / (gate + '.d')),
            str(tree / 'tests' / (gate + '.cpp')), '-o', str(executable)])
        run(gate + '_selftest', [str(executable), '--selftest'], env=env)
        run(gate + '_bad_argument', [str(executable), '--unknown'], 2, env)
    if build == 'o2':
        run('probe_compile', common + flags + [str(tree / 'bench/full_ball_tower_probe.cpp'), '-o', str(RUN / 'probe')])
    manifest['sources_after'] = sources(tree)
    manifest['antidrift'] = manifest['sources_before'] == manifest['sources_after']
    if not manifest['antidrift']: raise RuntimeError('source drift')
elif MODE == 'mutant':
    tree = BASE / 'mutant_final'
    shutil.copytree(BASE / 'cache_final', tree)
    header = tree / 'src/forest/full_ball_tower.hpp'
    source = header.read_text()
    before = 'if (entry.token == absent || entry.sites != key) return absent;'
    if source.count(before) != 1: raise RuntimeError('mutant site count')
    header.write_text(source.replace(before, 'if (entry.token == absent) return absent;'))
    manifest['sources_before'] = sources(tree)
    executable = RUN / 'cache_mutant'
    run('compile', common + ['-O2', str(tree / 'tests/resolver_cache_gate.cpp'), '-o', str(executable)])
    run('selftest', [str(executable), '--selftest'], 1)
    manifest['sources_after'] = sources(tree)
    manifest['antidrift'] = manifest['sources_before'] == manifest['sources_after']
elif MODE == 'diagnostics':
    manifest['sources_before'] = {**sources(BASE / 'baseline_final'), **sources(BASE / 'cache_final')}
    results = []
    run('context_before', ['ps', '-eo', 'pid,ppid,pcpu,pmem,etime,comm', '--sort=-pcpu'])
    schedule = [(1000, 0, ['baseline', 'plain', 'seed'])]
    for n, repetition, order in schedule:
        for kind in order:
            name = f'n{n}_{repetition}_{kind}'
            executable = BASE / ('run_final_' + kind + '_o2') / 'probe'
            argv = ['/usr/bin/time', '-v', str(executable), f'--n={n}', '--s=8', '--kmax=10', '--threads=1']
            run(name, argv)
            row = json.loads((RUN / (name + '.stdout')).read_text())
            row.update({'variant': kind, 'repetition': repetition})
            match = re.search(r'Maximum resident set size \(kbytes\): (\d+)', (RUN / (name + '.stderr')).read_text())
            if not match: raise RuntimeError('missing RSS')
            row['max_rss_kib'] = int(match.group(1))
            results.append(row)
            (RUN / 'results.json').write_text(json.dumps(results, indent=2) + '\n')
    run('context_after', ['ps', '-eo', 'pid,ppid,pcpu,pmem,etime,comm', '--sort=-pcpu'])
    summary = {}
    for n in [1000]:
        rows = [row for row in results if row['n'] == n and row['repetition'] >= 0]
        if len({row['payload_digest'] for row in rows}) != 1 or len({row['input_digest'] for row in rows}) != 1:
            raise RuntimeError('paired object mismatch')
        summary[n] = {}
        for kind in ['baseline', 'plain', 'seed']:
            branch = [row for row in rows if row['variant'] == kind]
            summary[n][kind] = {'tower_median_s': statistics.median(row['tower_s'] for row in branch),
                'total_median_s': statistics.median(row['total_s'] for row in branch),
                'resolver_meb_calls': branch[0]['resolver_meb_calls'], 'resolver_supports': branch[0]['resolver_supports'],
                'max_rss_kib': max(row['max_rss_kib'] for row in branch),
                'cache_bytes': branch[0].get('cache_bytes', 0), 'cache_hits': branch[0].get('cache_hits', 0)}
    (RUN / 'summary.json').write_text(json.dumps(summary, indent=2) + '\n')
    manifest['sources_after'] = {**sources(BASE / 'baseline_final'), **sources(BASE / 'cache_final')}
    manifest['antidrift'] = manifest['sources_before'] == manifest['sources_after']
    if not manifest['antidrift']: raise RuntimeError('diagnostic source drift')
else:
    raise ValueError(MODE)
(RUN / 'receipt.json').write_text(json.dumps(manifest, indent=2) + '\n')
print('completed', MODE, flush=True)
