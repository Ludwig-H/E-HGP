#!/usr/bin/env python3
"""Portable pure encoding/wrapper controls; every subprocess is mocked, no native/cloud/build."""
from pathlib import Path
import argparse, hashlib, importlib.util, json, struct, subprocess, sys, tempfile
from unittest.mock import patch
ROOT = Path(__file__).resolve().parent
BENCH = ROOT / 'sources/morsehgp3D_v11/bench'
sys.path.insert(0, str(BENCH))
import catalogue_profiles as driver
import catalogue_semantic as semantic
W = struct.Struct('<Q')
NONE = 2**32-1

def need(value, message):
    if not value: raise ValueError(message)

def fixture(bits=18, factor=1, a=7, ids=(4294967294,17,4000000000,81), mode='base'):
    # Exact regular tetrahedron: squared radii a²/2, 2a²/3, 3a²/4.
    from itertools import combinations
    points=((0,0,0),(a,a,0),(a,0,a),(0,a,a))
    levels=[(0,1),(a*a,2),(2*a*a,3),(3*a*a,4)]
    if mode == 'level': levels[1]=(a*a+1,2)
    balls=[]
    for q in (2,3,4):
        for support in combinations(range(4),q):
            p=0; inner=[]; shell=list(support); rank=q-1; qmin=q
            if mode == 'interior' and support==(0,1): p=1; inner=[2]
            if mode == 'shell' and support==(0,1): shell=[0,1,2]
            if mode == 'qmin' and support==(0,1,2): qmin=2; support=(0,1)
            if mode == 'omit' and support==(0,2): continue
            balls.append((qmin,p,len(shell),rank,tuple(support)+(NONE,)*(4-qmin),inner,shell))
    balls.sort(key=lambda b:(b[3],b[4]))
    out=bytearray(b'MHGP11CAT1')
    def words(*values):
        for v in values: out.extend(W.pack(v))
    words(bits,5,4)
    for xyz, identity in zip(points, ids): words(*xyz,1,identity)
    words(len(levels))
    for num,den in levels:
        for value,budget in zip((num*factor,den*factor),(8*bits+12,6*bits+8)):
            limbs=2 if budget<=127 else (budget+63)//64
            words(0,limbs)
            words(*((value>>(64*i))&(2**64-1) for i in range(limbs)))
    words(len(balls))
    for q,p,m,rank,support,inner,shell in balls: words(q,p,m,rank,*support)
    offset=0;words(offset)
    for q,p,m,rank,support,inner,shell in balls: offset+=p+m;words(offset)
    for q,p,m,rank,support,inner,shell in balls: words(*inner,*shell)
    return bytes(out)

def event_lines(bits=18):
    events=[{'phase':'cloud','points':4,'sites':4,'cloud_ns':20,'read_ns':10},
      {'phase':'catalogue','status':'ok','coord_bits':bits,'kmax':5,'balls':11,'levels':4,'incidences':28,
       'wall_ns':30,'generation_passes':2,'peak_reserved_bytes':100,'reserved_after_bytes':50,
       'logical':dict.fromkeys(sorted(driver.LOGICAL),7)}, {'phase':'exit','status':'ok'}]
    return '\n'.join(json.dumps(e) for e in events).encode()

