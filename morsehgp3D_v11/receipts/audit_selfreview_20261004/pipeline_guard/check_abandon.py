#!/usr/bin/env python3
"""Bounded stdlib state model; no C++/atomics/threads executed."""
from pathlib import Path
import hashlib,json,re
R=Path(__file__).resolve().parent
N=(1<<32)-1
checks=0

def need(ok,message):
 global checks
 checks+=1
 if not ok:raise RuntimeError(message)

class View:
 def __init__(self,closed=0,done=False,abandoned=False,script=()):
  self.closed=closed;self.done=done;self.abandoned=abandoned;self.script=script;self.blocks=0
 def block(self):
  if self.blocks>=len(self.script):raise RuntimeError('model script exhausted')
  self.closed,self.done,self.abandoned=self.script[self.blocks];self.blocks+=1

def wait(view,level,post_guard):
 while not(view.done or level<view.closed):
  if view.abandoned:return False
  view.block()
 return not view.abandoned if post_guard else True

def main():
 manifest=json.loads((R/'SOURCE_BEFORE.json').read_text())
 for path,e in manifest['files'].items():need(hashlib.sha256((R/e['copy']).read_bytes()).hexdigest()==e['sha256'],'snapshot differs '+path)
 header=(R/'source/morsehgp3D_v11/src/tower/forest_internal.hpp').read_text()
 follow=(R/'source/morsehgp3D_v11/src/tower/forest_vertical.cpp').read_text()
 gate=(R/'source/morsehgp3D_v11/tests/tower/forest_pipeline_test.cpp').read_text()
 cmake=(R/'source/morsehgp3D_v11/tests/tower/tests.cmake').read_text()
 table=json.loads((R/'source/morsehgp3D_v11/tests/mutants/tower.json').read_text())
 selected=[m for m in table['mutants'] if m['id']=='pipeline_abandon_apres_reveil']
 need(len(selected)==1,'new mutant id not unique');mutant=selected[0]
 need(header.count(mutant['cherche'])==1,'mutant target not unique')
 need(mutant['remplace']=='  return true;\n}','mutant does not remove post-wake guard')
 need(mutant['porte']=='mhgp11_tower_pipeline_abandon','mutant gate mismatch')
 function=re.search(r'bool await_lower\(View& low, LevelRank level\) noexcept \{(.*?)\n\}',header,re.S)
 need(function is not None,'await_lower missing')
 expected='while (!follow_lower_ready(level, low.done, low.closed)) { if (low.abandoned) return false; low.block(); } return !low.abandoned;'
 need(' '.join(function.group(1).split())==expected,'model not bound to current helper body')
 caller='if (!await_lower(low, level)) return {};'
 need(caller in follow and follow.index(caller)<follow.index('MHGP11_TRY(sweep.advance(level, work, low.nodes));'),'caller does not exit before seed/sweep reads')
 need('GROUPS decisions equivalence abandon' in cmake and 'MHGP11_TEST(abandon, 700)' in gate,'new gate not declared')
 need('S{kNone, false, true}' in gate and 'CHECK(!tower_detail::await_lower(v, LevelRank{5})); CHECK_EQ(v.blocks, 1u)' in gate,'deterministic after-wake fixture missing')
 cases=[('abandon_after_one_wake',0,False,False,[(N,False,True)],False,1),
        ('already_abandoned',N,False,True,[],False,0),
        ('already_ready',9,False,False,[],True,0),
        ('three_announcements',0,False,False,[(3,False,False),(5,False,False),(6,False,False)],True,3),
        ('announce_then_abandon',0,False,False,[(3,False,False),(N,False,True)],False,2),
        ('normal_finish',0,False,False,[(N,True,False)],True,1)]
 records=[]
 for name,closed,done,abandoned,script,result,blocks in cases:
  v=View(closed,done,abandoned,script);got=wait(v,5,True)
  need(got==result and v.blocks==blocks,'script differs '+name)
  mutant_view=View(closed,done,abandoned,script);mutant_result=wait(mutant_view,5,False)
  records.append({'name':name,'result':got,'blocks':v.blocks,'mutant_return_true_result':mutant_result,'mutant_killed':mutant_result!=result})
 need(records[0]['mutant_killed'],'first deterministic native fixture would not kill mutant')
 # Valid LevelRank boundaries, abort always makes closed ready but is never permission to read.
 for level in (0,1,5,(1<<31)-1,(1<<31),N-1):
  v=View(closed=0,script=[(N,False,True)])
  need(not wait(v,level,True) and v.blocks==1,'abandon at rank boundary')
  w=View(closed=0,script=[(N,False,True)])
  need(wait(w,level,False),'old/mutant causal counter-case missing')
  f=View(closed=0,script=[(N,True,False)])
  need(wait(f,level,True),'normal finish refused')
 base=json.loads((R/'BASE_SOURCE.json').read_text())
 for p,e in base['files'].items():need(hashlib.sha256((R/e['copy']).read_bytes()).hexdigest()==e['sha256'],'base snapshot differs '+p)
 output={'status':'PASS','checks':checks,'author_head_before':manifest['author_head_before'],
         'wip_header_sha256':manifest['files']['morsehgp3D_v11/src/tower/forest_internal.hpp']['sha256'],
         'wip_vertical_sha256':manifest['files']['morsehgp3D_v11/src/tower/forest_vertical.cpp']['sha256'],
         'scripted_cases':records,'mutant_target_occurrences':1,
         'source_verdict':'post-wake abandon guard present and used before dependent reads; deterministic gate causal',
         'scope':'stdlib value-state model only; no C++ atomic wait, pthread, native compilation or race/liveness qualification',
         'native_runs':0,'fits':0,'gcp_actions':0}
 print(json.dumps(output,indent=2,sort_keys=True))
if __name__=='__main__':main()
