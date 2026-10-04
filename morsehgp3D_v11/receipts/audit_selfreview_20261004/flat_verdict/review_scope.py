#!/usr/bin/env python3
"""Metadata/source-only recoupe; never imports the product or fits a model."""
import ast
import hashlib
import json
from pathlib import Path
HERE=Path(__file__).resolve().parent
checks=0
def need(condition, message):
 global checks
 checks+=1
 if not condition: raise RuntimeError(message)
def read(name): return json.loads((HERE/name).read_text())
before=read('SOURCE_BEFORE.json');after=read('SOURCE_AFTER.json')
for entry in before['files']:
 if entry['capture']:
  raw=(HERE/entry['capture']).read_bytes()
  need(len(raw)==entry['bytes'] and hashlib.sha256(raw).hexdigest()==entry['sha256'], 'frozen source '+entry['path'])
for entry in after['files']:
 need(entry['unchanged_from_before'], 'published source unchanged '+entry['path'])
origins=read('metadata_origins.json');scope=read('session_scope.json')
for session in ('claudeflat0','claudeflat1a','claudeflat1b'):
 s=scope[session]
 need(s['status']=='completed' and s['targeted_shutdown_certified'] is True and s['observed_after']['status']=='TERMINATED','target closure '+session)
 for c in s['commands']: need(c['status']=='ok' and c['exit_code']=='0','recorded command '+session+'/'+c['name'])
counts={}
for session,expected in [('claudeflat1a',(1654,119088)),('claudeflat1b',(641,46152))]:
 origin=origins[session]
 need(origin['receipt_records_archive_sha'] and origin['regular_members']==170 and origin['manifest_verified_entries']==169 and origin['manifest_unlisted']==['results/MANIFEST.sha256'],'previously hashed archive coverage '+session)
 for path,info in origin['selected_metadata'].items():
  need(hashlib.sha256((HERE/path).read_bytes()).hexdigest()==info['sha256'],'exact metadata member '+path)
 gate=read('metadata/'+session+'/flat_gate.json')
 need(gate['mode']=='native' and gate['verdict']=='conforme' and not gate['refusals'] and not gate['disagreements'],'played gate '+session)
 need((gate['clouds'],gate['comparisons'])==expected,'counts '+session)
 need(len(gate['fixtures'])==32 and len(gate['mutants'])==9,'fixture/mutant nonvacuitiy '+session)
 for f in gate['fixtures']: need(f['ok'] is True, 'fixture '+session+'/'+f['fixture'])
 for key,value in gate['mutants'].items(): need(value['killed'] is True and bool(value['how']),'causal mutant record '+session+'/'+key)
 counts[session]={'clouds':gate['clouds'],'comparisons':gate['comparisons'],'fixtures':len(gate['fixtures']),'mutants':len(gate['mutants'])}
source=HERE/'source/morsehgp3D_v11/bench/points_flat_gate.py'
module=ast.parse(source.read_bytes())
lines=next(n for n in module.body if isinstance(n,ast.Assign) and any(isinstance(t,ast.Name) and t.id=='LINES' for t in n.targets))
choices=ast.literal_eval(lines.value)
need(choices==( ('eom',1),('eom',3),('leaf',1) ), 'exact played selection choices')
need(('eom',2) not in choices,'z2 absent from plateau/oracle choice loop')
calls=[]
positions={'flat':4,'bench_labels':2,'oracle_labels':4}
for n in ast.walk(module):
 if isinstance(n,ast.Call) and isinstance(n.func,ast.Name) and n.func.id in positions:
  pos=positions[n.func.id]
  if len(n.args)>pos:
   z=n.args[pos]
   expr=ast.unparse(z)
   need(expr in ('1','3','z'),'no alternate explicit z2 path in frozen fixtures/compare_cloud')
   calls.append({'line':n.lineno,'function':n.func.id,'z':expr})
need(bool(calls),'nonempty AST scope inventory')
oracle=ast.parse((HERE/'source/morsehgp3D_v11/bench/points_flat_oracle.py').read_bytes())
head=next(n for n in oracle.body if isinstance(n,ast.FunctionDef) and n.name=='head')
scores=next(n for n in oracle.body if isinstance(n,ast.FunctionDef) and n.name=='scores')
need(any(isinstance(n,ast.Call) and isinstance(n.func,ast.Name) and n.func.id=='scores' and ast.unparse(n.args[-1])=='z' for n in ast.walk(head)), 'generic z passed from oracle head to scores')
need(any(isinstance(n,ast.Call) and isinstance(n.func,ast.Attribute) and n.func.attr=='power' and ast.unparse(n.args[0])=='z' for n in ast.walk(scores)), 'generic exact power(z) in oracle scores')
checks_lidar=read('lidar_decision.normal.json')
need((HERE/'lidar_decision.normal.json').read_bytes()==(HERE/'lidar_decision.optimized.json').read_bytes(),'normal and optimized decision replay equal')
need(checks_lidar['checks']==89 and checks_lidar['observed_source_defect'] is True,'decision proof exists')
print(json.dumps({'scope':'pinned source AST and existing receipt metadata only; no fit/native/cloud',
 'checks':checks,'decision_helper_checks':89,'source_before_pin':before['pin'],'source_after_pin':after['after_pin'],
 'flat_gates':counts,'played_rules':choices,'z2_explicit_call_sites':0,'z_call_inventory':sorted(calls,key=lambda v:v['line']),
 'oracle_generic_z_interface_present':True,'oracle_z2_runtime_executed_by_this_review':False},indent=2,sort_keys=True))
