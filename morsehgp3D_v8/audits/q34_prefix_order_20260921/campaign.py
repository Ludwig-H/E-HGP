#!/usr/bin/env python3
"""Capture and replay a bounded order comparison; all native failures retained."""
import gzip
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import struct
import subprocess
import sys
import time

BASE=Path(__file__).resolve().parent
ROOT=BASE.parents[2]
SPEC=importlib.util.spec_from_file_location('prefix_baseline',BASE.parent/'q34_global_contract_20260921/prefix_model.py')
OLD=importlib.util.module_from_spec(SPEC);SPEC.loader.exec_module(OLD)
require=OLD.require

def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def save(path,value):path.write_text(json.dumps(value,sort_keys=True,indent=2)+'\n')
def load(path):
    raw=path.read_bytes()
    return json.loads(gzip.decompress(raw) if path.suffix=='.gz' else raw)
def execute(cmd):
    start=time.monotonic()
    env=dict(os.environ,PYTHONDONTWRITEBYTECODE='1',ASAN_OPTIONS='detect_leaks=1:halt_on_error=1',UBSAN_OPTIONS='halt_on_error=1')
    try:
        r=subprocess.run(cmd,cwd=ROOT,text=True,capture_output=True,timeout=240,env=env)
        return dict(command=cmd,returncode=r.returncode,stdout=r.stdout,stderr=r.stderr,seconds=time.monotonic()-start)
    except BaseException as error:
        def text(value):return value.decode(errors='replace') if isinstance(value,bytes) else value or ''
        return dict(command=cmd,returncode=None,stdout=text(getattr(error,'stdout','')),stderr=text(getattr(error,'stderr','')),error=repr(error),seconds=time.monotonic()-start)

def inputs():
    folder=BASE/'.inputs';folder.mkdir(exist_ok=True)
    small=[]
    for name,points in list(OLD.fixtures().items())[:6]:
        path=folder/(name+'.u16le');raw=b''.join(struct.pack('<HHH',*p) for p in points)
        require(not path.exists() or path.read_bytes()==raw,'fixture overwrite refused')
        path.write_bytes(raw);small.append(path)
    large=[BASE.parent/'lidar08_20260914/prepared'/f'single_{scan:06}'/f'n{n}.u16le'
           for scan in (0,100,200) for n in (8000,16000,32000)]
    return small,large

