#!/usr/bin/env python3
"""Create-only recording of two PURE r2 judge replays; inert without --execute.

No compiler, C++ binary, old runner, Git, GCP or write inside r2 is invoked.
Explicit derivative of record.py 2f5a6073; the r1 script and captures stay unchanged.
Both subprocesses use the frozen snapshot controller and explicit external pins.
"""
from __future__ import annotations

import argparse
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import resource
import signal
import subprocess
import sys
import time

ROOT = Path('/workspaces/E-HGP')
BASE = ROOT / 'build/v7_meb_filter_checks_20260906'
OUTPUT = BASE / 'checks_r2'
RUN = ROOT / 'build/v7_meb_filter_qualification_20260906_r2'
DRIVER = RUN / 'snapshot/build/v7_meb_filter_qualification_20260906/capture.py'
DRIVER_SHA = '0f5f0f6c9cb5b86117cb92334d764e16baee060089f177e1e688a42eae6a874c'
RUN_SHA = '981f3b3e67f3f8e731aceca964c9faaae32b16b372f58704b205585374f83e87'
SOURCE_SHA = '7e881f998a2f6bcac8e709a3ee3fa0c176973a7670e2a22960a1d46641627172'
SIGNALS = (signal.SIGINT, signal.SIGTERM, signal.SIGHUP)


def require(ok, reason):
    if not ok:
        raise ValueError(reason)


def read(path, limit=64 << 20):
    require(path.is_file() and not any(p.is_symlink() for p in (path, *path.parents)), 'file.kind')
    require(path.stat().st_size <= limit, 'file.size')
    with path.open('rb') as stream:
        raw = stream.read(limit + 1)
    require(len(raw) <= limit, 'file.size_changed')
    return raw


def sha(raw):
    return hashlib.sha256(raw).hexdigest()


def encoded(value):
    return (json.dumps(value, sort_keys=True, indent=2, allow_nan=False) + '\n').encode()


def unique(pairs):
    value = {}
    for key, entry in pairs:
        require(key not in value, 'json.duplicate_key')
        value[key] = entry
    return value


def decoded(raw):
    def invalid(_):
        raise ValueError('json.nonfinite')
    return json.loads(raw, object_pairs_hook=unique, parse_constant=invalid)


def save(path, value):
    with path.open('xb') as stream:
        stream.write(value if isinstance(value, bytes) else encoded(value))


def inputs(own_pin):
    own, python = Path(__file__).resolve(), Path(sys.executable).resolve()
    pins = {str(path): sha(read(path)) for path in (own, python, DRIVER, RUN / 'run.json', RUN / 'source_pins.json')}
    require(pins[str(own)] == own_pin and pins[str(DRIVER)] == DRIVER_SHA
            and pins[str(RUN / 'run.json')] == RUN_SHA, 'external.protocol_pin')
    report = decoded(read(RUN / 'run.json'))
    sources = decoded(read(RUN / 'source_pins.json'))
    require(sha(encoded(sources)) == report['source_map_sha256'] == SOURCE_SHA
            and report['status'] == 'completed', 'external.source_or_run')
    for name, pin in sources.items():
        path = Path(name)
        require(not path.is_absolute() and '..' not in path.parts, 'source.path')
        require(sha(read(RUN / 'snapshot' / path)) == pin, 'snapshot.pin:' + name)
    expected = dict(compiler=0, head=0, worktree=0)
    for mode in ('o2', 'san'):
        for gate in ('budget_gate', 'trajectory_gate', 'geometry_gate', 'rational_bridge'):
            expected[mode + '_' + gate + '_compile'] = 0
        for label in ('rational', 'ordinals', 'budget_gate', 'trajectory_gate', 'geometry_gate'):
            expected[mode + '_' + label] = 0
        for gate in ('budget_gate', 'trajectory_gate', 'geometry_gate'):
            expected[mode + '_' + gate + '_bad_argument'] = 2
        for label in ('budget_gate_charge_after', 'geometry_gate_charge_after', 'trajectory_order_mutant',
                      'trajectory_admissible_order_mutant'):
            expected[mode + '_' + label] = 4
    for mutant in ('skip_shell', 'ordinal_plus_one', 'q4_rescale_level'):
        expected[mutant] = expected[mutant + '_rational_bridge_compile'] = 0
    require(set(report['commands']) == set(expected) and len(expected) == 41, 'run.command_inventory')
    for label, code in expected.items():
        row = report['commands'][label]
        require(type(row['exit_code']) is int and type(row['expected_exit_code']) is int
                and row['exit_code'] == row['expected_exit_code'] == code and row['interrupted'] is None,
                'run.command_terminal:' + label)
    return pins


def present(pid):
    try:
        os.killpg(pid, 0)
        return True
    except ProcessLookupError:
        return False


def child_limits():
    os.sched_setaffinity(0, {0})
    resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
    resource.setrlimit(resource.RLIMIT_CPU, (30, 30))
    resource.setrlimit(resource.RLIMIT_FSIZE, (16 << 20, 16 << 20))


