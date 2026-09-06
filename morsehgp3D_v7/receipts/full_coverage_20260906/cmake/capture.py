#!/usr/bin/env python3
"""Fresh Release CMake: three explicit targets, six tests, no benchmarks."""
import hashlib
import json
import os
from pathlib import Path
import signal
import subprocess
import sys
import time
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[2]
BASE = Path(__file__).resolve().parent
OUT = BASE / 'run_r1'
BUILD = BASE / 'build_r1'
TARGETS = ('mhgp7_full_coverage_certificate_gate', 'mhgp7_local_plateau_gate', 'mhgp7_full_certificate_gate')
TESTS = {'mhgp7_full_coverage_certificate': 0, 'mhgp7_full_coverage_certificate_bad_argument': 2,
         'mhgp7_local_plateau': 0, 'mhgp7_local_plateau_bad_argument': 2,
         'mhgp7_full_certificate': 0, 'mhgp7_full_certificate_rejects': 0}
PATTERN = '^(' + '|'.join(TESTS) + ')$'


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
    error = None
    binaries = {}
    try:
        for name, argv in (
            ('configure', ['cmake', '-S', str(source), '-B', str(BUILD),
                           '-DCMAKE_BUILD_TYPE=Release', '-DCMAKE_EXPORT_COMPILE_COMMANDS=ON',
                           '-DMHGP7_DIGEST_BOOST_INCLUDE_DIR=' + str(ROOT / 'build/v7_boost_gate/extracted/usr/include')]),
            ('build', ['cmake', '--build', str(BUILD), '--target', *TARGETS, '--parallel', '1']),
            ('inventory', ['ctest', '--test-dir', str(BUILD), '-N', '--show-only=json-v1', '-R', PATTERN]),
            ('ctest', ['ctest', '--test-dir', str(BUILD), '-V', '--output-on-failure',
                       '--output-junit', str(OUT / 'junit.xml'), '-R', PATTERN, '--parallel', '1']),
        ):
            started = time.time_ns()
            row = dict(argv=argv, cwd=str(ROOT), pid=None, exit_code=None, closed=False,
                       started_ns=started, imposed_timeout=None, added_resource_limits=False)
            try:
                with (OUT / (name + '.stdout')).open('xb') as stdout, (OUT / (name + '.stderr')).open('xb') as stderr:
                    child = subprocess.Popen(argv, cwd=ROOT, stdout=stdout, stderr=stderr, start_new_session=True)
                    row['pid'] = child.pid
                    try:
                        row['exit_code'] = child.wait()
                    except BaseException:
                        row['interrupted'] = True
                        os.killpg(child.pid, signal.SIGKILL)
                        row['exit_code'] = child.wait()
                        raise
                    finally:
                        try:
                            os.killpg(child.pid, 0)
                            os.killpg(child.pid, signal.SIGKILL)
                        except ProcessLookupError:
                            row['closed'] = True
            finally:
                row.update(ended_ns=time.time_ns(),
                           stdout_sha256=sha(OUT / (name + '.stdout')),
                           stderr_sha256=sha(OUT / (name + '.stderr')))
                commands.append(row)
                save(name + '.command.json', row)
                print(name, row['exit_code'], 'closed=' + str(row['closed']), flush=True)
            if row['exit_code'] != 0 or not row['closed']:
                raise ValueError(name + ' failed')
            if name == 'build':
                binaries = {target: sha(BUILD / target) for target in TARGETS}
            if name == 'inventory':
                tests = json.loads((OUT / 'inventory.stdout').read_text())['tests']
                if len(tests) != 6 or {test['name'] for test in tests} != set(TESTS):
                    raise ValueError('exact six-test inventory')
                for test in tests:
                    if '-DEXPECTED=' + str(TESTS[test['name']]) not in test['command']:
                        raise ValueError('exact expected exit code')
        cases = ET.parse(OUT / 'junit.xml').getroot().findall('.//testcase')
        if len(cases) != 6 or {case.attrib['name'] for case in cases} != set(TESTS):
            raise ValueError('exact six executed tests')
        if any(case.find('failure') is not None or case.find('error') is not None or case.find('skipped') is not None for case in cases):
            raise ValueError('test failure or skip')
        dependencies = []
        artifacts = ['compile_commands.json']
        for target in TARGETS:
            relative = 'CMakeFiles/' + target + '.dir/tests/' + target.removeprefix('mhgp7_') + '.cpp.o.d'
            dependencies.extend((BUILD / relative).read_text().replace('\\\n', ' ').split(':', 1)[1].split())
            artifacts.extend((relative, 'CMakeFiles/' + target + '.dir/flags.make'))
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
        after = {path.relative_to(ROOT).as_posix(): sha(path) for path in sorted(names)}
        save('source_candidates_after.json', after)
        if before != after or binaries != {target: sha(BUILD / target) for target in TARGETS}:
            raise ValueError('source or binary drift')
        for relative in artifacts:
            target = OUT / relative
            target.parent.mkdir(parents=True, exist_ok=True)
            with target.open('xb') as stream:
                stream.write((BUILD / relative).read_bytes())
        status = 'completed'
    except BaseException as exc:
        error = type(exc).__name__ + ': ' + str(exc)
    finally:
        save('receipt.json', dict(status=status, commands=commands, GCP_used=False,
            cpp_closed=all(row['closed'] for row in commands), expected_tests=TESTS, binaries=binaries,
            new_time_quotas=False, public_status='not_claimed', scope='three targets and six explicit CTests only',
            recorder_sha256=sha(Path(__file__)), error=error))
    return 0 if status == 'completed' else 1


if __name__ == '__main__':
    sys.exit(main())
