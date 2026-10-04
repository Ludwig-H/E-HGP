#!/usr/bin/env python3
"""Scalar model of stable size order and original-ordinal two-pass outputs; not CUDA."""
import hashlib
import json
from pathlib import Path
import struct

ROOT=Path(__file__).resolve().parent
CHECKS=0

def need(value, why):
    global CHECKS
    CHECKS+=1
    if not value:
        raise ValueError(why)


def check_sources():
    before=json.loads((ROOT/'BEFORE.json').read_text())
    contents={}
    for row in before['sources']:
        raw=(ROOT/row['copy']).read_bytes()
        need(hashlib.sha256(raw).hexdigest()==row['sha256'], 'source hash '+row['path'])
        contents[Path(row['path']).name]=raw.decode()
    cuda=contents['leaf_batch_cuda.cu']
    need(cuda.count('const u64 j = v.order[thread];')==2, 'both kernels map thread to original j')
    for fragment in ['status[j] = static_cast<u8>(s);','balls[j] = s == leaf_device::kOk ? sink.balls : 0;',
                     'incidences[j] = s == leaf_device::kOk ? sink.incidences : 0;',
                     'FillSink sink{records, population, record_begin[j], population_begin[j]};',
                     'record_begin[j + 1]', 'population_begin[j + 1]',
                     'if (thread >= v.count) return;',
                     'order[start[leaf_device::kMaxSites - view.jobs[j].m]++] = static_cast<u32>(j);',
                     'view.count > (u64{1} << 32)', 'if (view.count != 0) count_kernel']:
        need(fragment in cuda, 'ordinal/padding/limit source anchor '+fragment)
    for name in ['leaf_batch_cuda.cu','leaf_batch.cpp']:
        need('view.count > view.cloud_sites' not in contents[name], 'false cardinality guard removed')
        need('view.count > leaf_device::kMaxBatchJobs' in contents[name], 'explicit resource guard')
    need('kMaxBatchJobs = u64{1} << 40' in contents['leaf_device_predicates.hpp'], 'common count cap')


def size_order(ms):
    start=[0]*34
    for m in ms:
        need(1<=m<=32, 'certified view domain')
        start[32-m+1]+=1
    for b in range(1,34):
        start[b]+=start[b-1]
    order=[None]*len(ms)
    for j,m in enumerate(ms):
        at=start[32-m]
        need(0<=at<len(ms), 'bucket insertion in bounds')
        order[at]=j
        start[32-m]+=1
    return order


def leaf_payload(j,m):
    # Synthetic deterministic sinks, not geometry: unresolved sheets have no committed records/counts.
    status=1 if j%7==3 else 0
    records=[(j,r,1+(m+r)%4) for r in range((j+m)%4)] if status==0 else []
    population=[j*64+p for p in range(sum(rec[2] for rec in records))]
    counters=[(j+1)*(f+1)+(m%5) for f in range(15)]
    return status,records,population,counters


def model(ms, order):
    n=len(ms)
    status=[None]*n
    balls=[None]*n
    incidences=[None]*n
    block_totals=[]
    visits=[]
    padded=0
    for block in range((n+127)//128):
        counts=[0]*15
        for lane in range(128):
            thread=block*128+lane
            if thread>=n:
                padded+=1
                continue
            j=order[thread]
            need(0<=j<n, 'device order in bounds')
            visits.append(j)
            s,rs,pop,local=leaf_payload(j,ms[j])
            status[j]=s
            balls[j]=len(rs) if s==0 else 0
            incidences[j]=len(pop) if s==0 else 0
            if s==0:
                counts=[x+y for x,y in zip(counts,local)]
        block_totals.append(counts)
    need(sorted(visits)==list(range(n)), 'one count invocation per original ordinal')
    totals=[sum(row[f] for row in block_totals) for f in range(15)]
    record_begin=[]
    population_begin=[]
    nr=np=0
    for j in range(n):
        record_begin.append(nr)
        population_begin.append(np)
        nr+=balls[j]
        np+=incidences[j]
    records=[None]*nr
    population=[None]*np
    writes_r=[]
    writes_p=[]
    for thread in range(((n+127)//128)*128):
        if thread>=n:
            continue
        j=order[thread]
        if status[j]!=0:
            continue
        s,rs,pop,_=leaf_payload(j,ms[j])
        at=record_begin[j]
        pat=population_begin[j]
        for original,r,cardinal in rs:
            records[at]=(original,r,pat,cardinal)
            writes_r.append(at)
            at+=1
            pat+=cardinal
        for i,value in enumerate(pop):
            population[population_begin[j]+i]=value
            writes_p.append(population_begin[j]+i)
        need(at==(record_begin[j+1] if j+1<n else nr), 'record end checked using original next j')
        need(pat==(population_begin[j+1] if j+1<n else np), 'population end checked using original next j')
    need(sorted(writes_r)==list(range(nr)), 'record positions exactly once/no overlap')
    need(sorted(writes_p)==list(range(np)), 'population positions exactly once/no overlap')
    encoded=b''.join(struct.pack('<4Q',*record) for record in records)+b''.join(struct.pack('<Q',x) for x in population)
    return dict(status=status,records=records,population=population,totals=totals,
                padding=padded,sha256=hashlib.sha256(encoded).hexdigest())


def main():
    check_sources()
    cases=[[],[1],[32],[16,32,1,32,16,1],list(range(1,33)),list(range(32,0,-1)),
           [1,32]*40]+[[1+(j*17)%32 for j in range(n)] for n in [31,32,33,127,128,129,257]]
    observed=[]
    for ms in cases:
        order=size_order(ms)
        need(order==sorted(range(len(ms)),key=lambda j:-ms[j]), 'stable complete descending permutation')
        ordered=model(ms,order)
        serial=model(ms,list(range(len(ms))))
        need(ordered==serial, 'all original-ordinal outputs and resolved-only ledger equal')
        observed.append(dict(jobs=len(ms),padded=ordered['padding'],records=len(ordered['records']),
                             population=len(ordered['population']),sha256=ordered['sha256']))
    ms=[32]*16+[16]*32
    order=size_order(ms)
    need(len({ms[j] for j in order[:32]})==2, 'bucket boundary may share a warp; no semantics assumed')
    bound=32*3*(32+496+4960+35960)
    need(bound==3979008 and bound<2**22, 'per-leaf bound')
    need((2**40)*bound<2**62, 'common exact u64 reduction bound')
    need((2**32)*bound<2**54, 'CUDA exact u64 reduction bound')
    need(80<=2**40 and 80<=2**32 and 80>9, 'cube9/80 regression guard removed in both backends')
    capacities=[]
    for count in [0,1,80,2**32-1,2**32,2**32+1,2**40,2**40+1]:
        host=count<=2**40
        cuda=host and count<=2**32
        if cuda:
            need(count==0 or count-1<=2**32-1, 'last mapped original ordinal fits u32')
            grid=(count+127)//128
            need(grid<=2**32-1, 'grid cast fits unsigned (not device limit qualification)')
            need(count*4<2**64, 'host/device order bytes safe after CUDA cap')
        capacities.append(dict(count=count,host_cardinality_admitted=host,cuda_cardinality_admitted=cuda))
    print(json.dumps(dict(schema='ehgp.audit.gpu_order008.v1',verdict='pass',checks=CHECKS,
                         cases=observed,capacities=capacities,native_executed=False,
                         scope='scalar scheduling and sink model, geometry bound to unchanged77; not CUDA execution'),
                     sort_keys=True,indent=2))

if __name__=='__main__':
    main()
