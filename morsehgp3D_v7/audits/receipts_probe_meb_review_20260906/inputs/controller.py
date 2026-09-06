#!/usr/bin/env python3
"""Fresh FULL v4 capture. Inert without an explicit action and source pin.

Only the pinned old controller's helpers/build are imported, never its main,
micro or heavy scheduler. Every heavy action runs exactly one declared job;
refusals are recorded and do not prevent the paired arm being attempted.
No Git mutation, GCP, cap adaptation or inherited geometry qualification.
"""
from __future__ import annotations

import argparse
import fcntl
import hashlib
import json
import os
from pathlib import Path
import resource
import shlex
import signal
import subprocess
import sys
import time
import types

ROOT = Path('/workspaces/E-HGP')
BASE = ROOT / 'build/v7_meb_probe_20260906_controller'
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
SCHEMA = 'mhgp7-meb-probe-controller-v1'
PROBE_SCHEMA = 'mhgp7-full-gabriel-probe-v4'
ACCOUNTING = 'full_successor_reads_writes_no_last_pair_v2'
MEB_ACCOUNTING = 'reference_ordinal_plus_native_z_q3_q4_proposal_v2'
LARGE_P = 584000000
SEQUENCE = [[n, s, 'lazy', 1000000, p]
            for s in (8, 10, 12) for n in (8000, 16000, 32000)
            for p in ((0, LARGE_P) if s != 10 else (LARGE_P, 0))]


def require(ok, reason):
    if not ok:
        raise ValueError(reason)


def encoded(value):
    return (json.dumps(value, indent=2, sort_keys=True) + '\n').encode()


def sha(raw):
    return hashlib.sha256(raw).hexdigest()


