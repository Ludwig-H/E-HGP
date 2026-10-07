#!/usr/bin/env python3
"""Audit synthétique des outils de données et du cache ; aucune donnée ni requête réseau."""
import contextlib
import copy
import hashlib
import importlib.util
import io
import json
import os
from pathlib import Path
import random
import shutil
import struct
import subprocess
import sys
import tempfile
from unittest import mock

HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[3]
V12=ROOT/'morsehgp3D_v12'
DATA=V12/'bench/data'
PIN='1f7642e105aebd76632c58c63fdfd5b5c0824779'
OLD='4147c546000b198b5239646063bfb1e3ed6d28fc'
SCHEMA='mhgp12.benchmark_inputs.v1'
FLAGS=['-B']+(['-O'] if sys.flags.optimize else [])
ENV=dict(os.environ,PYTHONDONTWRITEBYTECODE='1')

def need(ok,msg):
    if not ok:raise RuntimeError(msg)
def digest(data):return hashlib.sha256(data).hexdigest()
def gitfile(pin,rel):return subprocess.check_output(['git','-C',str(ROOT),'show',pin+':'+rel])
def source_hashes():
    paths=[p for p in DATA.rglob('*') if p.is_file() and '__pycache__' not in p.parts]
    paths += [V12/'bench/data_cache.py',V12/'bench/donnees_test.py',V12/'bench/data_cache_test.py',V12/'docs/DONNEES.md']
    out={}
    for path in paths:
        rel=path.relative_to(ROOT).as_posix();raw=path.read_bytes()
        need(raw==gitfile(PIN,rel),'source differs from pin: '+rel);out[rel]=digest(raw)
    return out

def run(args,**kw):return subprocess.run([str(a) for a in args],capture_output=True,text=True,timeout=60,env=ENV,**kw)
def py(path,*args,isolated=False):
    flags=['-I']+FLAGS if isolated else ['-S']+FLAGS
    return run([sys.executable]+flags+[path]+list(args))
def load(name,path):
    spec=importlib.util.spec_from_file_location(name,path);module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module);return module

def scene(folder,name,points,ids=None,mults=None):
    folder.mkdir(parents=True,exist_ok=True)
    ids=list(range(len(points))) if ids is None else ids
    points_ids=sorted(zip(points,ids),key=lambda x:x[0]);points=[p for p,_ in points_ids];ids=[i for _,i in points_ids]
    coords=b''.join(struct.pack('<III',*p) for p in points);idbytes=struct.pack('<%dI'%len(ids),*ids)
    (folder/(name+'.u32le')).write_bytes(coords);(folder/(name+'.ids.u32le')).write_bytes(idbytes)
    record={'name':name,'count':len(points),'coordinates':name+'.u32le','sha256':digest(coords),
            'point_ids':name+'.ids.u32le','ids_sha256':digest(idbytes),'duplicate_sites':len(points)-len(set(points))}
    if mults is not None:
        multbytes=struct.pack('<%dI'%len(mults),*mults)
        (folder/(name+'.mult.u32le')).write_bytes(multbytes);record.update(mult=name+'.mult.u32le',mult_sha256=digest(multbytes))
    return record

def write_manifest(path,records,**extras):
    obj=dict(schema=SCHEMA,cases=records,**extras);path.write_text(json.dumps(obj));return obj

