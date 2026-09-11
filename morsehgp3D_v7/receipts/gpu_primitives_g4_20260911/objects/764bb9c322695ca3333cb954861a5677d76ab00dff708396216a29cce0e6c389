#!/usr/bin/env python3
"""In-memory predicates and mocked processes only. No key/file/cloud access."""
import copy
from contextlib import redirect_stdout
import importlib.util
import io
import json
from pathlib import Path
import signal
import subprocess
import tarfile
from unittest.mock import patch

spec = importlib.util.spec_from_file_location('full_session', Path(__file__).with_name('full_probe_session_v7.py'))
session = importlib.util.module_from_spec(spec)
spec.loader.exec_module(session)


def main():
    positives = rejected = 0
    generation = '2026-09-06T09:00:00.123456-07:00'
    other = '2026-09-06T09:00:01.123456-07:00'
    handoff = dict(session.TARGET, schema='e-hgp.start-handoff.v3', last_start_timestamp=generation)
    lifecycle = dict(session.TARGET, schema='e-hgp.lifecycle-state.v1', state='targeted_running', generation=generation)
    for h, l in ((handoff, lifecycle), (handoff, None), (None, lifecycle)):
        session.need(session.generation_from_records(h, l) == generation, 'preserve exact GCE offset string')
        positives += 1
    session.need(session.epoch(generation) == session.epoch('2026-09-06T16:00:00.123456Z'), 'offset instant')
    positives += 1
    for known, observed, unreadable, expected in ((generation, None, True, generation),
            (generation, None, False, generation), (generation, generation, False, generation),
            (None, generation, False, generation), (None, None, False, None)):
        session.need(session.closure_generation(known, observed, unreadable) == expected, 'closure proof')
        positives += 1
    bad = [lambda: session.closure_generation(generation, other),
           lambda: session.closure_generation(None, None, True),
           lambda: session.generation_from_records(handoff, dict(lifecycle, generation=other)),
           lambda: session.generation_from_records(dict(handoff, instance='other'), lifecycle),
           lambda: session.generation_from_records(None, dict(lifecycle, generation='')),
           lambda: session.generation_from_records(None, dict(lifecycle, state='invented')),
           lambda: session.epoch('2026-09-06T16:00:00')]
    now = session.epoch(generation) + 200
    mark = dict(session.TARGET, schema='e-hgp.guard-mark.v1', mark='double_guard_verified',
                generation=generation, max_run_seconds='3600', guest_shutdown_minutes='30',
                date_utc='2026-09-06T16:02:00Z')
    schedule = dict(MODE='poweroff', USEC=str(int((session.epoch(generation) + 1900) * 1000000)))
    session.need(session.guard_deadline(mark, schedule, generation, now) == int(session.epoch(generation) + 1900), 'deadline')
    positives += 1
    bad.extend([lambda: session.guard_deadline(dict(mark, mark='guest_guard_pending'), schedule, generation, now),
                lambda: session.guard_deadline(dict(mark, generation=other), schedule, generation, now),
                lambda: session.guard_deadline(dict(mark, guest_shutdown_minutes='45'), schedule, generation, now),
                lambda: session.guard_deadline(mark, dict(schedule, MODE='reboot'), generation, now),
                lambda: session.guard_deadline(mark, dict(schedule, USEC='1'), generation, now),
                lambda: session.guard_deadline(mark, dict(schedule, USEC=str(int((now+9000)*1000000))), generation, now)])

    class Archive:
        def __init__(self, entries):
            self.entries = entries
        def getmembers(self):
            return self.entries

    directory = tarfile.TarInfo('output/')
    directory.type = tarfile.DIRTYPE
    file = tarfile.TarInfo('output/receipt.json')
    session.need(len(list(session.archive_members(Archive([directory, file]), 'output'))) == 2, 'archive positive')
    positives += 1
    for name in ('/output/x', 'output/../escape', './output/x', 'output//x', 'elsewhere/x'):
        bad.append(lambda name=name: list(session.archive_members(Archive([tarfile.TarInfo(name)]), 'output')))
    for kind in (tarfile.SYMTYPE, tarfile.LNKTYPE, tarfile.CHRTYPE):
        member = copy.copy(file)
        member.type = kind
        bad.append(lambda member=member: list(session.archive_members(Archive([member]), 'output')))
    bad.append(lambda: list(session.archive_members(Archive([file, copy.copy(file)]), 'output')))
    for function in bad:
        try:
            function()
        except (ValueError, KeyError):
            rejected += 1
        else:
            raise ValueError('negative predicate admitted')

    # Exercise the actual cleanup method without invoking any real subprocess.
    class MemoryPath:
        def __truediv__(self, _name):
            return self
        def write_bytes(self, _value):
            pass

    mocked_commands = 0
    for kind in ('leader_exited_child_keeps_pipe', 'guard_interrupt', 'critical_log_failure'):
        calls = []
        alive = [kind != 'critical_log_failure']
        class Process:
            pid = 123456
            returncode = 0 if kind != 'guard_interrupt' else None
            communicates = 0
            def poll(self):
                return self.returncode
            def communicate(self, timeout=None):
                self.communicates += 1
                if self.communicates == 1 and kind == 'leader_exited_child_keeps_pipe':
                    raise subprocess.TimeoutExpired('mock', 1)
                if self.communicates == 1 and kind == 'guard_interrupt':
                    raise InterruptedError('mock signal')
                self.returncode = 0
                return b'ok\n', b''
            def wait(self):
                self.returncode = 0
                return 0
        def killpg(pid, sig):
            session.need(pid == 123456, 'own group only')
            calls.append(('group', sig))
            if not alive[0]:
                raise ProcessLookupError()
            if sig != 0:
                alive[0] = False
        def kill(pid, sig):
            session.need(pid == 123456, 'own leader only')
            calls.append(('leader', sig))
            alive[0] = False
        def save(_path, _value):
            if kind == 'critical_log_failure':
                raise OSError('mock full disk')
        with patch.object(session.subprocess, 'Popen', return_value=Process()), \
             patch.object(session.os, 'killpg', side_effect=killpg), patch.object(session.os, 'kill', side_effect=kill), \
             patch.object(session.signal, 'signal', return_value=signal.SIG_DFL), patch.object(session, 'save', side_effect=save), \
             redirect_stdout(io.StringIO()):
            commands = session.Commands(MemoryPath(), {})
            try:
                code, _, _ = commands.run('mock', ['NEVER_EXECUTED'], timeout=1,
                    critical=kind == 'critical_log_failure', guard=kind == 'guard_interrupt')
                session.need(kind == 'critical_log_failure' and code == 0, 'only ordinary success returns')
            except (InterruptedError, subprocess.TimeoutExpired):
                session.need(kind != 'critical_log_failure', 'critical closure cannot fail from logs')
            session.need(commands.rows[0]['group_closed'] is True, 'mock group closed')
        if kind == 'guard_interrupt':
            session.need(('leader', signal.SIGTERM) in calls and ('group', signal.SIGTERM) not in calls, 'guard children not signalled')
        if kind == 'leader_exited_child_keeps_pipe':
            session.need(('group', signal.SIGTERM) in calls, 'orphan group cleaned despite leader exit')
        mocked_commands += 1
    session.need(positives == 11 and rejected == 22 and mocked_commands == 3, 'nonvacuum counts')
    print(json.dumps(dict(status='passed', positive_predicates=positives, rejected_predicates=rejected,
                          mocked_commands=mocked_commands, real_subprocesses=0, GCP_used=False, key_read=False), sort_keys=True))


if __name__ == '__main__':
    main()