def sources():
    paths = {p for p in (ROOT / PRODUCT / 'src').rglob('*') if p.is_file()}
    paths.update(ROOT / name for name in (PROBE, DIGEST, JUDGE, FIRST_C, LOCAL,
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
        compilation = any(Path(arg).name in ('g++', 'c++') for arg in argv)
        sanitizer_execution = any(Path(arg).name == 'local_san' for arg in argv)
        fsize = (512 if compilation else 64) << 20
        intent = dict(id=label, command=shlex.join(argv), argv=argv, cwd=str(ROOT),
            started=c.now(), expected_rc=list(expected), outer_timeout_seconds=timeout,
            drain_term_grace_seconds=10,
            process_vm_max_bytes=None if sanitizer_execution else 26 << 30,
            sanitizer_virtual_address_reservation=sanitizer_execution,
            cpu_limit_seconds=620, file_size_limit_bytes=fsize,
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
            resource.setrlimit(resource.RLIMIT_CPU, (620, 620))
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

    def local_binding(self, binary, depfile, source_root, overrides=None):
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
        require({LOCAL, MEB, F} <= files.keys(), 'local_dependencies_nonvacuum')
        require(binary.is_file() and not binary.is_symlink(), 'local_binary_regular')
        return dict(source_root=str(source_root), files=files, depfile_sha256=c.sha(depfile),
                    binary=str(binary), binary_sha256=c.sha(binary), overrides=overrides)

    def local_run(self, directory, label, binary, depfile, source_root, admitted, arg, code):
        c = self.c
        before = self.local_binding(binary, depfile, source_root, admitted['overrides'])
        require(encoded(before) == encoded(admitted), 'local_pre_run_drift')
        c.save(directory / (label + '.local_before.json'), before)
        try:
            c.checked(directory, label, ['/usr/bin/taskset', '-c', '0', str(binary), arg], expected=code)
        finally:
            after = self.local_binding(binary, depfile, source_root, admitted['overrides'])
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

    def protocol(self, binary, k, sequence):
        value = self.c.protocol(binary, k, sequence)
        value.update(schema='mhgp7-full-gabriel-mono-observation-v4', probe_schema=PROBE_SCHEMA,
            successor_accounting=ACCOUNTING, meb_accounting=MEB_ACCOUNTING,
            meb_header_sha256=self.reviewed[MEB],
            first_c_sha256=self.reviewed[FIRST_C], source_map_sha256=self.pin,
            measurement_kind='same_binary_P0_vs_explicit_filtered_proposals',
            continuation_policy='each_predeclared_attempt_independent_after_closed_predecessor',
            max_meb_proposal_supports_per_order_max=LARGE_P)
        return value

    def probe_command(self, plan, index):
        n, s, policy, cache, p = plan['planned_sequence'][index]
        k = plan['kmax']
        label = f'n{n}_s{s}_k{k}_{policy}_c{cache}_p{p}'
        argv = ['timeout', '--signal=TERM', '--kill-after=10s', '600s',
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
            outer_timeout_seconds=620, drain_term_grace_seconds=10, process_vm_max_bytes=26 << 30,
            sanitizer_virtual_address_reservation=False, cpu_limit_seconds=620,
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
        record = self.command(directory, label, argv, (0, 2, 3), 620, merged=True)
        record.update(terminal=None, orders=[])
        try:
            raw = directory / (label + '.raw.txt')
            require(raw.stat().st_size <= 1 << 20, 'output_size')
            rows = [c.parse(line) for line in raw.read_text().splitlines() if line.startswith('{')]
            require(len(rows) >= 2 and rows[0]['type'] == 'configuration'
                    and rows[-1]['type'] == 'terminal', 'terminal_missing')
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
                        output = c.read_json(directory / (job + '.stdout'))
                        require(output[field] == ('selftests_passed' if testing else 'valid'), 'judge_status')
                        require(output['probe_schema'] == PROBE_SCHEMA
                                and output['meb_accounting'] == MEB_ACCOUNTING, 'judge_version')
                        if testing:
                            require(len(output['mutants_killed']) == (80 if judge_name == 'judge' else 47),
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
            base = ['--n=8', '--s=8', '--kmax=10', '--alias-policy=eager']
            negatives = {
                'P_missing': base, 'P_duplicate_zero': base + ['--meb-proposal-supports=0'] * 2,
                'P_duplicate_nonzero': base + ['--meb-proposal-supports=1'] * 2,
                'P_empty': base + ['--meb-proposal-supports='],
                'P_negative': base + ['--meb-proposal-supports=-1'],
                'P_float': base + ['--meb-proposal-supports=1.0'],
                'P_over_cap': base + [f'--meb-proposal-supports={LARGE_P + 1}'],
                'P_u64_overflow': base + ['--meb-proposal-supports=18446744073709551616'],
                'unknown': base + ['--meb-proposal-supports=0', '--unknown'],
                'n9': ['--n=9'] + base[1:] + ['--meb-proposal-supports=0'],
                's9': [base[0], '--s=9'] + base[2:] + ['--meb-proposal-supports=0'],
                'k0': base[:2] + ['--kmax=0', base[3], '--meb-proposal-supports=0'],
                'eager_cache': base + ['--cache-entries=0', '--meb-proposal-supports=0'],
                'lazy_no_cache': base[:3] + ['--alias-policy=lazy', '--meb-proposal-supports=0'],
                'policy_missing': base[:3] + ['--meb-proposal-supports=0'],
            }
            for label, args in negatives.items():
                c.checked(directory, 'reject_' + label, ['/usr/bin/taskset', '-c', '0', str(binary)] + args, expected=2)
                row = c.read_json(directory / ('reject_' + label + '.stdout'))
                require(row['schema'] == PROBE_SCHEMA and row['outcome'] == 'invalid_input'
                        and row['reason'] == 'probe_arguments' and row['exit_code'] == 2
                        and row['completed_orders_diagnostic'] == 0 and row['certificate_digest'] == '', 'parser_rejection')
            result['parser_rejects'] = len(negatives)
            c.checked(directory, 'digest_selftest', ['/usr/bin/taskset', '-c', '0', str(binary), '--digest-selftest'])
            digest = c.read_json(directory / 'digest_selftest.stdout')
            require(digest['passed'] is True and digest['checks'] == 24, 'digest_nonvacuum')
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
            result.update(status='completed', reason='72_micro_and_local_sentinel_qualification')
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