def verify_cases(tmp,old_verify):
    folder=tmp/'verify';record=scene(folder,'pair',[(0,0,0),(1,2,3)])
    path=folder/'manifest.json';base=write_manifest(path,[record]);rows={}
    examples={
        'valid':(base,0),
        'empty_manifest':(dict(schema=SCHEMA,cases=[]),2),
        'null_hash':(dict(schema=SCHEMA,cases=[dict(record,sha256=None)]),2),
        'wrong_hash':(dict(schema=SCHEMA,cases=[dict(record,sha256='0'*64)]),1),
        'upper_hash':(dict(schema=SCHEMA,cases=[dict(record,sha256='A'*64)]),2),
        'bool_count':(dict(schema=SCHEMA,cases=[dict(record,count=True)]),2),
        'zero_count':(dict(schema=SCHEMA,cases=[dict(record,count=0)]),2),
        'path_name':(dict(schema=SCHEMA,cases=[dict(record,coordinates='../pair.u32le')]),2),
        'crops_not_list':(dict(base,crops={}),2),
        'late_invalid_crop':(dict(base,crops=[dict(record,sha256=None)]),2),
        'same_name_different_hash':(dict(schema=SCHEMA,cases=[record,dict(record,sha256='0'*64)]),2),
        'identical_file_reused':(dict(schema=SCHEMA,cases=[record,dict(record,name='alias')]),0),
        'distinct_missing_mult':(dict(schema=SCHEMA,cases=[dict(record,distinct=record)]),2),
        'mult_without_digest':(dict(schema=SCHEMA,cases=[dict(record,mult='extra.u32le')]),2),
    }
    for name,(obj,want) in examples.items():
        path.write_text(json.dumps(obj));p=py(DATA/'verify_inputs.py',path)
        need(p.returncode==want and not p.stderr,'verify '+name)
        rows[name]={'exit':p.returncode,'refused_before_payload_reads':want==2 and 'aucun fichier lu' in p.stdout}
    original={}
    for name in ['valid','empty_manifest','null_hash','wrong_hash']:
        path.write_text(json.dumps(examples[name][0]));p=py(old_verify,path)
        expected=0 if name in ['valid','empty_manifest','null_hash'] else 1
        need(p.returncode==expected,'historical verify '+name);original[name]=p.returncode
    # Prove ordering independently of the refusal string: forbid both probes before admission.
    module=load('audit_verify',DATA/'verify_inputs.py');path.write_text(json.dumps(examples['late_invalid_crop'][0]))
    with mock.patch.object(module.Path,'is_file',side_effect=RuntimeError('payload probe forbidden')) as probes, \
         mock.patch.object(module,'sha256_file',side_effect=RuntimeError('payload read forbidden')) as reads, \
         contextlib.redirect_stdout(io.StringIO()):
        need(module.main([str(path)])==2,'late invalid not refused');need(probes.call_count==reads.call_count==0,'payload accessed before admission')
    # Verify all three distinct payloads; mutate a multiplicity after a valid baseline.
    drec=scene(folder,'distinct',[(0,0,0),(1,2,3)],mults=[2,1]);write_manifest(path,[dict(record,distinct=drec)])
    p=py(DATA/'verify_inputs.py',path);need(p.returncode==0,'distinct positive')
    (folder/drec['mult']).write_bytes(struct.pack('<II',2,2));p=py(DATA/'verify_inputs.py',path)
    need(p.returncode==1 and 'EMPREINTE distinct.mult.u32le' in p.stdout,'distinct mult corruption')
    # Positive --measure and wrong geometric metadata, using the actual CLI.
    write_manifest(path,[record]);p=py(DATA/'verify_inputs.py',path,'--measure',isolated=True);need(p.returncode==0,'measure positive')
    write_manifest(path,[dict(record,extent_mm=[9,9,9])]);p=py(DATA/'verify_inputs.py',path,'--measure',isolated=True)
    need(p.returncode==1 and 'extent' in p.stdout,'measure extent')
    return {'new':rows,'historical':original,'admission_before_any_payload_probe':True,
            'distinct_mult_mutation_refused':True,'measurement_valid_and_bad_extent_checked':True}

def replay_cases(tmp,old_replay):
    elsewhere=tmp/'elsewhere';elsewhere.mkdir();out=tmp/'replay_output'
    for folder,filename in [('synth','manifest.json'),('semantickitti','manifest_v12set.json')]:
        d=out/'data'/folder;r=scene(d,folder,[(0,0,0),(4,1,6)])
        write_manifest(d/filename,[r])
    def replay(path,steps,root):
        e={k:v for k,v in ENV.items() if k not in ['ROOT','PY']}
        if root is not None:e.update(ROOT=str(root),PY=sys.executable)
        return subprocess.run(['bash',str(path)]+steps,cwd=elsewhere,env=e,capture_output=True,text=True,timeout=60)
    old=replay(old_replay,['verify'],out);need(old.returncode==2 and '/scripts/verify_inputs.py' in old.stderr,'historical replay')
    nominal=replay(DATA/'replay_all.sh',['verify'],out)
    need(nominal.returncode==0 and nominal.stdout.count('0 ecarts')==2,'current replay')
    no_data=replay(DATA/'replay_all.sh',['verify'],tmp/'empty_output');need(no_data.returncode==2,'empty replay admitted')
    tools=replay(DATA/'replay_all.sh',['outils'],None);need(tools.returncode==0,'outils rootless')
    # Script-only stage does not need ROOT, unknown stage must not create the destination.
    forbidden=tmp/'not_created';p=replay(DATA/'replay_all.sh',['unknown'],forbidden)
    need(p.returncode==2 and not forbidden.exists(),'unknown stage side effect')
    return {'historical_verify_exit':old.returncode,'current_verify_exit':0,'synthetic_manifests_verified':2,
            'empty_root_exit':no_data.returncode,'outils_without_root_exit':tools.returncode,'unknown_stage_exit':p.returncode}