def verify(row,path,k,limit):
    raw=path.read_bytes();points=list(struct.iter_unpack('<HHH',raw));n=len(points)
    require(row['status']=='PASS' and row['n']==n and row['K']==k and row['samples_per_stratum']==limit,'command mismatch')
    h=14695981039346656037
    for word in [n,*[x for p in points for x in p]]:
        for byte in int(word).to_bytes(8,'little'):
            h=((h^byte)*1099511628211)&((1<<64)-1)
    require(h==row['input_fnv_words'],'input identity mismatch')
    require(row['front_total_pairs']==n*(n-1)//2,'pair population mismatch')
    require(row['query_count']==len(row['queries']) and row['oracle_point_tests']==n*row['query_count'],'oracle work mismatch')
    require(row['orders']==['global','near','circular','pivotpath'],'order schema mismatch')
    require(row['work_fields']==['geom_nodes','H','Xi','structural_cut','distance_box_tests','positive_blocks','negative_blocks','leaf','peak_stack','order_rank_tests','prepared_start_uses'],'work schema mismatch')
    seen=set();rects=[set(),set()]
    for q in row['queries']:
        key=tuple(q['rect']),tuple(q['ids']),q['q']
        require(key not in seen,'duplicate query');seen.add(key)
        require(q['q'] in (3,4) and q['mask'] & (1 << (q['q']-2)) and 0<=q['oracle_count']<=n,'invalid lane')
        require(q['alive']==(q['oracle_count']<k+2-q['q']),'wrong threshold')
        require(0<=q['pivot_rank']<n,'invalid pivot')
        rects[q['stratum']].add(tuple(q['rect']))
        require(len(q['work'])==4,'missing order')
        for order,w in enumerate(q['work']):
            require(len(w)==11 and all(type(v) is int and v>=0 for v in w),'bad work')
            require(w[0]==w[1] and w[2]<=w[0] and w[7]<=w[0] and w[8]<=49,'bad work identity')
            require(w[10]==(1 if order==2 else 0),'circular start not shared')
        if n<=10:
            a,b=[points[i] for i in q['ids']]
            count=sum(OLD.witness(a,b,z,q['q']) for z in points)
            require(count==q['oracle_count'],'independent Python small oracle differs')
    require([len(r) for r in rects]==row['sampled_rectangles'],'sample inventory differs')
    require(all(t==min(limit,p) for t,p in zip(row['sampled_rectangles'],row['population_rectangles'])),'wrong reservoir cardinality')
    require(row['query_count']>0,'empty query sample')

def run():
    folder=BASE/'capture';require(not folder.exists(),'capture exists');folder.mkdir()
    build=BASE/'.build';build.mkdir(exist_ok=True)
    small,large=inputs()
    common=['-std=c++20','-O2','-Wall','-Wextra','-Wpedantic','-Werror','-pthread','-I',str(ROOT/'morsehgp3D_v8/src'),str(BASE/'order_probe.cpp')]
    libs=[ROOT/'build'/name/'libmhgp8_p0.a' for name in ['v8_q34_indexed_20260921','v8_q34_indexed_sanitize_20260921']]
    manifest=dict(status='running',records=[],input_sha256={str(p.relative_to(ROOT)):sha(p) for p in small+large})
    def append(record):
        path=folder/f'{len(manifest["records"]):04d}.json.gz'
        path.write_bytes(gzip.compress(json.dumps(record,sort_keys=True).encode(),mtime=0))
        manifest['records'].append(dict(path=path.name,sha256=sha(path)));save(folder/'MANIFEST.json',manifest)
        require(record['returncode']==0 and not record['stderr'],'failed native command retained')
    dependency=execute(['g++',*common,'-MM']);append(dependency)
    deps=dependency['stdout'].split(':',1)[1].replace('\\\n',' ').split()
    paths=[Path(p) if Path(p).is_absolute() else ROOT/p for p in deps]+libs+[Path(__file__),BASE/'SNAPSHOT.json',OLD.PREVIOUS] if hasattr(OLD,'PREVIOUS') else [Path(p) if Path(p).is_absolute() else ROOT/p for p in deps]+libs+[Path(__file__),BASE/'SNAPSHOT.json',Path(OLD.__file__)]
    manifest['source_sha256']={str(p.relative_to(ROOT)):sha(p) for p in paths}
    for compiler,lib,name,flags in [('g++',libs[0],'release',[]),('clang++',libs[1],'sanitize',['-fsanitize=address,undefined','-fno-omit-frame-pointer'])]:
        append(execute([compiler,*common,*flags,str(lib),'-o',str(build/name)]))
    manifest['binary_sha256']={str(p.relative_to(ROOT)):sha(p) for p in [build/'release',build/'sanitize']}
    planned=[(p,k,32,kind,'small') for p in small for k in (3,10) for kind in ('release','sanitize')]
    planned += [(p,k,32,'release','lidar') for p in large for k in (5,10)]
    for path,k,limit,kind,regime in planned:
        record=dict(input=str(path.relative_to(ROOT)),K=k,limit=limit,binary=kind,regime=regime,**execute([str(build/kind),str(path),str(k),str(limit)]))
        append(record);verify(json.loads(record['stdout']),path,k,limit)
        print('PASS',regime,path.parent.name,path.name,k,kind,flush=True)
    require(all(sha(ROOT/p)==h for p,h in manifest['source_sha256'].items()),'sources moved')
    require(all(sha(ROOT/p)==h for p,h in manifest['input_sha256'].items()),'inputs moved')
    manifest['status']='completed';save(folder/'MANIFEST.json',manifest)
    save(folder/'COMPLETION.json',dict(status='completed',manifest_sha256=sha(folder/'MANIFEST.json'),records=len(manifest['records'])))
    read()

def read():
    folder=BASE/'capture';m=load(folder/'MANIFEST.json');c=load(folder/'COMPLETION.json')
    require(m['status']==c['status']=='completed' and c['manifest_sha256']==sha(folder/'MANIFEST.json'),'incomplete capture')
    for section in ['source_sha256','input_sha256','binary_sha256']:
        require(all(sha(ROOT/p)==h for p,h in m[section].items()),'closed dependency changed')
    rows=[];small={}
    for entry in m['records']:
        path=folder/entry['path'];require(sha(path)==entry['sha256'],'record changed');r=load(path)
        require(r['returncode']==0 and not r['stderr'],'failed record')
        if 'regime' not in r:continue
        path=ROOT/r['input'];row=json.loads(r['stdout']);verify(row,path,r['K'],r['limit'])
        require(r['command']==[str(BASE/'.build'/r['binary']),str(path),str(r['K']),str(r['limit'])],'record command mismatch')
        if r['regime']=='small':
            key=r['input'],r['K']
            if key in small:require(small[key]==row,'release/sanitizer differs')
            else:small[key]=row
        rows.append(dict(regime=r['regime'],input=r['input'],K=r['K'],n=row['n'],queries=row['query_count']))
    require(len(rows)==42 and len(small)==12 and len(m['records'])==45,'capture inventory differs')
    print(json.dumps(dict(status='passed',records=len(m['records']),calls=len(rows),small_cases=len(small),lidar_calls=sum(r['regime']=='lidar' for r in rows),queries=sum(r['queries'] for r in rows),rows=rows),sort_keys=True))

if __name__=='__main__':
    require(sys.argv[1:] in [['run'],['read']],'usage: campaign.py run|read')
    run() if sys.argv[1]=='run' else read()
