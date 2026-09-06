#!/usr/bin/env python3
"""Capture two pure reader invocations, never a C++/GCP process."""
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import signal
import subprocess
import time

ROOT = Path('/workspaces/E-HGP')
BASE = Path(__file__).resolve().parent
READER = ROOT/'build/v7_extra_shell_20260906/read_extra_shell.py'
TRACE = ROOT/'build/v7_extra_shell_20260906/run_r3/n50000_k10.stderr'
PINS = {READER: 'a6d7d26615101eb3f6793f3bf3a08f681b693d7db5c7e601587552924b1d6c5c',
        TRACE: '3cd74b330c62978d8c3eedd175e12bf5fe02893facb2e008150c32b5054aea72'}


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def write(path, value):
    with path.open('x') as stream:
        json.dump(value, stream, sort_keys=True, indent=2)
        stream.write('\n')


def snapshot():
    result = {str(path): sha(path) for path in PINS}
    if any(result[str(path)] != pin for path, pin in PINS.items()):
        raise ValueError('reader/trace pin changed')
    return result


def main():
    out = BASE/'checks'
    out.mkdir()
    before = snapshot()
    write(out/'sources_before.json', before)
    commands = []
    for name, flags in [('normal', []), ('optimized', ['-O'])]:
        argv = ['python3', '-B', *flags, str(READER), str(TRACE), '--expected-n', '50000', '--mixed-stderr']
        start, utc = time.monotonic(), datetime.now(timezone.utc).isoformat()
        with (out/(name+'.stdout')).open('xb') as stdout, (out/(name+'.stderr')).open('xb') as stderr:
            child = subprocess.Popen(argv, cwd=ROOT, stdin=subprocess.DEVNULL, stdout=stdout, stderr=stderr, start_new_session=True)
            try:
                code = child.wait()
            finally:
                if child.poll() is None:
                    os.killpg(child.pid, signal.SIGTERM)
                    child.wait()
        command = dict(argv=argv, cwd=str(ROOT), started_utc=utc, pid=child.pid, exit_code=code,
            elapsed_seconds=time.monotonic()-start, watchdog=None,
            stdout_sha256=sha(out/(name+'.stdout')), stderr_sha256=sha(out/(name+'.stderr')))
        write(out/(name+'.command.json'), command)
        commands.append(command)
    after = snapshot()
    write(out/'sources_after.json', after)
    equal = (out/'normal.stdout').read_bytes() == (out/'optimized.stdout').read_bytes()
    passed = all(row['exit_code'] == 0 for row in commands) and equal and before == after
    write(out/'receipt.json', dict(status='completed' if passed else 'failed', commands=commands,
        outputs_byte_equal=equal, sources_stable=before == after, capture_sha256=sha(Path(__file__)),
        diagnostic_only=True, engine_invoked=False, global_parents_reconstructed=False))
    if not passed:
        raise ValueError('pure reader qualification failed; captures preserved')
    print(json.dumps(dict(status='completed', outputs_byte_equal=equal, receipt_sha256=sha(out/'receipt.json'))))


if __name__ == '__main__':
    main()
