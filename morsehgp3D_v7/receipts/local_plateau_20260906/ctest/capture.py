#!/usr/bin/env python3
"""Fresh local CMake qualification, no cloud or benchmarks."""
import hashlib
import json
import os
from pathlib import Path
import signal
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parents[2]
BASE = Path(__file__).resolve().parent
OUT = BASE / 'run_r1'
BUILD = BASE / 'build_r1'


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def save(name, value):
    with (OUT / name).open('x') as stream:
        json.dump(value, stream, indent=2, sort_keys=True)
        stream.write('\n')


def main():
    OUT.mkdir()
    if BUILD.exists():
        raise ValueError('fresh build required')
    source = ROOT / 'morsehgp3D_v7'
    names = {source / 'CMakeLists.txt', source / 'cmake/run_expect.cmake'}
    for folder in ('src', 'tests', 'oracle'):
        names.update(path for path in (source / folder).rglob('*')
                     if path.is_file() and path.suffix in ('.cpp', '.hpp', '.h'))
    before = {path.relative_to(ROOT).as_posix(): sha(path) for path in sorted(names)}
    save('source_candidates_before.json', before)
    commands = []
    status = 'failed'
    try:
        for name, argv in (
            ('configure', ['cmake', '-S', str(source), '-B', str(BUILD),
                           '-DCMAKE_BUILD_TYPE=Release', '-DCMAKE_EXPORT_COMPILE_COMMANDS=ON',
                           '-DMHGP7_DIGEST_BOOST_INCLUDE_DIR=' + str(ROOT / 'build/v7_boost_gate/extracted/usr/include')]),
            ('build', ['cmake', '--build', str(BUILD), '--target', 'mhgp7_local_plateau_gate', '--parallel', '2']),
            ('inventory', ['ctest', '--test-dir', str(BUILD), '-N', '--show-only=json-v1', '-R', '^mhgp7_local_plateau']),
            ('ctest', ['ctest', '--test-dir', str(BUILD), '-V', '--output-on-failure',
                       '--output-junit', str(OUT / 'junit.xml'), '-R', '^mhgp7_local_plateau']),
        ):
            started = time.time_ns()
            with (OUT / (name + '.stdout')).open('xb') as stdout, (OUT / (name + '.stderr')).open('xb') as stderr:
                child = subprocess.Popen(argv, cwd=ROOT, stdout=stdout, stderr=stderr, start_new_session=True)
                try:
                    code = child.wait()
                finally:
                    if child.poll() is None:
                        os.killpg(child.pid, signal.SIGTERM)
                        try:
                            child.wait(timeout=5)
                        except subprocess.TimeoutExpired:
                            os.killpg(child.pid, signal.SIGKILL)
                            child.wait()
            row = dict(argv=argv, cwd=str(ROOT), pid=child.pid, exit_code=code,
                       started_ns=started, ended_ns=time.time_ns(), imposed_timeout=None,
                       stdout_sha256=sha(OUT / (name + '.stdout')),
                       stderr_sha256=sha(OUT / (name + '.stderr')))
            commands.append(row)
            save(name + '.command.json', row)
            print(name, code, flush=True)
            if code != 0:
                raise ValueError(name + ' failed')
        depfile = BUILD / 'CMakeFiles/mhgp7_local_plateau_gate.dir/tests/local_plateau_gate.cpp.o.d'
        dependencies = depfile.read_text().replace('\\\n', ' ').split(':', 1)[1].split()
        project, external = {}, {}
        for name in dependencies:
            path = Path(name).resolve()
            if path.is_relative_to(source):
                key = path.relative_to(ROOT).as_posix()
                if key not in before or sha(path) != before[key]:
                    raise ValueError('project source changed: ' + key)
                project[key] = before[key]
            else:
                external[str(path)] = sha(path)
        for name in ('CMakeLists.txt', 'cmake/run_expect.cmake'):
            path = source / name
            key = path.relative_to(ROOT).as_posix()
            if sha(path) != before[key]:
                raise ValueError('CMake control changed')
            project[key] = before[key]
        save('project_sources.json', project)
        save('external_sources_observed_after.json', external)
        for relative in ('compile_commands.json', 'CMakeFiles/mhgp7_local_plateau_gate.dir/flags.make',
                         'CMakeFiles/mhgp7_local_plateau_gate.dir/tests/local_plateau_gate.cpp.o.d'):
            target = OUT / relative
            target.parent.mkdir(parents=True, exist_ok=True)
            with target.open('xb') as stream:
                stream.write((BUILD / relative).read_bytes())
        status = 'completed'
    finally:
        save('receipt.json', dict(status=status, commands=commands, GCP_used=False,
            new_time_quotas=False, public_status='not_claimed', scope='two local quotient CTests only',
            recorder_sha256=sha(Path(__file__))))
    return 0


if __name__ == '__main__':
    sys.exit(main())
