#!/usr/bin/env python3
"""One-shot closure, run only after parent review; preserves failed child outputs."""
import hashlib
import json
import os
import subprocess
import sys
import time
from pathlib import Path

ROOT=Path(__file__).absolute().parent
SOURCES=('README.txt','probe.py','read.py','record.py','base_snapshot.py','principe_snapshot.py',
 'gamma_reference_snapshot.py','qsqrt_snapshot.py','preflight_snapshot.py','preflight_failure.json')
SHARED={
 'base_snapshot.py':'/tmp/mmta-c4ab-four-fixtures.RGuOI6lg/probe.py',
 'principe_snapshot.py':'/workspaces/E-HGP/build/v10-verrou-points/juge_final/principe_libre/principe.py',
 'gamma_reference_snapshot.py':'/tmp/mmta-c4ab-four-fixtures.RGuOI6lg/gamma_reference_snapshot.py',
 'qsqrt_snapshot.py':'/workspaces/E-HGP/build/v10-verrou-points/ancrage_marges/qsqrt.py'}
OUTPUTS=('capture_normal.json','capture_optimized.json','execution.json')
TIMEOUT_SECONDS=10
def require(c,m):
 if not c: raise RuntimeError(m)
def sha(p): return hashlib.sha256(p.read_bytes()).hexdigest()
def save_execution(runs,before,shared_before):
 e={'capture_root':str(ROOT),'python_executable':sys.executable,'python_realpath':os.path.realpath(sys.executable),
    'python_sha256':sha(Path(os.path.realpath(sys.executable))),'python_version':sys.version,
    'runs':runs,'source_before':before,'source_after':{n:sha(ROOT/n) for n in SOURCES},
    'shared_before':shared_before,'shared_after':{p:sha(Path(p)) for p in SHARED.values()},
    'stdlib':'external Python standard library; archive not hermetic','native_calls':0,'GCP':'unused'}
 (ROOT/'execution.json').write_text(json.dumps(e,sort_keys=True,indent=1)+'\n'); return e
def main():
 require(not (ROOT/'manifest.json').exists(),'already closed')
 require({p.name for p in ROOT.iterdir()}==set(SOURCES),'fresh inventory')
 before={n:sha(ROOT/n) for n in SOURCES}; shared_before={p:sha(Path(p)) for p in SHARED.values()}
 require(all(before[n]==shared_before[p] for n,p in SHARED.items()),'source differs from snapshot')
 runs=[]; results=[]
 for mode,name in (([],OUTPUTS[0]),(['-O'],OUTPUTS[1])):
  argv=[sys.executable,'-B',*mode,str(ROOT/'probe.py')]
  begin=time.monotonic()
  try:
   r=subprocess.run(argv,capture_output=True,text=True,timeout=TIMEOUT_SECONDS)
   raw={'argv':argv,'exit_code':r.returncode,'stdout':r.stdout,'stderr':r.stderr,'timed_out':False}
  except subprocess.TimeoutExpired as exc:
   def partial(value):
    if value is None: return ''
    return value.decode('utf-8',errors='surrogateescape') if isinstance(value,bytes) else value
   raw={'argv':argv,'exit_code':None,'stdout':partial(exc.stdout),'stderr':partial(exc.stderr),'timed_out':True}
  raw['duration_seconds']=time.monotonic()-begin
  raw['timeout_seconds']=TIMEOUT_SECONDS
  runs.append(raw)
  (ROOT/name).write_text(raw['stdout'],encoding='utf-8',errors='surrogateescape')
  save_execution(runs,before,shared_before)
  require(raw['exit_code']==0 and raw['timed_out'] is False and not raw['stderr'],'child failed or timed out; raw execution preserved')
  results.append(json.loads(raw['stdout']))
 e=save_execution(runs,before,shared_before)
 require(results[0]==results[1],'normal/-O differ')
 require(e['source_before']==e['source_after'],'source changed during capture')
 require(e['shared_before']==e['shared_after'],'shared source changed during capture')
 files={n:sha(ROOT/n) for n in (*SOURCES,*OUTPUTS)}
 manifest={'format':'mmta-cr-hard-mask-k2-v1','files':files,'inventory':sorted((*files,'manifest.json'))}
 (ROOT/'manifest.json').write_text(json.dumps(manifest,sort_keys=True,indent=1)+'\n')
 print(json.dumps({'directory':str(ROOT),'manifest_sha256':sha(ROOT/'manifest.json'),'inventory':manifest['inventory']}))
if __name__=='__main__': main()
