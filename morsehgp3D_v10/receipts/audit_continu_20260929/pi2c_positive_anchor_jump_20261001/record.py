#!/usr/bin/env python3
"""One-shot closure of this small private proof, preserving raw child outputs."""
import hashlib
import json
import os
import subprocess
import sys
from pathlib import Path

ROOT=Path(__file__).absolute().parent
SHARED={
 'pipeline_snapshot.py':'/workspaces/E-HGP/build/v10-verrou-points/juge_final/parametres/pipeline.py',
 'mmt_pond_snapshot.py':'/workspaces/E-HGP/build/v10-verrou-points/juge_final/parametres/mmt_pond.py',
 'qsqrt_snapshot.py':'/workspaces/E-HGP/build/v10-verrou-points/ancrage_marges/qsqrt.py',
}
SOURCES=('README.md','check.py','read.py','record.py','pipeline_snapshot.py','mmt_pond_snapshot.py','qsqrt_snapshot.py','preflight_failure.json')
OUTPUTS=('capture_normal.json','capture_optimized.json','execution.json')

def require(c,m):
 if not c: raise RuntimeError(m)

def sha(p): return hashlib.sha256(p.read_bytes()).hexdigest()

def main():
 require(not (ROOT/'manifest.json').exists(),'already closed')
 require({p.name for p in ROOT.iterdir()}==set(SOURCES),'fresh inventory')
 before={name:sha(ROOT/name) for name in SOURCES}
 shared_before={path:sha(Path(path)) for path in SHARED.values()}
 require(all(shared_before[path]==before[name] for name,path in SHARED.items()),'shared snapshot changed before capture')
 executions=[]; results=[]
 for mode,name in (([],OUTPUTS[0]),(['-O'],OUTPUTS[1])):
  argv=[sys.executable,'-B',*mode,str(ROOT/'check.py')]
  r=subprocess.run(argv,capture_output=True,text=True,timeout=10)
  raw={'argv':argv,'exit_code':r.returncode,'stdout':r.stdout,'stderr':r.stderr}
  executions.append(raw)
  require(r.returncode==0 and not r.stderr,'child failed: '+str(raw))
  result=json.loads(r.stdout); results.append(result)
  (ROOT/name).write_text(r.stdout)
 require(results[0]==results[1],'normal/-O differ')
 after={name:sha(ROOT/name) for name in SOURCES}
 require(before==after,'sources changed')
 shared_after={path:sha(Path(path)) for path in SHARED.values()}
 require(shared_before==shared_after,'shared sources changed during captures')
 execution={'capture_root':str(ROOT),'runs':executions,'source_before':before,'source_after':after,'shared_before':shared_before,'shared_after':shared_after,
            'python_executable':sys.executable,'python_realpath':os.path.realpath(sys.executable),'python_sha256':sha(Path(os.path.realpath(sys.executable))),
            'python_version':sys.version,'stdlib':'external Python standard library; archive not hermetic','GCP':'unused','native_calls':0}
 (ROOT/OUTPUTS[2]).write_text(json.dumps(execution,sort_keys=True,indent=1)+'\n')
 files={name:sha(ROOT/name) for name in (*SOURCES,*OUTPUTS)}
 manifest={'format':'pi2c-positive-anchor-v2','files':files,'inventory':sorted((*files,'manifest.json')),'capture_count':2,'cases':6,'mutants':2}
 (ROOT/'manifest.json').write_text(json.dumps(manifest,sort_keys=True,indent=1)+'\n')
 print(json.dumps({'directory':str(ROOT),'manifest_sha256':sha(ROOT/'manifest.json'),'inventory':manifest['inventory']}))

if __name__=='__main__': main()
