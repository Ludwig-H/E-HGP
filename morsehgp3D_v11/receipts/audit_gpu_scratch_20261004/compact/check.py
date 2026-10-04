#!/usr/bin/env python3
"""Independent scalar compact-record roundtrip/scratch model; no native execution."""
from fractions import Fraction
import hashlib
import itertools
import json
from pathlib import Path
import re
import struct

ROOT=Path(__file__).resolve().parent
CHECKS=0
NO=2**32-1
NOL=255

def need(value, why):
    global CHECKS
    CHECKS+=1
    if not value: raise ValueError(why)


def source_pins():
    sources={}
    for row in json.loads((ROOT/'BEFORE.json').read_text())['sources']:
        raw=(ROOT/row['copy']).read_bytes()
        need(hashlib.sha256(raw).hexdigest()==row['sha256'], 'pinned source '+row['path'])
        sources[Path(row['copy']).name]=raw.decode()
    header=sources['leaf_batch.hpp']
    need('u8 support[4];' in header and 'u8 p, m, qmin, pad;' in header, '8-byte fields')
    need('if (sites[i] == site) return static_cast<u8>(i);' in header and 'return kNoLocal;' in header,
         'local rank lookup and sentinel')
    for anchor in ['population[population_at++] = local_rank(sites, m, interior[i]);',
                   'population[population_at++] = local_rank(sites, m, shell[i]);',
                   'fits && balls < kScratchRecords && incidences + need <= kScratchPopulation']:
        need(anchor in header, 'encoder/scratch source anchor '+anchor)
    decoder=sources['single_pass_batch.cpp']
    for anchor in ['const u32* local = view.sites + job.begin;', 'r.support[k] != kNoLocal',
                   'r.support[k] >= job.m', 'rank >= job.m', 'if (n > p1 - at)',
                   'return at == p1 ? Outcome{}', 'result.record_begin[j + 1]',
                   'r.qmin == 2 ? num::Sphere::through', 'r.qmin == 3 ? num::Sphere::through']:
        need(anchor in decoder, 'decoder source anchor '+anchor)
    need('if (support[i] != generated[i]) return;' in sources['leaf_device.hpp'], 'presentation arity retained')
    need('if (count != q && !canonical(s, count, support, qmin)) return;' in sources['leaf_device.hpp'],
         'canonical support determined before encoding')
    need('stored[j] = resolved && sink.fits;' in sources['leaf_batch.cpp'], 'partial/unresolved scratch discarded')
    need('if (status[j] != leaf_device::kOk) return {};' in sources['leaf_batch.cpp'], 'no unresolved outputs committed')
    need('plain.batch_leaves = plain.cuda_leaves = false;' in decoder, 'whole leaf CPU fallback without requeue')
    records,population=re.search(r'kScratchRecords = (\d+), kScratchPopulation = (\d+)',header).groups()
    return int(records),int(population),sources


def local_rank(sites, site):
    return sites.index(site) if site in sites else NOL


def encode(sites, support, interior, shell):
    padded=list(support)+[NO]*(4-len(support))
    record=bytes([local_rank(sites,v) for v in padded]+[len(interior),len(shell),len(support),0])
    pop=bytes(local_rank(sites,v) for v in list(interior)+list(shell))
    need(len(record)==8, 'record byte length')
    return record,pop


def decode(sites, record, pop, cloud_sites=NO):
    q=record[6]
    if q<2 or q>4: raise LookupError('arity')
    support=[]
    for i in range(4):
        rank=record[i]
        if i>=q:
            if rank!=NOL: raise LookupError('padding')
            support.append(NO)
        else:
            if rank>=len(sites) or sites[rank]>=cloud_sites: raise LookupError('support rank')
            support.append(sites[rank])
    n=record[4]+record[5]
    if n!=len(pop): raise LookupError('population span')
    if any(rank>=len(sites) for rank in pop): raise LookupError('population rank')
    p=record[4]
    return tuple(support),tuple(sites[rank] for rank in pop[:p]),tuple(sites[rank] for rank in pop[p:]),q


def solve(matrix,rhs):
    rows=[[Fraction(v) for v in row]+[Fraction(b)] for row,b in zip(matrix,rhs)]
    n=len(rows)
    for j in range(n):
        at=next(i for i in range(j,n) if rows[i][j])
        rows[j],rows[at]=rows[at],rows[j]
        div=rows[j][j]
        rows[j]=[v/div for v in rows[j]]
        for i in range(n):
            if i!=j:
                fac=rows[i][j]
                rows[i]=[v-fac*w for v,w in zip(rows[i],rows[j])]
    return [row[-1] for row in rows]


def dot(a,b): return sum(x*y for x,y in zip(a,b))

def sphere(points):
    a=points[0]
    diffs=[tuple(y-x for x,y in zip(a,b)) for b in points[1:]]
    weights=solve([[dot(u,v) for v in diffs] for u in diffs],[Fraction(dot(u,u),2) for u in diffs])
    center=tuple(Fraction(a[j])+sum(w*u[j] for w,u in zip(weights,diffs)) for j in range(3))
    beta=sum((c-x)**2 for c,x in zip(center,a))
    return center,beta


