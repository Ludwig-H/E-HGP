#!/usr/bin/env python3
"""Bounded fresh CMake qualification; captures, never performance claims."""
from __future__ import annotations

import hashlib
import json
import os
from pathlib import Path
import resource
import signal
import subprocess
import time
from datetime import datetime, timezone

ROOT = Path('/workspaces/E-HGP')
BASE = Path(__file__).resolve().parent
CAPTURE = BASE / 'capture'
SOURCE = ROOT / 'morsehgp3D_v7'
TARGETS = [
    'mhgp7_full_certificate_gate', 'mhgp7_full_gabriel_gate',
    'mhgp7_full_gabriel_allocation_gate', 'mhgp7_full_gabriel_lazy_gate',
    'mhgp7_full_gabriel_lazy_allocation_gate', 'mhgp7_full_gabriel_singleton_gate',
    'mhgp7_full_gabriel_successor_gate', 'mhgp7_full_gabriel_digest_gate',
    'mhgp7_full_gabriel_meb_gate', 'mhgp7_meb_proposal_local_gate',
]


def stamp() -> str:
    return datetime.now(timezone.utc).isoformat()


def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def dump(path: Path, value: object) -> None:
    path.write_text(json.dumps(value, indent=2, sort_keys=True) + '\n')


def pins() -> dict[str, str]:
    paths = [SOURCE / 'CMakeLists.txt', Path(__file__).resolve()]
    for name in ('src', 'oracle', 'tests', 'cmake'):
        paths.extend(p for p in (SOURCE / name).rglob('*') if p.is_file())
    return {str(p.relative_to(ROOT)): sha(p) for p in sorted(paths)}


def limits() -> None:
    resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
    resource.setrlimit(resource.RLIMIT_FSIZE, (64 << 20, 64 << 20))
    resource.setrlimit(resource.RLIMIT_CPU, (1200, 1200))


def main() -> int:
    CAPTURE.mkdir(exist_ok=False)
    before = pins()
    dump(CAPTURE / 'sources_before.json', before)
    env = os.environ.copy()
    for key in ('CXXFLAGS', 'CPPFLAGS', 'LDFLAGS', 'CFLAGS', 'MAKEFLAGS'):
        env.pop(key, None)
    env.update(ASAN_OPTIONS='detect_leaks=1:halt_on_error=1',
               UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1', LC_ALL='C')
    report: dict = {'schema': 'mhgp7-full-meb-product-qualification-v1',
                    'started': stamp(), 'public_status': 'not_claimed',
                    'performance_capture': False, 'GCP': 'not_used',
                    'environment_overrides': {k: env[k] for k in
                       ('ASAN_OPTIONS', 'UBSAN_OPTIONS', 'LC_ALL')},
                    'commands': [], 'binaries': {}}
    deadline = time.monotonic() + 3600

    def run(name: str, argv: list[str], timeout: int) -> None:
        output = CAPTURE / (name + '.stdout')
        error = CAPTURE / (name + '.stderr')
        rec = {'name': name, 'argv': argv, 'cwd': str(ROOT), 'started': stamp(),
               'timeout_seconds': timeout, 'status': 'running'}
        report['commands'].append(rec)
        dump(CAPTURE / 'run.json', report)
        with output.open('wb') as out, error.open('wb') as err:
            process = subprocess.Popen(argv, cwd=ROOT, env=env, stdout=out, stderr=err,
                                       start_new_session=True, preexec_fn=limits)
            rec['pid'] = process.pid
            try:
                code = process.wait(timeout=max(1, min(timeout, deadline - time.monotonic())))
                rec.update(exit_code=code, status='completed')
            except BaseException:
                os.killpg(process.pid, signal.SIGKILL)
                process.wait()
                rec.update(exit_code=process.returncode, status='interrupted_or_timeout')
                raise
            finally:
                try:
                    os.killpg(process.pid, 0)
                except ProcessLookupError:
                    rec['process_group_closed'] = True
                else:
                    os.killpg(process.pid, signal.SIGKILL)
                    rec['process_group_closed'] = False
                rec.update(ended=stamp(), stdout_sha256=sha(output), stderr_sha256=sha(error))
                dump(CAPTURE / 'run.json', report)
        print(f'{name}: exit={code}', flush=True)
        if code != 0 or not rec['process_group_closed']:
            raise RuntimeError(f'{name}: nonzero exit or residual process group')

    try:
        run('compiler', ['/usr/bin/c++', '--version'], 10)
        run('cmake', ['cmake', '--version'], 10)
        run('git_head', ['git', 'rev-parse', 'HEAD'], 10)
        run('git_status', ['git', 'status', '--short', '--untracked-files=normal'], 10)
        for mode in ('release', 'san'):
            build = BASE / mode
            if build.exists():
                raise RuntimeError(f'fresh build required: {build}')
            options = ['-DCMAKE_BUILD_TYPE=Release', '-DCMAKE_EXPORT_COMPILE_COMMANDS=ON',
                       '-DMHGP7_DIGEST_BOOST_INCLUDE_DIR=/workspaces/E-HGP/build/v7_boost_gate/extracted/usr/include']
            if mode == 'san':
                options += ['-DCMAKE_CXX_FLAGS_RELEASE=-O1 -g -fsanitize=address,undefined '
                            '-fno-sanitize-recover=all -fno-omit-frame-pointer -fno-pie',
                            '-DCMAKE_EXE_LINKER_FLAGS=-fsanitize=address,undefined -no-pie']
            run(mode + '_configure', ['cmake', '-S', str(SOURCE), '-B', str(build)] + options, 60)
            run(mode + '_build', ['cmake', '--build', str(build), '--parallel', '2',
                                  '--target'] + TARGETS, 1200)
            report['binaries'][mode] = {name: sha(build / name) for name in TARGETS}
            run(mode + '_ctest', ['ctest', '--test-dir', str(build), '-V', '--output-on-failure',
                                  '-R', '^mhgp7_(full_|meb_proposal_)'], 600)
        report['status'] = 'passed'
    except BaseException as error:
        report.update(status='failed', failure=f'{type(error).__name__}: {error}')
        print(report['failure'], flush=True)
    finally:
        after = pins()
        dump(CAPTURE / 'sources_after.json', after)
        report['sources_stable'] = before == after
        if before != after:
            report['status'] = 'invalid_source_change'
        report['ended'] = stamp()
        dump(CAPTURE / 'run.json', report)
    return 0 if report['status'] == 'passed' else 1


if __name__ == '__main__':
    raise SystemExit(main())