def review():
    variants=[(bits,factor,fixture(bits,factor)) for bits in (18,21,24) for factor in (1,3)]
    decoded=[semantic.decode(blob,bits,5,4) for bits,factor,blob in variants]
    need(len({x['sha256'] for x in decoded})==1,'limb padding/rational scale not normalized')
    need(len({hashlib.sha256(blob).hexdigest() for bits,factor,blob in variants})==6,'raw controls not distinct')
    baseline=decoded[0]['sha256']; sensitive={}
    for name,kwargs in [('xyz',{'a':8}),('ids',{'ids':(4294967293,17,4000000000,81)}),
       ('levels',{'mode':'level'}),('balls',{'mode':'omit'}),('interior',{'mode':'interior'}),
       ('shell',{'mode':'shell'}),('qmin',{'mode':'qmin'})]:
        value=semantic.decode(fixture(**kwargs),18,5,4)
        need(value['sha256']!=baseline,name+' not represented in semantic digest')
        sensitive[name]=value
    # A structurally valid altered interior is accepted: this is explicitly an encoding check, no geometry proof.
    need(sensitive['interior']['incidences']==29 and sensitive['shell']['incidences']==29,'incidence sensitivity')
    refused=[]
    for name,blob in [('truncated',fixture()[:-1]),('trailing',fixture()+b'x'),('wrong_profile',fixture(21))]:
        try: semantic.decode(blob,18,5,4)
        except ValueError: refused.append(name)
        else: raise ValueError(name+' accepted')
    process=[]
    with tempfile.TemporaryDirectory(prefix='mhgp11_profile_audit_toy_') as folder:
        root=Path(folder);case={'name':'toy','count':4,'coordinates':'xyz','point_ids':'ids'}
        args=argparse.Namespace(work=root,data=root)
        for mode in ('ok','refused','timeout','wrong_bits'):
            def child(argv, **kwargs):
                need(argv[0]==str(root/'never_executed') and kwargs['timeout']==30,'unmocked subprocess')
                if mode=='timeout': raise subprocess.TimeoutExpired(argv,30,output=b'{"phase":"cloud"}',stderr=b'partial')
                if mode=='refused': return subprocess.CompletedProcess(argv,2,b'{"phase":"exit","status":"invalid_input"}',b'')
                Path(argv[3]).write_bytes(fixture())
                return subprocess.CompletedProcess(argv,0,event_lines(21 if mode=='wrong_bits' else 18),b'')
            checkpoints=[]
            with patch.object(driver.subprocess,'run',side_effect=child):
                result=driver.measure(root/'never_executed',case,18,5,args,
                                      lambda row:checkpoints.append(json.loads(json.dumps(row))))
            wanted={'ok':'ok','refused':'refused','timeout':'timeout','wrong_bits':'artifact_error'}[mode]
            need(result['status']==wanted,mode+' verdict')
            need(len(checkpoints)==1 and not (root/'toy_b18_k5.bin').exists(),'checkpoint/cleanup')
            if mode=='ok': need(checkpoints[0]['status']=='pending_semantic' and 'semantic' not in checkpoints[0], 'pre-decoder checkpoint')
            if mode in ('refused','timeout'): need('catalogue_ms' not in result and 'semantic' not in result,'incomplete run given completed timing')
            process.append({'mode':mode,'checkpoint':checkpoints[0],'result':result})
        # Exact file hashes and compiled bits are tied to the recorded green configurations.
        args=argparse.Namespace(builds=root/'builds',qualification=root/'matrix/summary.json')
        args.qualification.parent.mkdir()
        args.qualification.write_text(json.dumps({'conforming':True,'exit_code':0,'complete':True,
           'configurations':[{'name':name,'status':'ok'} for name in driver.PROFILES.values()]}))
        for bits,name in driver.PROFILES.items():
            exe=args.builds/name/'build/mhgp11_catalogue_bench';exe.parent.mkdir(parents=True)
            exe.write_bytes(('NOT EXECUTABLE '+str(bits)).encode())
            cache='MHGP11_COORD_BITS:STRING=%d\n'%bits
            prov=args.qualification.parent/name/'build_provenance.json';prov.parent.mkdir()
            prov.write_text(json.dumps({'schema':'ehgp.v11.build_provenance.v1','complete':True,'errors':[],
               'files':[{'path':exe.name,'size':exe.stat().st_size,'sha256':driver.base.digest(exe)},
                        {'path':'CMakeCache.txt','size':len(cache),'sha256':hashlib.sha256(cache.encode()).hexdigest(),'text':cache}]}))
        checked=driver.checked_builds(args);need(set(checked)=={18,21,24},'positive provenance control')
        exe=Path(checked[21]['path']);exe.write_bytes(b'CHANGED')
        try:driver.checked_builds(args)
        except ValueError: provenance_rejected=True
        else: raise ValueError('changed binary accepted')
    plan=json.loads((BENCH/'plans/catalogue_profiles_g4.json').read_text())
    matrix=json.loads((ROOT/'sources/morsehgp3D_v11/tools/g4_matrix.json').read_text())
    options={c['name']:c['cmake_options'] for c in matrix['configurations']}
    need(all('-DMHGP11_COORD_BITS=%d'%bits in options[name] for bits,name in driver.PROFILES.items()),'explicit bits disagree')
    need(sum(c['timeout_seconds'] for c in plan['commands'])==1600,'plan deadlines changed')
    out={'verdict':'conforme','scope':'autonomous toy encoding/wrapper; no native/build/cloud',
         'source_commit':'9df77494732b03ddf11dbcf1dcb11d96bef54a3b','native_executions':0,
         'encoding_variants':6,'normalized_sha256':baseline,'sensitivity_controls':sensitive,
         'refusals':refused,'process_controls':process,'changed_binary_rejected':provenance_rejected,
         'matrix_explicit_profiles':driver.PROFILES,'plan_command_deadlines_seconds':[c['timeout_seconds'] for c in plan['commands']],
         'native_schedule_bound_seconds':driver.NATIVE_BUDGET}
    print(json.dumps(out,indent=2,sort_keys=True))
if __name__=='__main__':review()
