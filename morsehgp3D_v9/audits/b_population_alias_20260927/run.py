#!/usr/bin/env python3
"""Fresh local reproduction, with retained command failures and source closure."""
from __future__ import annotations

import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import time

HERE = Path(__file__).resolve().parent
V9 = HERE.parent.parent
REPO = V9.parent
BUILD = Path('/workspaces/E-HGP/build/v9-audit-population-alias-20260927')
RECEIPT = V9 / 'receipts/population_alias_20260927'
COMMON = ['-std=c++20', '-Wall', '-Wextra', '-Wpedantic', '-Werror',
          '-pthread', '-I', str(V9 / 'src')]
EXPECTED = {
    'schema': 'mhgp9_population_alias_audit_v2', 'status': 'reproduced',
    'copy_isolated': True, 'move_row_alias_retained': True,
    'move_point_alias_retained': True, 'revalidation_rejects': True,
    'certified_forest_changed': True, 'rebuilt_forest_accepts': True,
    'before': [0, 1], 'after': [1, 99], 'GCP_used': False,
}


def need(value: bool, message: str) -> None:
    if not value:
        raise RuntimeError(message)


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def recipe() -> list[tuple[str, list[str], int]]:
    source = str(HERE / 'repro.cpp')
    return [
        ('version_release', ['c++', '--version'], 0),
        ('version_sanitize', ['clang++', '--version'], 0),
        ('dependencies_release', ['c++', *COMMON, '-MM', source], 0),
        ('dependencies_sanitize', ['clang++', *COMMON, '-MM', source], 0),
        ('build_release', ['c++', *COMMON, '-O3', '-DNDEBUG', source,
                           '-o', str(BUILD / 'release')], 0),
        ('build_sanitize', ['clang++', *COMMON, '-O1', '-g',
                            '-fno-omit-frame-pointer', '-fsanitize=address,undefined',
                            '-fno-sanitize-recover=all', source,
                            '-o', str(BUILD / 'sanitize')], 0),
        ('reproduce_release', [str(BUILD / 'release')], 0),
        ('reproduce_sanitize', [str(BUILD / 'sanitize')], 0),
        ('expected_shift_stop', [str(BUILD / 'sanitize'), '--shift'], 1),
    ]


def save(manifest: dict) -> None:
    (RECEIPT / 'manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')


