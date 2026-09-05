#!/usr/bin/env python3
"""Create-only FULLv2 probe controller. Prepared only; ROOT authorizes runs.

Commands, all relative to /workspaces/E-HGP:
  capture.py build --id NAME
  capture.py micro --id NAME --build-receipt PATH --build-sha256 SHA
  capture.py prepare-heavy --id NAME --phase paired|scale16|scale32
      --micro-receipt PATH --micro-sha256 SHA
      --qualification-receipt PATH --qualification-sha256 SHA
      [--paired-receipt PATH --paired-sha256 SHA]
  capture.py attempt --directory PATH --index N --go-reviewed-heavy
  capture.py close-heavy --directory PATH

The first two commands launch a build or 24 SMALL n=8 attempts respectively.
prepare-heavy/close-heavy launch no probe. An attempt command launches exactly
one predeclared heavy job, never its successor. A refused/censored/invalid
predecessor blocks later jobs in that paired directory. 16k and 32k each need
a separate preparation/admission from a fully successful 8k comparison.

No public writes, Git mutation, GCP, SAN build, production CLI, cap adaptation
or time-based ranking. All artifacts remain under this private controller.
This is process/provenance consistency, not a hermetic or geometric proof.
"""
from __future__ import annotations

import argparse
from datetime import datetime, timezone
import fcntl
import hashlib
import json
import os
from pathlib import Path
import platform
import re
import resource
import shlex
import signal
import subprocess
import sys
import time
from typing import Any

ROOT = Path('/workspaces/E-HGP')
BASE = ROOT / 'build/v7_full_lazy_20260905_probe_controller'
PROBE = 'morsehgp3D_v7/bench/full_gabriel_lazy_probe.cpp'
DIGEST = 'morsehgp3D_v7/bench/full_gabriel_semantic_digest.hpp'
JUDGE = 'morsehgp3D_v7/bench/full_gabriel_lazy_probe_audit.py'
PRODUCER = 'morsehgp3D_v7/src/forest/full_gabriel.hpp'
PINS = {
    PROBE: 'f21e3c70bda4cc9adaa0960ed19c9a6a8aa6b09413f6c6d63bae884c98e9486a',
    DIGEST: '671b2dfb51f1385ee7301bd6b03ef64e62c0d768c92534a6f09589726ce9adc3',
    JUDGE: '8d8a612aa973cb79e60e97a6675f63684ddd8892cfc550716c20620c4d6930ef',
    PRODUCER: (
        '13c6cc72ab5065d498827bf89c6bc2a321b5e896c93a60263de52b9d800a2627'
    ),
}
QUALIFICATION_SHA = (
    '28a203ea7f46699e9845252bc02f46c9719c2380cef3e4e95d1f5d935a0abdc8'
)
QUALIFIED_TESTS = sorted([
    'mhgp7_full_certificate', 'mhgp7_full_certificate_rejects',
    'mhgp7_full_gabriel', 'mhgp7_full_gabriel_rejects',
    'mhgp7_full_gabriel_bad_argument', 'mhgp7_full_gabriel_allocation',
    'mhgp7_full_gabriel_allocation_bad_argument',
    'mhgp7_full_gabriel_lazy', 'mhgp7_full_gabriel_lazy_rejects',
    'mhgp7_full_gabriel_lazy_bad_argument',
    'mhgp7_full_gabriel_lazy_allocation',
    'mhgp7_full_gabriel_lazy_allocation_bad_argument',
    'mhgp7_full_gabriel_digest', 'mhgp7_full_gabriel_digest_bad_argument',
])
AUTHORITY = (
    'full_horizontal_relative_to_supplied_complete_exact_regular_'
    'gabriel_catalogues'
)
SCHEMA = 'mhgp7-full-lazy-probe-controller-v1'
SIGNALS = (signal.SIGINT, signal.SIGTERM, signal.SIGHUP)
PAIRED = [
    [8000, 8, 'eager', 0], [8000, 8, 'lazy', 1000000],
    [8000, 10, 'lazy', 1000000], [8000, 10, 'eager', 0],
    [8000, 12, 'eager', 0], [8000, 12, 'lazy', 1000000],
]
Json = dict[str, Any]


def require(ok: bool, reason: str) -> None:
    if not ok:
        raise ValueError(reason)


