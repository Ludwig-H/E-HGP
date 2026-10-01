#!/usr/bin/env python3
"""One-shot reviewed capture; preserve failures and close only after success."""
import hashlib
import json
import math
import os
import subprocess
import sys
import time
from pathlib import Path

ROOT=Path(__file__).absolute().parent
SOURCES=('README.txt','er_snapshot.py','probe.py','read.py','record.py')
OUTPUTS=('capture_normal.json','capture_optimized.json','execution.json')
SHARED=Path('/workspaces/E-HGP/build/v10-verrou-points/juge_final/echelle_relative/er.py')
PIN='99e8ba720f417af9b267f8b70ea554be26c7545ea20e673a7dc89248debe321d'
TIMEOUT=15

def require(c,m):
 if not c: raise RuntimeError(m)
def sha(p): return hashlib.sha256(p.read_bytes()).hexdigest()
def write_new(name,obj):
 with (ROOT/name).open('x',encoding='utf-8') as f:
  json.dump(obj,f,sort_keys=True,indent=1,allow_nan=False); f.write('\n')
def text_bytes(v):
 if v is None: return ''
 return v.decode('utf-8',errors='surrogateescape') if isinstance(v,bytes) else v

def run():
 require(not any((ROOT/n).exists() for n in OUTPUTS+('manifest.json',)),'capture already exists; use a new packet')
 before={n:sha(ROOT/n) for n in SOURCES}
 shared_before=sha(SHARED)
 require(shared_before==PIN and before['er_snapshot.py']==PIN,'source pin before')
 python=sys.executable
 require(Path(python).is_absolute(),'absolute Python executable')
 records=[]
 for optimized,name in ((False,OUTPUTS[0]),(True,OUTPUTS[1])):
  argv=[python,'-B']+(['-O'] if optimized else [])+[str(ROOT/'probe.py')]
  start=time.monotonic()
  try:
   p=subprocess.run(argv,stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=TIMEOUT,check=False)
   row={'argv':argv,'stdout':text_bytes(p.stdout),'stderr':text_bytes(p.stderr),'exit_code':p.returncode,'timed_out':False}
  except subprocess.TimeoutExpired as e:
   row={'argv':argv,'stdout':text_bytes(e.stdout),'stderr':text_bytes(e.stderr),'exit_code':None,'timed_out':True}
  row.update(duration_seconds=time.monotonic()-start,timeout_seconds=TIMEOUT)
  write_new(name,row); records.append(row)
 after={n:sha(ROOT/n) for n in SOURCES}
 shared_after=sha(SHARED)
 real_python=Path(os.path.realpath(python))
 execution={'origin':str(ROOT),'python':python,'python_realpath':str(real_python),'python_sha256':sha(real_python),
            'python_version':sys.version,'source_before':before,'source_after':after,
            'shared_source':str(SHARED),'shared_source_before':shared_before,'shared_source_after':shared_after,
            'captures':list(OUTPUTS[:2]),'timeout_seconds':TIMEOUT}
 write_new('execution.json',execution)
 require(before==after and shared_before==shared_after==PIN,'source changed during capture')
 for r in records:
  require(type(r['exit_code']) is int and r['exit_code']==0 and r['timed_out'] is False,'terminal failure retained')
  require(math.isfinite(r['duration_seconds']) and r['duration_seconds']>=0,'duration')
  require(r['stderr']=='','stderr failure retained')
 require(records[0]['stdout']==records[1]['stdout'],'normal/-O mismatch retained')
 files={n:sha(ROOT/n) for n in SOURCES+OUTPUTS}
 write_new('manifest.json',{'format':'er-entry-star-archive-v1','files':files})
 print(json.dumps({'manifest_sha256':sha(ROOT/'manifest.json'),'files':sorted(files)+['manifest.json']},sort_keys=True))

if __name__=='__main__': run()