def run() -> None:
    need(not BUILD.exists(), 'build already exists; do not overwrite a capture')
    need(not RECEIPT.exists(), 'receipt already exists; do not overwrite a capture')
    BUILD.mkdir(parents=True)
    RECEIPT.mkdir(parents=True)
    manifest = {'schema': 'mhgp9_population_alias_capture_v2', 'status': 'running',
                'commands': [], 'sources_before': {}, 'sources_after': {},
                'git_head': subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=REPO,
                                                     text=True).strip(),
                'GCP_used': False, 'restored_from_prior_artifacts': False,
                'build': str(BUILD), 'worktree': str(REPO)}
    environment = dict(os.environ, ASAN_OPTIONS='detect_leaks=1:halt_on_error=1',
                       UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1:exitcode=1')
    save(manifest)
    try:
        for index, (name, argv, expected_code) in enumerate(recipe()):
            if index == 4:
                dependencies = {HERE / 'run.py', HERE / 'repro.cpp'}
                for dep in ('dependencies_release', 'dependencies_sanitize'):
                    tokens = (RECEIPT / f'{dep}.stdout').read_text().replace('\\\n', ' ').split()
                    dependencies.update(Path(token).resolve() for token in tokens[1:])
                need(all(path.is_file() for path in dependencies), 'missing declared dependency')
                manifest['sources_before'] = {str(path): digest(path) for path in sorted(dependencies)}
                save(manifest)
            command = {'name': name, 'argv': argv, 'expected_exit': expected_code,
                       'cwd': str(REPO), 'status': 'running',
                       'environment': {key: environment[key] for key in ('ASAN_OPTIONS', 'UBSAN_OPTIONS')}}
            manifest['commands'].append(command)
            save(manifest)
            start = time.monotonic()
            with (RECEIPT / f'{name}.stdout').open('wb') as out, \
                    (RECEIPT / f'{name}.stderr').open('wb') as err:
                process = subprocess.run(argv, cwd=REPO, env=environment, stdout=out,
                                         stderr=err, check=False)
            command.update(exit_code=process.returncode, wall_s=time.monotonic() - start,
                           status='completed')
            command.update(stdout_sha256=digest(RECEIPT / f'{name}.stdout'),
                           stderr_sha256=digest(RECEIPT / f'{name}.stderr'))
            save(manifest)
            need(process.returncode == expected_code, f'{name}: unexpected exit {process.returncode}')
        manifest['sources_after'] = {path: digest(Path(path)) for path in manifest['sources_before']}
        need(manifest['sources_before'] == manifest['sources_after'], 'source drift')
        manifest['binaries'] = {str(BUILD / name): digest(BUILD / name) for name in ('release', 'sanitize')}
        manifest['status'] = 'completed'
        save(manifest)
        check()
    except BaseException as error:
        manifest['status'] = 'failed'
        manifest['failure'] = f'{type(error).__name__}: {error}'
        save(manifest)
        raise


def check() -> None:
    manifest = json.loads((RECEIPT / 'manifest.json').read_text())
    need(manifest['status'] == 'completed', 'capture incomplete')
    need(manifest['schema'] == 'mhgp9_population_alias_capture_v2', 'schema')
    need(manifest['GCP_used'] is False and manifest['restored_from_prior_artifacts'] is False, 'scope')
    need(manifest['git_head'] == 'ddf4776d754a8db59a1333e11d56b39d8cb6f51a', 'audited commit')
    need(manifest['sources_before'] == manifest['sources_after'], 'source closure')
    need(str(HERE / 'run.py') in manifest['sources_before'] and
         str(V9 / 'src/common/raw_vector.hpp') in manifest['sources_before'], 'dependency coverage')
    for path, expected in {**manifest['sources_before'], **manifest['binaries']}.items():
        need(digest(Path(path)) == expected, f'live hash: {path}')
    need(len(manifest['commands']) == len(recipe()), 'command count')
    for record, (name, argv, code) in zip(manifest['commands'], recipe()):
        need(record['name'] == name and record['argv'] == argv, 'recipe mismatch')
        need(record['exit_code'] == code and record['expected_exit'] == code and
             record['status'] == 'completed' and record['cwd'] == str(REPO), 'command status')
        need(record['environment'] == {'ASAN_OPTIONS': 'detect_leaks=1:halt_on_error=1',
             'UBSAN_OPTIONS': 'halt_on_error=1:print_stacktrace=1:exitcode=1'}, 'sanitizer environment')
        for stream in ('stdout', 'stderr'):
            need(digest(RECEIPT / f'{name}.{stream}') == record[f'{stream}_sha256'], 'output hash')
    for name in ('reproduce_release', 'reproduce_sanitize'):
        need(json.loads((RECEIPT / f'{name}.stdout').read_text()) == EXPECTED, 'alias outcome')
        need((RECEIPT / f'{name}.stderr').read_bytes() == b'', 'unexpected reproduction diagnostic')
    need(json.loads((RECEIPT / 'expected_shift_stop.stdout').read_text()) ==
         {'stage': 'before_product_shift', 'shell_size': 32}, 'shift setup')
    diagnostic = (RECEIPT / 'expected_shift_stop.stderr').read_text()
    need('full_coverage_certificate.hpp:272:' in diagnostic and
         'shift exponent 32 is too large for 32-bit type' in diagnostic, 'expected product UB absent')
    print(json.dumps({'status': 'passed', 'commands': len(recipe()),
                      'sources': len(manifest['sources_before']), 'GCP_used': False}))


if __name__ == '__main__':
    need(len(sys.argv) == 2 and sys.argv[1] in ('run', 'check'), 'usage: run.py run|check')
    run() if sys.argv[1] == 'run' else check()