def geometry_fixtures():
    lattice=sorted(p for p in itertools.product(range(-5,6),repeat=3) if dot(p,p)==27)
    need(len(lattice)==32, 'complete integer cospherical shell32')
    shell=[tuple(x+5 for x in p) for p in lattice]
    # All sites here have exact nonnegative small coordinates, not native Cloud construction.
    triangle=[(100+i,i,0) for i in range(32)]
    for j,p in {0:(0,0,0),16:(2,2,0),31:(2,0,2),1:(1,1,1)}.items(): triangle[j]=p
    tetra=[(100+i,i,0) for i in range(32)]
    for j,p in {0:(0,0,0),10:(2,2,0),20:(2,0,2),31:(0,2,2),1:(1,1,1)}.items(): tetra[j]=p
    cases=[('shell32',shell,[0,31]),('acute3',triangle,[0,16,31]),('strict4',tetra,[0,10,20,31])]
    output=[]
    for name,xyz,support_local in cases:
        ids=[1000+67*j for j in range(31)]+[NO-1]
        center,beta=sphere([xyz[j] for j in support_local])
        inside=[];on=[]
        for j,p in enumerate(xyz):
            power=sum((x-c)**2 for x,c in zip(p,center))-beta
            if power<0: inside.append(ids[j])
            if power==0: on.append(ids[j])
        support=[ids[j] for j in support_local]
        record,pop=encode(ids,support,inside,on)
        decoded=decode(ids,record,pop)
        need(decoded==(tuple(support+[NO]*(4-len(support))),tuple(inside),tuple(on),len(support)),
             'exact support/I/U/global SiteIdx roundtrip '+name)
        need(ids[31]==NO-1 and 31 in record[:len(support)], 'rank31/high global SiteIdx valid')
        need(all(record[i]==NOL for i in range(len(support),4)), 'padding sentinel preserved')
        need(sphere([xyz[ids.index(v)] for v in decoded[0][:decoded[3]]])==(center,beta),
             'same decoded support gives exact same center/level')
        need(len(inside)+len(on)<=32, 'byte fields bounded by distinct leaf sites')
        # Half-open owner membership is unchanged because the exact center is unchanged.
        need(all(0<=c<12 for c in center), 'lower included owner box')
        output.append(dict(name=name,qmin=len(support),p=len(inside),shell=len(on),
                           beta=str(beta),record_hex=record.hex(),population_bytes=len(pop)))
    need(output[0]['shell']==32 and output[0]['p']==0, 'all32 shell contacts retained')
    need(output[1]['qmin']==3 and output[1]['p']==1 and output[1]['shell']==3, 'acute triangle fixture')
    need(output[2]['qmin']==4 and output[2]['p']==1 and output[2]['shell']==4, 'strict tetra fixture')
    # Guarded bad compact values: causal decoder branches, not malformed inputs published by producer.
    good,pop=encode(list(range(100,132)),[100,131],[],[100,131])
    malformed=[]
    for label,raw,seq in [('support32',bytes([32])+good[1:],pop),('supportFF',bytes([255])+good[1:],pop),
                          ('badpad',good[:2]+bytes([0])+good[3:],pop),
                          ('qmin1',good[:6]+bytes([1])+good[7:],pop),
                          ('qmin5',good[:6]+bytes([5])+good[7:],pop),
                          ('population32',good,bytes([0,32])),('populationFF',good,bytes([0,255])),
                          ('population_short',good,pop[:1]),('population_long',good,pop+bytes([0]))]:
        try: decode(list(range(100,132)),raw,seq)
        except LookupError: malformed.append(label)
        else: raise ValueError('malformed decoder accepted '+label)
        need(True,'malformed rejected '+label)
    return output,malformed



def representation_sweep():
    cases=0
    for m in range(2,33):
        sites=[1000+129*i for i in range(m-1)]+[NO-1]
        for q in range(2,min(4,m)+1):
            for p in range(min(13-q,m-q)+1):
                inside=sites[:p]
                shell=sites[p:]
                support=shell[:q-1]+[shell[-1]]
                record,pop=encode(sites,support,inside,shell)
                need(decode(sites,record,pop)==(tuple(support+[NO]*(4-q)),tuple(inside),tuple(shell),q),
                     'all admissible typed counts and qmin/localrank roundtrip')
                need(all(v<32 for v in pop) and len(pop)==m and record[4]+record[5]==m,
                     'all incidences representable and populations neither collapsed nor truncated')
                need(tuple(record[:q])!=tuple(support),'causal rejection of interpreting local bytes as global IDs')
                cases+=1
    return cases


