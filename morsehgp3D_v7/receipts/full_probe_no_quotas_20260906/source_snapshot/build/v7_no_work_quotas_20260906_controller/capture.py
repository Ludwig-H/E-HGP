#!/usr/bin/env python3
"""Fresh FULL v5 capture without arbitrary operation quotas. Inert without an explicit action and source pin.

Only the pinned old controller's helpers/build are imported, never its main,
micro or heavy scheduler. Every heavy action runs exactly one declared job;
refusals are recorded and do not prevent the paired arm being attempted.
No Git mutation, GCP or inherited geometry qualification. V4 captures remain frozen.
"""
from __future__ import annotations

import argparse
import fcntl
import hashlib
import json
import os
from pathlib import Path
import re
import resource
import shlex
import signal
import subprocess
import sys
import time
import types

ROOT = Path('/workspaces/E-HGP')
BASE = ROOT / 'build/v7_no_work_quotas_20260906_controller'
IMPORTED = ROOT / 'build/v7_full_lazy_20260905_probe_controller/capture.py'
IMPORT_PIN = '417ccc3b47bb7591405f3af99bf7591bf2019794aa4535077436ce4889c4adfa'
PRODUCT = 'morsehgp3D_v7/'
PROBE = PRODUCT + 'bench/full_gabriel_lazy_probe.cpp'
DIGEST = PRODUCT + 'bench/full_gabriel_semantic_digest.hpp'
JUDGE = PRODUCT + 'bench/full_gabriel_lazy_probe_audit.py'
FIRST_C = PRODUCT + 'bench/full_gabriel_cache_policy_audit.py'
FULL = PRODUCT + 'src/forest/full_gabriel.hpp'
MEB = PRODUCT + 'src/forest/meb_proposal.hpp'
F = PRODUCT + 'src/forest/silent_incidence.hpp'
LOCAL = PRODUCT + 'tests/meb_proposal_local_gate.cpp'
PINS = {
    FULL: 'a946e31dde8fbd8ec528d6f5e94f9c727998acc172b4dd29c084dd522c730d1d',
    MEB: 'f922544b5cfdc214de96ecd49520e318ea8632d14a8142ef21fd248f9cc38fb3',
    F: 'f75a136a320ddd1ace025436874585c2226e6150b8f2ebc37920b8dfc7e36c76',
}
SCHEMA = 'mhgp7-no-work-quotas-probe-controller-v1'
PROBE_SCHEMA = 'mhgp7-full-gabriel-probe-v5'
ACCOUNTING = 'full_successor_reads_writes_no_last_pair_v2'
MEB_ACCOUNTING = 'reference_ordinal_plus_native_z_q3_q4_proposal_v2'
LARGE_P = 'unlimited'
MAX_U64 = (1 << 64) - 1
LIMITS = PRODUCT + 'bench/full_gabriel_probe_limits.hpp'
LIMITS_TEST = PRODUCT + 'tests/full_gabriel_probe_limits_gate.cpp'
MINIMA_TEST = PRODUCT + 'tests/full_gabriel_minima_quotient_gate.py'
CMAKE_GATE = PRODUCT + 'cmake/run_expect.cmake'
LIMITS_PROFILE = 'memory_guarded_no_operation_quotas_v1'
EXTRA_SEQUENCE = [[2, 8, 'eager', 0, LARGE_P], [9, 8, 'eager', 0, 0],
                  [8, 8, 'eager', 0, 584000001], [8, 8, 'eager', 0, MAX_U64],
                  [8, 8, 'lazy', 1000001, LARGE_P]]
SEQUENCE = [[n, s, 'lazy', 1000000, p]
            for s in (8, 10, 12) for p in (LARGE_P, 0)
            for n in (8000, 16000, 32000)]


def require(ok, reason):
    if not ok:
        raise ValueError(reason)


def encoded(value):
    return (json.dumps(value, indent=2, sort_keys=True) + '\n').encode()


def sha(raw):
    return hashlib.sha256(raw).hexdigest()


def sources():
    paths = {p for p in (ROOT / PRODUCT / 'src').rglob('*') if p.is_file()}
    paths.update(ROOT / name for name in (PROBE, DIGEST, JUDGE, FIRST_C, LOCAL, LIMITS, LIMITS_TEST, MINIMA_TEST, CMAKE_GATE,
                                          PRODUCT + 'CMakeLists.txt'))
    paths.update((Path(__file__).resolve(), IMPORTED))
    require(all(not p.is_symlink() for p in paths), 'source_symlink')
    values = {str(p.relative_to(ROOT)): sha(p.read_bytes()) for p in sorted(paths)}
    for name, pin in PINS.items():
        require(values[name] == pin, 'product_authority:' + name)
    require(values[str(IMPORTED.relative_to(ROOT))] == IMPORT_PIN, 'import_authority')
    return values


