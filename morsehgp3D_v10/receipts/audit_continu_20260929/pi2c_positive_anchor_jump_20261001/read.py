#!/usr/bin/env python3
"""Static archive reader: external manifest SHA BEFORE JSON; never execute payloads."""
import hashlib
import json
import re
import stat
import sys
from pathlib import Path

SOURCES={'README.md','check.py','read.py','record.py','pipeline_snapshot.py','mmt_pond_snapshot.py','qsqrt_snapshot.py','preflight_failure.json'}
PINS={
 'pipeline_snapshot.py':'8fb7eba774b2547bb34aafecd1405964d1d4a56687f308562a435f258be01734',
 'mmt_pond_snapshot.py':'abbeac86b171e851a4e782a5d573496e2e4a9e118ceddf4c820db21a6c66b2af',
 'qsqrt_snapshot.py':'a1d1ed969fa5afc7b9a7e1746e07d770da45c5f80e9c2c80e034ad358394984d',
}
SHARED={
 'pipeline_snapshot.py':'/workspaces/E-HGP/build/v10-verrou-points/juge_final/parametres/pipeline.py',
 'mmt_pond_snapshot.py':'/workspaces/E-HGP/build/v10-verrou-points/juge_final/parametres/mmt_pond.py',
 'qsqrt_snapshot.py':'/workspaces/E-HGP/build/v10-verrou-points/ancrage_marges/qsqrt.py',
}
FILES={'README.md','check.py','read.py','record.py','pipeline_snapshot.py','mmt_pond_snapshot.py','qsqrt_snapshot.py',
       'preflight_failure.json','capture_normal.json','capture_optimized.json','execution.json','manifest.json'}

def require(c,m):
 if not c: raise ValueError(m)

def pairs(items):
 out={}
 for k,v in items:
  require(k not in out,'duplicate JSON key'); out[k]=v
 return out

def parse(data):
 def reject(v): raise ValueError('nonfinite JSON')
 return json.loads(data,object_pairs_hook=pairs,parse_constant=reject)

def main():
 require(len(sys.argv)==3,'usage read.py DIRECTORY EXTERNAL_MANIFEST_SHA256')
 root=Path(sys.argv[1]).absolute(); expected=sys.argv[2]
 require(re.fullmatch('[0-9a-f]{64}',expected) is not None,'external SHA format')
 for p in (root,*root.parents): require(stat.S_ISDIR(p.lstat().st_mode),'root/ancestor not real directory')
 require({p.name for p in root.iterdir()}==FILES,'closed inventory')
 for p in root.iterdir(): require(stat.S_ISREG(p.lstat().st_mode),'payload not regular')
 raw=(root/'manifest.json').read_bytes()
 require(hashlib.sha256(raw).hexdigest()==expected,'external manifest SHA mismatch')
 manifest=parse(raw)
 require(manifest['format']=='pi2c-positive-anchor-v2','format')
 require(set(manifest['files'])==FILES-{'manifest.json'} and manifest['inventory']==sorted(FILES),'manifest inventory')
 original={}
 for name,digest in manifest['files'].items():
  require(isinstance(digest,str) and re.fullmatch('[0-9a-f]{64}',digest) is not None,'payload SHA format')
  original[name]=(root/name).read_bytes()
  require(hashlib.sha256(original[name]).hexdigest()==digest,'payload SHA '+name)
 normal=parse(original['capture_normal.json']); opt=parse(original['capture_optimized.json'])
 require(normal==opt,'normal/-O mismatch')
 require(normal['K']==2 and normal['mcs']==3 and normal['eta']=='2/3' and normal['kappa']=='4','parameters')
 require(len(normal['cases'])==6 and len(normal['mutants'])==2 and normal['native_calls']==0,'nonvacuity')
 require({c['epsilon'] for c in normal['cases']}=={'0','1/8','1/32','1/128','1/1024','1/4096'},'case inventory')
 require({m['mutant'] for m in normal['mutants']}=={'premature_xby','omit_xay'},'mutant inventory')
 for c in normal['cases']:
  limit=c['epsilon']=='0'
  require(c['A']==('16' if limit else '9'),'anchor')
  require(c['T_half']==('6145/288' if limit else '12'),'date-square oracle')
  require(c['date_x']==('1*sqrt(6145/288)' if limit else '1*sqrt(12)'),'date')
  require(c['height_xa']==('5' if limit else '1*sqrt(12)'),'height')
  require(len(c['triple_MEBs'])==10 and c['mcs2_control']=='all omega=1, A=9, date_x=sqrt(12)','exhaustive/control')
 execution=parse(original['execution.json'])
 expected_source={name:manifest['files'][name] for name in SOURCES}
 require(execution['source_before']==expected_source and execution['source_after']==expected_source,'source capture exact inventory/pins')
 expected_shared={path:PINS[name] for name,path in SHARED.items()}
 require(execution['shared_before']==expected_shared and execution['shared_after']==expected_shared,'shared capture exact inventory/pins')
 require(all(manifest['files'][name]==digest for name,digest in PINS.items()),'snapshot known pins')
 require(normal['snapshot_pins_before']==PINS and normal['snapshot_pins_after']==PINS,'capture snapshot pins')
 require(isinstance(execution['python_executable'],str) and Path(execution['python_executable']).is_absolute(),'python executable')
 require(isinstance(execution['capture_root'],str) and Path(execution['capture_root']).is_absolute(),'capture origin')
 require(isinstance(execution['python_sha256'],str) and re.fullmatch('[0-9a-f]{64}',execution['python_sha256']) is not None,'python capture SHA')
 expected_argv=[
  [execution['python_executable'],'-B',str(Path(execution['capture_root'])/'check.py')],
  [execution['python_executable'],'-B','-O',str(Path(execution['capture_root'])/'check.py')],
 ]
 require([run['argv'] for run in execution['runs']]==expected_argv,'two exact argv')
 require(len(execution['runs'])==2,'commands')
 for run,name in zip(execution['runs'],('capture_normal.json','capture_optimized.json')):
  require(run['exit_code']==0 and not run['stderr'] and run['stdout'].encode()==original[name],'raw correspondence')
 for name,data in original.items(): require((root/name).read_bytes()==data,'payload changed during read')
 require((root/'manifest.json').read_bytes()==raw,'manifest changed during read')
 require({p.name for p in root.iterdir()}==FILES,'inventory changed')
 print('PASS: 6 exact Gamma2/Pi2c/MMt cases, 10 triples each, 2 causal mutants, normal/-O; static archive only')

if __name__=='__main__':
 try: main()
 except (ValueError,KeyError,TypeError,OSError) as exc:
  print('REFUSED: '+str(exc),file=sys.stderr); sys.exit(2)
