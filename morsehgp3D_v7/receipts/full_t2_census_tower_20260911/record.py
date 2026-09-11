#!/usr/bin/env python3
"""Record private T2 compiler/test invocations without changing closed evidence."""
import hashlib
import json
import os
import pathlib
import shutil
import subprocess
import sys
import time

HERE = pathlib.Path(__file__).resolve().parent
BOOST = pathlib.Path('/workspaces/E-HGP/build/v7_boost_gate/extracted/usr/include')


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    if len(sys.argv) != 3 or sys.argv[2] not in ('o2', 'san'):
        return 2
    destination = HERE / sys.argv[1]
    destination.mkdir()
    source = destination / 'source'
    source.mkdir()
    shutil.copytree(HERE / 'source', source / 'source')
    for name in ('t2_gate.cpp', 't2_oracle.hpp'):
        shutil.copyfile(HERE / name, source / name)
    paths = sorted(p for p in source.rglob('*') if p.is_file())
    pins = {str(p.relative_to(source)): digest(p) for p in paths}
    (destination / 'sources.before.json').write_text(json.dumps(pins, indent=2) + '\n')
    flags = ['-O2'] if sys.argv[2] == 'o2' else [
        '-O1', '-g', '-fno-omit-frame-pointer', '-fsanitize=address,undefined',
        '-fno-sanitize-recover=all']
    binary = destination / 'gate'
    commands = [('compile', ['g++', '-std=c++20', '-Wall', '-Wextra', '-Wpedantic', '-Werror',
        '-pthread', *flags, '-isystem', str(BOOST), '-MMD', '-MF', str(destination / 'gate.d'),
        str(source / 't2_gate.cpp'), '-o', str(binary)], 0, None)]
    commands += [(name, [str(binary), '--' + name], 0, None)
                 for name in ('historical', 'line12', 'shell14', 'spatial12', 'rejects')]
    commands += [(name, [str(binary), '--' + name], 1, diagnostic) for name, diagnostic in (
        ('mutant-assignment', 'T2 unassigned subset'),
        ('mutant-open', 'T2.historical.exact_Gamma'),
        ('mutant-adjacency', 'T2.historical.exact_Gamma'),
        ('mutant-census', 'T2.census.exhaustive_ball_inventory'))]
    commands.append(('invalid-args', [str(binary), '--invalid'], 2, None))
    environment = dict(os.environ, ASAN_OPTIONS='detect_leaks=1:halt_on_error=1',
                       UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1')
    result = []
    for name, command, expected, diagnostic in commands:
        print('start=' + name, flush=True)
        (destination / (name + '.intent.json')).write_text(json.dumps({
            'command': command, 'cwd': str(HERE), 'expected_exit': expected,
            'causal_diagnostic': diagnostic,
            'sanitizer_environment': {k: environment[k] for k in ('ASAN_OPTIONS', 'UBSAN_OPTIONS')}}, indent=2) + '\n')
        start = time.monotonic()
        with (destination / (name + '.stdout')).open('wb') as out, \
                (destination / (name + '.stderr')).open('wb') as err:
            process = subprocess.run(command, cwd=HERE, env=environment, stdout=out, stderr=err, check=False)
        causal = diagnostic is None or diagnostic in (destination / (name + '.stderr')).read_text()
        row = {'name': name, 'exit': process.returncode, 'expected_exit': expected,
               'causal_diagnostic_matched': causal, 'elapsed_s': time.monotonic() - start}
        result.append(row)
        (destination / (name + '.result.json')).write_text(json.dumps(row, indent=2) + '\n')
        print(json.dumps(row), flush=True)
        if process.returncode != expected or not causal:
            break
    after = {str(p.relative_to(source)): digest(p) for p in paths}
    (destination / 'sources.after.json').write_text(json.dumps(after, indent=2) + '\n')
    passed = pins == after and len(result) == len(commands) and all(
        row['exit'] == row['expected_exit'] and row['causal_diagnostic_matched'] for row in result)
    (destination / 'summary.json').write_text(json.dumps({
        'status': 'passed' if passed else 'failed', 'stable_sources': pins == after,
        'commands': result, 'binary_sha256': digest(binary) if binary.exists() else None,
        'scope': 'private_bounded_real_census_FULL_K1_K10_not_universal_completeness'
    }, indent=2) + '\n')
    return 0 if passed else 1


if __name__ == '__main__':
    sys.exit(main())
