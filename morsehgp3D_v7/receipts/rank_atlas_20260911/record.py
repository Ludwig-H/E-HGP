#!/usr/bin/env python3
"""Create-only local O2/SAN qualification. No cloud, no source mutation."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import time

BASE = Path(__file__).resolve().parent


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def dump(path, value):
    path.write_text(json.dumps(value, indent=2, sort_keys=True) + '\n')


def sources():
    files = [BASE / name for name in ('atlas_gate.cpp', 'rank_atlas.hpp', 'record.py', 'README.md')]
    files.extend(sorted((BASE / 'source').rglob('*')))
    return {str(p.relative_to(BASE)): digest(p) for p in files if p.is_file()}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--out', required=True)
    parser.add_argument('--san', action='store_true')
    args = parser.parse_args()
    if Path(args.out).name != args.out:
        parser.error('out must be one new child name')
    out = BASE / args.out
    out.mkdir(exist_ok=False)
    commands = []
    before = sources()
    dump(out / 'sources_before.json', before)
    compiler = Path(shutil.which('g++') or '/usr/bin/g++').resolve()
    dump(out / 'compiler_pin.json', {'path': str(compiler), 'sha256': digest(compiler)})
    environment = os.environ.copy()
    if args.san:
        environment.update(ASAN_OPTIONS='detect_leaks=1:halt_on_error=1',
                           UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1')

    def run(name, argv, expected):
        started = time.monotonic()
        with (out / f'{name}.stdout').open('wb') as stdout, (out / f'{name}.stderr').open('wb') as stderr:
            result = subprocess.run(argv, cwd=BASE, env=environment, stdout=stdout, stderr=stderr, check=False)
        row = {'name': name, 'argv': argv, 'cwd': str(BASE), 'expected': expected,
               'returncode': result.returncode, 'elapsed_s': time.monotonic() - started}
        if args.san:
            row['environment'] = {key: environment[key] for key in ('ASAN_OPTIONS', 'UBSAN_OPTIONS')}
        commands.append(row)
        dump(out / 'commands.json', commands)
        print(json.dumps(row), flush=True)
        return result.returncode == expected

    ok = run('compiler', [str(compiler), '--version'], 0)
    flags = ['-O1', '-g', '-fsanitize=address,undefined', '-fno-omit-frame-pointer'] if args.san else ['-O2', '-DNDEBUG']
    exe = out / 'atlas_gate'
    ok = run('compile', [str(compiler), '-std=c++20', '-Wall', '-Wextra', '-Wpedantic', '-Werror',
              '-pthread', *flags, '-MMD', '-MF', str(out / 'gate.d'), str(BASE / 'atlas_gate.cpp'),
              '-o', str(exe)], 0) and ok
    if ok:
        dump(out / 'binary.json', {'path': str(exe), 'sha256': digest(exe)})
        for name, option, expected in [('selftest', '--selftest', 0), ('date', '--date', 4),
                ('offset', '--offset', 4), ('mask', '--mask', 4), ('contribution', '--contribution', 4),
                ('admission', '--admission', 4), ('permutation', '--permutation', 4), ('unknown', '--unknown', 2)]:
            ok = run(name, [str(exe), option], expected) and ok
        ok = run('missing', [str(exe)], 2) and ok
        if digest(exe) != json.loads((out / 'binary.json').read_text())['sha256']:
            ok = False
    after = sources()
    dump(out / 'sources_after.json', after)
    stable = before == after and digest(compiler) == json.loads((out / 'compiler_pin.json').read_text())['sha256']
    ok = ok and stable
    dump(out / 'receipt.json', {'status': 'passed' if ok else 'failed', 'source_stable': stable,
         'commands': len(commands), 'san': args.san, 'device_executed': False, 'gcp_used': False,
         'scope': 'bounded rank/admission/representative atlas differential, not geometry or FULL forest'})
    return 0 if ok else 1


if __name__ == '__main__':
    sys.exit(main())