def now() -> str:
    return datetime.now(timezone.utc).isoformat()


def sha(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open('rb') as stream:
        for data in iter(lambda: stream.read(1048576), b''):
            digest.update(data)
    return digest.hexdigest()


def unique(pairs: list[tuple[str, Any]]) -> Json:
    result: Json = {}
    for key, value in pairs:
        require(key not in result, 'duplicate_key')
        result[key] = value
    return result


def parse(text: str) -> Any:
    return json.loads(
        text, object_pairs_hook=unique,
        parse_constant=lambda _: require(False, 'nonfinite'),
    )


def read_json(path: Path) -> Json:
    require(path.stat().st_size <= 8 << 20, 'metadata_size_budget')
    value = parse(path.read_text(encoding='utf-8'))
    require(isinstance(value, dict), 'metadata_object')
    return value


def save(path: Path, value: Any) -> None:
    data = (json.dumps(value, indent=2, sort_keys=True) + '\n').encode('utf-8')
    with path.open('xb') as stream:
        stream.write(data)


def owned(path: str | Path) -> Path:
    result = Path(path).resolve()
    require(result.is_relative_to(BASE.resolve()) and result != BASE,
            'controller_path_scope')
    return result


def new_directory(kind: str, name: str) -> Path:
    require(re.fullmatch(r'[a-z0-9][a-z0-9_-]{0,63}', name) is not None,
            'run_id')
    directory = BASE / f'{kind}_{name}'
    directory.mkdir(exist_ok=False)
    return directory


def snapshot(binary: Path | None = None) -> Json:
    paths = [p for p in (ROOT / 'morsehgp3D_v7/src').rglob('*') if p.is_file()]
    paths.extend(ROOT / name for name in (PROBE, DIGEST, JUDGE))
    paths.extend([
        Path(__file__).resolve(), ROOT / 'morsehgp3D_v7/CMakeLists.txt',
    ])
    require(len(paths) < 2048, 'source_inventory_budget')
    files = {str(p.relative_to(ROOT)): sha(p) for p in sorted(set(paths))}
    for name, expected in PINS.items():
        require(files[name] == expected, 'unreviewed_source:' + name)
    result: Json = {'files': files}
    if binary is not None:
        result.update(binary=str(binary.relative_to(ROOT)),
                      binary_sha256=sha(binary))
    return result


def interrupted(signum: int, _frame: Any) -> None:
    raise InterruptedError(f'signal {signum}')


def child_limit() -> None:
    soft, hard = resource.getrlimit(resource.RLIMIT_AS)
    ceiling = 26 << 30
    for value in (soft, hard):
        if value != resource.RLIM_INFINITY:
            ceiling = min(ceiling, value)
    resource.setrlimit(resource.RLIMIT_AS, (ceiling, hard))


def drain(process: subprocess.Popen[bytes]) -> None:
    # Kill the process GROUP even when its original parent already exited.
    # Signals are shielded until the child is reaped and records are closed.
    for signum in (signal.SIGTERM, signal.SIGKILL):
        try:
            os.killpg(process.pid, signum)
        except ProcessLookupError:
            pass
        if signum == signal.SIGTERM:
            try:
                process.wait(timeout=10)
            except subprocess.TimeoutExpired:
                pass
    process.wait(timeout=5)


def command(
    directory: Path, label: str, argv: list[str], expected: tuple[int, ...],
    timeout: int, merged: bool = False,
) -> Json:
    intent = {
        'id': label, 'command': shlex.join(argv), 'argv': argv,
        'cwd': str(ROOT), 'started': now(), 'expected_rc': list(expected),
        'outer_timeout_seconds': timeout, 'drain_term_grace_seconds': 10,
        'process_vm_max_bytes': 26 << 30,
        'capture': 'merged_exact_bytes' if merged else 'separate_exact_bytes',
    }
    save(directory / (label + '.intent.json'), intent)
    stdout = directory / (label + ('.raw.txt' if merged else '.stdout'))
    stderr = directory / (label + '.stderr')
    record = dict(intent)
    process = None
    error = None
    timed_out = False
    start = time.monotonic()
    prior_handlers: dict[int, Any] = {}
    env = child_environment()
    try:
        with stdout.open('xb') as out:
            if merged:
                process = subprocess.Popen(
                    argv, cwd=ROOT, env=env, stdout=out,
                    stderr=subprocess.STDOUT, start_new_session=True,
                    preexec_fn=child_limit,
                )
                process.wait(timeout=timeout)
            else:
                with stderr.open('xb') as err:
                    process = subprocess.Popen(
                        argv, cwd=ROOT, env=env, stdout=out, stderr=err,
                        start_new_session=True, preexec_fn=child_limit,
                    )
                    process.wait(timeout=timeout)
    except BaseException as exc:
        timed_out = isinstance(exc, subprocess.TimeoutExpired)
        error = f'{type(exc).__name__}: {exc}'
    finally:
        for sig in SIGNALS:
            prior_handlers[sig] = signal.signal(sig, signal.SIG_IGN)
        try:
            if process is not None:
                try:
                    drain(process)
                except BaseException as exc:
                    error = f'{error}; drain:{type(exc).__name__}:{exc}'
            rc = None if process is None else process.returncode
            status = (
                'completed' if error is None and rc in expected else 'failed'
            )
            if timed_out or rc in (124, 137, -signal.SIGKILL):
                status = 'censored'
            record.update(
                ended=now(), elapsed_seconds=time.monotonic() - start,
                exit_code=rc, status=status, error=error,
                streams={
                    path.name: {
                        'bytes': path.stat().st_size, 'sha256': sha(path),
                    }
                    for path in (stdout, stderr) if path.exists()
                },
            )
            save(directory / (label + '.command.json'), record)
        finally:
            for sig, previous in prior_handlers.items():
                signal.signal(sig, previous)
    return record


def checked(
    directory: Path, label: str, argv: list[str], expected: int = 0,
    timeout: int = 30,
) -> Json:
    record = command(directory, label, argv, (expected,), timeout)
    require(record['status'] == 'completed', 'command_failed:' + label)
    return record


def child_environment() -> dict[str, str]:
    # Deliberate whitelist: no inherited preload/compiler/include flags,
    # credentials, Python hooks or sanitizer environment in these probes.
    return {
        'PATH': '/usr/bin:/bin', 'LC_ALL': 'C', 'LANG': 'C', 'TZ': 'UTC',
        'OMP_NUM_THREADS': '1', 'MKL_NUM_THREADS': '1',
        'OPENBLAS_NUM_THREADS': '1', 'NUMEXPR_NUM_THREADS': '1',
        'GIT_OPTIONAL_LOCKS': '0', 'GIT_CONFIG_GLOBAL': '/dev/null',
        'GIT_CONFIG_NOSYSTEM': '1',
    }


def metadata(directory: Path) -> None:
    commands = {
        'head': ['git', 'rev-parse', 'HEAD'],
        'worktree': [
            'git', 'status', '--porcelain=v1', '--untracked-files=all',
            '--', 'morsehgp3D_v7',
        ],
        'compiler_version': ['/usr/bin/g++', '--version'],
    }
    for label, argv in commands.items():
        checked(directory, label, argv)
    host = {
        'observed': now(), 'uname': list(platform.uname()),
        'python': sys.version, 'python_executable': sys.executable,
        'python_executable_sha256': sha(Path(sys.executable).resolve()),
        'libc': platform.libc_ver(),
        'affinity': sorted(os.sched_getaffinity(0)),
        'page_size': os.sysconf('SC_PAGE_SIZE'),
        'cpu_count': os.cpu_count(),
        'rlimit_as': list(resource.getrlimit(resource.RLIMIT_AS)),
        'fixed_child_env': child_environment(),
        'tools': {
            name: sha(Path(name).resolve()) for name in (
                '/usr/bin/g++', '/usr/bin/time', '/usr/bin/taskset',
                '/usr/bin/timeout',
            )
        },
        'meminfo': Path('/proc/meminfo').read_text(),
        'cpuinfo': Path('/proc/cpuinfo').read_text(),
    }
    require({0, 6}.issubset(set(host['affinity'])),
            'required_cpus_unavailable')
    save(directory / 'host.json', host)


def seal(
    directory: Path, result: Json, before: Json, binary: Path | None,
) -> None:
    try:
        after = snapshot(binary)
        save(directory / 'sources_after.json', after)
        result['sources_stable'] = before == after
        if before != after:
            result.update(status='failed', reason='source_or_binary_drift')
    except BaseException as exc:
        result.update(status='failed', sources_stable=False,
                      reason=f'final_snapshot:{type(exc).__name__}:{exc}')
    result.update(schema=SCHEMA, ended=now(),
                  artifacts={
                      str(p.relative_to(directory)): sha(p)
                      for p in sorted(directory.rglob('*')) if p.is_file()
                  })
    save(directory / 'receipt.json', result)


def verify_receipt(path: str | Path, pin: str, kind: str) -> tuple[Path, Json]:
    location = owned(path)
    require(re.fullmatch(r'[0-9a-f]{64}', pin) is not None,
            'receipt_pin_format')
    require(sha(location) == pin, 'receipt_pin')
    receipt = read_json(location)
    require(receipt['status'] == 'completed' and receipt['kind'] == kind
            and receipt['sources_stable'] is True, 'closed_admission')
    for name, expected in receipt['artifacts'].items():
        target = (location.parent / name).resolve()
        require(target.is_relative_to(location.parent), 'artifact_escape')
        require(sha(target) == expected, 'artifact_drift:' + name)
    before = read_json(location.parent / 'sources_before.json')
    binary = ROOT / receipt['binary'] if 'binary' in receipt else None
    if kind == 'build':
        require(binary is not None and sha(binary) == receipt['binary_sha256'],
                'admitted_binary_drift')
        require(snapshot() == before, 'admitted_sources_drift')
    else:
        require(snapshot(binary) == before, 'admitted_sources_drift')
    return location, receipt


def build(name: str) -> None:
    directory = new_directory('build', name)
    before = snapshot()
    save(directory / 'sources_before.json', before)
    result: Json = {'kind': 'build', 'status': 'failed', 'started': now()}
    binary = directory / 'full_gabriel_lazy_probe'
    try:
        metadata(directory)
        depfile = directory / 'probe.d'
        argv = [
            '/usr/bin/taskset', '-c', '0', '/usr/bin/g++', '-std=c++20',
            '-O3', '-DNDEBUG', '-Wall', '-Wextra', '-Wpedantic', '-Werror',
            '-pthread', '-MMD', '-MF', str(depfile), str(ROOT / PROBE),
            '-o', str(binary),
        ]
        checked(directory, 'compile', argv, timeout=600)
        text = depfile.read_text().replace('\\\n', ' ')
        dependencies = shlex.split(text.split(':', 1)[1])
        require(dependencies, 'empty_dependencies')
        pins = {}
        for value in dependencies:
            path = (ROOT / value).resolve()
            relative = str(path.relative_to(ROOT))
            require(relative in before['files'], 'unadmitted_dependency')
            require(sha(path) == before['files'][relative], 'dependency_drift')
            pins[relative] = before['files'][relative]
        require(set((PROBE, DIGEST, PRODUCER)).issubset(pins),
                'required_dependency_absent')
        save(directory / 'dependencies.json', pins)
        result.update(status='completed', reason='compile_only_no_probe',
                      binary=str(binary.relative_to(ROOT)),
                      binary_sha256=sha(binary), dependency_count=len(pins))
    except BaseException as exc:
        result.update(reason=f'{type(exc).__name__}: {exc}')
    finally:
        # Build's before/after sources intentionally exclude its new binary;
        # the final binary pin is separately admitted by every later command.
        seal(directory, result, before, None)
    require(result['status'] == 'completed', 'build_failed')


def protocol(binary: Path, k: int, sequence: list[list[Any]]) -> Json:
    return {
        'schema': 'mhgp7-full-gabriel-mono-observation-v2',
        'public_status': 'not_claimed', 'authority': AUTHORITY,
        'scope': 'horizontal_relative_orders_not_integrated_inter_k_tower',
        'family': 'uniform', 'seed': 3, 'coord': 65536, 'threads': 1,
        'cpu_affinity': [6], 'kmax': k, 'planned_sequence': sequence,
        'timeout_seconds_each': 600, 'kill_grace_seconds': 10,
        'process_vm_soft_limit_max_gib': 26, 'named_payload_proxy_gib': 8,
        'binary': str(binary.relative_to(ROOT)), 'binary_sha256': sha(binary),
        'probe_sha256': PINS[PROBE], 'producer_sha256': PINS[PRODUCER],
        'digest_header_sha256': PINS[DIGEST], 'judge_sha256': PINS[JUDGE],
        'paired_F_speedup_claim': False, 'slo_claim': False, 'gcp_used': False,
        'comparison_s': 'semantic_horizontal_forest_digest_not_oracle',
        'timing_reference': 'elapsed includes digests and provisional output',
        'output_capture': 'merged stdout/stderr exact bytes plus GNU time -v',
        'continuation_policy': 'explicit reviewed next attempt; no cap change',
    }


def prepare_directory(directory: Path, plan: Json, binary: Path) -> None:
    save(directory / 'protocol.json', plan)
    save(directory / 'sources_before.json', snapshot(binary))


def probe_attempt(
    directory: Path, index: int, selftests: bool = False,
) -> Json:
    plan = read_json(directory / 'protocol.json')
    binary = ROOT / plan['binary']
    baseline = read_json(directory / 'sources_before.json')
    require(snapshot(binary) == baseline, 'pre_attempt_drift')
    n, separation, policy, cache = plan['planned_sequence'][index]
    k = plan['kmax']
    label = f'n{n}_s{separation}_k{k}_{policy}_c{cache}'
    save(directory / (label + '.sources_before.json'), snapshot(binary))
    argv = [
        'timeout', '--signal=TERM', '--kill-after=10s', '600s',
        'taskset', '-c', '6', '/usr/bin/time', '-v', plan['binary'],
        f'--n={n}', f'--s={separation}', f'--kmax={k}',
        f'--alias-policy={policy}',
    ]
    if policy == 'lazy':
        argv.append(f'--cache-entries={cache}')
    record = command(directory, label, argv, (0, 2, 3), 620, merged=True)
    record.update(terminal=None, orders=[])
    try:
        raw = directory / (label + '.raw.txt')
        require(raw.stat().st_size <= 1 << 20, 'probe_output_size_budget')
        rows = [parse(line) for line in raw.read_text().splitlines()
                if line.startswith('{')]
        terminals = [row for row in rows if row.get('type') == 'terminal']
        record['orders'] = [row for row in rows if row.get('type') == 'order']
        require(len(terminals) == 1, 'missing_or_multiple_terminal')
        record['terminal'] = terminals[0]
        require(record['status'] == 'completed', 'process_not_completed')
    except BaseException as exc:
        if record['status'] != 'censored':
            record['status'] = 'failed'
        record['capture_error'] = f'{type(exc).__name__}: {exc}'
    save(directory / (label + '.receipt.json'), record)
    verdict: Json = {'status': 'failed', 'attempt_id': label, 'started': now()}
    try:
        require(record['status'] == 'completed', 'nonterminal_capture')
        for optimized in (False, True):
            tag = 'optimized' if optimized else 'normal'
            interpreter = ['/usr/bin/taskset', '-c', '0', sys.executable, '-B']
            if optimized:
                interpreter.append('-O')
            receipt_path = str(directory / (label + '.receipt.json'))
            checked(directory, label + '.judge_' + tag,
                    interpreter + [str(ROOT / JUDGE), receipt_path])
            output = read_json(
                directory / (label + '.judge_' + tag + '.stdout'),
            )
            require(output['audit_status'] == 'valid', 'judge_output')
            if selftests:
                checked(directory, label + '.selftest_' + tag,
                        interpreter + [str(ROOT / JUDGE), '--selftest',
                                       receipt_path])
                test = read_json(
                    directory / (label + '.selftest_' + tag + '.stdout'),
                )
                require(test['audit_status'] == 'selftests_passed'
                        and len(test['mutants_killed']) == 19,
                        'judge_mutant_nonvacuum')
        if n == 8 and k == 10 and record['exit_code'] == 0:
            last = record['orders'][-1]
            require(last['k'] == 8 and last['certificate_nodes'] == 1
                    and last['certificate_minima'] == 1
                    and last['certificate_parent_refs'] == 0
                    and last['connection_catalogue_records'] == 0,
                    'micro_K_equals_n_terminal')
        verdict.update(status='completed', capture_valid=True,
                       attempt_success=record['exit_code'] == 0,
                       outcome=record['terminal']['outcome'],
                       certificate_digest=record['terminal'][
                           'certificate_digest'
                       ],
                       input_digest=record['terminal']['input_digest'])
    except BaseException as exc:
        verdict.update(reason=f'{type(exc).__name__}: {exc}')
    finally:
        try:
            after = snapshot(binary)
            save(directory / (label + '.sources_after.json'), after)
            require(after == baseline, 'post_attempt_drift')
        except BaseException as exc:
            verdict.update(status='failed',
                           reason=f'final_sources:{type(exc).__name__}:{exc}')
        verdict['ended'] = now()
        save(directory / (label + '.verdict.json'), verdict)
    return verdict


def micro(name: str, build_path: str, build_pin: str) -> None:
    admitted, compilation = verify_receipt(build_path, build_pin, 'build')
    binary = ROOT / compilation['binary']
    require(sha(binary) == compilation['binary_sha256'], 'binary_pin')
    directory = new_directory('micro', name)
    before = snapshot(binary)
    save(directory / 'sources_before.json', before)
    result: Json = {
        'kind': 'micro', 'status': 'failed', 'started': now(),
        'binary': str(binary.relative_to(ROOT)), 'binary_sha256': sha(binary),
        'build_receipt': str(admitted), 'build_receipt_sha256': build_pin,
        'attempts': [],
    }
    try:
        metadata(directory)
        checked(directory, 'digest_selftest', [
            '/usr/bin/taskset', '-c', '0', str(binary), '--digest-selftest',
        ])
        test = read_json(directory / 'digest_selftest.stdout')
        require(test['passed'] is True and test['checks'] == 24
                and test['failures'] == 0, 'digest_selftest_nonvacuum')
        base_args = ['--n=8', '--s=8', '--kmax=10']
        eager = ['--alias-policy=eager']
        negatives = {
            'missing': ['--n=8', '--s=8'] + eager,
            'duplicate': ['--n=8', '--n=8', '--s=8', '--kmax=10'] + eager,
            'n9': ['--n=9', '--s=8', '--kmax=10'] + eager,
            's9': ['--n=8', '--s=9', '--kmax=10'] + eager,
            'k0': ['--n=8', '--s=8', '--kmax=0'] + eager,
            'unknown': base_args + eager + ['--unknown'],
            'policy_missing': base_args,
            'policy_duplicate': base_args + eager + eager,
            'cache_eager': base_args + eager + ['--cache-entries=0'],
            'lazy_no_cache': base_args + ['--alias-policy=lazy'],
            'cache_overflow': base_args + [
                '--alias-policy=lazy', '--cache-entries=1000001',
            ],
        }
        for label, options in negatives.items():
            name = 'reject_' + label
            checked(directory, name,
                    ['/usr/bin/taskset', '-c', '0', str(binary)] + options,
                    expected=2)
            negative = read_json(directory / (name + '.stdout'))
            require((directory / (name + '.stderr')).stat().st_size == 0
                    and negative['schema'] == 'mhgp7-full-gabriel-probe-v2'
                    and negative['type'] == 'terminal'
                    and negative['terminal_status'] == 'failed'
                    and negative['outcome'] == 'invalid_input'
                    and negative['reason'] == 'probe_arguments'
                    and negative['exit_code'] == 2
                    and negative['completed_orders_diagnostic'] == 0
                    and negative['certificate_digest'] == ''
                    and negative['complete_requested_horizontal_orders']
                    is False, 'negative_parser_judge')
        require(len(negatives) == 11, 'parser_nonvacuum')
        result['parser_rejects'] = len(negatives)
        for k in (5, 10):
            section = directory / f'k{k}'
            section.mkdir(exist_ok=False)
            sequence = [[8, s, policy, cache] for s in (8, 10, 12)
                        for policy, cache in (
                            ('eager', 0), ('lazy', 0),
                            ('lazy', 1), ('lazy', 1000000),
                        )]
            prepare_directory(section, protocol(binary, k, sequence), binary)
            values = []
            for index in range(12):
                value = probe_attempt(section, index,
                                      selftests=k == 10 and index == 0)
                result['attempts'].append(value)
                require(value['status'] == 'completed'
                        and value['attempt_success'], 'micro_attempt_failed')
                values.append(value)
            require(len({v['input_digest'] for v in values}) == 1
                    and len({v['certificate_digest'] for v in values}) == 1,
                    'micro_semantic_digest_disagreement')
        require(len(result['attempts']) == 24, 'micro_nonvacuum')
        result.update(status='completed', reason='24_micro_and_digest_judges')
    except BaseException as exc:
        result.update(reason=f'{type(exc).__name__}: {exc}')
    finally:
        seal(directory, result, before, binary)
    require(result['status'] == 'completed', 'micro_failed')


def qualification(path: str, pin: str) -> Json:
    # The separate ROOT controller owns this source-qualified receipt. Its
    # precise format is explicitly checked here, not inferred from a filename.
    location = Path(path).resolve()
    require(location.is_relative_to(ROOT / 'build'), 'qualification_scope')
    require(pin == QUALIFICATION_SHA and sha(location) == pin,
            'qualification_pin')
    value = read_json(location)
    require(value['status'] == 'completed' and not value['errors']
            and value['public_status'] == 'not_claimed'
            and value['schema'] == 'mhgp7-private-full-lazy-qualification-v1',
            'qualification_not_completed')
    require([p['mode'] for p in value['phases']] == ['release', 'san'],
            'qualification_phase_inventory')
    live = snapshot()['files']
    for phase in value['phases']:
        require(phase['status'] == 'completed' and not phase['errors']
                and phase['sources_stable'] is True
                and phase['binaries_stable'] is True
                and phase['compile_binding_stable'] is True,
                'qualification_stability')
        inventory = phase['inventory']
        outputs = phase['test_outputs']
        require(inventory['tests'] == 14
                and inventory['names'] == QUALIFIED_TESTS
                and inventory['commands_codes_and_timeouts_exact'] is True
                and outputs['fresh'] is True,
                'qualification_inventory')
        for key, flag in (('junit', 'all_run'),
                          ('last_test', 'all_blocks_passed')):
            require(outputs[key]['tests'] == 14
                    and outputs[key]['names'] == QUALIFIED_TESTS
                    and outputs[key][flag] is True,
                    'qualification_test_outputs')
        require(outputs['last_test']['terminal_footer'] is True,
                'qualification_terminal_footer')
        for name, expected in phase['sources_after'].items():
            if name in live:
                require(live[name] == expected,
                        'qualified_source_drift:' + name)
        require(phase['sources_after'][PRODUCER] == PINS[PRODUCER]
                and phase['sources_after'][DIGEST] == PINS[DIGEST],
                'qualification_producer_binding')
        for copy in outputs['copies'].values():
            archived = copy['archive']
            require(sha(Path(archived['path'])) == archived['sha256']
                    == copy['source']['sha256'], 'qualification_raw_copy')
    return {'path': str(location), 'sha256': pin}


def prepare_heavy(args: argparse.Namespace) -> None:
    micro_path, admitted = verify_receipt(
        args.micro_receipt, args.micro_sha256, 'micro',
    )
    binary = ROOT / admitted['binary']
    qualified = qualification(args.qualification_receipt,
                              args.qualification_sha256)
    if args.phase == 'paired':
        sequence = PAIRED
        prior = None
    else:
        require(args.paired_receipt is not None
                and args.paired_sha256 is not None,
                'paired_admission_required')
        path, closed = verify_receipt(args.paired_receipt,
                                      args.paired_sha256, 'heavy')
        require(closed['phase'] == 'paired' and closed['all_successful']
                and closed['semantic_digests_equal'], 'paired_not_successful')
        prior = {'path': str(path), 'sha256': args.paired_sha256}
        sequence = [[16000 if args.phase == 'scale16' else 32000,
                     8, 'lazy', 1000000]]
    directory = new_directory('heavy', args.id)
    prepare_directory(directory, protocol(binary, 10, sequence), binary)
    metadata(directory)
    save(directory / 'admission.json', {
        'phase': args.phase, 'started': now(),
        'micro_receipt': str(micro_path), 'micro_sha256': args.micro_sha256,
        'qualification': qualified, 'paired': prior,
        'no_probe_launched': True,
    })
    with (directory / 'session.lock').open('xb'):
        pass


def heavy_attempt(path: str, index: int, go: bool) -> None:
    require(go, 'explicit_heavy_go_required')
    directory = owned(path)
    with (directory / 'session.lock').open('rb') as lock:
        fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
        require(not (directory / 'receipt.json').exists(), 'already_closed')
        plan = read_json(directory / 'protocol.json')
        admission = read_json(directory / 'admission.json')
        verify_receipt(admission['micro_receipt'],
                       admission['micro_sha256'], 'micro')
        qualification(**{
            'path': admission['qualification']['path'],
            'pin': admission['qualification']['sha256'],
        })
        require(0 <= index < len(plan['planned_sequence']), 'attempt_index')
        for n, s, policy, cache in plan['planned_sequence'][:index]:
            label = f'n{n}_s{s}_k10_{policy}_c{cache}'
            previous = read_json(directory / (label + '.verdict.json'))
            require(previous['status'] == 'completed'
                    and previous['attempt_success'],
                    'prior_attempt_not_success')
        result = probe_attempt(directory, index)
        print(json.dumps(result, sort_keys=True))
        require(result['status'] == 'completed', 'heavy_capture_failed')


def close_heavy(path: str) -> None:
    directory = owned(path)
    with (directory / 'session.lock').open('rb') as lock:
        fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
        require(not (directory / 'receipt.json').exists(), 'already_closed')
        plan = read_json(directory / 'protocol.json')
        admission = read_json(directory / 'admission.json')
        before = read_json(directory / 'sources_before.json')
        binary = ROOT / plan['binary']
        result: Json = {
            'kind': 'heavy', 'phase': admission['phase'], 'status': 'failed',
            'started': admission['started'], 'binary': plan['binary'],
            'binary_sha256': plan['binary_sha256'], 'attempts': [],
            'all_successful': False, 'semantic_digests_equal': False,
        }
        try:
            for n, s, policy, cache in plan['planned_sequence']:
                label = f'n{n}_s{s}_k10_{policy}_c{cache}'
                record = read_json(directory / (label + '.verdict.json'))
                result['attempts'].append(record)
                require(record['status'] == 'completed', 'invalid_capture')
            result['all_successful'] = all(
                item['attempt_success'] for item in result['attempts']
            )
            if result['all_successful']:
                result['semantic_digests_equal'] = (
                    len({v['input_digest'] for v in result['attempts']}) == 1
                    and len({v['certificate_digest']
                             for v in result['attempts']}) == 1
                )
                require(result['semantic_digests_equal'],
                        'digest_disagreement')
            result.update(status='completed', reason='closed_captures_not_SLO')
        except BaseException as exc:
            result.update(reason=f'{type(exc).__name__}: {exc}')
        finally:
            seal(directory, result, before, binary)
        print(json.dumps(result, sort_keys=True))


def main() -> None:
    for sig in SIGNALS:
        signal.signal(sig, interrupted)
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest='action', required=True)
    build_cli = commands.add_parser('build')
    build_cli.add_argument('--id', required=True)
    micro_cli = commands.add_parser('micro')
    micro_cli.add_argument('--id', required=True)
    micro_cli.add_argument('--build-receipt', required=True)
    micro_cli.add_argument('--build-sha256', required=True)
    prepare = commands.add_parser('prepare-heavy')
    prepare.add_argument('--id', required=True)
    prepare.add_argument('--phase', choices=('paired', 'scale16', 'scale32'),
                         required=True)
    for name in ('micro-receipt', 'micro-sha256', 'qualification-receipt',
                 'qualification-sha256'):
        prepare.add_argument('--' + name, required=True)
    prepare.add_argument('--paired-receipt')
    prepare.add_argument('--paired-sha256')
    attempt = commands.add_parser('attempt')
    attempt.add_argument('--directory', required=True)
    attempt.add_argument('--index', type=int, required=True)
    attempt.add_argument('--go-reviewed-heavy', action='store_true')
    close = commands.add_parser('close-heavy')
    close.add_argument('--directory', required=True)
    args = parser.parse_args()
    if args.action == 'build':
        build(args.id)
    elif args.action == 'micro':
        micro(args.id, args.build_receipt, args.build_sha256)
    elif args.action == 'prepare-heavy':
        prepare_heavy(args)
    elif args.action == 'attempt':
        heavy_attempt(args.directory, args.index, args.go_reviewed_heavy)
    else:
        close_heavy(args.directory)


if __name__ == '__main__':
    try:
        main()
    except BaseException as error:
        if isinstance(error, SystemExit):
            raise
        print(json.dumps({'status': 'failed',
                          'reason': f'{type(error).__name__}: {error}'}))
        sys.exit(1)
