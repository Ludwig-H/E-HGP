#!/usr/bin/env python3
"""Independent q4 seed/cell join: immutable inputs, full records, closed receipts."""
import argparse
import gzip
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import struct
import subprocess
import time

BASE = Path(__file__).resolve().parent
ROOT = BASE.parents[2]


def require(value, message):
    if not value:
        raise RuntimeError(message)


def load_module(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


ORACLE = load_module('_join_oracle', BASE.parent/'q34_global_contract_20260921/oracle.py')


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def write(path, obj):
    path.write_text(json.dumps(obj, sort_keys=True, indent=2)+'\n')


def load(path):
    raw = path.read_bytes()
    return json.loads(gzip.decompress(raw) if path.suffix == '.gz' else raw)


def execute(command, timeout=240):
    env = dict(os.environ, PYTHONDONTWRITEBYTECODE='1',
               ASAN_OPTIONS='detect_leaks=1:halt_on_error=1',
               UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1')
    start = time.monotonic()
    try:
        result = subprocess.run(command, capture_output=True, text=True, timeout=timeout, env=env)
        return dict(command=command, returncode=result.returncode, stdout=result.stdout,
                    stderr=result.stderr, wall_seconds=time.monotonic()-start)
    except BaseException as error:
        def decoded(value):
            return value.decode(errors='replace') if isinstance(value, bytes) else (value or '')
        return dict(command=command, returncode=None, error=repr(error),
                    stdout=decoded(getattr(error, 'stdout', '')),
                    stderr=decoded(getattr(error, 'stderr', '')), wall_seconds=time.monotonic()-start)


def append(folder, manifest, record):
    path = folder/f'{len(manifest["records"]):04d}.json.gz'
    path.write_bytes(gzip.compress(json.dumps(record, sort_keys=True).encode(), mtime=0))
    manifest['records'].append(dict(path=path.name, sha256=sha(path)))
    write(folder/'MANIFEST.json', manifest)
    if record['returncode'] != 0 or record['stderr']:
        backup = folder/'failed_source'
        backup.mkdir(exist_ok=True)
        for name in ('probe.cpp', 'campaign.py', 'math_checks.py'):
            if (BASE/name).exists():
                (backup/name).write_bytes((BASE/name).read_bytes())
        raise RuntimeError('Failed command preserved: '+str(record['command']))


def flags():
    return ['-std=c++20', '-Wall', '-Wextra', '-Wpedantic', '-Werror',
            '-fno-access-control', '-I', str(ROOT/'morsehgp3D_v8/src')]


def libraries():
    return [ROOT/'build'/b/'libmhgp8_p0.a' for b in
            ('v8_q34_indexed_20260921', 'v8_q34_indexed_sanitize_20260921')]


def pins():
    result = subprocess.run(['g++', *flags(), '-MM', str(BASE/'probe.cpp')],
                            capture_output=True, text=True, check=True)
    paths = {Path(x) for x in result.stdout.replace('\\\n', ' ').split(':', 1)[1].split()}
    paths.update(libraries())
    paths.update(BASE/name for name in ('campaign.py', 'math_checks.py'))
    paths.update(BASE.parent/p for p in ('q34_global_contract_20260921/oracle.py',
                'q4_center_blocks_20260920/oracle_gate.py', 'q4_center_blocks_20260920/fixtures.py'))
    require(sha(BASE/'snapshot/q4_local.cpp') == sha(ROOT/'morsehgp3D_v8/src/lanes/q4_local.cpp'),
            'Explicit q4 local snapshot differs from reviewed product')
    return {str(p.resolve().relative_to(ROOT)): sha(p) for p in sorted(paths)}


def save(name, points):
    folder = BASE/'.inputs'
    folder.mkdir(exist_ok=True)
    path = folder/(name+'.u16le')
    raw = b''.join(struct.pack('<HHH', *p) for p in points)
    require(not path.exists() or path.read_bytes() == raw, 'Input overwrite refused')
    path.write_bytes(raw)
    return path


def close(folder, manifest, before):
    require(before == pins(), 'Sources/library changed during run')
    for name, expected in manifest.get('binaries', {}).items():
        require(sha(ROOT/name) == expected, 'Executable changed')
    manifest['status'] = 'completed'
    write(folder/'MANIFEST.json', manifest)
    write(folder/'COMPLETION.json', dict(status='completed', records=len(manifest['records']),
                                       manifest_sha256=sha(folder/'MANIFEST.json')))


def begin(folder, scope):
    require(not folder.exists(), 'Receipt destination already exists')
    folder.mkdir(parents=True)
    before = pins()
    manifest = dict(schema='mhgp8_audit_q4_seed_cell_join_v1', status='started',
                    public_status='not_claimed', scope=scope, sources=before, records=[],
                    cpu_affinity=sorted(os.sched_getaffinity(0)),
                    timing_scope='Serial local probes, concurrent workspace load; no global timing claim')
    write(folder/'MANIFEST.json', manifest)
    return before, manifest


def small_cases():
    for name, points in ORACLE.fixtures():
        if len(points) >= 2:
            yield name, points
    yield 'coincident_seed_lines', [(15,20,20),(24,23,20),(20,15,20),(23,16,20)]
    shell = next(p for name, p in ORACLE.fixtures() if name == 'shell30')
    yield 'shell30_inside', shell+[(20,20,20)]


def qualify(folder):
    before, manifest = begin(folder, 'Small exact Fraction oracle; reference/alive/join records and contacts')
    build = BASE/'.build'/folder.name
    require(not build.exists(), 'Build exists')
    build.mkdir(parents=True)
    commands = [['g++', '--version'], ['clang++', '--version'],
                ['g++', *flags(), '-O2', str(BASE/'probe.cpp'), str(libraries()[0]),
                 '-pthread', '-o', str(build/'release')],
                ['clang++', *flags(), '-O1', '-g', '-fsanitize=address,undefined',
                 '-fno-omit-frame-pointer', str(BASE/'probe.cpp'), str(libraries()[1]),
                 '-pthread', '-o', str(build/'sanitize')]]
    commands += [['python3', '-B', *options, str(BASE/'math_checks.py')] for options in ([], ['-O'])]
    for command in commands:
        append(folder, manifest, execute(command))
        print('PASS command', len(manifest['records']), flush=True)
    manifest['binaries'] = {str((build/b).relative_to(ROOT)): sha(build/b) for b in ('release', 'sanitize')}
    counts = dict(calls=0, outputs=0, max_shell=0)
    for name, points in small_cases():
        path = save(name, points)
        all_expected = ORACLE.expected(points, 10, 4, counts)
        edges = sorted({ORACLE.owner(points, tuple(r['support'])) for r in all_expected})[:2]
        if (0, 1) not in edges:
            edges.append((0, 1))
        for a, b in edges:
            for k in (3, 5, 10):
                expected = ORACLE.normalize([r for r in all_expected if r['depth'] < k-2
                    and ORACLE.owner(points, tuple(r['support'])) == (a, b)])
                for grain in (1, 8, 64):
                    for binary in (build/'release', build/'sanitize'):
                        command = [str(binary), str(path), str(k), str(a), str(b), str(grain)]
                        record = dict(case=name, n=len(points), kmax=k, edge=[a,b], grain=grain,
                                      input_sha256=sha(path), **execute(command))
                        append(folder, manifest, record)
                        data = json.loads(record['stdout'])
                        require(ORACLE.normalize(data['records']) == expected, 'Independent oracle differs: '+name)
                        counts['calls'] += 1
                        counts['outputs'] += len(expected)
                        counts['max_shell'] = max([counts['max_shell']]+[len(r[-1]) for r in expected])
        print('PASS oracle', name, flush=True)
    require(counts['outputs'] > 0 and counts['max_shell'] == 30, 'Vacuous small qualification')
    manifest['counts'] = counts
    close(folder, manifest, before)


def lidar_cases():
    for scan in (0, 100, 200):
        directory = BASE.parent/'lidar08_20260914/prepared'/f'single_{scan:06d}'
        declared = {x['n']:x['sha256'] for x in load(directory/'METADATA.json')['samples']}
        small = list(struct.iter_unpack('<HHH', (directory/'n8000.u16le').read_bytes()))
        order = sorted(range(1, len(small)), key=lambda i:(ORACLE.distance(small[0], small[i]), i))
        for n in (8000, 16000, 32000):
            path = directory/f'n{n}.u16le'
            require(sha(path) == declared[n], 'LiDAR hash changed')
            require(list(struct.iter_unpack('<HHH', path.read_bytes()[:48000])) == small, 'Nested prefix changed')
            for rank in (32, 512):
                for k in (5, 10):
                    for grain in ((8, 64) if n == 8000 else (64,)):
                        yield f'lidar_{scan:06d}_{n}_rank{rank}', path, n, (0, order[rank-1]), k, grain


def measure(folder, binary):
    before, manifest = begin(folder, 'Selected separate-scan LiDAR edges; no full pipeline or representativeness claim')
    manifest['binaries'] = {str(binary.relative_to(ROOT)):sha(binary)}
    for name, path, n, (a,b), k, grain in lidar_cases():
        command = [str(binary), str(path), str(k), str(a), str(b), str(grain)]
        append(folder, manifest, dict(case=name, n=n, kmax=k, edge=[a,b], grain=grain,
               rank_selected_at_n=8000, input_sha256=sha(path), **execute(command)))
        print('PASS LiDAR', name, k, grain, flush=True)
    close(folder, manifest, before)


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('mode', choices=['qualify','measure'])
    parser.add_argument('folder', type=Path)
    parser.add_argument('--binary', type=Path)
    args = parser.parse_args()
    if args.mode == 'qualify':
        qualify(args.folder.resolve())
    else:
        require(args.binary is not None, 'measure requires --binary')
        measure(args.folder.resolve(), args.binary.resolve())
