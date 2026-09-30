"""Bounded audit capture: prints JSON only; no file writes or native calls."""
import datetime as dt
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import time
ROOT=Path(__file__).resolve().parent
FILES=('check.py','PROTOCOL.txt','record.py')
def h(p): return hashlib.sha256(p.read_bytes()).hexdigest()
def now(): return dt.datetime.now(dt.timezone.utc).isoformat()
before={n:h(ROOT/n) for n in FILES}; frozen=now(); commands=[]
for optimize in (False,True):
    argv=[sys.executable,'-B']+(['-O'] if optimize else [])+[str(ROOT/'check.py')]
    start=now(); ns=time.monotonic_ns()
    r=subprocess.run(argv,cwd=ROOT,text=True,capture_output=True,check=False,timeout=20)
    commands.append(dict(name='optimized' if optimize else 'normal',argv=argv,code=r.returncode,
                         start_utc=start,end_utc=now(),start_ns=ns,end_ns=time.monotonic_ns(),
                         stdout=r.stdout,stderr=r.stderr))
after={n:h(ROOT/n) for n in FILES}
ok=before==after and all(c['code']==0 and not c['stderr'] for c in commands) and commands[0]['stdout']==commands[1]['stdout']
print(json.dumps(dict(schema='mhgp10.uniform_band_contact.v1',status='CAPTURED' if ok else 'FAIL',
                     frozen_utc=frozen,closed_utc=now(),sources_before=before,sources_after=after,
                     commands=commands,native_calls=0,GCP_used=False),sort_keys=True,indent=2))
raise SystemExit(0 if ok else 1)
