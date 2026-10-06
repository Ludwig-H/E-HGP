"""Model of per-lane phase ordering only; no CUDA execution."""
import argparse,itertools,json
from pathlib import Path

def check(ok,msg):
    if not ok:raise ValueError(msg)

def proof():
    schedules=[]
    for order in itertools.permutations(('read_lane0','reject_lane0','read_lane1')):
        if order.index('read_lane0')>order.index('reject_lane0'):continue
        flag=0;seen={}
        for event in order:
            if event.startswith('read_'):seen[event]=flag
            else:flag=1
        schedules.append(dict(events=list(order), snapshots=seen, final_unresolved=flag))
    mixed=[s for s in schedules if s['snapshots']['read_lane0']!=s['snapshots']['read_lane1']]
    check(len(schedules)==3 and len(mixed)==1,'phase witness')
    fixed=[s for s in schedules if s['events'].index('read_lane1')<s['events'].index('reject_lane0')]
    check(len(fixed)==2 and all(set(s['snapshots'].values())=={0} for s in fixed),'read-completion barrier')
    size=2552+2*32*8+496*2+2*496*4+3*4
    aligned=(size+7)//8*8
    check(aligned==8040,'conditional shared size')
    return dict(scope='Allowed source-level ITS ordering model; no compiled schedule, sanitizer result or false FULL demonstrated',native_runs=0,cuda_runs=0,conditional_abi='u32 size/align4, u64 size/align8, u16 size/align2, struct align8; no sizeof executed',CoopShared_bytes=aligned,unfixed_allowed_schedules=schedules,mixed_guard_witness=mixed[0],snapshot_then_sync_allowed_schedules=fixed,final_unresolved_all_schedules=1,correction='snapshot shared guard into a private bool, then __syncwarp(), then branch on private bool; alternatively atomic read',gate='G4 racecheck/synccheck with injected early unresolved in one pair and delayed peer guard, plus whole-leaf discard identity')

p=argparse.ArgumentParser(description=__doc__);p.add_argument('--check',type=Path);a=p.parse_args()
r=proof()
if a.check:
    check(json.loads(a.check.read_text())==r,'frozen JSON differs')
    print('coop CUDA source model PASS: 3 allowed schedules, 1 mixed guard, 0 after snapshot barrier; ABI conditional 8040; native0 cuda0')
else:print(json.dumps(r,indent=2,ensure_ascii=False,sort_keys=True))
