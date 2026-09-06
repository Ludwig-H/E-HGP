#!/usr/bin/env python3
"""Inert unless --execute: private physical mutants and FULL form-fault overlay.

--snapshot is read-only and prints the source/Boost/toolchain admission digest.
No product file, old receipt, audit file, Git index or GCP resource is modified.
System headers/libraries are host dependencies, not a hermetic toolchain claim.
"""
from __future__ import annotations

import argparse
from datetime import datetime, timezone
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

ROOT = Path('/workspaces/E-HGP')
REL = 'build/v7_meb_product_mutation_qualification_20260906'
FAULT = 'build/v7_meb_product_fault_20260906'
PRODUCT = 'morsehgp3D_v7'
HELPER = PRODUCT + '/src/forest/meb_proposal.hpp'
FULL = PRODUCT + '/src/forest/full_gabriel.hpp'
F = PRODUCT + '/src/forest/silent_incidence.hpp'
LOCAL_GATE = PRODUCT + '/tests/meb_proposal_local_gate.cpp'
FULL_GATE = PRODUCT + '/tests/full_gabriel_meb_gate.cpp'
BOOST = ROOT / 'build/v7_boost_gate/extracted/usr/include'
COMPILER = Path('/usr/bin/c++').resolve()
PINS = {
    HELPER: 'f922544b5cfdc214de96ecd49520e318ea8632d14a8142ef21fd248f9cc38fb3',
    FULL: 'a946e31dde8fbd8ec528d6f5e94f9c727998acc172b4dd29c084dd522c730d1d',
    F: 'f75a136a320ddd1ace025436874585c2226e6150b8f2ebc37920b8dfc7e36c76',
    LOCAL_GATE: '273eb73d73fc1b4ac59048e5ff49660e81710dda57815a0c12266d8d91ff9930',
    FULL_GATE: 'd673fcadf2433c4571206e4c37571be3b3548bb4b148e3b6f45bdcb548bed808',
    FAULT + '/fault_hook.hpp': '61f9e5413ba8a6fc41ae506e4c24c601803fda9877113b099a6adeff8f923d6d',
    FAULT + '/full_fault_gate.cpp': '079ee371a04eabd78dead3413b004e9d4511766bcc5cdccf47a63b1666b3673f',
}
P0_CAUSE = 'P=0 retains only F work with A=c'
MUTATIONS = {
    'charge_after': (HELPER,
        '  ++work->meb_proposal_supports;\n  observer->before_form(*work, limits, q);\n'
        '  const bool built = form(ix, sites, slots, q, result);',
        '  observer->before_form(*work, limits, q);\n'
        '  const bool built = form(ix, sites, slots, q, result);\n'
        '  ++work->meb_proposal_supports;',
        LOCAL_GATE, '--rejects', 'observer.prospective_P_charge'),
    'drop_A': (HELPER,
        '      work.reference_supports += stats.meb_supports - prior;',
        '      // Private mutation: omit the real-F work mirror.',
        LOCAL_GATE, '--selftest', 'physical_F.only_real_fallback_supports_not_virtual_ordinal'),
    'reset_Work': (FULL,
        '  bool miniball(const std::array<i32, 11>& sites, size_t n, LocalBall& ball) {\n',
        '  bool miniball(const std::array<i32, 11>& sites, size_t n, LocalBall& ball) {\n'
        '    meb_work = {};  // Private mutation: incorrectly reset the order budget.\n',
        FULL_GATE, '--selftest', P0_CAUSE),
    'drop_FULL_P_mirror': (FULL,
        '    out.stats.meb_proposal = meb_work;',
        '    // Private mutation: omit the external FULL Work mirror.',
        FULL_GATE, '--selftest', P0_CAUSE),
}
HOOK_OLD = '  void before_form(const Work&, const Limits&, u8) const noexcept {}'
HOOK_NEW = ('  void before_form(const Work& work, const Limits&, u8 q) const {\n'
            '    ::mhgp7_test_before_form(work.meb_proposal_supports, work.certified, q);\n  }')


def require(ok, reason):
    if not ok:
        raise ValueError(reason)


def sha(raw):
    return hashlib.sha256(raw).hexdigest()


def encoded(obj):
    return (json.dumps(obj, sort_keys=True, indent=2) + '\n').encode()


