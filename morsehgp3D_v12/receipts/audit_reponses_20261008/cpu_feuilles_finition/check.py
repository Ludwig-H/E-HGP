#!/usr/bin/env python3
"""Source/metadata arithmetic only. Does not rerun or read cloud inputs."""
import argparse
import hashlib
import json
import math
from pathlib import Path
import statistics as s
import subprocess
HERE=Path(__file__).resolve().parent
def need(v,m):
    if not v:raise ValueError(m)
def sha(b):return hashlib.sha256(b).hexdigest()
def main():
    a=argparse.ArgumentParser();a.add_argument('--repo',type=Path,required=True);a.add_argument('--returned-full',type=Path,required=True);args=a.parse_args()
    c=json.loads((HERE/'capture.json').read_bytes())
    def git(pin,p):return subprocess.check_output(['git','show',pin+':'+p],cwd=args.repo)
    def v12(pin,p):return git(pin,'morsehgp3D_v12/'+p)
    for p,h in c['source_pins'].items():need(sha(v12(c['source_m'],p))==h,p)
    for p in c['same_i_m']:need(v12(c['source_i'],p)==v12(c['source_m'],p),'I/M '+p)
    for p in c['same_m_reference']:need(v12(c['source_m'],p)==v12(c['reference_commit'],p),'M/reference '+p)
    for p,h in c['history'].items():need(sha(v12(c['reference_commit'],p))==h,p)
    for p,h in c['v11'].items():
        if p!='commit':need(sha(git(c['v11']['commit'],'morsehgp3D_v11/'+p))==h,'v11 '+p)
    mpath=HERE.parent/'session_m_admission/results.json';need(sha(mpath.read_bytes())==c['m_results_sha256'],'M admission')
    history=json.loads(v12(c['reference_commit'],'receipts/audit_reponses_20261008/session_i_catalogue/mesures.json'))
    previous={};current={};hot={f:[] for f in ('ng00','ng01','ng02')}
    for f in hot:
        a,b=(history['cpu'][f+':K5:L'+str(n)] for n in (16,24))
        previous[f]={'C16_ns':a['median_ns'],'C24_ns':b['median_ns'],'ratio16_over24':a['median_ns']/b['median_ns'],
            'difference_of_stage_medians_16_minus24_ns':{k:a['stages'][k]['median_ns']-b['stages'][k]['median_ns'] for k in a['stages']}}
    for name,h in c['cpu_jsonl_sha256'].items():
        raw=(args.returned_full/'brut'/name).read_bytes();need(sha(raw)==h,name)
        rows=[json.loads(x) for x in raw.splitlines()];full=[r for r in rows if r.get('phase')=='full']
        need(len(full)==5,'five passes');f=name.split('_')[1];hot[f]+=full[1:]
    for f,rows in hot.items():
        need(len(rows)==12,'twelve hot')
        current[f]={'warm_passes':len(rows),'wall_median_ns':s.median(r['wall_ns'] for r in rows),
            'C_median_ns':s.median(r['etapes_ns']['C'] for r in rows),
            'median_C_over_wall':s.median(r['etapes_ns']['C']/r['wall_ns'] for r in rows),
            'c_stage_medians_ns':{k:s.median(r['c_ns'][k] for r in rows) for k in rows[0]['c_ns']},
            'C_if_finish_free_conditional_median_ns':s.median(r['etapes_ns']['C']-r['c_ns']['fin_etage'] for r in rows),
            'wall_if_C_finish_free_conditional_median_ns':s.median(r['wall_ns']-r['c_ns']['fin_etage'] for r in rows)}
    print(json.dumps({'I_cpu_catalogue_one_process_nine_hot_per_cell':previous,'M_cpu_full_three_processes_four_hot_each':current,
        'potential_q2_q3_q4_supports_per_full_leaf':{n:sum(math.comb(n,k) for k in (2,3,4)) for n in (16,24)},
        'new_native_runs':0},sort_keys=True,indent=2))
if __name__=='__main__':main()
