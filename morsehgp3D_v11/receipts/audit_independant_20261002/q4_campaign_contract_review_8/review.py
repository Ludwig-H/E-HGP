#!/usr/bin/env python3
"""Pure contract checks for pinned q4 bench; no native execution/build/cloud."""
from pathlib import Path
import copy, hashlib, importlib.util, json, sys
ROOT=Path(__file__).resolve().parent
BENCH=ROOT/'sources/morsehgp3D_v11/bench'
TESTS=ROOT/'sources/morsehgp3D_v11/tests/catalogue'
sys.path.insert(0,str(BENCH));sys.path.insert(0,str(TESTS))
import catalogue_profiles as driver
import catalogue_semantic as semantic
from bench_semantic_test import fixture

def need(value, message):
    if not value:raise ValueError(message)

def review():
    blob,_=fixture(21)
    original=semantic.decode(blob,21,5,4)
    with_counts=semantic.decode(blob,21,5,4,arity_counts=True)
    counts=with_counts.pop('qmin_counts')
    need(with_counts==original and counts=={'2':6,'3':4,'4':1},'arity diagnostics changed semantic digest')
    name='uniform_u18_n8000'
    def row(bits,candidates,status='ok'):
      return dict(case=name,kmax=5,coord_bits=bits,status=status,qmin_counts=counts,
        semantic=original,events=[{},dict(generation_passes=2,logical=dict.fromkeys(driver.LOGICAL,7),
                                         work=dict(q4_candidates=candidates,q4_levels=1))])
    agree=[row(bits,9) for bits in (18,21,24)]
    q4=next(c for c in driver.q4_comparisons(agree) if c['case']==name and c['kmax']==5)
    need(q4['status']=='equal','complete q4 witness')
    different=[row(18,9,'timeout'),row(21,9),row(24,10)]
    q4=next(c for c in driver.q4_comparisons(different) if c['case']==name and c['kmax']==5)
    ordinary=next(c for c in driver.comparisons(different) if c['case']==name and c['kmax']==5)
    need(q4['status']=='different' and ordinary['status']=='incomplete' and ordinary['semantic_equal'],'partial q4 difference hidden or changed semantic digest')
    refused=[]
    for mode,value in [('bool',True),('negative',-1)]:
      bad=row(21,9);bad['events'][1]['work']['q4_candidates']=value
      try:driver.q4_signature(bad)
      except ValueError:refused.append(mode)
      else:raise ValueError(mode+' q4 count accepted')
    # The initial audit expectation applied the final reader's u64 bound to the pilot.
    # Preserve that failed probe; the actual native C++ field cannot emit this toy out-of-domain value.
    oversized=row(21,2**64)
    need(driver.q4_signature(oversized)==(2**64,1),'pilot bound asymmetry changed')
    reader_path=ROOT/'reader_copies/morsehgp3D_v11/receipts/catalogue_q4_20261002/check.py'
    spec=importlib.util.spec_from_file_location('q4_reader_bound',reader_path)
    reader=importlib.util.module_from_spec(spec);spec.loader.exec_module(reader)
    try:reader.q4_signature(oversized)
    except reader.old.foundation.Refusal:final_reader_refused_oversized=True
    else:raise ValueError('final reader accepted out-of-u64 q4 count')
    plan=json.loads((BENCH/'plans/q4_levels_g4.json').read_text())
    commands=plan['commands'];need([c['name'] for c in commands]==['matrice','asan18','profiles'],'three-command plan')
    need([c['timeout_seconds'] for c in commands]==[400,100,1100] and '--supplement' in commands[-1]['argv'],'plan budget/prequalification binding')
    need('--qualification' in commands[-1]['argv'] and '../' in commands[-1]['argv'][-1],'sibling command reports')
    legacy=json.loads((BENCH/'plans/catalogue_profiles_g4.json').read_text())
    need('--supplement' not in legacy['commands'][-1]['argv'],'legacy-plan preflight witness changed')
    source=(BENCH/'catalogue_profiles.py').read_text()
    need("'supplement'" in source and 'required=True' in source,'required CLI supplement')
    outputs={}
    for name in ['semantic','collector','reader_selftest']:
      normal=(ROOT/(name+'_normal.stdout')).read_bytes();opt=(ROOT/(name+'_opt.stdout')).read_bytes()
      need(normal==opt,name+' normal/-O differs')
      outputs[name]=normal.decode().strip()
    runs=json.loads((ROOT/'review_runs.json').read_text())
    need(all(r['exit_code']==r['expected_exit_code'] for r in runs),'replay failure')
    need('required: --supplement' in (ROOT/'legacy_plan_parser.stderr').read_text(),'legacy parser verdict')
    print(json.dumps({'verdict':'conforme','source_commit':'ffc2ff95f0ae7296bdc522df81df34c58c3fdf47','native_executions':0,'cloud_actions':0,'arity_counts':counts,'semantic_sha256':original['sha256'],'diagnostics_do_not_change_digest':True,'partial_q4_difference':q4,'partial_semantic_comparison':ordinary,'q4_count_refusals':refused,'out_of_u64_toy_pilot_accepts_final_reader_refuses':final_reader_refused_oversized,'plan_commands':[{'name':c['name'],'timeout_seconds':c['timeout_seconds']} for c in commands],'native_schedule_bound_seconds':driver.NATIVE_BUDGET,'legacy_plan_parser_exit_code':2,'pure_replays':outputs,'qualification':'pending; no closed q4 receipt captured'},indent=2,sort_keys=True))
if __name__=='__main__':review()
