#!/usr/bin/env python3
"""Hash-first static archive reader. Never executes payloads or contacts shared sources."""
import hashlib
import json
import math
import re
import stat
import sys
from fractions import Fraction as F
from pathlib import Path

SOURCES={'README.txt','probe.py','read.py','record.py','base_snapshot.py','principe_snapshot.py',
 'gamma_reference_snapshot.py','qsqrt_snapshot.py','preflight_snapshot.py','preflight_failure.json'}
FILES=SOURCES|{'capture_normal.json','capture_optimized.json','execution.json','manifest.json'}
PINS={
 'base_snapshot.py':'357f3233ea749e80b0e611cfdb80dd2b85add2e587889f1da876407a371bbb19',
 'principe_snapshot.py':'c4ab293273d9b84d370852f0953986fac4be8c27a50e9c50459523a9087e58bb',
 'gamma_reference_snapshot.py':'c99080876e3b62d5aebc184e28f501acae281fb4db023c8a07378bbcd8c88f29',
 'qsqrt_snapshot.py':'a1d1ed969fa5afc7b9a7e1746e07d770da45c5f80e9c2c80e034ad358394984d'}
SHARED={
 'base_snapshot.py':'/tmp/mmta-c4ab-four-fixtures.RGuOI6lg/probe.py',
 'principe_snapshot.py':'/workspaces/E-HGP/build/v10-verrou-points/juge_final/principe_libre/principe.py',
 'gamma_reference_snapshot.py':'/tmp/mmta-c4ab-four-fixtures.RGuOI6lg/gamma_reference_snapshot.py',
 'qsqrt_snapshot.py':'/workspaces/E-HGP/build/v10-verrou-points/ancrage_marges/qsqrt.py'}
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
def digest(b): return hashlib.sha256(b).hexdigest()
def triangle_meb(P,t):
 a,b,c=(P[i] for i in t)
 squared=lambda u,v:sum((x-y)**2 for x,y in zip(u,v))
 l1,l2,L=sorted((squared(a,b),squared(a,c),squared(b,c)))
 if L>=l1+l2: return L/4
 det=(b[0]-a[0])*(c[1]-a[1])-(b[1]-a[1])*(c[0]-a[0])
 require(det!=0,'acute triangle has zero determinant')
 return l1*l2*L/(4*det*det)
def height(case,key=(0,1),baseline=False):
 hs=case['baseline_heights' if baseline else 'heights']
 return next(F(value) for xy,value,_approx in hs if tuple(xy)==key)
