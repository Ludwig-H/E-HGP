#!/usr/bin/env python3
"""Static hash-first reader. It never executes archived Python."""
import argparse
import hashlib
import json
import math
import re
import stat
import sys
from pathlib import Path

ROOT=Path(__file__).absolute().parent
SOURCES=('README.txt','er_snapshot.py','probe.py','read.py','record.py')
OUTPUTS=('capture_normal.json','capture_optimized.json','execution.json')
FILES=set(SOURCES+OUTPUTS)
PIN='99e8ba720f417af9b267f8b70ea554be26c7545ea20e673a7dc89248debe321d'
SHARED='/workspaces/E-HGP/build/v10-verrou-points/juge_final/echelle_relative/er.py'
SCOPE='actual AST StructureER.__init__, exact Gamma1 star data of n integer collinear sites; no native engine/ER rule execution'

def require(c,m):
 if not c: raise ValueError(m)
def sha(p): return hashlib.sha256(p.read_bytes()).hexdigest()
def pairs(items):
 out={}
 for k,v in items:
  require(k not in out,'duplicate JSON key'); out[k]=v
 return out
def bad_constant(v): raise ValueError('nonfinite JSON constant: '+v)
def parse(raw): return json.loads(raw,object_pairs_hook=pairs,parse_constant=bad_constant)
def keys(obj,expected,where): require(type(obj) is dict and set(obj)==set(expected),'keys: '+where)
def uint(v): return type(v) is int and v>=0
def digest(v): return type(v) is str and re.fullmatch('[0-9a-f]{64}',v) is not None
def vector_digest(v):
 return hashlib.sha256(json.dumps(v,sort_keys=True,separators=(',',':'),ensure_ascii=True,allow_nan=False).encode('ascii')).hexdigest()

def inspect_root():
 for p in (ROOT,)+tuple(ROOT.parents):
  s=p.lstat(); require(not stat.S_ISLNK(s.st_mode) and stat.S_ISDIR(s.st_mode),'root/ancestor must be a nonsymlink directory')
 entries=list(ROOT.iterdir())
 require({p.name for p in entries}==FILES|{'manifest.json'},'closed real inventory')
 for p in entries:
  require(stat.S_ISREG(p.lstat().st_mode),'payload must be a regular nonsymlink file')