def multi_leaf_prefix():
    local_lists=[list(range(1000,1032)),list(range(3000,3002)),list(range(8000,8032))]
    statuses=[0,1,0]
    streams=[]
    for local,status in zip(local_lists,statuses):
        if status!=0:
            streams.append([])
        else:
            pair=encode(local,[local[0],local[-1]],[],[local[0],local[-1]])
            full=encode(local,[local[0],local[-1]],[],local)
            streams.append([pair,full])
    jobs=[];gathered=[]
    for local in local_lists:
        jobs.append((len(gathered),len(local)))
        gathered.extend(local)
    rb=[];pb=[];records=[];population=bytearray()
    for stream in streams:
        rb.append(len(records));pb.append(len(population))
        for record,pop in stream:
            records.append(record);population.extend(pop)
    decoded=[None]*len(records)
    for j in [2,1,0]:
        if statuses[j]!=0: continue
        r1=rb[j+1] if j+1<len(jobs) else len(records)
        p1=pb[j+1] if j+1<len(jobs) else len(population)
        begin,m=jobs[j]
        local=gathered[begin:begin+m]
        at=pb[j]
        for i in range(rb[j],r1):
            record=records[i];n=record[4]+record[5]
            need(n<=p1-at,'each compact population fits original leaf span')
            decoded[i]=decode(local,record,population[at:at+n])+(at,)
            at+=n
        need(at==p1,'prefix population exhausted exactly including multiple records')
    expected=[]
    offset=0
    for local,stream in zip(local_lists,streams):
        for record,pop in stream:
            expected.append(decode(local,record,pop)+(offset,))
            offset+=len(pop)
    need(decoded==expected,'gathered job.begin plus original prefixes survives reverse work order and unresolved gap')
    return dict(jobs=len(jobs),records=len(records),population=len(population),execution_order=[2,1,0])


def scratch_case(records,pops,R,P):
    count=inc=0
    fits=True
    stored_r=[];stored_p=bytearray()
    for record,pop in zip(records,pops):
        if fits and count<R and inc+len(pop)<=P:
            stored_r.append(record);stored_p.extend(pop)
        else: fits=False
        count+=1;inc+=len(pop)
    full=b''.join(records),b''.join(pops)
    copied=(b''.join(stored_r),bytes(stored_p)) if fits else full
    need(copied==full,'copy fits OR full replay; never use partial scratch')
    need(count==len(records) and inc==len(full[1]),'count continues after capacity exceeded')
    return dict(records=count,population=inc,fits=fits,replayed=not fits)


def scratch_guards(R,P):
    ids=list(range(32))
    pair,small=encode(ids,[0,31],[],[0,31])
    shell,wide=encode(ids,[0,31],[],ids)
    one31,pop31=encode(ids,[0,31],[],ids[:30]+[31])
    output=[]
    for records,pops in [([],[]),([pair]*R,[small]*R),([pair]*(R+1),[small]*(R+1)),
                          ([shell]*32,[wide]*32),([shell]*32+[pair],[wide]*32+[small]),
                          ([shell]*31+[one31,pair],[wide]*31+[pop31,small])]:
        output.append(scratch_case(records,pops,R,P))
    need(output[1]['fits'] and not output[2]['fits'],'records128/129 boundary')
    need(output[3]['population']==1024 and output[3]['fits'],'population1024 inclusive')
    need(output[5]['population']==1025 and not output[5]['fits'],'population1025 rejected from cache only')
    # Concatenate two leaves with identical local rank values but distinct global sites.
    begin=0
    for local in [list(range(100,132)),list(range(2000,2032))]:
        record,pop=encode(local,[local[0],local[31]],[],[local[0],local[31]])
        support,_,u,_=decode(local,record,pop)
        need(support[:2]==tuple(u) and u==(local[0],local[31]),'decode using own leaf job.begin context')
        begin+=len(pop)
    # Causal nonvacuity countermodel: one replayed emitting leaf + one empty leaf.
    replayed=output[2]
    jobs=2;fill_jobs=1;stored_nonempty=0
    need(0<fill_jobs<jobs and stored_nonempty==0 and replayed['records']>0,
         'existing gate does not establish a nonempty copied branch')
    return output,dict(jobs=jobs,fill_jobs=fill_jobs,stored_nonempty=stored_nonempty,
                       existing_assertion_passes=True,actual_fixture_failure_established=False)


def main():
    R,P,sources=source_pins()
    need((R,P)==(128,1024),'published scratch constants')
    need(struct.calcsize('8B')==8,'byte encoding width')
    fixtures,malformed=geometry_fixtures()
    sweep=representation_sweep()
    prefixes=multi_leaf_prefix()
    scratch,gap=scratch_guards(R,P)
    # Read-only relevance check of native gate, not its execution or inherited campaign.
    need("need(0 < last['fill_jobs'] < last['jobs']" in sources['full_leaf_lanes.py'],'actual native gate assertion')
    print(json.dumps(dict(schema='ehgp.audit.compact_record.v1',verdict='pass',checks=CHECKS,
                         fixtures=fixtures,representation_cases=sweep,multi_leaf_prefix=prefixes,malformed_rejected=malformed,scratch=scratch,gate_countermodel=gap,
                         native=False,scope='independent Fraction geometry plus scalar packing/decoding model; not C++/CUDA'),
                     sort_keys=True,indent=2))

if __name__=='__main__': main()
