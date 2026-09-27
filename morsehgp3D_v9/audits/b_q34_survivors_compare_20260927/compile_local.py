#!/usr/bin/env python3
"""Local fresh CUDA compilation with actual flags/responses/deps closed. No GPU."""
import argparse
import importlib.util
import json
import os
from pathlib import Path
import shlex
import sys
sys.dont_write_bytecode=True
HERE=Path(__file__).resolve().parent
PRIOR=HERE.parent/'b_q34_survivors_device_gate_20260927/run.py'
import hashlib
if hashlib.sha256(PRIOR.read_bytes()).hexdigest()!='d81fd3c23892ee9a7ff22dc721787050b050e7563fc4c5fe2fdff9b015a958ce':
    raise ValueError('frozen dependency collector changed')
spec=importlib.util.spec_from_file_location('frozen_gate_compile_helpers',PRIOR)
base=importlib.util.module_from_spec(spec);spec.loader.exec_module(base)
need,sha,read,save=base.need,base.sha,base.read,base.save
TARGETS=('mhgp9_survivors_compare','mhgp9_survivors_device_gate')
GEN=Path('/workspaces/E-HGP/build/v9-q3-payload-integration-20260926/libmhgp9_gen.a')
GATE=dict(schema='mhgp9_survivors_compare_v1',status='passed',mode='gate',cuda_executed=False,cases=52,operators=208,
    queries=26576,S=22408,planned=440,fallbacks=11872,reordered=224,empty=8,first_cuda_calls=0,first_implementation_calls=0)
def sources():
    pins=base.sources()
    paths=[PRIOR,GEN,*[p for p in HERE.iterdir() if p.suffix in ('.py','.cpp','.hpp') or p.name=='CMakeLists.txt']]
    paths += [p for p in (base.ROOT/'morsehgp3D_v9/src').rglob('*') if p.suffix in ('.hpp','.h','.cpp','.cu')]
    for folder in ('b_q34_filtered_resident_20260927','b_q34_resident_survivors_20260927'):
        paths += [p for p in (HERE.parent/folder).iterdir() if p.suffix in ('.hpp','.cpp','.cu') or p.name=='CMakeLists.txt']
    paths += [base.ROOT/'morsehgp3D_v9/tests/gen/front_fixtures.hpp']
    pins.update({str(p.resolve()):sha(p) for p in paths});return pins
def binaries(build): return [build/TARGETS[0],build/'device_gate'/TARGETS[1]]
def responses(build): return {str(p.resolve()):sha(p) for p in sorted(build.rglob('*.rsp'))}
def links(build,source_pins):
    result={}
    def expand(argv,cwd,seen):
        out=[]
        for token in argv:
            if not token.startswith('@'): out.append(token);continue
            p=(cwd/token[1:]).resolve();need(p.is_relative_to(build) and p not in seen,'local acyclic response')
            out.extend(expand(shlex.split(p.read_text()),cwd,seen|{p}))
        return out
    for binary in binaries(build):
        p=binary.parent/'CMakeFiles'/(binary.name+'.dir')/'link.txt'
        argv=expand(shlex.split(p.read_text()),binary.parent,set());archives=set()
        for token in argv:
            if token.endswith('.a'): archives.add((Path(token) if Path(token).is_absolute() else binary.parent/token).resolve())
        search=[(Path(a[2:]) if Path(a[2:]).is_absolute() else binary.parent/a[2:]).resolve() for a in argv if a.startswith('-L')]
        for name in ('cudart_static','cudadevrt'):
            need('-l'+name in argv,'CUDA runtime archive expected')
            found=next((p.resolve() for d in search for p in (d/('lib'+name+'.so'),d/('lib'+name+'.a')) if p.is_file()),None)
            need(found==(base.TOOLKIT/'lib'/('lib'+name+'.a')).resolve(),'actual CUDA archive path');archives.add(found)
        expected={(base.TOOLKIT/'lib/libcudart_static.a').resolve(),(base.TOOLKIT/'lib/libcudadevrt.a').resolve(),Path('/usr/lib/x86_64-linux-gnu/librt.a')}
        if binary.name==TARGETS[0]: expected.add(GEN.resolve())
        need(archives==expected and all(source_pins.get(str(a))==sha(a) for a in archives),'actual archives pinned before linking')
        result[str(p)]=dict(sha256=sha(p),archives={str(a):sha(a) for a in sorted(archives)})
    return result
