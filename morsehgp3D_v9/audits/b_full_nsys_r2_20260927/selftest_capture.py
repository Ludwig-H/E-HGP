#!/usr/bin/env python3
"""Small offline tests; never start a VM or invoke a controller."""
from unittest.mock import patch
import json
import types

import capture as c


def main():
    positive = rejected = 0

    def refuse(call):
        nonlocal rejected
        try:
            call()
        except ValueError:
            rejected += 1
        else:
            raise RuntimeError('mutation accepted')

    def git(argv, **_kwargs):
        c.need(argv[:4] == ['git', '-C', str(c.ROOT), '--no-replace-objects'] and
               argv[4] == 'show', 'read-only Git recipe')
        relative = argv[5].split(':', 1)[1]
        return types.SimpleNamespace(stdout=(c.ROOT/relative).read_bytes())

    with patch.object(c.subprocess, 'run', side_effect=git):
        c.committed('1'*40)
        positive += 1
        for bad in ('main', '1'*39, '../main', None):
            refuse(lambda: c.committed(bad))
    with patch.object(c.subprocess, 'run', return_value=types.SimpleNamespace(stdout=b'changed')):
        refuse(lambda: c.committed('1'*40))

    # The real immutable helpers and FULL package can be checked offline.
    native, wait = c.runtime()
    c.need(wait is c.wait_owned, 'cooperative wait port')
    c.package_check(native)
    positive += 1
    with patch.object(c, 'SNAPSHOT_SHA', '0'*64):
        refuse(lambda: c.package_check(native))
    with patch.object(c, 'MANIFEST_SHA', '0'*64):
        refuse(lambda: c.package_check(native))
    with patch.object(c, 'NATIVE_COMMIT', '0'*40):
        refuse(lambda: c.package_check(native))
    with patch.object(c, 'PINS', {'gcp-migration/full_probe_session_v7.py': '0'*64}):
        refuse(c.runtime)
    # No process is actually spawned. Check that timeout/PID-write failure
    # still interrupt and join the owned controller, never kill its cleanup.
    class Process:
        pid = 123
        def __init__(self, timeout=False):
            self.running = True
            self.timeout = timeout
            self.waits = []
            self.signals = []
        def poll(self):
            return None if self.running else 0
        def send_signal(self, sig):
            self.signals.append(sig)
        def wait(self, timeout=None):
            self.waits.append(timeout)
            if self.timeout and len(self.waits) == 1:
                raise c.subprocess.TimeoutExpired('controller', timeout)
            self.running = False
            return 0

    with patch.object(c.signal, 'signal', return_value=c.signal.SIG_DFL), patch.object(c, 'save'):
        for timeout, waits, signals in ((False, [900], []), (True, [900, None], [c.signal.SIGINT])):
            process = Process(timeout)
            c.need(c.wait_owned(process, c.HERE) == 0 and not process.running and
                   process.waits == waits and process.signals == signals, 'joined controller')
            positive += 1
    with patch.object(c.signal, 'signal', return_value=c.signal.SIG_DFL), \
         patch.object(c, 'save', side_effect=OSError('PID write refused')):
        process = Process()
        try:
            c.wait_owned(process, c.HERE)
        except OSError:
            pass
        else:
            raise ValueError('PID write failure lost')
        c.need(not process.running and process.waits == [None] and
               process.signals == [c.signal.SIGINT], 'PID failure still joins controller')
        positive += 1
    c.need(positive == 5 and rejected == 9, 'nonvacuous offline test count')
    print(json.dumps(dict(status='passed', positive=positive, rejected=rejected,
                         GCP_used=False, GPU_executed=False)))


if __name__ == '__main__':
    main()