class Controller:
    def __init__(self, pin):
        self.reviewed = sources()
        require(sha(encoded(self.reviewed)) == pin, 'reviewed_source_map_required')
        raw = IMPORTED.read_bytes()
        require(sha(raw) == IMPORT_PIN, 'import_changed')
        c = types.ModuleType('pinned_inert_capture_helpers')
        c.__file__ = str(IMPORTED)
        exec(compile(raw, str(IMPORTED), 'exec'), c.__dict__)
        self.c, self.pin = c, pin
        c.BASE, c.SCHEMA = BASE, SCHEMA
        c.PINS = {name: self.reviewed[name] for name in (PROBE, DIGEST, JUDGE, FULL)}
        c.snapshot = self.snapshot
        c.command = self.command

    def snapshot(self, binary=None):
        value = sources()
        require(value == self.reviewed, 'source_drift')
        result = dict(files=value, source_map_sha256=self.pin)
        if binary is not None:
            require(binary.resolve().is_relative_to(BASE), 'binary_scope')
            result.update(binary=str(binary.relative_to(ROOT)), binary_sha256=self.c.sha(binary))
        return result

    def command(self, directory, label, argv, expected, timeout, merged=False):
        c = self.c
        compilation = any(Path(arg).name in ('g++', 'c++') for arg in argv) or (
            '--build' in argv and any(Path(arg).name == 'cmake' for arg in argv))
        sanitizer_execution = any(Path(arg).name in ('local_san', 'limits_san') for arg in argv)
        fsize = (512 if compilation else 64) << 20
        intent = dict(id=label, command=shlex.join(argv), argv=argv, cwd=str(ROOT),
            started=c.now(), expected_rc=list(expected), outer_timeout_seconds=timeout,
            drain_term_grace_seconds=10,
            process_vm_max_bytes=None if sanitizer_execution else 26 << 30,
            sanitizer_virtual_address_reservation=sanitizer_execution,
            cpu_limit_seconds=1220, file_size_limit_bytes=fsize,
            capture='merged_exact_bytes' if merged else 'separate_exact_bytes')
        c.save(directory / (label + '.intent.json'), intent)
        stdout = directory / (label + ('.raw.txt' if merged else '.stdout'))
        stderr = directory / (label + '.stderr')
        proc, error, closed = None, None, False
        start = time.monotonic()

        def limits():
            if not sanitizer_execution:
                c.child_limit()
            resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
            resource.setrlimit(resource.RLIMIT_CPU, (1220, 1220))
            resource.setrlimit(resource.RLIMIT_FSIZE, (fsize, fsize))

        env = c.child_environment()
        env.update(ASAN_OPTIONS='detect_leaks=1:halt_on_error=1',
                   UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1', LSAN_OPTIONS='exitcode=23')
        try:
            with stdout.open('xb') as out, stderr.open('xb') as err:
                proc = subprocess.Popen(argv, cwd=ROOT, env=env, stdin=subprocess.DEVNULL,
                    stdout=out, stderr=subprocess.STDOUT if merged else err,
                    start_new_session=True, preexec_fn=limits)
                c.save(directory / (label + '.spawn.json'), dict(pid=proc.pid, pgid=proc.pid, utc=c.now()))
                proc.wait(timeout=timeout)
        except BaseException as exc:
            error = type(exc).__name__ + ': ' + str(exc)
        finally:
            previous = {sig: signal.signal(sig, signal.SIG_IGN) for sig in c.SIGNALS}
            try:
                closure_errors = []
                if proc is not None:
                    try:
                        c.drain(proc)
                    except BaseException as exc:
                        closure_errors.append('drain: ' + type(exc).__name__ + ': ' + str(exc))
                    try:
                        os.killpg(proc.pid, 0)
                    except ProcessLookupError:
                        closed = True
                    except OSError as exc:
                        closure_errors.append('group_check: ' + type(exc).__name__ + ': ' + str(exc))
                streams, unavailable = {}, []
                for path in (stdout, stderr):
                    try:
                        require(path.is_file() and not path.is_symlink(), 'stream_not_regular')
                        streams[path.name] = dict(bytes=path.stat().st_size, sha256=c.sha(path))
                    except BaseException as exc:
                        unavailable.append(path.name)
                        closure_errors.append('stream:' + path.name + ': ' + type(exc).__name__ + ': ' + str(exc))
                rc = None if proc is None else proc.returncode
                status = 'completed' if error is None and not closure_errors and closed and rc in expected else 'failed'
                if rc in (124, 137, -9) or (error and error.startswith('TimeoutExpired:')):
                    status = 'censored'
                record = dict(intent, ended=c.now(), elapsed_seconds=time.monotonic() - start,
                    pid=None if proc is None else proc.pid, process_group_closed=closed,
                    exit_code=rc, status=status, error=error, closure_errors=closure_errors,
                    unavailable_streams=unavailable, streams=streams)
                c.save(directory / (label + '.command.json'), record)
            finally:
                for sig, handler in previous.items():
                    signal.signal(sig, handler)
        return record

    def copy_sources(self, directory):
        directory.mkdir(exist_ok=False)
        target = directory / 'source_snapshot'
        target.mkdir(exist_ok=False)
        for name, pin in self.reviewed.items():
            raw = (ROOT / name).read_bytes()
            require(sha(raw) == pin, 'copy_source_drift')
            path = target / name
            path.parent.mkdir(parents=True, exist_ok=True)
            with path.open('xb') as stream:
                stream.write(raw)

    def admission(self, written, kind):
        path, sep, pin = written.rpartition('=')
        require(sep, 'PATH=SHA256 admission required')
        location, receipt = self.c.verify_receipt(path, pin, kind)
        require(receipt['schema'] == SCHEMA, 'admission_schema')
        actual = {str(p.relative_to(location.parent)): self.c.sha(p)
                  for p in location.parent.rglob('*') if p.is_file() and p != location}
        require(encoded(actual) == encoded(receipt['artifacts']), 'admission_exact_artifacts')
        require(encoded(self.c.read_json(location.parent / 'sources_before.json'))
                == encoded(self.c.read_json(location.parent / 'sources_after.json')), 'admission_snapshots')
        return location, receipt

    def local_binding(self, binary, depfile, source_root, overrides=None, required=None):
        """Actual -MMD dependencies, not an assumed inventory of headers."""
        c = self.c
        overrides = {} if overrides is None else overrides
        require(set(overrides) <= {MEB} and source_root.is_absolute(), 'local_override_scope')
        if overrides:
            require(source_root != ROOT and overrides[MEB] != self.reviewed[MEB], 'mutant_pin')
        text = depfile.read_text().replace('\\\n', ' ')
        target, sep, dependencies = text.partition(':')
        require(sep and shlex.split(target) == [str(binary)], 'local_dependency_target')
        files = {}
        for written in shlex.split(dependencies):
            path = (ROOT / written).resolve()
            require(path.is_relative_to(source_root), 'local_dependency_escape')
            name = str(path.relative_to(source_root))
            require(name in self.reviewed and not (source_root / name).is_symlink(), 'local_dependency_unknown')
            expected = overrides.get(name, self.reviewed[name])
            require(c.sha(path) == expected, 'local_dependency_drift:' + name)
            files[name] = expected
        required = {LOCAL, MEB, F} if required is None else required
        require(required <= files.keys(), 'local_dependencies_nonvacuum')
        require(binary.is_file() and not binary.is_symlink(), 'local_binary_regular')
        return dict(source_root=str(source_root), files=files, depfile_sha256=c.sha(depfile),
                    binary=str(binary), binary_sha256=c.sha(binary), overrides=overrides)

    def local_run(self, directory, label, binary, depfile, source_root, admitted, arg, code, required=None):
        c = self.c
        before = self.local_binding(binary, depfile, source_root, admitted['overrides'], required)
        require(encoded(before) == encoded(admitted), 'local_pre_run_drift')
        c.save(directory / (label + '.local_before.json'), before)
        try:
            c.checked(directory, label, ['/usr/bin/taskset', '-c', '0', str(binary), arg], expected=code)
        finally:
            after = self.local_binding(binary, depfile, source_root, admitted['overrides'], required)
            c.save(directory / (label + '.local_after.json'), after)
            require(encoded(after) == encoded(before), 'local_post_run_drift')

    def local_result(self, directory, label, mode):
        c = self.c
        require((directory / (label + '.stderr')).read_bytes() == b'', 'local_stderr')
        if mode == 'unknown':
            require((directory / (label + '.stdout')).read_bytes() == b'', 'local_unknown_stdout')
            return None
        row = c.read_json(directory / (label + '.stdout'))
        calls, exceptions = (111, 0) if mode == 'selftest' else (45, 3)
        expected = dict(schema='mhgp7-meb-proposal-local-v1', status='passed', test_mode=mode,
            public_status='not_claimed', calls=calls, terminal_comparisons=2 * calls,
            all_13_F_stats_comparisons=2 * calls, all_5_Work_comparisons=calls,
            exception_boundaries=exceptions, max_boundaries=0 if mode == 'selftest' else 8)
        counts = ('success', 'capped', 'shell', 'proposal_forms', 'physical_F_supports',
                  'fast_q2', 'fast_q3', 'fast_q4', 'q4_raw')
        require(set(row) == set(expected) | set(counts), 'local_result_fields')
        for key, value in expected.items():
            require(key in row and type(row[key]) is type(value) and row[key] == value, 'local_result:' + key)
        for key in counts:
            require(type(row.get(key)) is int and row[key] >= 0, 'local_count:' + key)
        for key in ('success', 'shell', 'proposal_forms', 'physical_F_supports'):
            require(row[key] > 0, 'local_nonvacuum:' + key)
        for key in (('fast_q2', 'fast_q3', 'fast_q4', 'q4_raw') if mode == 'selftest' else ('capped',)):
            require(row[key] > 0, 'local_nonvacuum:' + key)
        return row

    def build(self, name):
        self.c.build(name)
        directory = BASE / ('build_' + name)
        # Original receipt remains sealed; supplemental source copies are external.
        self.copy_sources(BASE / ('build_' + name + '_sources'))

    def limits_result(self, directory, label, testing):
        require((directory / (label + '.stderr')).read_bytes() == b'', 'limits_stderr')
        if testing == 'unknown':
            require((directory / (label + '.stdout')).read_bytes() == b'', 'limits_unknown_stdout')
            return None
        row = self.c.read_json(directory / (label + '.stdout'))
        expected = dict(schema='mhgp7-full-probe-limits-gate-v1', status='passed', public_status='not_claimed',
                        checks=52, work_ceilings=7, actual_successor_MAX_cases=3, full_hierarchy_built=False)
        require(encoded(row) == encoded(expected), 'limits_gate_result')
        return row

    def limits_admission(self, directory, results):
        c = self.c
        require(set(results) == {'O2', 'san'}, 'limits_modes')
        for mode in ('O2', 'san'):
            binary, depfile = directory / ('limits_' + mode), directory / ('limits_' + mode + '.d')
            binding = self.local_binding(binary, depfile, ROOT, required={LIMITS, LIMITS_TEST, FULL, MEB, F})
            require(encoded(c.read_json(directory / ('limits_' + mode + '.dependencies.json'))) == encoded(binding),
                    'limits_dependencies_admission')
            for test, code in (('selftest', 0), ('unknown', 2)):
                label = 'limits_' + mode + '_' + test
                record = c.read_json(directory / (label + '.command.json'))
                require(record['status'] == 'completed' and record['process_group_closed'] is True
                        and record['error'] is None and type(record['exit_code']) is int and record['exit_code'] == code,
                        'limits_closed_result')
                require(record['closure_errors'] == [] and record['unavailable_streams'] == [], 'limits_stream_closure')
                intent = c.read_json(directory / (label + '.intent.json'))
                spawn = c.read_json(directory / (label + '.spawn.json'))
                require(encoded({key: record[key] for key in intent}) == encoded(intent), 'limits_intent_mirror')
                require(type(record['pid']) is int and record['pid'] > 0 and type(spawn['pid']) is int
                        and type(spawn['pgid']) is int and record['pid'] == spawn['pid'] == spawn['pgid'], 'limits_spawn')
                streams = {name: dict(bytes=(directory / name).stat().st_size, sha256=c.sha(directory / name))
                           for name in (label + '.stdout', label + '.stderr')}
                require(encoded(record['streams']) == encoded(streams), 'limits_streams')
                require(encoded(record['argv']) == encoded(['/usr/bin/taskset', '-c', '0', str(binary), '--' + test]),
                        'limits_command_admission')
                require(encoded(record['expected_rc']) == encoded([code]), 'limits_expected_exit')
                for side in ('before', 'after'):
                    require(encoded(c.read_json(directory / (label + '.local_' + side + '.json'))) == encoded(binding),
                            'limits_snapshot_admission')
                row = self.limits_result(directory, label, test)
                if test == 'selftest':
                    require(encoded(row) == encoded(results[mode]), 'limits_stdout_admission')
        require(encoded(results['O2']) == encoded(results['san']), 'limits_sanitizer_disagreement')

    def limits_qualification(self, directory):
        c = self.c
        results = {}
        for mode, flags in (('O2', ['-O2']), ('san', ['-O1', '-g', '-fsanitize=address,undefined',
                    '-fno-sanitize-recover=all', '-fno-omit-frame-pointer', '-fno-pie', '-no-pie'])):
            binary = directory / ('limits_' + mode)
            depfile = directory / ('limits_' + mode + '.d')
            c.checked(directory, 'compile_limits_' + mode, ['/usr/bin/taskset', '-c', '0', '/usr/bin/g++',
                '-std=c++20', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-MMD', '-MF',
                str(depfile)] + flags + [str(ROOT / LIMITS_TEST), '-o', str(binary)], timeout=600)
            required = {LIMITS, LIMITS_TEST, FULL, MEB, F}
            binding = self.local_binding(binary, depfile, ROOT, required=required)
            c.save(directory / ('limits_' + mode + '.dependencies.json'), binding)
            for arg, code in (('--selftest', 0), ('--unknown', 2)):
                label = 'limits_' + mode + '_' + arg[2:]
                self.local_run(directory, label, binary, depfile, ROOT, binding, arg, code, required)
                row = self.limits_result(directory, label, arg[2:])
                if arg == '--selftest':
                    results[mode] = row
        self.limits_admission(directory, results)
        return results

    def cmake_binding(self, directory):
        c = self.c
        build = directory / 'cmake'
        binary = build / 'mhgp7_full_gabriel_probe_limits_gate'
        obj = build / 'CMakeFiles/mhgp7_full_gabriel_probe_limits_gate.dir/tests/full_gabriel_probe_limits_gate.cpp.o'
        depfile = Path(str(obj) + '.d')
        target, sep, deps = depfile.read_text().replace('\\\n', ' ').partition(':')
        targets = shlex.split(target)
        require(sep and len(targets) == 1 and (build / targets[0]).resolve() == obj, 'cmake_dependency_target')
        files, system_headers = {}, {}
        for value in shlex.split(deps):
            path = (build / value).resolve()
            # CMake includes system headers in .d; bind only project-owned
            # headers to the reviewed map. System identities are observed,
            # not promoted to reviewed/hermetic source authority.
            if not path.is_relative_to(ROOT):
                require(path.is_relative_to('/usr/include') or path.is_relative_to('/usr/lib/gcc'),
                        'cmake_system_dependency_scope')
                system_headers[str(path)] = c.sha(path)
                continue
            name = str(path.relative_to(ROOT))
            require(name in self.reviewed and c.sha(path) == self.reviewed[name], 'cmake_dependency_pin')
            files[name] = self.reviewed[name]
        require({LIMITS_TEST, LIMITS, FULL, MEB, F} <= files.keys(), 'cmake_dependency_nonvacuum')
        commands = c.parse((build / 'compile_commands.json').read_text())
        require(type(commands) is list, 'cmake_compile_commands')
        selected = [item for item in commands if item['file'] == str(ROOT / LIMITS_TEST)]
        require(len(selected) == 1, 'cmake_compile_target_unique')
        args = shlex.split(selected[0]['command'])
        require(all(flag in args for flag in ('-std=c++20', '-O3', '-DNDEBUG', '-Wall', '-Wextra', '-Wpedantic', '-Werror'))
                and not any('MHGP7_TESTING' in arg for arg in args), 'cmake_nominal_release_flags')
        return dict(binary=str(binary), binary_sha256=c.sha(binary), files=files, system_headers_observed=system_headers,
            depfile_sha256=c.sha(depfile), compile_commands_sha256=c.sha(build / 'compile_commands.json'), compile_argv=args)

    def cmake_admission(self, directory, count):
        c = self.c
        require(type(count) is int and count == 6, 'cmake_new_gates_nonvacuum')
        binding = self.cmake_binding(directory)
        for side in ('before', 'after'):
            require(encoded(c.read_json(directory / ('cmake_ctest.' + side + '.json'))) == encoded(binding),
                    'cmake_ctest_stability')
        text = (directory / 'cmake_ctest.stdout').read_text()
        passed = re.findall(r'Test\s+#\s*\d+:\s+(\S+)\s+\.+\s+Passed\b', text)
        expected = {'mhgp7_full_gabriel_probe_limits', 'mhgp7_full_gabriel_probe_limits_bad_argument'} | {
            'mhgp7_full_minima_quotient_' + mode + '_' + test
            for mode in ('normal', 'optimized') for test in ('selftest', 'unknown')}
        require(len(passed) == 6 and set(passed) == expected
                and '100% tests passed, 0 tests failed out of 6' in text, 'cmake_ctest_exact_six')
        for label in ('cmake_configure', 'cmake_build', 'cmake_ctest'):
            record = c.read_json(directory / (label + '.command.json'))
            require(record['status'] == 'completed' and record['process_group_closed'] is True
                    and type(record['exit_code']) is int and record['exit_code'] == 0, 'cmake_closed_command')

    def cmake_qualification(self, directory):
        c = self.c
        build = directory / 'cmake'
        require(not build.exists(), 'cmake_fresh_directory')
        c.checked(directory, 'cmake_configure', ['/usr/bin/taskset', '-c', '0', 'cmake', '-S', str(ROOT / PRODUCT),
            '-B', str(build), '-DCMAKE_BUILD_TYPE=Release', '-DCMAKE_EXPORT_COMPILE_COMMANDS=ON',
            '-DMHGP7_DIGEST_BOOST_INCLUDE_DIR=/workspaces/E-HGP/build/v7_boost_gate/extracted/usr/include'], timeout=60)
        c.checked(directory, 'cmake_build', ['/usr/bin/taskset', '-c', '0', 'cmake', '--build', str(build),
            '--target', 'mhgp7_full_gabriel_probe_limits_gate', '--parallel', '1'], timeout=600)
        before = self.cmake_binding(directory)
        c.save(directory / 'cmake_ctest.before.json', before)
        try:
            c.checked(directory, 'cmake_ctest', ['/usr/bin/taskset', '-c', '0', 'ctest', '--test-dir', str(build),
                '-R', '^mhgp7_(full_gabriel_probe_limits|full_minima_quotient)', '--output-on-failure', '--parallel', '1'], timeout=400)
        finally:
            c.save(directory / 'cmake_ctest.after.json', self.cmake_binding(directory))
        self.cmake_admission(directory, 6)
        return 6

    def metadata_binding(self, row, proposal=None):
        expected = dict(schema=PROBE_SCHEMA, successor_accounting=ACCOUNTING,
                        meb_accounting=MEB_ACCOUNTING, limits_profile=LIMITS_PROFILE)
        if proposal is not None:
            require(proposal == 'unlimited' or (type(proposal) is int and 0 <= proposal <= MAX_U64), 'proposal_domain')
            expected.update(max_meb_proposal_supports_per_order=MAX_U64 if proposal == 'unlimited' else proposal,
                meb_proposal_budget_kind='unlimited' if proposal == 'unlimited' else 'disabled' if proposal == 0 else 'finite')
        if row.get('type') in ('configuration', 'terminal'):
            expected['legacy_F_fold_guard_applied'] = False
        for key, value in expected.items():
            require(encoded(row[key]) == encoded(value), 'metadata:' + key)

    def parser_result(self, directory, label, proposal):
        row = self.c.read_json(directory / (label + '.stdout'))
        self.metadata_binding(row, proposal)
        expected = dict(type='terminal', outcome='invalid_input', reason='probe_arguments', exit_code=2,
                        completed_orders_diagnostic=0, certificate_digest='', input_digest='',
                        complete_requested_horizontal_orders=False, integrated_inter_k_tower=False)
        for key, value in expected.items():
            require(encoded(row[key]) == encoded(value), 'parser_rejection:' + key)
        require((directory / (label + '.stderr')).read_bytes() == b'', 'parser_stderr')
        return row

    def extra_negatives(self):
        base = ['--n=8', '--s=8', '--kmax=10', '--alias-policy=eager']
        return {
            'P_duplicate_unlimited': (base + ['--meb-proposal-supports=unlimited'] * 2, 'unlimited'),
            'P_duplicate_mixed': (base + ['--meb-proposal-supports=unlimited', '--meb-proposal-supports=0'], 'unlimited'),
            'n_u64_max': ([f'--n={MAX_U64}'] + base[1:] + ['--meb-proposal-supports=0'], 0),
            'cache_u64_max': (base[:3] + ['--alias-policy=lazy', f'--cache-entries={MAX_U64}', '--meb-proposal-supports=0'], 0),
        }

    def extra_admission(self, directory, binary, attempts, reject_count):
        c = self.c
        section = directory / 'extra_domain'
        plan = self.protocol(binary, 10, EXTRA_SEQUENCE)
        require(encoded(c.read_json(section / 'protocol.json')) == encoded(plan), 'extra_protocol_admission')
        require(encoded(c.read_json(section / 'sources_before.json')) == encoded(self.snapshot(binary)), 'extra_sources_admission')
        require(type(attempts) is list and len(attempts) == 5 and type(reject_count) is int and reject_count == 4,
                'extra_nonvacuum')
        values = []
        for index in range(len(EXTRA_SEQUENCE)):
            label, _ = self.probe_command(plan, index)
            value = c.read_json(section / (label + '.verdict.json'))
            require(value['attempt_id'] == label and value['status'] == 'completed'
                    and value['capture_valid'] is True and value['attempt_success'] is True, 'extra_attempt_admission')
            values.append(value)
        require(encoded(values) == encoded(attempts), 'extra_attempt_mirrors')
        for name, (args, proposal) in self.extra_negatives().items():
            label = 'reject_' + name
            record = c.read_json(section / (label + '.command.json'))
            require(record['status'] == 'completed' and record['process_group_closed'] is True
                    and record['error'] is None and type(record['exit_code']) is int and record['exit_code'] == 2,
                    'extra_parser_closed')
            require(encoded(record['argv']) == encoded(['/usr/bin/taskset', '-c', '0', str(binary)] + args),
                    'extra_parser_command')
            self.parser_result(section, label, proposal)

    def extra_qualification(self, directory, binary):
        c = self.c
        section = directory / 'extra_domain'
        section.mkdir(exist_ok=False)
        c.prepare_directory(section, self.protocol(binary, 10, EXTRA_SEQUENCE), binary)
        values = []
        for index in range(len(EXTRA_SEQUENCE)):
            value = self.probe(section, index)
            values.append(value)
            require(value['status'] == 'completed' and value['attempt_success'] is True, 'extra_probe_failed')
        for name, (args, proposal) in self.extra_negatives().items():
            label = 'reject_' + name
            c.checked(section, label, ['/usr/bin/taskset', '-c', '0', str(binary)] + args, expected=2)
            self.parser_result(section, label, proposal)
        self.extra_admission(directory, binary, values, len(self.extra_negatives()))
        return values

    def protocol(self, binary, k, sequence):
        value = self.c.protocol(binary, k, sequence)
        value.update(schema='mhgp7-full-gabriel-mono-observation-v5', probe_schema=PROBE_SCHEMA,
            successor_accounting=ACCOUNTING, meb_accounting=MEB_ACCOUNTING,
            meb_header_sha256=self.reviewed[MEB],
            limits_header_sha256=self.reviewed[LIMITS],
            limits_profile=LIMITS_PROFILE,
            timeout_seconds_each=1200,
            first_c_sha256=self.reviewed[FIRST_C], source_map_sha256=self.pin,
            measurement_kind='same_binary_P0_vs_explicit_filtered_proposals',
            continuation_policy='each_predeclared_attempt_independent_after_closed_predecessor',
            max_meb_proposal_supports_per_order_max=MAX_U64)
        return value

    def probe_command(self, plan, index):
        n, s, policy, cache, p = plan['planned_sequence'][index]
        k = plan['kmax']
        label = f'n{n}_s{s}_k{k}_{policy}_c{cache}_p{p}'
        argv = ['timeout', '--signal=TERM', '--kill-after=10s', '1200s',
                'taskset', '-c', '6', '/usr/bin/time', '-v', plan['binary'],
                f'--n={n}', f'--s={s}', f'--kmax={k}', f'--alias-policy={policy}']
        if policy == 'lazy':
            argv.append(f'--cache-entries={cache}')
        argv.append(f'--meb-proposal-supports={p}')
        return label, argv

    def admitted_micro(self, written):
        path, micro = self.admission(written, 'micro')
        require(type(micro['parser_rejects']) is int and micro['parser_rejects'] == 15
                and type(micro['attempts']) is list and len(micro['attempts']) == 72, 'micro_admission')
        self.limits_admission(path.parent, micro['limits_qualification'])
        self.cmake_admission(path.parent, micro['cmake_new_gates'])
        require(all(v['status'] == 'completed' and v['capture_valid'] is True
                    and v['attempt_success'] is True for v in micro['attempts']), 'micro_attempt_admission')
        build_path, build = self.admission(micro['build_receipt'] + '=' + micro['build_receipt_sha256'], 'build')
        require(micro['binary'] == build['binary'] and micro['binary_sha256'] == build['binary_sha256'],
                'micro_build_binary_binding')
        values = []
        for k in (5, 10):
            sequence = [[8, s, policy, cache, p] for s in (8, 10, 12)
                        for policy, cache in (('eager', 0), ('lazy', 0), ('lazy', 1), ('lazy', 1000000))
                        for p in (0, 1, LARGE_P)]
            plan = self.protocol(ROOT / micro['binary'], k, sequence)
            section = path.parent / f'k{k}'
            require(encoded(self.c.read_json(section / 'protocol.json')) == encoded(plan), 'micro_protocol_binding')
            for index in range(len(sequence)):
                label, _ = self.probe_command(plan, index)
                value = self.c.read_json(section / (label + '.verdict.json'))
                require(value['attempt_id'] == label, 'micro_attempt_identity')
                values.append(value)
        require(encoded(values) == encoded(micro['attempts']), 'micro_attempt_mirrors')
        for mode in ('O2', 'san'):
            rows = {test: self.local_result(path.parent, 'local_' + mode + '_' + test, test)
                    for test in ('selftest', 'rejects', 'unknown')}
            require(encoded(rows) == encoded(micro['local_qualification'][mode]), 'micro_local_mirror')
        require(encoded(micro['local_qualification']['O2']) == encoded(micro['local_qualification']['san']),
                'micro_local_differential')
        self.extra_admission(path.parent, ROOT / micro['binary'], micro['extra_domain_attempts'], micro['extra_parser_rejects'])
        return path, micro, build_path

    def predecessor(self, directory, plan, index, before):
        """Closed transport/source proof only; a refusal/censor is not a success."""
        c = self.c
        label, argv = self.probe_command(plan, index)
        names = [label + suffix for suffix in ('.intent.json', '.spawn.json', '.command.json',
                 '.receipt.json', '.sources_before.json', '.sources_after.json', '.verdict.json',
                 '.raw.txt', '.stderr')]
        for name in names:
            require((directory / name).is_file() and not (directory / name).is_symlink(), 'predecessor_missing:' + name)
        intent = c.read_json(directory / (label + '.intent.json'))
        command = c.read_json(directory / (label + '.command.json'))
        receipt = c.read_json(directory / (label + '.receipt.json'))
        spawn = c.read_json(directory / (label + '.spawn.json'))
        verdict = c.read_json(directory / (label + '.verdict.json'))
        expected = dict(id=label, argv=argv, command=shlex.join(argv), cwd=str(ROOT), expected_rc=[0, 2, 3],
            outer_timeout_seconds=1220, drain_term_grace_seconds=10, process_vm_max_bytes=26 << 30,
            sanitizer_virtual_address_reservation=False, cpu_limit_seconds=1220,
            file_size_limit_bytes=64 << 20, capture='merged_exact_bytes')
        require(set(intent) == set(expected) | {'started'}, 'predecessor_intent_fields')
        for key, value in expected.items():
            require(encoded(intent[key]) == encoded(value), 'predecessor_intent:' + key)
        require(all(encoded(command[key]) == encoded(value) for key, value in intent.items()), 'predecessor_command_intent')
        require(type(command['pid']) is int and command['pid'] > 0
                and type(spawn['pid']) is int and type(spawn['pgid']) is int
                and spawn['pid'] == spawn['pgid'] == command['pid'], 'predecessor_spawn')
        require(command['process_group_closed'] is True and type(command['exit_code']) is int
                and command['status'] in ('completed', 'failed', 'censored'), 'predecessor_not_closed')
        require(command['unavailable_streams'] == [] and command['closure_errors'] == [], 'predecessor_capture_closure')
        if command['status'] == 'completed':
            require(command['error'] is None and command['exit_code'] in (0, 2, 3), 'predecessor_completed_transport')
        streams = {name: dict(bytes=(directory / name).stat().st_size, sha256=c.sha(directory / name))
                   for name in (label + '.raw.txt', label + '.stderr')}
        require(encoded(command['streams']) == encoded(streams), 'predecessor_stream_binding')
        for key, value in command.items():
            if key != 'status':
                require(encoded(receipt[key]) == encoded(value), 'predecessor_receipt:' + key)
        require(receipt['status'] == command['status'] or
                (command['status'] == 'completed' and receipt['status'] == 'failed'
                 and type(receipt.get('capture_error')) is str), 'predecessor_status_promotion')
        require(verdict['attempt_id'] == label and verdict['status'] in ('completed', 'failed'), 'predecessor_verdict')
        if verdict['status'] == 'completed':
            require(receipt['status'] == 'completed' and verdict['capture_valid'] is True
                    and type(verdict['attempt_success']) is bool
                    and verdict['attempt_success'] == (receipt['exit_code'] == 0), 'predecessor_verdict_promotion')
        for suffix in ('.sources_before.json', '.sources_after.json'):
            require(encoded(c.read_json(directory / (label + suffix))) == encoded(before), 'predecessor_source_binding')
        return dict(index=index, attempt_id=label, transport_status=command['status'],
                    verdict_status=verdict['status'], success_not_required=True,
                    artifacts={name: c.sha(directory / name) for name in names})

    def probe(self, directory, index, selftest=False):
        c = self.c
        plan = c.read_json(directory / 'protocol.json')
        binary = ROOT / plan['binary']
        before = c.read_json(directory / 'sources_before.json')
        require(self.snapshot(binary) == before, 'pre_attempt_drift')
        label, argv = self.probe_command(plan, index)
        c.save(directory / (label + '.sources_before.json'), before)
        record = self.command(directory, label, argv, (0, 2, 3), 1220, merged=True)
        record.update(terminal=None, orders=[])
        try:
            raw = directory / (label + '.raw.txt')
            require(raw.stat().st_size <= 1 << 20, 'output_size')
            rows = [c.parse(line) for line in raw.read_text().splitlines() if line.startswith('{')]
            require(len(rows) >= 2 and rows[0]['type'] == 'configuration'
                    and rows[-1]['type'] == 'terminal', 'terminal_missing')
            for row in rows:
                self.metadata_binding(row, plan['planned_sequence'][index][4])
            record.update(terminal=rows[-1], orders=rows[1:-1])
        except BaseException as exc:
            record.update(capture_error=str(exc))
            if record['status'] != 'censored':
                record['status'] = 'failed'
        receipt = directory / (label + '.receipt.json')
        c.save(receipt, record)
        verdict = dict(status='failed', attempt_id=label, started=c.now())
        try:
            require(record['status'] == 'completed', 'nonterminal_capture')
            for judge_name, script, field in (('judge', JUDGE, 'audit_status'),
                                             ('first_c', FIRST_C, 'supplement_status')):
                for optimized in (False, True):
                    tag = 'optimized' if optimized else 'normal'
                    python = ['/usr/bin/taskset', '-c', '0', sys.executable, '-B']
                    if optimized:
                        python.append('-O')
                    for testing in ((False, True) if selftest else (False,)):
                        job = label + '.' + judge_name + '_' + tag + ('_selftest' if testing else '')
                        args = python + [str(ROOT / script)] + (['--selftest'] if testing else []) + [str(receipt)]
                        c.checked(directory, job, args)
                        require((directory / (job + '.stderr')).read_bytes() == b'', 'judge_stderr')
                        output = c.read_json(directory / (job + '.stdout'))
                        require(output[field] == ('selftests_passed' if testing else 'valid'), 'judge_status')
                        require(output['probe_schema'] == PROBE_SCHEMA
                                and output['meb_accounting'] == MEB_ACCOUNTING, 'judge_version')
                        p = plan['planned_sequence'][index][4]
                        metadata = dict(successor_accounting=ACCOUNTING, limits_profile=LIMITS_PROFILE,
                            max_meb_proposal_supports_per_order=MAX_U64 if p == 'unlimited' else p,
                            meb_proposal_budget_kind='unlimited' if p == 'unlimited' else 'disabled' if p == 0 else 'finite')
                        for key, expected in metadata.items():
                            require(encoded(output[key]) == encoded(expected), 'judge_metadata:' + key)
                        if testing:
                            require(type(output['mutants_killed']) is list
                                    and all(type(name) is str for name in output['mutants_killed'])
                                    and len(set(output['mutants_killed'])) == len(output['mutants_killed'])
                                    and len(output['mutants_killed']) == (40 if judge_name == 'judge' else 25),
                                    'judge_mutant_nonvacuum')
                    normal = directory / (label + '.' + judge_name + '_normal.stdout')
                    if optimized:
                        require(normal.read_bytes() == (directory / (label + '.' + judge_name + '_optimized.stdout')).read_bytes(),
                                'judge_optimized_disagrees')
                        if selftest:
                            require((directory / (label + '.' + judge_name + '_normal_selftest.stdout')).read_bytes()
                                    == (directory / (label + '.' + judge_name + '_optimized_selftest.stdout')).read_bytes(),
                                    'judge_selftest_optimized_disagrees')
            verdict.update(status='completed', capture_valid=True, attempt_success=record['exit_code'] == 0,
                outcome=record['terminal']['outcome'], input_digest=record['terminal']['input_digest'],
                certificate_digest=record['terminal']['certificate_digest'])
        except BaseException as exc:
            verdict['reason'] = type(exc).__name__ + ': ' + str(exc)
        finally:
            try:
                after = self.snapshot(binary)
                c.save(directory / (label + '.sources_after.json'), after)
                require(after == before, 'post_attempt_drift')
            except BaseException as exc:
                verdict.update(status='failed', reason=str(exc))
            verdict['ended'] = c.now()
            c.save(directory / (label + '.verdict.json'), verdict)
        return verdict

    def micro(self, name, build):
        c = self.c
        path, admitted = self.admission(build, 'build')
        binary = ROOT / admitted['binary']
        directory = c.new_directory('micro', name)
        before = self.snapshot(binary)
        c.save(directory / 'sources_before.json', before)
        result = dict(kind='micro', status='failed', started=c.now(), attempts=[],
            binary=str(binary.relative_to(ROOT)), binary_sha256=c.sha(binary), build_receipt=str(path),
            build_receipt_sha256=c.sha(path))
        try:
            c.metadata(directory)
            result['limits_qualification'] = self.limits_qualification(directory)
            result['cmake_new_gates'] = self.cmake_qualification(directory)
            base = ['--n=8', '--s=8', '--kmax=10', '--alias-policy=eager']
            negatives = {
                'P_missing': base, 'P_duplicate_zero': base + ['--meb-proposal-supports=0'] * 2,
                'P_duplicate_nonzero': base + ['--meb-proposal-supports=1'] * 2,
                'P_empty': base + ['--meb-proposal-supports='],
                'P_negative': base + ['--meb-proposal-supports=-1'],
                'P_float': base + ['--meb-proposal-supports=1.0'],
                'P_plus_sign': base + ['--meb-proposal-supports=+1'],
                'P_u64_overflow': base + ['--meb-proposal-supports=18446744073709551616'],
                'unknown': base + ['--meb-proposal-supports=0', '--unknown'],
                'n1': ['--n=1'] + base[1:] + ['--meb-proposal-supports=0'],
                's9': [base[0], '--s=9'] + base[2:] + ['--meb-proposal-supports=0'],
                'k0': base[:2] + ['--kmax=0', base[3], '--meb-proposal-supports=0'],
                'eager_cache': base + ['--cache-entries=0', '--meb-proposal-supports=0'],
                'lazy_no_cache': base[:3] + ['--alias-policy=lazy', '--meb-proposal-supports=0'],
                'policy_missing': base[:3] + ['--meb-proposal-supports=0'],
            }
            for label, args in negatives.items():
                c.checked(directory, 'reject_' + label, ['/usr/bin/taskset', '-c', '0', str(binary)] + args, expected=2)
                self.parser_result(directory, 'reject_' + label, 1 if label == 'P_duplicate_nonzero' else 0)
            result['parser_rejects'] = len(negatives)
            c.checked(directory, 'digest_selftest', ['/usr/bin/taskset', '-c', '0', str(binary), '--digest-selftest'])
            digest = c.read_json(directory / 'digest_selftest.stdout')
            self.metadata_binding(digest)
            require(digest['passed'] is True and type(digest['checks']) is int and digest['checks'] == 24
                    and type(digest['failures']) is int and digest['failures'] == 0
                    and type(digest['expected_checks']) is int and digest['expected_checks'] == 24
                    and digest['type'] == 'digest_selftest', 'digest_nonvacuum')
            # Test delta only: actual producer/header remains pinned to its own 30+30 qualification.
            local_results = {}
            for mode, flags in (('O2', ['-O2']), ('san', ['-O1', '-g', '-fsanitize=address,undefined',
                        '-fno-sanitize-recover=all', '-fno-omit-frame-pointer', '-fno-pie', '-no-pie'])):
                local = directory / ('local_' + mode)
                depfile = directory / ('local_' + mode + '.d')
                c.checked(directory, 'compile_local_' + mode, ['/usr/bin/taskset', '-c', '0', '/usr/bin/g++',
                    '-std=c++20', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-MMD', '-MF',
                    str(depfile)] + flags + [str(ROOT / LOCAL), '-o', str(local)], timeout=600)
                binding = self.local_binding(local, depfile, ROOT)
                c.save(directory / ('local_' + mode + '.dependencies.json'), binding)
                local_results[mode] = {}
                for arg, code in (('--selftest', 0), ('--rejects', 0), ('--unknown', 2)):
                    label = 'local_' + mode + '_' + arg[2:]
                    self.local_run(directory, label, local, depfile, ROOT, binding, arg, code)
                    local_results[mode][arg[2:]] = self.local_result(directory, label, arg[2:])
            require(encoded(local_results['O2']) == encoded(local_results['san']), 'local_O2_san_disagreement')
            result['local_qualification'] = local_results
            # Physical private mutant, never a switch in the product.
            variant = directory / 'q4_before_q3'
            self.copy_sources(variant)
            header = variant / 'source_snapshot' / MEB
            raw = header.read_text()
            start = raw.index('  for (size_t a = 0; a < old_count; ++a)')
            middle = raw.index('  for (size_t a = 0; a < old_count; ++a)', start + 1)
            end = raw.index('  return Attempt::kRejected;\n}', middle)
            old = raw[start:end]
            new = raw[middle:end] + raw[start:middle]
            require(raw.count(old) == 1 and '}, 3)' in old and '}, 4)' in old, 'mutant_unique_site')
            changed = raw[:start] + new + raw[end:]
            # Mechanical rewrite of this newly created private copy only.
            header.write_text(changed)
            mutation_pin = sha(changed.encode())
            require(c.sha(header) == mutation_pin, 'mutant_written_bytes')
            c.save(variant / 'mutation.json', dict(target=MEB, before_sha256=self.reviewed[MEB],
                after_sha256=mutation_pin, old=old, new=new, product_mutated=False))
            mutant = variant / 'local_mutant'
            c.checked(directory, 'compile_tetra_mutant', ['/usr/bin/taskset', '-c', '0', '/usr/bin/g++',
                '-std=c++20', '-O2', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-MMD', '-MF',
                str(variant / 'local_mutant.d'), str(variant / 'source_snapshot' / LOCAL), '-o', str(mutant)], timeout=600)
            mutant_binding = self.local_binding(mutant, variant / 'local_mutant.d', variant / 'source_snapshot',
                                                {MEB: mutation_pin})
            c.save(variant / 'local_mutant.dependencies.json', mutant_binding)
            self.local_run(directory, 'tetra_mutant', mutant, variant / 'local_mutant.d',
                           variant / 'source_snapshot', mutant_binding, '--selftest', 1)
            cause = 'meb proposal local rejected: tetrahedron.P3_requires_reference_fallback'
            require((directory / 'tetra_mutant.stderr').read_bytes() == (cause + '\n').encode()
                    and (directory / 'tetra_mutant.stdout').read_bytes() == b'', 'mutant_wrong_cause')
            result['tetra_mutant_first_cause'] = cause
            for k in (5, 10):
                section = directory / f'k{k}'
                section.mkdir()
                sequence = [[8, s, policy, cache, p] for s in (8, 10, 12)
                            for policy, cache in (('eager', 0), ('lazy', 0), ('lazy', 1), ('lazy', 1000000))
                            for p in (0, 1, LARGE_P)]
                c.prepare_directory(section, self.protocol(binary, k, sequence), binary)
                values = []
                for index in range(len(sequence)):
                    value = self.probe(section, index, selftest=k == 10 and index in (0, 1, 2, 9, 10, 11))
                    result['attempts'].append(value)
                    require(value['status'] == 'completed' and value['attempt_success'], 'micro_capture_failed')
                    values.append(value)
                require(len({v['input_digest'] for v in values}) == 1
                        and len({v['certificate_digest'] for v in values}) == 1, 'micro_semantic_disagreement')
            require(len(result['attempts']) == 72, 'micro_nonvacuum')
            result['extra_domain_attempts'] = self.extra_qualification(directory, binary)
            result['extra_parser_rejects'] = len(self.extra_negatives())
            result.update(status='completed', reason='72_micro_plus_5_domain_4_parser_extras_and_local_qualification')
        except BaseException as exc:
            result['reason'] = type(exc).__name__ + ': ' + str(exc)
        finally:
            c.seal(directory, result, before, binary)
        require(result['status'] == 'completed', 'micro_failed')

    def prepare(self, name, micro):
        c = self.c
        path, admitted, _ = self.admitted_micro(micro)
        directory = c.new_directory('heavy', name)
        binary = ROOT / admitted['binary']
        plan = self.protocol(binary, 10, SEQUENCE)
        plan.update(micro_receipt=str(path), micro_receipt_sha256=c.sha(path))
        c.prepare_directory(directory, plan, binary)
        c.metadata(directory)

    def attempt(self, path, index, go):
        require(go, 'explicit_reviewed_heavy_GO_required')
        c = self.c
        directory = c.owned(path)
        plan = c.read_json(directory / 'protocol.json')
        require(type(index) is int and 0 <= index < len(SEQUENCE), 'attempt_index')
        micro_path, micro, build_path = self.admitted_micro(plan['micro_receipt'] + '=' + plan['micro_receipt_sha256'])
        binary = ROOT / micro['binary']
        expected = self.protocol(binary, 10, SEQUENCE)
        expected.update(micro_receipt=str(micro_path), micro_receipt_sha256=c.sha(micro_path))
        require(encoded(plan) == encoded(expected), 'heavy_complete_protocol_admission')
        before = self.snapshot(binary)
        require(encoded(c.read_json(directory / 'sources_before.json')) == encoded(before), 'heavy_admission_sources')
        predecessors = [self.predecessor(directory, plan, i, before) for i in range(index)]
        c.save(directory / f'admission_{index:02d}.json', dict(schema=SCHEMA, index=index,
            protocol_sha256=c.sha(directory / 'protocol.json'), sources=before,
            micro_receipt=str(micro_path), micro_receipt_sha256=c.sha(micro_path),
            build_receipt=str(build_path), build_receipt_sha256=c.sha(build_path),
            predecessors=predecessors, public_status='not_claimed'))
        value = self.probe(directory, index)
        print(json.dumps(value, sort_keys=True))
        require(value['status'] == 'completed', 'attempt_capture_invalid_or_censored')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('action', choices=('snapshot', 'build', 'micro', 'prepare', 'attempt'))
    parser.add_argument('--source-sha256')
    parser.add_argument('--id')
    parser.add_argument('--admission')
    parser.add_argument('--directory')
    parser.add_argument('--index', type=int)
    parser.add_argument('--go-reviewed-heavy', action='store_true')
    args = parser.parse_args()
    if args.action == 'snapshot':
        values = sources()
        print(json.dumps(dict(files=values, source_map_sha256=sha(encoded(values))), indent=2, sort_keys=True))
        return
    controller = Controller(args.source_sha256)
    BASE.mkdir(exist_ok=True)
    with (BASE / 'controller.lock').open('a') as lock:
        fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
        for sig in controller.c.SIGNALS:
            signal.signal(sig, controller.c.interrupted)
        if args.action == 'build': controller.build(args.id)
        elif args.action == 'micro': controller.micro(args.id, args.admission)
        elif args.action == 'prepare': controller.prepare(args.id, args.admission)
        elif args.action == 'attempt': controller.attempt(args.directory, args.index, args.go_reviewed_heavy)


if __name__ == '__main__':
    try:
        main()
    except BaseException as error:
        if isinstance(error, SystemExit):
            raise
        print(json.dumps(dict(status='failed', reason=type(error).__name__ + ': ' + str(error)), sort_keys=True))
        sys.exit(1)