def expected_cases():
 rows=[]
 for n in (2,8,32,128,512):
  for reverse_children in (False,True):
   for root_first in (False,True):
    children=list(range(n-1,-1,-1) if reverse_children else range(n))
    total=0
    for i in range(n):
     cv=dict(((n,1),(i,0)) if root_first else ((i,0),(n,1)))
     naive=[v for v in cv if not any(ch in cv for ch in (children if v==n else []))]
     require(sorted(naive)==[i],'independent minima')
     entries=set(cv)
     for u in cv: entries.discard(n if u<n else -1)
     require(sorted(entries)==[i],'independent parent-discard')
     broken=set(cv)
     for u in cv: broken.discard(u)
     require(not broken,'semantic mutant must empty profile')
     total+=children.index(i)+1
    require(total==n*(n+1)//2,'permutation membership sum')
    expected_digest=vector_digest([[i] for i in range(n)])
    mutant_digest=vector_digest([[] for _ in range(n)])
    require(expected_digest!=mutant_digest,'causal vector digest distinction')
    rows.append({'n':n,'H':n+1,'D':2*n,'root_first':root_first,'reverse_children':reverse_children,
                 'actual_child_memberships':total,'parent_discard_operations':2*n,'entry_lists_equal':True,
                 'entry_source_sha256':expected_digest,'entry_parent_discard_sha256':expected_digest,
                 'entry_mutant_self_sha256':mutant_digest})
 return rows

def judge_output(d):
 keys(d,('scope','source_sha256','executed_cases','causal_discard_self_mutants_rejected','large_n_analytic_only','alpha2','positive_anchor_ER_K1_rule','GCP','native_calls'),'probe output')
 require(d['scope']==SCOPE and d['source_sha256']==PIN,'scope/pin')
 require(type(d['executed_cases']) is list and d['executed_cases']==expected_cases(),'20 cases exact')
 for row in d['executed_cases']:
  require(type(row['root_first']) is bool and type(row['reverse_children']) is bool and row['entry_lists_equal'] is True,'case bool types')
  require(all(uint(row[k]) for k in ('n','H','D','actual_child_memberships','parent_discard_operations')),'case integer types')
  require(all(digest(row[k]) for k in ('entry_source_sha256','entry_parent_discard_sha256','entry_mutant_self_sha256')),'case vector digest types')
 require(type(d['causal_discard_self_mutants_rejected']) is int and d['causal_discard_self_mutants_rejected']==20,'20 causal mutants')
 expected=[{'n':n,'H':n+1,'D':2*n,'child_memberships':n*(n+1)//2,'parent_discard_operations':2*n} for n in (8000,16000,32000)]
 require(type(d['large_n_analytic_only']) is list and d['large_n_analytic_only']==expected,'analytic-only counts')
 require(all(all(uint(v) for v in row.values()) for row in d['large_n_analytic_only']),'analytic integer types')
 require(type(d['alpha2']) is int and d['alpha2']==0 and type(d['native_calls']) is int and d['native_calls']==0,'no point rule/native')
 require(d['positive_anchor_ER_K1_rule']=='not executed / outside positive-anchor domain' and d['GCP']=='unused','domain/GCP')

def run(expected_sha):
 require(digest(expected_sha),'external SHA256 syntax')
 inspect_root()
 require(sha(ROOT/'manifest.json')==expected_sha,'external manifest SHA256 mismatch')
 m=parse((ROOT/'manifest.json').read_bytes())
 keys(m,('format','files'),'manifest'); require(m['format']=='er-entry-star-archive-v1','manifest format')
 keys(m['files'],FILES,'hash inventory')
 for name,h in m['files'].items(): require(digest(h) and sha(ROOT/name)==h,'payload hash: '+name)
 require(m['files']['er_snapshot.py']==PIN,'original source pin')
 e=parse((ROOT/'execution.json').read_bytes())
 keys(e,('origin','python','python_realpath','python_sha256','python_version','source_before','source_after','shared_source','shared_source_before','shared_source_after','captures','timeout_seconds'),'execution')
 for name in ('origin','python','python_realpath'):
  require(type(e[name]) is str and Path(e[name]).is_absolute() and str(Path(e[name]))==e[name] and '..' not in Path(e[name]).parts,'absolute canonical lexical path: '+name)
 require(digest(e['python_sha256']) and type(e['python_version']) is str and len(e['python_version'])>0,'Python provenance')
 expected_sources={n:m['files'][n] for n in SOURCES}
 require(e['source_before']==expected_sources and e['source_after']==expected_sources,'source inventories before/after')
 require(e['shared_source']==SHARED and e['shared_source_before']==e['shared_source_after']==PIN,'shared source before/after')
 require(e['captures']==list(OUTPUTS[:2]) and type(e['timeout_seconds']) is int and e['timeout_seconds']==15,'execution capture protocol')
 captures=[]
 for optimized,name in ((False,OUTPUTS[0]),(True,OUTPUTS[1])):
  c=parse((ROOT/name).read_bytes())
  keys(c,('argv','stdout','stderr','exit_code','timed_out','duration_seconds','timeout_seconds'),'capture')
  argv=[e['python'],'-B']+(['-O'] if optimized else [])+[e['origin']+'/probe.py']
  require(c['argv']==argv,'exact capture argv')
  require(type(c['exit_code']) is int and c['exit_code']==0 and c['timed_out'] is False,'terminal code0 required')
  require(type(c['timeout_seconds']) is int and c['timeout_seconds']==15,'capture timeout')
  require(type(c['duration_seconds']) in (int,float) and math.isfinite(c['duration_seconds']) and c['duration_seconds']>=0,'finite capture duration')
  require(type(c['stdout']) is str and c['stderr']=='','raw capture streams')
  judge_output(parse(c['stdout'])); captures.append(c)
 require(captures[0]['stdout']==captures[1]['stdout'],'normal/-O output identity')
 inspect_root()
 for name,h in m['files'].items(): require(sha(ROOT/name)==h,'post-read payload hash: '+name)
 require(sha(ROOT/'manifest.json')==expected_sha,'post-read manifest hash')
 print('PASS: 20 executed star cases, 20 causal controls; three large-n rows analytic only; static archive reader, no native/GCP')

if __name__=='__main__':
 p=argparse.ArgumentParser(); p.add_argument('--manifest-sha256',required=True); a=p.parse_args()
 try: run(a.manifest_sha256)
 except (ValueError,OSError,UnicodeError,TypeError,KeyError) as e:
  print('REFUSE: '+str(e),file=sys.stderr); sys.exit(2)