def crop_cases(tmp,old_crop):
    folder=tmp/'column';r=scene(folder,'column',[(0,0,z) for z in range(4)])
    manifest=folder/'manifest.json';base=write_manifest(manifest,[r])
    p=py(old_crop,'--manifest',manifest,'--sizes','2',isolated=True);need(p.returncode==0,'old column crop')
    old=json.loads(manifest.read_text())['crops'][0]
    need(old['count']==2 and old['crop']['radius_chebyshev_mm']==0,'old column shape')
    manifest.write_text(json.dumps(base));p=py(DATA/'crop_scenes.py','--manifest',manifest,'--sizes','2',isolated=True)
    need(p.returncode==0 and json.loads(manifest.read_text())['crops']==[],'new column not skipped')
    clouds=[[(0,0,0),(10,0,0),(0,10,0),(10,10,0)]+[(5,5,z) for z in range(4)]+[(6,5,9)],
            [(0,0,0),(2**32-1,0,2),(0,2**32-1,4),(2**32-1,2**32-1,6),
             (2**31-1,2**31-1,3),(2**31-1,2**31-1,8),(2**31,2**31,9)]]
    rng=random.Random(1407)
    for _ in range(5):
        points={(0,0,0),(24,24,24)}
        while len(points)<23:points.add(tuple(rng.randrange(25) for _ in range(3)))
        clouds.append(sorted(points))
    rows=[]
    for ci,points in enumerate(clouds):
        folder=tmp/('crop_%d'%ci);ids=[1000+7*i for i in range(len(points))]
        record=scene(folder,'scene',points,ids);path=folder/'manifest.json';write_manifest(path,[record])
        targets=[1,2,3,5,8,len(points),len(points)+1]
        p=py(DATA/'crop_scenes.py','--manifest',path,'--sizes',*map(str,targets),isolated=True)
        need(p.returncode==0,'crop cloud '+str(ci));crops={c['crop']['target_sites']:c for c in json.loads(path.read_text())['crops']}
        cx=(min(x for x,y,z in points)+max(x for x,y,z in points))//2
        cy=(min(y for x,y,z in points)+max(y for x,y,z in points))//2
        distances=[max(abs(x-cx),abs(y-cy)) for x,y,z in points];previous=set()
        for n in sorted(set(targets)):
            if n>=len(points):need(n not in crops,'oversize not skipped');continue
            radius=sorted(distances)[n-1];selected={i for i,d in enumerate(distances) if d<=radius}
            if len(selected)==len(points):need(n not in crops,'full footprint not skipped');continue
            c=crops[n];need(c['count']==len(selected) and c['crop']['radius_chebyshev_mm']==radius,'crop count/radius')
            actual_ids=struct.unpack('<%dI'%c['count'],(folder/c['point_ids']).read_bytes());expect_ids={ids[i] for i in selected}
            need(set(actual_ids)==expect_ids,'crop site identity mismatch');need(previous<=expect_ids,'non-nested');previous=expect_ids
            ordered=sorted(selected,key=lambda i:points[i]);mins=[min(points[i][a] for i in ordered) for a in range(3)]
            expected_bytes=b''.join(struct.pack('<III',*(points[i][a]-mins[a] for a in range(3))) for i in ordered)
            need((folder/c['coordinates']).read_bytes()==expected_bytes,'crop coordinate mismatch')
            rows.append({'cloud':ci,'target':n,'actual':c['count'],'radius':radius})
        p=py(DATA/'verify_inputs.py',path,'--measure',isolated=True);need(p.returncode==0,'crop verify measurement')
    return {'historical_column_count':2,'current_column_crops':0,'independent_closed_square_clouds':len(clouds),
            'selections':rows,'all_written_coordinates_ids_and_nesting_exact':True,'real_crops_regenerated':False}

