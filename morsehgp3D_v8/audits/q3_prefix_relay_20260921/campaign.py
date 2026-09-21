#!/usr/bin/env python3
"""Qualify the q3 suffix handoff, not a shared-centre filter or a speed claim."""
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


def require(value,message):
    if not value: raise RuntimeError(message)


def module(name,path):
    spec=importlib.util.spec_from_file_location(name,path)
    result=importlib.util.module_from_spec(spec); spec.loader.exec_module(result)
    return result


ORACLE=module('_relay_oracle',BASE.parent/'q34_global_contract_20260921/oracle.py')


def sha(path): return hashlib.sha256(path.read_bytes()).hexdigest()


def write(path,value): path.write_text(json.dumps(value,sort_keys=True,indent=2)+'\n')


def execute(command):
    start=time.monotonic()
    env=dict(os.environ,PYTHONDONTWRITEBYTECODE='1',ASAN_OPTIONS='detect_leaks=1:halt_on_error=1',
             UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1')
    try:
        p=subprocess.run(command,capture_output=True,text=True,timeout=180,env=env)
        return dict(command=command,returncode=p.returncode,stdout=p.stdout,stderr=p.stderr,wall_seconds=time.monotonic()-start)
    except BaseException as error:
        def dec(value): return value.decode(errors='replace') if isinstance(value,bytes) else (value or '')
        return dict(command=command,returncode=None,error=repr(error),stdout=dec(getattr(error,'stdout','')),
                    stderr=dec(getattr(error,'stderr','')),wall_seconds=time.monotonic()-start)


def flags():
    return ['-std=c++20','-Wall','-Wextra','-Wpedantic','-Werror',
            '-I',str(BASE/'snapshot/include'),'-I',str(BASE/'snapshot/include/lanes')]


def libraries():
    return [ROOT/'build'/p/'libmhgp8_p0.a' for p in ('v8_q34_indexed_20260921','v8_q34_indexed_sanitize_20260921')]


def pins():
    paths={BASE/n for n in ('probe.cpp','campaign.py','forest_model.py')}
    for compiler in ('g++','clang++'):
        p=subprocess.run([compiler,*flags(),'-MM',str(BASE/'probe.cpp')],capture_output=True,text=True,check=True)
        deps={Path(x).resolve() for x in p.stdout.replace('\\\n',' ').split(':',1)[1].split()}
        require(all(str(x).startswith(str(BASE)) for x in deps),'Live product compile dependency')
        paths.update(deps)
    paths.update(libraries())
    paths.update(BASE.parent/p for p in ('q34_global_contract_20260921/oracle.py',
        'q4_center_blocks_20260920/oracle_gate.py','q4_center_blocks_20260920/fixtures.py'))
    return {str(p.relative_to(ROOT)):sha(p) for p in sorted(paths)}


def append(folder,m,record):
    with gzip.open(folder/'COMMANDS.jsonl.gz','ab') as stream:
        stream.write((json.dumps(record,sort_keys=True)+'\n').encode())
    m['commands']+=1; write(folder/'MANIFEST.json',m)
    if record['returncode']!=0 or record['stderr']:
        fail=folder/'failed_sources'; fail.mkdir(exist_ok=True)
        for name in ('probe.cpp','campaign.py','forest_model.py'):
            (fail/name).write_bytes((BASE/name).read_bytes())
        raise RuntimeError('Command failed; original outputs preserved')


def save(name,points):
    (BASE/'.inputs').mkdir(exist_ok=True)
    path=BASE/'.inputs'/(name+'.u16le'); raw=b''.join(struct.pack('<HHH',*p) for p in points)
    require(not path.exists() or path.read_bytes()==raw,'Input overwrite refused')
    path.write_bytes(raw); return path


def cases():
    for name,points in ORACLE.fixtures():
        if len(points)>=3:
            yield name,points
    yield 'prefix_credit',[(20,20,20),(40,20,20),(30,32,20),(30,31,20),(30,37,20)]


def check(data,points,a,b,k,limit,counts):
    require(data['status'] in ('passed','completed','PASS'),'Probe not successful')
    require((data['n'],data['K'],data['edge'],data['sample_limit'])==(len(points),k,[a,b],limit),'Command/report mismatch')
    order=data['spatial_order']; require(sorted(order)==list(range(len(points))),'Invalid global permutation')
    cuts={x['name']:x for x in data['cuts']}
    require(set(cuts)=={'root','EOF','q25','q50','q75'},'Unexpected seams')
    eligible=[]
    for x,p in enumerate(points):
        if x not in (a,b) and ORACLE.acute(points[a],points[b],p) and ORACLE.owner(points,tuple(sorted((a,b,x))))==(a,b):
            eligible.append(x)
    actual=[r['x'] for r in data['records']]
    require(len(set(actual))==len(actual) and set(actual)<=set(eligible),'Invalid/duplicate seed')
    if limit==0:require(set(actual)==set(eligible),'Missing seed in uncapped small gate')
    else:require(len(actual)==min(limit,len(eligible)),'Sample count mismatch')
    threshold=k-1
    for r in data['records']:
        x=r['x']; ids=tuple(sorted((a,b,x))); value=ORACLE.ball3([points[i] for i in ids])
        require(value is not None,'Nonpositive q3 seed')
        c,rad=value; coefficients=ORACLE.key(c,rad)
        require(tuple(map(int,r['coefficients']))==coefficients and tuple(r['support'])==ids,'Incorrect exact ball key/support')
        A,B,C,D,E=coefficients
        depth=0; shell=[]; prefixes={0:0}
        target={v['rank'] for v in cuts.values()}
        for rank,original in enumerate(order):
            px,py,pz=points[original]
            power=A*(px*px+py*py+pz*pz)+B*px+C*py+D*pz+E
            depth+=power<0
            if power==0:shell.append(original)
            if rank+1 in target:prefixes[rank+1]=depth
        shell.sort(); accepted=depth<threshold
        ref=r['reference']
        require((ref['accepted'],ref['depth'],ref['shell'])==(accepted,min(depth,threshold),shell if accepted else []),'Reference versus rational sphere census')
        require(len(r['relays'])==len(cuts),'Missing handoff')
        for relay in r['relays']:
            cut=cuts[relay['cut']]
            require(relay['cursor']==cut['cursor'] and relay['rank']==cut['rank'],'Wrong seam identity')
            require(relay['incoming_count']==min(threshold,prefixes[cut['rank']]),'Wrong prefix credit')
            require((relay['accepted'],relay['depth'],relay['shell'])==(ref['accepted'],ref['depth'],ref['shell']),'Handoff census/shell mismatch')
            require(relay['count_global_root_visits']==(1 if relay['cut']=='root' else 0),'Global count root restarted')
            counts['relays']+=1
            counts['positive_prefixes']+=relay['incoming_count']>0
            counts['accepted_relays']+=accepted
        counts['seeds']+=1; counts['global_oracle_point_tests']+=len(points)
        counts['max_shell']=max(counts['max_shell'],len(shell) if accepted else 0)
    counts['calls']+=1


def run(folder):
    require(not folder.exists(),'Receipt already exists');folder.mkdir(parents=True)
    build=BASE/'.build'/folder.name;require(not build.exists(),'Build already exists');build.mkdir(parents=True)
    before=pins(); m=dict(status='started',sources=before,commands=0,
        scope='Exact suffix handoff; prefix brute-force is a test oracle, not an implemented common filter',
        cpu_affinity=sorted(os.sched_getaffinity(0)))
    write(folder/'MANIFEST.json',m)
    commands=[['g++','--version'],['clang++','--version'],
        ['g++',*flags(),'-O2',str(BASE/'probe.cpp'),str(libraries()[0]),'-pthread','-o',str(build/'release')],
        ['clang++',*flags(),'-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer',
         str(BASE/'probe.cpp'),str(libraries()[1]),'-pthread','-o',str(build/'sanitize')]]
    commands += [[sys.executable,'-B',*option,str(BASE/'forest_model.py')] for option in ([],['-O'])]
    for command in commands:append(folder,m,execute(command))
    m['binaries']={str((build/b).relative_to(ROOT)):sha(build/b) for b in ('release','sanitize')}
    counts=dict(calls=0,seeds=0,relays=0,positive_prefixes=0,accepted_relays=0,global_oracle_point_tests=0,max_shell=0)
    for name,points in cases():
        path=save(name,points)
        eligible=ORACLE.expected(points,10,2,{})
        edges={(0,1)}
        if eligible:edges.add(ORACLE.owner(points,tuple(eligible[0]['support'])))
        for a,b in sorted(edges):
            for k in (3,5,10):
                for binary in ('release','sanitize'):
                    record=dict(case=name,n=len(points),k=k,edge=[a,b],sample_limit=0,input_sha256=sha(path),
                        **execute([str(build/binary),str(path),str(k),str(a),str(b),'0']))
                    append(folder,m,record);check(json.loads(record['stdout']),points,a,b,k,0,counts)
        print('PASS small',name,flush=True)
    for scan in (0,100,200):
        directory=BASE.parent/'lidar08_20260914/prepared'/f'single_{scan:06d}'
        small=list(struct.iter_unpack('<HHH',(directory/'n8000.u16le').read_bytes()))
        anchor=3000;order=sorted((i for i in range(len(small)) if i!=anchor),key=lambda i:(ORACLE.distance(small[anchor],small[i]),i))
        a,b=sorted((anchor,order[3]))
        declared={s['n']:s['sha256'] for s in json.loads((directory/'METADATA.json').read_text())['samples']}
        for n in (8000,16000,32000):
            path=directory/f'n{n}.u16le'; require(sha(path)==declared[n],'LiDAR input changed')
            points=list(struct.iter_unpack('<HHH',path.read_bytes()));require(points[:8000]==small,'Nested prefix changed')
            for k in (5,10):
                record=dict(case=f'lidar_{scan:06d}_{n}',n=n,k=k,edge=[a,b],sample_limit=32,input_sha256=sha(path),
                    **execute([str(build/'release'),str(path),str(k),str(a),str(b),'32']))
                append(folder,m,record);check(json.loads(record['stdout']),points,a,b,k,32,counts)
        print('PASS LiDAR scan',scan,flush=True)
    require(counts['seeds'] and counts['positive_prefixes'] and counts['accepted_relays'],'Vacuous relay campaign')
    require(before==pins(),'Frozen dependency changed')
    require(all(sha(ROOT/name)==h for name,h in m['binaries'].items()),'Binary changed')
    m.update(status='completed',counts=counts,commands_sha256=sha(folder/'COMMANDS.jsonl.gz'))
    write(folder/'MANIFEST.json',m)
    write(folder/'COMPLETION.json',dict(status='completed',manifest_sha256=sha(folder/'MANIFEST.json')))


if __name__=='__main__':
    require(len(sys.argv)==2,'usage: campaign.py new_receipt_folder')
    run(Path(sys.argv[1]).resolve())