def main():
 require(len(sys.argv)==3,'usage read.py DIRECTORY EXTERNAL_MANIFEST_SHA256')
 root=Path(sys.argv[1]).absolute(); expected=sys.argv[2]
 require(re.fullmatch('[0-9a-f]{64}',expected) is not None,'external SHA format')
 for p in (root,*root.parents): require(stat.S_ISDIR(p.lstat().st_mode),'root/ancestor not real directory')
 require({p.name for p in root.iterdir()}==FILES,'closed inventory')
 for p in root.iterdir(): require(stat.S_ISREG(p.lstat().st_mode),'payload not regular')
 raw=(root/'manifest.json').read_bytes(); require(digest(raw)==expected,'external manifest SHA mismatch')
 man=parse(raw)
 require(man['format']=='mmta-cr-hard-mask-k2-v1','format')
 require(set(man['files'])==FILES-{'manifest.json'} and man['inventory']==sorted(FILES),'manifest inventory')
 payload={}
 for n,pin in man['files'].items():
  require(isinstance(pin,str) and re.fullmatch('[0-9a-f]{64}',pin) is not None,'payload SHA format')
  payload[n]=(root/n).read_bytes(); require(digest(payload[n])==pin,'payload SHA '+n)
 require(all(man['files'][n]==pin for n,pin in PINS.items()),'known snapshot pins')
 normal=parse(payload['capture_normal.json']); optimized=parse(payload['capture_optimized.json'])
 require(normal==optimized,'normal/-O mismatch')
 require(normal['parameters']=={'K':2,'mcs':3,'kappa':'3','eta':'1/2','lambda':'1/2'},'parameters')
 require(normal['native_calls']==0 and normal['GCP']=='unused' and normal['checks']==961,'scope/nonvacuity')
 require(normal['source_pins']=={n:p for n,p in PINS.items() if n!='base_snapshot.py'},'captured source pins')
 expected_cases={(v,e) for v in ('noyau','amas') for e in ('-1/128','0','1/128','1/1024','1/8192')}
 cases=normal['cases']; require(len(cases)==10 and {(c['variant'],c['epsilon']) for c in cases}==expected_cases,'cases')
 for c in cases:
  e=F(c['epsilon']); require(len(c['rows'])==5 and len(c['triple_MEBs'])==10,'sites/exhaustive triples')
  coordinates=c['coordinates']
  require(isinstance(coordinates,list) and len(coordinates)==5 and all(isinstance(p,list) and len(p)==2 and all(isinstance(v,str) for v in p) for p in coordinates),'coordinate schema')
  P=tuple(tuple(F(v) for v in p) for p in coordinates)
  require(P==((F(0),F(0)),(F(10),F(0)),(F(211,10)-e,F(0)),(F(-11),F(36,5)),(F(-11),F(-36,5))),'expected fixture coordinates')
  require({tuple(t) for t,_b in c['triple_MEBs']}=={(0,1,2),(0,1,3),(0,1,4),(0,2,3),(0,2,4),(0,3,4),(1,2,3),(1,2,4),(1,3,4),(2,3,4)},'triple inventory')
  for t,beta in c['triple_MEBs']: require(F(beta)==triangle_meb(P,t),'independent triangle MEB '+str(t))
  require(height(c,baseline=True)==F(111,10),'without CR positive control')
  expected_height=F(111,10) if c['variant']=='noyau' and e<=0 else F(211,20)-e/2
  require(height(c)==expected_height,'analytic height')
  if c['variant']=='noyau':
   row=c['rows'][0]; require(F(row['Ax'])==25 and F(row['rc'])==(F(111,10)-e)**2,'core/anchor')
   require(sum(F(p[1])==F(4321,100) for p in row['profile'])==(0 if e>0 else 2),'hard mask exercised')
  if c['variant']=='noyau' and not e:
   q=F(4321,100); gg=F(18671041,302500); J=F(12321,100); A0=F(44521,400); E=gg+F(25,2)
   L=((J-A0)/F(25,2))*(E-25); G=2*(gg-q)+(E-gg); mu=(G-L)/(G+L)
   require(F(c['rows'][0]['date'])==F(111,10)-15*mu and F(c['rows'][0]['W'])==L+G and mu>0,'plateau mass/date independent oracle')
 grid=normal['integer_grid_twins']; require(len(grid)==3 and {v['M'] for v in grid}=={1,13,816},'grid controls')
 for c in grid:
  M=c['M']; require(c['max_coordinate']==321*M<2**18 and c['single_site_move']==1,'grid domain')
  require(F(c['height_perturbed'])==F(211*M-1,2) and F(c['height_plateau'])==111*M,'grid heights')
  require(F(c['height_jump'])==F(11*M+1,2),'grid jump')
 execution=parse(payload['execution.json'])
 require(execution['source_before']==execution['source_after']=={n:man['files'][n] for n in SOURCES},'source inventory/pins before/after')
 require(execution['shared_before']==execution['shared_after']=={p:PINS[n] for n,p in SHARED.items()},'shared inventory/pins before/after')
 for key in ('python_executable','capture_root'):
  require(isinstance(execution[key],str) and Path(execution[key]).is_absolute(),'absolute recorded '+key)
 require(isinstance(execution['python_sha256'],str) and re.fullmatch('[0-9a-f]{64}',execution['python_sha256']) is not None,'Python SHA')
 expected_argv=[[execution['python_executable'],'-B',str(Path(execution['capture_root'])/'probe.py')],
                [execution['python_executable'],'-B','-O',str(Path(execution['capture_root'])/'probe.py')]]
 runs=execution['runs']; require(len(runs)==2 and [r['argv'] for r in runs]==expected_argv,'two exact argv')
 for r,n in zip(runs,('capture_normal.json','capture_optimized.json')):
  require(type(r['exit_code']) is int and r['exit_code']==0 and r['timed_out'] is False,'terminal command status')
  require(type(r['timeout_seconds']) is int and r['timeout_seconds']==10,'exact command timeout')
  elapsed=r['duration_seconds']
  require(type(elapsed) in (int,float) and math.isfinite(elapsed) and elapsed>=0,'finite command duration')
  require(r['stderr']=='' and r['stdout'].encode()==payload[n],'raw command/capture correspondence')
 failure=parse(payload['preflight_failure.json']); require(failure['exit_code']==1 and failure['source_sha256']==man['files']['preflight_snapshot.py'],'preserved preparation failure')
 for n,b in payload.items(): require((root/n).read_bytes()==b,'payload changed during read')
 require((root/'manifest.json').read_bytes()==raw and {p.name for p in root.iterdir()}==FILES,'archive changed during read')
 print('PASS: actual MMtA_CR nucleus hard-mask K2 counterexample, 10 rational cases/3 grid twins/961 guards, normal/-O; static archive only')
if __name__=='__main__':
 try: main()
 except (ValueError,KeyError,TypeError,OSError,StopIteration) as exc:
  print('REFUSED: '+str(exc),file=sys.stderr); sys.exit(2)