def cache_cases(tmp,old_cache):
    modules={'old':load('old_cache',old_cache),'new':load('new_cache',V12/'bench/data_cache.py')};rows={}
    for version,module in modules.items():
        cache=module.Cache(tmp/('cache_'+version),1000,0,0)
        try:
            payload=b'abcdef';sha=digest(payload);path=cache.object_path(sha);path.write_bytes(payload);path.chmod(0o400)
            dest=tmp/('build_'+version);dest.mkdir();need(module.link_into(cache,dest,'sample',sha)=='hardlink','real hardlink')
            with mock.patch.object(cache,'free',return_value=0):
                try:cache.make_room(1,set(),set());refused=False
                except module.EntryFailure:refused=True
            need(refused and (dest/'sample').read_bytes()==payload,'hardlink contents')
            need(path.exists()==(version=='new'),'hardlink causal outcome')
            rows[version]={'refused':refused,'cache_object_preserved':path.exists(),'eviction_count':len(cache.evicted),'build_bytes_preserved':True}
        finally:os.close(cache.lock)
    module=modules['new'];extra={}
    for kind in ['protected_floor','cap_then_floor','protected_partial']:
        cache=module.Cache(tmp/('cache_'+kind),40 if kind=='cap_then_floor' else 1000,0,0)
        try:
            linked=digest(b'L'*20);single=digest(b'S'*24)
            lp=cache.object_path(linked);lp.write_bytes(b'L'*20);os.utime(lp,(1,1))
            sp=cache.object_path(single);sp.write_bytes(b'S'*24);os.utime(sp,(2,2))
            dest=tmp/('build_'+kind);dest.mkdir();module.link_into(cache,dest,'linked',linked)
            # Free-space model derives from actual file existence, never from product counters.
            free=lambda:3+(24 if not sp.exists() else 0)
            protected={single} if kind=='protected_floor' else set();parts=set()
            if kind=='protected_partial':
                key='a'*64;(cache.partial/(key+'.part')).write_bytes(b'P'*8);parts={key};protected={single}
            with mock.patch.object(cache,'free',side_effect=free):
                try:cache.make_room(10,protected,parts);refused=False
                except module.EntryFailure:refused=True
            if kind=='cap_then_floor':
                need(not refused and lp.exists() and not sp.exists(),'combined floor/cap selection')
            else:need(refused and lp.exists() and sp.exists() and not cache.evicted,'protected admission mutated cache')
            need((dest/'linked').read_bytes()==b'L'*20,'linked build bytes changed')
            extra[kind]={'refused':refused,'eviction_count':len(cache.evicted),'linked_object_preserved':lp.exists()}
        finally:os.close(cache.lock)
    return {'historical_and_corrected_hardlink':rows,'independent_additional_cases':extra,
            'actual_disk_space_not_claimed':True,'network_called':False}

def developer_gates():
    rows={}
    for name,count in [('donnees_test.py',37),('data_cache_test.py',8)]:
        p=py(V12/'bench'/name);need(p.returncode==0 and not p.stderr,'developer gate '+name)
        lines=[json.loads(line) for line in p.stdout.splitlines()];last=lines[-1]
        need(last['cas']==count and last['ecarts']==[],'developer gate totals')
        for line in lines:line.pop('optimise',None)
        rows[name]={'exit':0,'cases':count,'normalized_output_sha256':digest(json.dumps(lines,sort_keys=True).encode())}
    return rows

def main():
    before=source_hashes();own=digest(Path(__file__).read_bytes());historical={}
    with tempfile.TemporaryDirectory(prefix='ehgp-juges-data-') as raw:
        tmp=Path(raw);olddata=tmp/'old/morsehgp3D_v12/bench/data';olddata.mkdir(parents=True)
        for rel in ['verify_inputs.py','crop_scenes.py','replay_all.sh','v12data/common.py','v12data/__init__.py']:
            name='morsehgp3D_v12/bench/data/'+rel;data=gitfile(OLD,name)
            p=olddata/rel;p.parent.mkdir(parents=True,exist_ok=True);p.write_bytes(data);historical[name]=digest(data)
        oldcache=tmp/'old/data_cache.py';data=gitfile(OLD,'morsehgp3D_v12/bench/data_cache.py');oldcache.write_bytes(data)
        historical['morsehgp3D_v12/bench/data_cache.py']=digest(data)
        results={'verification':verify_cases(tmp,olddata/'verify_inputs.py'),
                 'replay':replay_cases(tmp,olddata/'replay_all.sh'),
                 'crop':crop_cases(tmp,olddata/'crop_scenes.py'),
                 'cache':cache_cases(tmp,oldcache),'developer_gates':developer_gates()}
    need(source_hashes()==before and digest(Path(__file__).read_bytes())==own,'source changed')
    print(json.dumps({'schema':'audit.juges.data.v1','pin':PIN,'historical_pin':OLD,'source_sha256':before,
        'historical_source_sha256':historical,'witness_sha256':own,'results':results,'before_after_unchanged':True,
        'gcp_used':False,'real_data_used':False,'network_used':False,'native_builds':False},sort_keys=True,indent=2))
if __name__=='__main__':main()