def execute(own_pin):
    before = inputs(own_pin)
    require(0 in os.sched_getaffinity(0), 'CPU0.unavailable')
    require(not OUTPUT.exists() and not OUTPUT.is_symlink(), 'output.create_only')
    OUTPUT.mkdir()
    deadline = time.monotonic() + 30
    result = dict(schema='mhgp7-filtered-meb-pure-replays-v1', status='failed', commands={},
                  started_utc=datetime.now(timezone.utc).isoformat(), inputs_before=before,
                  run_path=str(RUN), run_sha256=RUN_SHA, source_sha256=SOURCE_SHA,
                  driver_path=str(DRIVER), driver_sha256=DRIVER_SHA, recorder_sha256=own_pin,
                  combined_deadline_seconds=30, engine_runs=0, compiler_runs=0,
                  writes_inside_r2=False, public_status='not_claimed', gcp_used=False)
    try:
        save(OUTPUT / 'record_r2.py', read(Path(__file__).resolve()))
        save(OUTPUT / 'capture.py', read(DRIVER))
        environment = {'PATH': '/usr/bin:/bin', 'LC_ALL': 'C', 'LANG': 'C', 'TZ': 'UTC',
                       'PYTHONDONTWRITEBYTECODE': '1'}
        save(OUTPUT / 'environment.json', dict(environment=environment, cpu_affinity=[0],
             host_uname=list(os.uname()), python=str(Path(sys.executable).resolve())))
        outputs = []
        for optimized in (False, True):
            label = 'optimized' if optimized else 'normal'
            argv = [str(Path(sys.executable).resolve()), '-B', '-I'] + (['-O'] if optimized else [])
            argv += [str(DRIVER), '--judge', str(RUN), '--source-sha', SOURCE_SHA, '--run-sha', RUN_SHA]
            remaining = deadline - time.monotonic() - 2
            require(remaining > 0, 'combined.deadline')
            intent = dict(argv=argv, cwd=str(OUTPUT), expected_exit_code=0, timeout_seconds=remaining,
                          started_utc=datetime.now(timezone.utc).isoformat(), cpu_affinity=[0])
            save(OUTPUT / (label + '.intent.json'), intent)
            process, error = None, None
            begin = time.monotonic()
            with (OUTPUT / (label + '.stdout')).open('xb') as stdout, (OUTPUT / (label + '.stderr')).open('xb') as stderr:
                try:
                    process = subprocess.Popen(argv, cwd=OUTPUT, env=environment, stdin=subprocess.DEVNULL,
                        stdout=stdout, stderr=stderr, start_new_session=True, preexec_fn=child_limits)
                    save(OUTPUT / (label + '.spawned.json'), dict(pid=process.pid, pgid=process.pid))
                    process.wait(timeout=remaining)
                    require(not present(process.pid), 'child.descendant_survived')
                except BaseException as exc:
                    error = type(exc).__name__ + ': ' + str(exc)
                finally:
                    handlers = {sig: signal.signal(sig, signal.SIG_IGN) for sig in SIGNALS}
                    try:
                        if process is not None:
                            for sig in (signal.SIGTERM, signal.SIGKILL):
                                try:
                                    os.killpg(process.pid, sig)
                                except ProcessLookupError:
                                    pass
                            process.wait(timeout=1)
                            require(not present(process.pid), 'child.group_not_closed')
                    except BaseException as exc:
                        error = (error or '') + '; cleanup:' + type(exc).__name__ + ':' + str(exc)
                    finally:
                        for sig, handler in handlers.items():
                            signal.signal(sig, handler)
            out, err = read(OUTPUT / (label + '.stdout')), read(OUTPUT / (label + '.stderr'))
            row = dict(intent, elapsed_seconds=time.monotonic() - begin,
                       ended_utc=datetime.now(timezone.utc).isoformat(), exit_code=None if process is None else process.returncode,
                       error=error, stdout_sha256=sha(out), stderr_sha256=sha(err),
                       stdout_bytes=len(out), stderr_bytes=len(err))
            save(OUTPUT / (label + '.command.json'), row)
            result['commands'][label] = row
            require(error is None and row['exit_code'] == 0 and not err, 'judge.command_failed:' + label)
            value = decoded(out)
            require(value['status'] == 'passed' and value['schema'] == 'mhgp7-private-filtered-meb-judgment-v1'
                    and value['public_status'] == 'not_claimed' and value['gcp'] == 'not_used', 'judge.result')
            outputs.append(out)
        require(len(outputs) == 2 and outputs[0] == outputs[1], 'judge.normal_optimized_bytes_differ')
        result.update(status='completed', normal_optimized_json_equal=True, normal_optimized_bytes_equal=True)
    except BaseException as exc:
        result['error'] = type(exc).__name__ + ': ' + str(exc)
    finally:
        try:
            result['inputs_after'] = inputs(own_pin)
            require(result['inputs_after'] == before, 'inputs.changed')
            require(time.monotonic() <= deadline, 'combined.deadline_at_closure')
        except BaseException as exc:
            result.update(status='failed', error=type(exc).__name__ + ': ' + str(exc))
        result['ended_utc'] = datetime.now(timezone.utc).isoformat()
        result['artifacts'] = {p.name: sha(read(p)) for p in sorted(OUTPUT.iterdir()) if p.is_file()}
        save(OUTPUT / 'receipt.json', result)
    print(json.dumps(dict(status=result['status'], receipt=str(OUTPUT / 'receipt.json'),
                         receipt_sha256=sha(read(OUTPUT / 'receipt.json')), error=result.get('error'))))
    require(result['status'] == 'completed', 'pure_replays.failed')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--execute', action='store_true')
    parser.add_argument('--expected-recorder-sha256')
    args = parser.parse_args()
    if not args.execute:
        print(json.dumps(dict(status='prepared_not_executed', judge_replays=2, engine_runs=0)))
        return
    execute(args.expected_recorder_sha256)


if __name__ == '__main__':
    def interrupted(signum, _frame):
        raise InterruptedError('signal:' + str(signum))
    for watched in SIGNALS:
        signal.signal(watched, interrupted)
    try:
        main()
    except (ValueError, OSError, RuntimeError) as exc:
        print(json.dumps(dict(status='failed', error=str(exc))), file=sys.stderr)
        sys.exit(1)