def utc():
    return datetime.now(timezone.utc).isoformat()


def read(path):
    require(path.is_file() and not path.is_symlink(), 'not_regular:' + str(path))
    a = path.stat()
    raw = path.read_bytes()
    b = path.stat()
    require((a.st_ino, a.st_size, a.st_mtime_ns, a.st_ctime_ns) ==
            (b.st_ino, b.st_size, b.st_mtime_ns, b.st_ctime_ns), 'changed_while_reading:' + str(path))
    return raw


def save(path, raw):
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open('xb') as stream:
        stream.write(raw)


def files(directory):
    require(directory.is_dir() and not directory.is_symlink(), 'directory:' + str(directory))
    for path in sorted(directory.rglob('*')):
        require(not path.is_symlink(), 'symlink:' + str(path))
        if path.is_file():
            yield path


def inputs(check_authority=True):
    names = set(PINS) | {REL + '/record.py', PRODUCT + '/CMakeLists.txt'}
    for part in ('src', 'oracle', 'tests'):
        names.update(str(p.relative_to(ROOT)) for p in files(ROOT / PRODUCT / part))
    values = {name: read(ROOT / name) for name in sorted(names)}
    if check_authority:
        for name, pin in PINS.items():
            require(sha(values[name]) == pin, 'authority.changed:' + name)
    # Pin Boost before compiling, never accept a previously unseen depfile hash.
    boost = {str(p.relative_to(BOOST)): sha(read(p)) for p in files(BOOST / 'boost')}
    require('boost/version.hpp' in boost, 'boost.headers_missing')
    pins = dict(sources={n: sha(v) for n, v in values.items()}, boost_headers=boost,
                compiler_path=str(COMPILER), compiler_sha256=sha(read(COMPILER)))
    return values, pins


def map_tree(root):
    return {str(p.relative_to(root)): sha(read(p)) for p in files(root)}


def strict_json(raw):
    def pairs(items):
        result = {}
        for key, value in items:
            require(key not in result, 'json.duplicate_key')
            result[key] = value
        return result
    return json.loads(raw, object_pairs_hook=pairs,
                      parse_constant=lambda _: require(False, 'json.nonfinite'))


def variant(values, out, label, target, old, new):
    require(values[target].decode().count(old) == 1, 'mutation.site_count:' + label)
    changed = values[target].decode().replace(old, new).encode()
    root = out / 'variants' / label
    root.mkdir(parents=True, exist_ok=False)
    expected = {}
    for name, raw in values.items():
        raw = changed if name == target else raw
        save(root / name, raw)
        expected[name] = sha(raw)
    require(expected[F] == PINS[F], 'variant.changed_F')
    save(out / 'variants' / (label + '.json'), encoded(dict(
        target=target, before_sha256=sha(values[target]), after_sha256=sha(changed),
        old=old, new=new, source_pins=expected)))
    return root, expected


def group_exists(pid):
    try:
        os.killpg(pid, 0)
        return True
    except ProcessLookupError:
        return False


def close_group(proc):
    # Always drain our own session, even when its leader already exited.
    try:
        os.killpg(proc.pid, signal.SIGKILL)
    except ProcessLookupError:
        pass
    proc.wait(timeout=2)
    until = time.monotonic() + 2
    while group_exists(proc.pid) and time.monotonic() < until:
        time.sleep(0.02)
    return not group_exists(proc.pid)


