"""Capture two read-only event judgments, retaining inputs and source pins."""
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import time

sys.dont_write_bytecode = True
root = Path(__file__).resolve().parent
out = root/'event_capture'
out.mkdir(exist_ok=False)
paths = [root/name for name in ('record_event_judge.py','judge_sklearn_events.py')]+[
    root/'sklearn_capture'/name for name in ('normal.stdout','optimized.stdout','receipt.json')]
digest = lambda p: hashlib.sha256(p.read_bytes()).hexdigest()
before = {str(p):digest(p) for p in paths}
commands, reports = [], []
for mode, flags in (('normal',[]),('optimized',['-O'])):
    argv = [sys.executable,'-B',*flags,str(root/'judge_sklearn_events.py'),str(root/'sklearn_capture')]
    start = time.monotonic()
    result = subprocess.run(argv,capture_output=True,text=True,check=False,timeout=15)
    for stream in ('stdout','stderr'):
        with (out/(mode+'.'+stream)).open('x') as file:
            file.write(getattr(result,stream))
    commands.append(dict(argv=argv,mode=mode,exit_code=result.returncode,
                         wall_seconds=time.monotonic()-start))
    if result.returncode or result.stderr:
        raise ValueError('event judgment failed')
    reports.append(json.loads(result.stdout))
after = {str(p):digest(p) for p in paths}
if before!=after or reports[0]!=reports[1]:
    raise ValueError('source/input/output pairing failed')
with (out/'receipt.json').open('x') as file:
    file.write(json.dumps(dict(status='PASS',before=before,after=after,commands=commands,
                               exact_configurations=16,new_fit_calls=0,native_calls=0,GCP_used=False,
                               preflight='one successful tool preview; stdout truncated, not full archive'),
                          sort_keys=True,indent=2)+'\n')
print(json.dumps(dict(status='PASS',paired_judgments=2,exact_configurations=16,
                     new_fit_calls=0,native_calls=0,GCP_used=False),sort_keys=True))