def jobs(build):
    result=[];counts={name:0 for name in TARGETS}
    for e in read(build/'compile_commands.json'):
        argv=e.get('arguments') or shlex.split(e['command'])
        names=[name for name in TARGETS if any('CMakeFiles/'+name+'.dir/' in a for a in argv)]
        if not names: continue
        need(len(names)==1,'one target per object');counts[names[0]]+=1
        cwd=Path(e['directory']).resolve();obj=(cwd/argv[argv.index('-o')+1]).resolve();need(obj.is_relative_to(build),'object inside build')
        converted=[];skip=False
        for token in argv:
            if skip: skip=False;continue
            if token in ('-o','-MF','-MT','-MQ'): skip=True;continue
            if token in ('-c','-MD','-MMD','-MP'): continue
            converted.append(token)
        need(not skip,'complete compiler options')
        result.append(dict(name='deps_'+str(len(result)),argv=converted+['-M'],cwd=str(cwd),object=str(obj)))
    need(counts=={TARGETS[0]:5,TARGETS[1]:1},'all six actual translation units');return result
def configure(build):
    return ['cmake','-S',str(HERE),'-B',str(build),'-DCMAKE_BUILD_TYPE=Release','-DCMAKE_EXPORT_COMPILE_COMMANDS=ON',
        '-DMHGP9_COMPARE_ENABLE_CUDA=ON','-DMHGP9_GEN_LIBRARY='+str(GEN),
        '-DCMAKE_CUDA_COMPILER='+str(base.TOOLKIT/'bin/nvcc'),'-DCMAKE_CUDA_ARCHITECTURES=120']
def after_recipe(build):
    compare,device=binaries(build)
    return [('build',['cmake','--build',str(build),'--target',*TARGETS,'-j2'],0),
        ('compare_gate',[str(compare),'--gate'],0),('high_host_gate',[str(device),'--host-gate'],0),
        ('refuse_missing_cuda',[str(compare),'--frame','missing.u32','--workers','4','--first','baseline'],2),
        ('refuse_workers',[str(compare),'--frame','missing.u32','--workers','8','--first','baseline','--cuda'],2),
        ('refuse_first',[str(compare),'--frame','missing.u32','--workers','4','--first','other','--cuda'],2)]
def build_pins(state):
    build=Path(state['build']);before=state['headers_before'];out={}
    need(state['response_before']==responses(build),'response file closure')
    need(state['link_before']==links(build,state['sources_before']),'actual link closure')
    need(state['compile_commands_sha']==sha(build/'compile_commands.json'),'actual compiler option closure')
    out.update(before);out.update(responses(build))
    for path,digest in before.items(): need(sha(path)==digest,'compiled dependency unchanged')
    for job in jobs(build):
        obj=Path(job['object']);dep=Path(str(obj)+'.d')
        out[str(obj)]=sha(obj);out[str(dep)]=sha(dep)
        need(set(base.dependency_paths(dep.read_text(),Path(job['cwd'])))<=set(before),'all compiled deps pre-pinned')
    for binary in binaries(build):
        for path in (binary,binary.parent/'CMakeFiles'/(binary.name+'.dir')/'flags.make',
                     binary.parent/'CMakeFiles'/(binary.name+'.dir')/'link.txt'):
            out[str(path)]=sha(path)
    out[str(build/'CMakeCache.txt')]=sha(build/'CMakeCache.txt');out[str(build/'compile_commands.json')]=sha(build/'compile_commands.json')
    return out
def validate_gate(value,expected):
    need(set(value)==set(expected) and all(type(value[k]) is type(v) and value[k]==v for k,v in expected.items()),'exact portable gate')