def execute(out, expected_source, cpu):
    started = time.monotonic()
    deadline = started + 1800
    values, pins = inputs()
    require(sha(encoded(pins)) == expected_source, 'sources.not_admitted')
    require(sha(values[REL + '/record.py']) == sha(read(Path(__file__).resolve())),
            'sources.executed_driver_differs_from_snapshot')
    require(cpu in os.sched_getaffinity(0), 'cpu.not_available')
    require(out.parent == ROOT / REL and out.name.startswith('run_') and not out.exists(),
            'output.must_be_fresh_run_directory_under_controller')
    out.mkdir(exist_ok=False)
    report = dict(schema='mhgp7-product-meb-mutation-run-v1', status='running',
        public_status='not_claimed', gcp='not_used', started_utc=utc(),
        source_map_sha256=expected_source, cpu=cpu, commands={}, binaries={},
        mutations={}, variants={}, fault={}, caps=dict(cpu_seconds=120, process_wall_seconds=300,
        campaign_wall_seconds=1800, compiler_file_bytes=512 << 20, execution_file_bytes=64 << 20),
        nominal_product_mutated=False, F_unchanged=True,
        toolchain_scope='compiler and Boost pinned; system headers/libraries are host dependencies')
    env = dict(PATH='/usr/bin:/bin', LC_ALL='C', LANG='C', TMPDIR=str(out / 'tmp'),
        OMP_NUM_THREADS='1', OPENBLAS_NUM_THREADS='1', PYTHONDONTWRITEBYTECODE='1',
        ASAN_OPTIONS='detect_leaks=1:halt_on_error=1',
        LSAN_OPTIONS='exitcode=23', UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1')
    completed_variants = []

    def child_limits(file_bytes):
        os.sched_setaffinity(0, {cpu})
        resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
        resource.setrlimit(resource.RLIMIT_CPU, (120, 120))
        resource.setrlimit(resource.RLIMIT_FSIZE, (file_bytes, file_bytes))

    def command(label, argv, cwd, expected_code=0):
        begin = time.monotonic()
        timeout = min(296.0, deadline - begin - 4.0)
        require(timeout > 0, 'campaign.deadline')
        executepin = sha(read(Path(argv[0])))
        file_bytes = (512 if Path(argv[0]) == COMPILER else 64) << 20
        base = out / 'commands' / label
        intent = dict(argv=argv, cwd=str(cwd), expected_exit_code=expected_code,
            executable_sha256=executepin, started_utc=utc(), wall_wait_seconds=timeout,
            cpu=cpu, cpu_seconds=120, process_wall_seconds=300,
            file_size_limit_bytes=file_bytes, stdin='DEVNULL')
        save(base.with_suffix('.intent.json'), encoded(intent))
        proc = None
        error = None
        closed = None
        try:
            with base.with_suffix('.stdout').open('xb') as stdout, base.with_suffix('.stderr').open('xb') as stderr:
                proc = subprocess.Popen(argv, cwd=cwd, env=env, stdin=subprocess.DEVNULL,
                    stdout=stdout, stderr=stderr, start_new_session=True,
                    preexec_fn=lambda: child_limits(file_bytes))
                save(base.with_suffix('.spawn.json'), encoded(dict(pid=proc.pid, pgid=proc.pid, utc=utc())))
                proc.wait(timeout=timeout)
        except BaseException as exc:
            error = type(exc).__name__ + ':' + str(exc)
        finally:
            if proc is not None:
                try:
                    closed = close_group(proc)
                except BaseException as exc:
                    closed = False
                    error = (error or '') + ';cleanup:' + type(exc).__name__ + ':' + str(exc)
        stdout = read(base.with_suffix('.stdout')) if base.with_suffix('.stdout').exists() else b''
        stderr = read(base.with_suffix('.stderr')) if base.with_suffix('.stderr').exists() else b''
        row = dict(intent, ended_utc=utc(), elapsed_seconds=time.monotonic() - begin,
            pid=None if proc is None else proc.pid, pgid=None if proc is None else proc.pid,
            exit_code=None if proc is None else proc.returncode, error=error, process_group_closed=closed,
            stdout_sha256=sha(stdout), stderr_sha256=sha(stderr), stdout_bytes=len(stdout), stderr_bytes=len(stderr))
        save(base.with_suffix('.json'), encoded(row))
        report['commands'][label] = row
        print(json.dumps(dict(command=label, code=row['exit_code'], error=error)), flush=True)
        require(error is None and closed is True and row['exit_code'] == expected_code and
                row['elapsed_seconds'] <= 300 and time.monotonic() <= deadline, 'command.failed:' + label)
        require(sha(read(Path(argv[0]))) == executepin, 'command.executable_changed')
        return stdout, stderr

    def compile_gate(label, root, expected, gate, san=False, hook=False):
        binary = out / 'bin' / label
        dep = out / 'dependencies' / (label + '.d')
        binary.parent.mkdir(exist_ok=True)
        dep.parent.mkdir(exist_ok=True)
        flags = ['-O1', '-g', '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                 '-fno-omit-frame-pointer', '-fno-pie', '-no-pie'] if san else ['-O2']
        argv = [str(COMPILER), '-std=c++20', '-Wall', '-Wextra', '-Wpedantic', '-Werror',
            '-pthread', *flags, '-I', str(root / PRODUCT), '-I', str(BOOST),
            '-MMD', '-MF', str(dep)]
        if hook:
            argv += ['-include', str(root / FAULT / 'fault_hook.hpp')]
        argv += [str(root / gate), '-o', str(binary)]
        _, stderr = command(label + '_compile', argv, root)
        require(stderr == b'', 'compile.stderr:' + label)
        target, body = read(dep).decode().replace('\\\n', ' ').split(':', 1)
        require(shlex.split(target) == [str(binary)], 'dependency.target')
        actual = {}
        for item in shlex.split(body):
            path = Path(item).resolve()
            if path.is_relative_to(root):
                name = str(path.relative_to(root))
                require(name in expected and sha(read(path)) == expected[name], 'dependency.unpinned_source:' + name)
            elif path.is_relative_to(BOOST):
                name = 'BOOST/' + str(path.relative_to(BOOST))
                require(name[6:] in pins['boost_headers'] and
                        sha(read(path)) == pins['boost_headers'][name[6:]], 'dependency.unpinned_boost')
            else:
                raise ValueError('dependency.outside_snapshot:' + str(path))
            actual[name] = sha(read(path))
        required = {gate, HELPER, F}
        if gate != LOCAL_GATE:
            required.add(FULL)
        if hook:
            required.add(FAULT + '/fault_hook.hpp')
        require(required <= set(actual), 'dependency.missing_authority')
        save(out / 'dependencies' / (label + '.json'), encoded(actual))
        binarypin = sha(read(binary))
        report['binaries'][label] = dict(path=str(binary.relative_to(out)), sha256=binarypin)
        return str(binary)

    try:
        (out / 'tmp').mkdir()
        save(out / 'inputs_before.json', encoded(pins))
        save(out / 'environment.json', encoded(env))
        save(out / 'host.json', encoded(dict(uname=list(os.uname()), affinity=sorted(os.sched_getaffinity(0)),
            python=sys.version, recorder_sha256=sha(values[REL + '/record.py']))))
        snap = out / 'snapshot'
        for name, raw in values.items():
            save(snap / name, raw)
        command('compiler_version', [str(COMPILER), '--version'], snap)
        for label, (target, old, new, gate, argument, cause) in MUTATIONS.items():
            root, expected = variant(values, out, label, target, old, new)
            completed_variants.append((root, expected))
            report['variants'][label] = dict(root=str(root.relative_to(out)), source_map_sha256=sha(encoded(expected)))
            binary = compile_gate(label, root, expected, gate)
            _, stderr = command(label + '_run', [binary, argument], root, 1)
            lines = stderr.decode().splitlines()
            require(lines, 'mutant.no_diagnostic:' + label)
            if gate == LOCAL_GATE:
                require(lines[0] == 'meb proposal local rejected: ' + cause, 'mutant.wrong_first_cause:' + label)
            else:
                require(lines[0].startswith('FAIL [') and '] ' in lines[0] and
                        lines[0].split('] ', 1)[1] == cause, 'mutant.wrong_first_FAIL:' + label)
            report['mutations'][label] = dict(status='rejected_as_expected', exit_code=1,
                first_diagnostic=lines[0], cause=cause, private_only=True)
        root, expected = variant(values, out, 'form_fault', HELPER, HOOK_OLD, HOOK_NEW)
        completed_variants.append((root, expected))
        report['variants']['form_fault'] = dict(root=str(root.relative_to(out)), source_map_sha256=sha(encoded(expected)))
        for mode in ('o2', 'san'):
            binary = compile_gate('form_fault_' + mode, root, expected,
                FAULT + '/full_fault_gate.cpp', san=mode == 'san', hook=True)
            stdout, stderr = command('form_fault_' + mode + '_selftest', [binary, '--selftest'], root)
            require(stderr == b'', 'fault.stderr')
            rows = stdout.decode().splitlines()
            require(len(rows) == 1, 'fault.stdout_shape')
            row = strict_json(rows[0])
            require(row.get('schema') == 'mhgp7-private-full-meb-form-fault-v1' and
                    row.get('status') == 'passed' and row.get('public_status') == 'not_claimed' and
                    row.get('nominal_noobserver_exception_claim') is False and
                    row.get('F_exception_coverage') == 'not_exercised', 'fault.scope')
            fixed = dict(cases=12, public_refusals=4, runtime_propagations=2, builder_propagations=6,
                         mirrors=10, compared_mirrors=8, baselines=2, retries=6, failures=0)
            require(set(row) == set(fixed) | {'schema', 'status', 'public_status',
                'nominal_noobserver_exception_claim', 'F_exception_coverage', 'paid_at_throw', 'checks'},
                'fault.closed_schema')
            require(all(type(row.get(k)) is int and row[k] == v for k, v in fixed.items()) and
                    type(row.get('paid_at_throw')) is int and row['paid_at_throw'] >= 36 and
                    type(row.get('checks')) is int and row['checks'] > 0, 'fault.fixed_counts')
            report['fault'][mode] = row
            stdout, stderr = command('form_fault_' + mode + '_unknown', [binary, '--unknown'], root, 2)
            require(stdout == stderr == b'', 'fault.unknown_output')
        require(report['fault']['o2'] == report['fault']['san'], 'fault.optimization_mismatch')
        require(len(report['commands']) == 15 and len(report['binaries']) == 6 and
                len(report['mutations']) == 4, 'campaign.inventory')
        report['status'] = 'completed'
    except BaseException as exc:
        report['status'] = 'failed'
        report['error'] = type(exc).__name__ + ':' + str(exc)
    finally:
        try:
            _, observed = inputs(check_authority=False)
            save(out / 'inputs_after.json', encoded(observed))
            report['sources_stable'] = observed == pins
            require(report['sources_stable'], 'live_sources.changed')
            require(map_tree(out / 'snapshot') == pins['sources'], 'snapshot.changed')
            for root, expected in completed_variants:
                require(map_tree(root) == expected, 'variant.changed:' + root.name)
            for item in report['binaries'].values():
                require(sha(read(out / item['path'])) == item['sha256'], 'binary.changed')
            for label, row in report['commands'].items():
                for suffix in ('stdout', 'stderr'):
                    require(sha(read(out / 'commands' / (label + '.' + suffix))) == row[suffix + '_sha256'],
                            'capture.changed:' + label)
            require(time.monotonic() <= deadline, 'campaign.deadline')
        except BaseException as exc:
            report['status'] = 'failed'
            report['verification_error'] = type(exc).__name__ + ':' + str(exc)
        report['ended_utc'] = utc()
        report['elapsed_seconds'] = time.monotonic() - started
        save(out / 'run.json', encoded(report))
    print(json.dumps(dict(status=report['status'], receipt=str(out / 'run.json'),
        sha256=sha(read(out / 'run.json')))))
    return 0 if report['status'] == 'completed' else 1


def interrupted(signum, _frame):
    raise RuntimeError('signal:' + str(signum))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    modes = parser.add_mutually_exclusive_group()
    modes.add_argument('--snapshot', action='store_true')
    modes.add_argument('--execute', type=Path)
    parser.add_argument('--source-sha')
    parser.add_argument('--expected-recorder-sha')
    parser.add_argument('--cpu', type=int, default=0)
    args = parser.parse_args()
    if not args.snapshot and args.execute is None:
        print(json.dumps(dict(schema='mhgp7-product-meb-mutation-plan-v1', engine_invoked=False,
            mutations=list(MUTATIONS), fault_modes=['o2', 'san'], commands=15,
            process_wall_seconds=300, campaign_wall_seconds=1800)))
        return 0
    if args.snapshot:
        values, pins = inputs()
        print(json.dumps(dict(source_sha256=sha(encoded(pins)), source_files=len(values),
            boost_headers=len(pins['boost_headers']), recorder_sha256=sha(values[REL + '/record.py']))))
        return 0
    require(args.source_sha and args.expected_recorder_sha and
            sha(read(Path(__file__).resolve())) == args.expected_recorder_sha, 'execute.missing_or_wrong_external_pins')
    for sig in (signal.SIGINT, signal.SIGTERM, signal.SIGHUP):
        signal.signal(sig, interrupted)
    return execute(args.execute.resolve(), args.source_sha, args.cpu)


if __name__ == '__main__':
    sys.exit(main())
