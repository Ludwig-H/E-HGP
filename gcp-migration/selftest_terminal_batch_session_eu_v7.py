#!/usr/bin/env python3
"""Pure target/CLI/import boundary tests. No cloud, key or real subprocess."""
from contextlib import redirect_stdout, redirect_stderr
import importlib.util
import io
import json
from pathlib import Path
import sys
from unittest.mock import patch

sys.dont_write_bytecode = True
spec = importlib.util.spec_from_file_location('tested_eu_binding',
    Path(__file__).with_name('terminal_batch_session_eu_v7.py'))
w = importlib.util.module_from_spec(spec)
spec.loader.exec_module(w)
checks = rejects = 0


def need(ok, reason):
    global checks
    checks += 1
    if not ok:
        raise ValueError(reason)


def reject(function, reason):
    global rejects
    try:
        function()
    except (ValueError, KeyError):
        rejects += 1
    else:
        raise ValueError('mutation survived: ' + reason)


def main():
    c = w.load_controller()
    expected = dict(project='devpod-gpu-exploration', zone='europe-west4-a', instance='ehgp-blackwell-spot')
    need(c.TARGET == expected and c.TARGET is not w.TARGET, 'one exact copied EU target')
    need(c.GUARDS == {'start_and_verify.sh': '73d76c674c71d997a803587a0b20186f668e7aa44f62d4c8b516e22e13469bc0',
                     'stop_and_verify.sh': 'ddcad77aa995ebb334fd3f341f7bb81ac94f749593fec98f885fb1c4b7956f3c'},
         'unchanged guard pins')
    need(c.sha(c.__file__) == w.CONTROLLER_SHA256, 'controller unchanged')
    data = w.binding()
    need(data['wrapper_sha256'] == w.sha(w.__file__) and data['target'] == expected
         and data['controller_sha256'] == w.CONTROLLER_SHA256
         and data['changed_controller_globals'] == ['TARGET']
         and data['controller_receipt_identifies_wrapper'] is False, 'separate honest binding')
    with patch.object(w, 'CONTROLLER_SHA256', '0' * 64):
        reject(w.load_controller, 'wrong controller source pin')
    generation = '2026-09-11T14:00:00.123456Z'
    handoff = dict(expected, schema='e-hgp.start-handoff.v3', last_start_timestamp=generation)
    lifecycle = dict(expected, schema='e-hgp.lifecycle-state.v1', state='targeted_running', generation=generation)
    need(c.generation_from_records(handoff, lifecycle) == generation, 'EU exact generation')
    for field in expected:
        foreign = dict(handoff, **{field: 'other'})
        reject(lambda foreign=foreign: c.generation_from_records(foreign, lifecycle), 'mixed handoff ' + field)
        foreign = dict(lifecycle, **{field: 'other'})
        reject(lambda foreign=foreign: c.generation_from_records(handoff, foreign), 'mixed lifecycle ' + field)
    reject(lambda: c.generation_from_records(dict(handoff, **w.ORIGINAL_TARGET), lifecycle), 'old US handoff')
    now = c.epoch(generation) + 200
    mark = dict(expected, schema='e-hgp.guard-mark.v1', mark='double_guard_verified', generation=generation,
                guest_shutdown_minutes='30', max_run_seconds='3600', date_utc='2026-09-11T14:02:00Z')
    schedule = dict(MODE='poweroff', USEC=str(int((c.epoch(generation) + 1900) * 1000000)))
    need(c.guard_deadline(mark, schedule, generation, now) == int(c.epoch(generation) + 1900), 'same 30 minute guard')
    for field in expected:
        foreign = dict(mark, **{field: 'other'})
        reject(lambda foreign=foreign: c.guard_deadline(foreign, schedule, generation, now), 'mixed guard ' + field)
    reject(lambda: c.guard_deadline(dict(mark, guest_shutdown_minutes='45'), schedule, generation, now), 'guard not weakened')
    reject(lambda: c.closure_generation(generation, '2026-09-11T14:00:01.123456Z'), 'generation mix not weakened')
    argv = ['eu-wrapper', '--snapshot=/absent/snapshot', '--manifest=/absent/manifest',
            '--worker=/absent/worker', '--session-dir=/absent/session', '--ssh-key=/absent/key',
            '--snapshot-sha256=' + 'a' * 64, '--manifest-sha256=' + 'b' * 64,
            '--worker-sha256=' + 'c' * 64, '--expected-controller-sha256=' + w.CONTROLLER_SHA256]
    output = io.StringIO()
    with patch.object(w, 'load_controller', return_value=c), patch.object(sys, 'argv', argv), \
            patch.object(c.subprocess, 'Popen', side_effect=RuntimeError('NO SUBPROCESS PERMITTED')) as popen, \
            redirect_stdout(output):
        need(w.main() == 0, 'inert unchanged CLI succeeds without file access')
        need(not popen.called, 'inert never invokes a subprocess')
    inert = json.loads(output.getvalue())
    need(inert == dict(status='inert', target=expected, guest_minutes=30, OSLogin_TTL='70m'), 'exact EU inert output')
    with patch.object(w, 'load_controller', return_value=c), patch.object(sys, 'argv', argv + ['--execute']), \
            patch.object(c, 'run_session', return_value=77) as run, \
            patch.object(c.subprocess, 'Popen', side_effect=RuntimeError('NO SUBPROCESS PERMITTED')) as popen:
        need(w.main() == 77 and run.call_count == 1 and not popen.called, 'execute delegates unchanged parser only, mocked')
        args = run.call_args.args[0]
        need(args.execute and args.expected_controller_sha256 == w.CONTROLLER_SHA256 and not args.bootstrap,
             'controller pin remains original, bootstrap remains off')
    with patch.object(w, 'load_controller', return_value=c), patch.object(sys, 'argv', argv + ['--zone=elsewhere']), \
            redirect_stderr(io.StringIO()):
        try:
            w.main()
        except SystemExit as error:
            need(error.code == 2, 'no target override CLI added')
        else:
            raise ValueError('target override CLI unexpectedly admitted')
    need(checks == 12 and rejects == 13, 'nonvacuity floors')
    print(json.dumps(dict(status='passed', checks=checks, rejections=rejects, GCP_used=False,
                          real_subprocesses=0, mocked_execute_calls=1, target=expected), sort_keys=True))


if __name__ == '__main__':
    main()
