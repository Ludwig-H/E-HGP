"""Capture two read-only proof runs; print receipt, never edit files."""
import datetime as dt
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parent
NAMES = ('check.py', 'PROTOCOL.txt', 'record.py')


def pins():
    return {n: hashlib.sha256((ROOT/n).read_bytes()).hexdigest() for n in NAMES}


def utc():
    return dt.datetime.now(dt.timezone.utc).isoformat()


before = pins()
commands = []
for name, flags in (('normal', ['-B']), ('optimized', ['-B', '-O'])):
    argv = [sys.executable, *flags, str(ROOT/'check.py')]
    start, ns = utc(), time.perf_counter_ns()
    p = subprocess.run(argv, cwd=ROOT, capture_output=True, text=True, check=False, timeout=20)
    commands.append(dict(name=name, argv=argv, cwd=str(ROOT), start_utc=start,
                         end_utc=utc(), start_ns=ns, end_ns=time.perf_counter_ns(),
                         code=p.returncode, stdout=p.stdout, stderr=p.stderr))
after = pins()
if before != after or any(c['code'] != 0 or c['stderr'] for c in commands):
    raise ValueError('capture failed or source changed')
if commands[0]['stdout'] != commands[1]['stdout']:
    raise ValueError('normal and optimized captures differ')
print(json.dumps(dict(schema='mhgp10.weighted_euler_majority.v1', status='CAPTURED',
                     sources_before=before, sources_after=after, commands=commands,
                     native_calls=0, GCP_used=False,
                     earlier_exploratory_runs=[dict(mode='normal', cases=390, final_source=False),
                                               dict(mode='optimized', cases=392, final_source=True)]),
                 sort_keys=True, indent=2))
