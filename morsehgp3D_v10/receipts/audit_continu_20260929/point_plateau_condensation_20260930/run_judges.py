"""Capture four archive-only independent judge executions, no native invocations."""
import datetime as dt
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import time

ROOT=Path(__file__).resolve().parent
FILES=('judge.py','run_judges.py','logs/native_normal.stdout','logs/native_ubsan.stdout')
def h(p): return hashlib.sha256(p.read_bytes()).hexdigest()
def now(): return dt.datetime.now(dt.timezone.utc).isoformat()
before={n:h(ROOT/n) for n in FILES}
commands=[]
for mode in ('normal','ubsan'):
    for opt in (False,True):
        argv=[sys.executable,'-B']+(['-O'] if opt else [])+[str(ROOT/'judge.py'),str(ROOT/('logs/native_'+mode+'.stdout'))]
        start=now(); ns=time.monotonic_ns()
        r=subprocess.run(argv,cwd=ROOT,text=True,capture_output=True,check=False,timeout=10)
        commands.append(dict(name='judge_'+mode+('_O' if opt else ''),argv=argv,code=r.returncode,
                             start_utc=start,end_utc=now(),start_ns=ns,end_ns=time.monotonic_ns(),
                             stdout=r.stdout,stderr=r.stderr))
after={n:h(ROOT/n) for n in FILES}
ok=before==after and all(type(c['code']) is int and c['code']==0 and not c['stderr'] for c in commands)
print(json.dumps(dict(schema='mhgp10.point_plateau.judges.v1',status='CAPTURED' if ok else 'FAIL',
                     commands=commands,sources_before=before,sources_after=after,new_native_calls=0),
                 sort_keys=True,indent=2))
raise SystemExit(0 if ok else 1)