def readback(directory):
    s=read(directory/'capture.json');need(s['status']=='completed' and s['CUDA_executed'] is False and s['GCP_used'] is False,'closed local scope')
    need(s['sources_before']==s['sources_after']==sources(),'LIVE sources')
    need(s['build_pins']==build_pins(s),'LIVE compiled closure');build=Path(s['build'])
    before=read(directory/'before_build.json')
    need(sha(directory/'before_build.json')==s['before_build_sha'] and before['status']=='failed' and
         all(value==s[key] for key,value in before.items() if key!='status'),'bound pre-build inventory')
    declared=set()
    for job in jobs(build): declared.update(base.dependency_paths((directory/(job['name']+'.stdout')).read_text(),Path(job['cwd'])))
    need(declared==set(s['headers_before']),'pre-build headers exactly from discovery stdout')
    planned=[('configure',configure(build),0)]+[(j['name'],j['argv'],0) for j in jobs(build)]+after_recipe(build)
    need(len(planned)==len(s['commands']),'complete recipe');last=None
    for row,(name,argv,code) in zip(s['commands'],planned):
        need(row['name']==name and row['argv']==argv and row['exit_code']==code and row['group_closed'] and
             row['ended_epoch']>=row['started_epoch'] and (last is None or row['started_epoch']>=last),'ordered command closure')
        last=row['ended_epoch'];need(row==read(directory/(name+'.command.json')),'stored command')
        need(all(row.get(k)==v for k,v in read(directory/(name+'.intent.json')).items()),'intent')
        for stream in ('stdout','stderr'): need(sha(directory/(name+'.'+stream))==row[stream+'_sha256'],'stream pin')
        stderr=(directory/(name+'.stderr')).read_text()
        if name=='compare_gate': validate_gate(read(directory/(name+'.stdout')),GATE);need(not stderr,'portable gate diagnostics')
        if name=='high_host_gate': validate_gate(read(directory/(name+'.stdout')),base.GATE);need(not stderr,'high fixture diagnostics')
        if name.startswith('refuse_'):
            cause={'refuse_missing_cuda':'compare.frame_requires_cuda_workers_first','refuse_workers':'compare.workers','refuse_first':'compare.first'}[name]
            need(not (directory/(name+'.stdout')).read_text() and stderr.strip()==cause,'causal CLI refusal')
    return dict(status='PASS',commands=len(planned),translation_units=6,dependencies=len(s['headers_before']),
        CUDA_compiled=True,CUDA_executed=False,GCP_used=False,gate=GATE)
def capture(directory,build):
    directory=directory.resolve();build=build.resolve();need(not directory.exists() and not build.exists(),'fresh local capture/build');directory.mkdir(parents=True)
    s=dict(status='failed',build=str(build),CUDA_executed=False,GCP_used=False,sources_before=sources())
    spec=importlib.util.spec_from_file_location('compare_local_commands',base.HELPER);helper=importlib.util.module_from_spec(spec);spec.loader.exec_module(helper)
    commands=helper.Commands(directory,dict(os.environ,PYTHONDONTWRITEBYTECODE='1'))
    original=Path.cwd()
    try:
        need(commands.run('configure',configure(build),timeout=None)[0]==0,'configure')
        s['response_before']=responses(build);s['link_before']=links(build,s['sources_before']);s['compile_commands_sha']=sha(build/'compile_commands.json')
        deps={}
        for job in jobs(build):
            os.chdir(job['cwd'])
            try: need(commands.run(job['name'],job['argv'],timeout=None)[0]==0,'dependency discovery')
            finally: os.chdir(original)
            for path in base.dependency_paths((directory/(job['name']+'.stdout')).read_text(),Path(job['cwd'])):
                digest=sha(path);need(path not in deps or deps[path]==digest,'dependency scan stable');deps[path]=digest
        need(s['response_before']==responses(build),'responses unchanged after discovery');s['headers_before']=deps
        save(directory/'before_build.json',s);s['before_build_sha']=sha(directory/'before_build.json')
        for name,argv,code in after_recipe(build): need(commands.run(name,argv,timeout=None)[0]==code,'command exit '+name)
        s['build_pins']=build_pins(s);s['status']='completed'
    except BaseException as error: s['error']=type(error).__name__+': '+str(error);raise
    finally:
        os.chdir(original);s['commands']=commands.rows;s['sources_after']={p:sha(p) for p in s['sources_before']};save(directory/'capture.json',s)
    value=readback(directory);save(directory/'summary.json',value);print(json.dumps(value,sort_keys=True))
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);group=parser.add_mutually_exclusive_group(required=True)
    group.add_argument('--capture',type=Path);group.add_argument('--readback',type=Path);parser.add_argument('--build',type=Path)
    args=parser.parse_args()
    if args.readback: print(json.dumps(readback(args.readback.resolve()),sort_keys=True))
    else: need(args.build is not None,'build required');capture(args.capture,args.build)
