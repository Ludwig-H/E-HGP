#!/usr/bin/env python3
"""Aggregate the fixed node-block replay by budget and success status."""
from collections import defaultdict
from hashlib import sha256
import json
from pathlib import Path

HERE=Path(__file__).resolve().parent
SOURCE=HERE/'RESULT.stdout'
OLD=HERE.parent/'paired_guards_precore_20260923'
OUT=HERE/'SUMMARY.json'


def rows():
    for line in SOURCE.read_text().splitlines():
        w=line.split()
        if w[0]!='ROW': continue
        (cap,budget,seed,a,b,F,mask,oracle,c3,c4,closed,visits,boxes,
         rejects,endpoints,candidates,pairs,pos3,pos4,exhausted,ns)=map(int,w[1:])
        yield dict(cap=cap,budget=budget,seed=seed,a=a,b=b,F=F,mask=mask,
                   oracle=oracle,c3=c3,c4=c4,closed=closed,visits=visits,
                   boxes=boxes,rejects=rejects,endpoints=endpoints,
                   candidates=candidates,pairs=pairs,pos3=pos3,pos4=pos4,
                   exhausted=exhausted,ns=ns)


def aggregate(items):
    fields=('F','visits','boxes','rejects','endpoints','candidates','pairs',
            'pos3','pos4','exhausted','ns')
    out={k:sum(row[k] for row in items) for k in fields}
    out.update(edges=len(items),closed=sum(row['closed'] for row in items),
               F_closed=sum(row['F'] for row in items if row['closed']),
               pair_fail3=out['pairs']-out['pos3'],
               pair_fail4=out['pairs']-out['pos4'])
    return out


def main():
    data=list(rows())
    assert len(data)==1920
    samples={}
    for name in ('RESULT.json','RESULT_SEED2.json'):
        old=json.loads((OLD/name).read_text())
        for row in old['rows']:
            samples[old['sample_seed'],row['a'],row['b']]=row
    assert len(samples)==120
    out={'schema':'mhgp9_audit_node_block_summary_v1',
         'result_sha256':sha256(SOURCE.read_bytes()).hexdigest(),
         'sample_F':sum(row['F'] for row in samples.values()),
         'results':{}}
    for cap in (1,2,4,8):
        for budget in (64,256):
            part=[r for r in data if (r['cap'],r['budget'])==(cap,budget)]
            assert len(part)==120
            open_=[r for r in part if not r['closed']]
            closed=[r for r in part if r['closed']]
            gained=[r for r in part if r['closed'] and not r['oracle']]
            lost=[r for r in part if r['oracle'] and not r['closed']]
            result={'all':aggregate(part),'closed':aggregate(closed),
                    'open':aggregate(open_),
                    'oracle_gained':len(gained),'oracle_lost':len(lost),
                    'oracle_gained_F':sum(r['F'] for r in gained),
                    'oracle_lost_F':sum(r['F'] for r in lost),
                    'by_seed':{},'cross_sector':{}}
            for seed in (230923,230924):
                result['by_seed'][str(seed)]=aggregate([r for r in part if r['seed']==seed])
            for label,predicate in (
                    ('same_physical_quarter',lambda old:old['sector_a']==old['sector_b']),
                    ('different_physical_quarters',lambda old:old['sector_a']!=old['sector_b'])):
                result['cross_sector'][label]=aggregate([r for r in part
                    if predicate(samples[r['seed'],r['a'],r['b']])])
            out['results'][f'cap{cap}_budget{budget}']=result
    # Larger budgets finish all sixteen candidates in each quadrant; their
    # work and decisions must stay identical to 256, regardless of timings.
    by={(r['cap'],r['budget'],r['seed'],r['a'],r['b']):r for r in data}
    for cap in (1,2,4,8):
        for budget in (1024,4096):
            for row in (r for r in data if r['cap']==cap and r['budget']==budget):
                ref=by[cap,256,row['seed'],row['a'],row['b']]
                assert {k:v for k,v in row.items() if k not in ('budget','ns')} == {
                    k:v for k,v in ref.items() if k not in ('budget','ns')}
    OUT.write_text(json.dumps(out,indent=2,sort_keys=True)+'\n')
    print(json.dumps({k:{'closed':v['all']['closed'],'F_closed':v['all']['F_closed'],
                         'visits':v['all']['visits'],'pairs':v['all']['pairs']}
                      for k,v in out['results'].items()},sort_keys=True))


if __name__=='__main__': main()
