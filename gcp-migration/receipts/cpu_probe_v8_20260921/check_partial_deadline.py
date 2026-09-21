#!/usr/bin/env python3
"""Local deterministic simulation: no subprocess, SSH or cloud command runs."""
import hashlib
import json
from pathlib import Path
import signal
import subprocess
import sys
import tempfile
import time
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / 'gcp-migration'))
import cpu_probe_worker_v8 as worker


def main():
    helper = worker.load_helper(ROOT)
    with tempfile.TemporaryDirectory(prefix='mhgp8-partial-deadline-') as temporary:
        output = Path(temporary)
        previous = {'case': {'n': 1000}, 'status': 'completed', 'output': {'q3': 7}}
        helper.save(output / 'probe_0.summary.json', previous)
        original = worker.sha(output / 'probe_0.summary.json')
        schedule = {'MODE': 'poweroff', 'USEC': str((int(time.time()) + 1800) * 1000000)}
        clock = helper.Worker(output, time.time() + 900, schedule)
        class Process:
            pid = 123456789
            killed = False
            returncode = None
            def wait(self, timeout=None):
                if timeout is not None:
                    raise subprocess.TimeoutExpired(['mock_probe'], timeout)
                self.returncode = -signal.SIGKILL
                return self.returncode
        process = Process()
        def killpg(pid, sig):
            worker.need(pid == process.pid, 'different process group')
            if sig == signal.SIGKILL:
                process.killed = True
            elif sig == 0 and process.killed:
                raise ProcessLookupError()
        observed = None
        with patch.object(helper, 'scheduled_text', lambda: ''.join(k+'='+v+'\n' for k,v in schedule.items())), \
             patch.object(helper.subprocess, 'Popen', return_value=process), \
             patch.object(helper.os, 'killpg', side_effect=killpg):
            try:
                clock.command('probe_1', ['mock_probe'])
            except helper.SessionDeadline:
                observed = 'SessionDeadline'
        worker.need(observed == 'SessionDeadline' and process.killed, 'deadline/group cleanup absent')
        row = json.loads((output / 'probe_1.command.json').read_text())
        worker.need(row['exit_code'] == -signal.SIGKILL and row['session_deadline_reached'] is True and
                    row['group_closed'] is True and row['residual_or_interrupted_group_killed'] is True,
                    'interrupted command receipt')
        worker.need(len(clock.commands) == 1 and clock.commands[0] == row, 'receipt list mismatch')
        worker.need(worker.sha(output / 'probe_0.summary.json') == original and
                    not (output / 'probe_1.summary.json').exists(), 'completed partial lost or unfinished promoted')
        for kind in ('stdout', 'stderr'):
            worker.need(worker.sha(output / ('probe_1.'+kind)) == row[kind+'_sha256'], 'raw partial hash')
        print(json.dumps(dict(status='PASS', GCP_used=False, real_subprocesses=0,
             helper_sha256=worker.HELPER_SHA, previous_summary_preserved=True,
             interrupted_summary_absent=True, interrupted_exit_code=row['exit_code'],
             interrupted_group_closed=row['group_closed'], command_raw_hashes_verified=True), sort_keys=True))


if __name__ == '__main__':
    main()
