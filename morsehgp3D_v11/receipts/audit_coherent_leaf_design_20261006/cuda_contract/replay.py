"""Ideal register/atomic-bit design models; no device execution."""
import argparse,json
from pathlib import Path

def check(v,m):
    if not v:raise ValueError(m)

def proof():
    cases=[]
    for leader_position in range(32):
        lanes=list(range(1,32));lanes.insert(leader_position,0)
        bit=0;leader_hits=0;physical_hits=0
        for visit in range(2):
            for lane in lanes:
                hit=bit!=0;physical_hits+=int(hit)
                if lane==0:leader_hits+=int(hit)
                bit=1
        cases.append(dict(leader_position=leader_position,logical_tests=2,leader_hits=leader_hits,leader_evaluations=2-leader_hits,physical_hits=physical_hits))
    check(cases[0]['leader_hits']==1 and all(c['leader_hits']==2 for c in cases[1:]),'redundant-cache counter witness')
    check(all(c['physical_hits']==63 for c in cases),'sum of redundant operations')
    bit=0;hits=0
    for unused in range(2):hits+=int(bit!=0);bit=1
    check(hits==1,'one leader one logical visit')
    masks=[]
    for m in (1,2,3,31,32):
        valid=(1<<m)-1
        shell=sum(1<<i for i in (1,3,31) if i<m)
        compact=[dict(lane=i,slot=(shell&((1<<i)-1)).bit_count()) for i in range(m) if shell>>i&1]
        check([x['slot'] for x in compact]==list(range(shell.bit_count())),'ordered compaction')
        masks.append(dict(m=m,valid_sites_mask=valid,participants=0xffffffff,shell=shell,compact=compact))
    return dict(scope='Q3 bounded ideal model, no scheduler, native or CUDA execution',native_runs=0,cuda_runs=0,redundant_atomic_cache=cases,expected_logical=dict(tests=2,hits=1,evaluations=1),physical_reduction_wrong=dict(tests=64,hits=63,evaluations=1),correction='one lane test-and-set/accounting per logical face, broadcast result; count logical visits once',ballot_compaction=masks)

p=argparse.ArgumentParser(description=__doc__);p.add_argument('--check',type=Path);a=p.parse_args();r=proof()
if a.check:
    check(json.loads(a.check.read_text())==r,'frozen JSON differs')
    print('Q3 design model PASS: 32 cache schedules, 31 wrong leader counters for redundant calls; 5 ordered full-mask compactions; native0 CUDA0')
else:print(json.dumps(r,indent=2,sort_keys=True))
