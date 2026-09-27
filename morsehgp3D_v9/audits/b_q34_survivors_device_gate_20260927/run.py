#!/usr/bin/env python3
"""CUDA compilation and portable fixture qualification only; never a GPU run.

Explicit port of the pre-build -M / post-build .o.d closure in the frozen
b_q34_resident_survivors_20260927/run.py, a9b75fca..., for one CUDA TU.
"""
import argparse
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import shlex
import shutil
import signal
import subprocess
import sys
sys.dont_write_bytecode=True
HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[2]
PRIOR=HERE.parent/'b_q34_resident_survivors_20260927'
TOOLKIT=Path('/workspaces/E-HGP/build/cuda-12.9-local')
HELPER=ROOT/'gcp-migration/full_probe_session_v7.py'
TARGET='mhgp9_survivors_device_gate'
SCHEMA='mhgp9_survivors_device_gate_capture_v1'
PROBE='mhgp9_survivors_device_gate_v1'
FROZEN={
    HERE/'device_gate.cu':'a944df2dcc8756ef8b1cccc201633318f0aefdc5cff85dc77cc8000b2f6d2bfe',
    HERE/'CMakeLists.txt':'061b7edaf7d81dd49cd0e30a228a206065bce83698cffffc0ebf87996bab5232',
    PRIOR/'run.py':'a9b75fca4c0598b22841b8dd57a91a120dc475f2fb84d558aad75b7d1abc8156',
    PRIOR/'device_cuda.cu':'af9259f6104cf0a0f12116af6be2860311b2cc32a989cf6fe70b78529bc86666',
    PRIOR/'device.hpp':'e042b21feec02075aa0f5eb2c3ee19f11339cc123efb166922c3d1def259e759',
    PRIOR/'collector.hpp':'093bd986331d92aa9c77598bac3417917fc25e43487b1864c492e035c5743654',
    HELPER:'177b25a0d72150dc331661fdf8da1ccde77ea17fb694d9c6af5b0929755160d8',
}
GATE=dict(schema=PROBE,status='passed',mode='host_fixture',cuda_executed=False,cases=24,refused=0,items=642,
    empty=6,single=6,empty_waves=1226,high32=624,high63=606,boundary_inversions=594,inversions=606,
    growths=0,growth_copy_bytes=0,upload_bytes=0,download_bytes=0,peak_bytes=0)
def need(ok,reason):
    if not ok: raise ValueError(reason)
def sha(path): return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def read(path):
    def unique(items):
        result={}
        for key,value in items:
            need(key not in result,'duplicate JSON key');result[key]=value
        return result
    return json.loads(Path(path).read_text(),object_pairs_hook=unique)
def save(path,value): path.write_text(json.dumps(value,sort_keys=True,indent=2,allow_nan=False)+'\n')
def sources():
    need(all(sha(path)==digest for path,digest in FROZEN.items()),'frozen gate and provenance unchanged')
    paths=list(FROZEN)
    paths += [p for p in HERE.iterdir() if p.suffix in ('.py','.cu','.hpp') or p.name=='CMakeLists.txt']
    paths += [PRIOR/name for name in ('device.hpp','collector.hpp','provenance.py')]
    for folder in (PRIOR,HERE.parent/'b_q34_filtered_resident_20260927'):
        paths += [folder/name for name in ('device.hpp','device_cpu.cpp','device_cuda.cu','device_stub.cpp','host.hpp','CMakeLists.txt')]
    for folder in ('include','bin','nvvm'):
        paths += [p for p in (TOOLKIT/folder).rglob('*') if p.is_file()]
    paths += [TOOLKIT/'lib/libcudart_static.a',TOOLKIT/'lib/libcudadevrt.a',Path('/usr/lib/x86_64-linux-gnu/librt.a')]
    paths += [Path(shutil.which(name)).resolve() for name in ('c++','cmake')]
    return {str(p):sha(p) for p in sorted({p.resolve() for p in paths})}
def provenance_value():
    spec=importlib.util.spec_from_file_location('device_gate_provenance',PRIOR/'provenance.py')
    module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module);return module.check()
def dependency_paths(text,cwd):
    text=text.replace('\\\n',' ');need(':' in text,'dependency file form')
    return sorted({str((Path(item) if Path(item).is_absolute() else cwd/item).resolve())
                   for item in shlex.split(text.split(':',1)[1])})
def dependency_recipe(build):
    rows=[]
    for entry in read(build/'compile_commands.json'):
        argv=entry.get('arguments') or shlex.split(entry['command'])
        if not any('CMakeFiles/'+TARGET+'.dir/' in part for part in argv): continue
        result=[];skip=False
        for token in argv:
            if skip: skip=False;continue
            if token in ('-o','-MF','-MT','-MQ'): skip=True;continue
            if token in ('-c','-MD','-MMD','-MP','-fsyntax-only'): continue
            result.append(token)
        need(not skip,'complete compiler options')
        rows.append(dict(name='target_'+str(len(rows)),cwd=str(Path(entry['directory']).resolve()),argv=result+['-M']))
    need(len(rows)==1,'one CUDA translation unit with frozen collector included');return rows
def link_archives(build):
    def expand(argv,seen):
        result=[]
        for token in argv:
            if not token.startswith('@'): result.append(token);continue
            path=(build/token[1:]).resolve();need(path.is_relative_to(build) and path not in seen,'local acyclic link response file')
            result.extend(expand(shlex.split(path.read_text()),seen|{path}))
        return result
    argv=expand(shlex.split((build/'CMakeFiles'/f'{TARGET}.dir/link.txt').read_text()),set())
    allowed={p.resolve() for p in (TOOLKIT/'lib/libcudart_static.a',TOOLKIT/'lib/libcudadevrt.a',Path('/usr/lib/x86_64-linux-gnu/librt.a'))}
    actual={(Path(token) if Path(token).is_absolute() else build/token).resolve() for token in argv if token.endswith('.a')}
    search=[Path(token[2:]).resolve() for token in argv if token.startswith('-L')]
    for library in ('cudart_static','cudadevrt'):
        need('-l'+library in argv,'expected CUDA archive linked')
        candidates=[directory/('lib'+library+suffix) for directory in search for suffix in ('.so','.a')]
        resolved=next((p.resolve() for p in candidates if p.is_file()),None)
        need(resolved==(TOOLKIT/'lib'/('lib'+library+'.a')).resolve(),'actual CUDA archive search resolution')
        actual.add(resolved)
    need(actual==allowed,'exact explicit/CUDA archives');return {str(p):sha(p) for p in sorted(actual)}
def response_pins(build):
    paths=sorted((build/'CMakeFiles'/f'{TARGET}.dir').rglob('*.rsp'))
    return {str(p.resolve()):sha(p) for p in paths}
def prepin(build):
    build=build.resolve();target=build/'headers_before.json';need(not target.exists(),'fresh prepin')
    rows=[];pins={};compile_sha=sha(build/'compile_commands.json');archives=link_archives(build)
    link_sha=sha(build/'CMakeFiles'/f'{TARGET}.dir/link.txt');responses=response_pins(build)
    for job in dependency_recipe(build):
        result=subprocess.run(job['argv'],cwd=job['cwd'],text=True,stdout=subprocess.PIPE,stderr=subprocess.PIPE,check=False)
        rows.append(job|dict(exit_code=result.returncode,stdout=result.stdout,stderr=result.stderr))
        need(result.returncode==0,'dependency discovery failed: '+result.stderr)
        for path in dependency_paths(result.stdout,Path(job['cwd'])): pins[path]=sha(path)
    need(pins and all(sha(path)==digest for path,digest in pins.items()),'dependency discovery closure')
    need(responses==response_pins(build),'response files unchanged during discovery')
    value=dict(schema='mhgp9_survivors_device_gate_dependencies_before_v1',build=str(build),
        compile_commands_sha256=compile_sha,link_command_sha256=link_sha,archives=archives,response_files=responses,
        commands=rows,headers=dict(sorted(pins.items())))
    with target.open('x') as stream: json.dump(value,stream,sort_keys=True,indent=2);stream.write('\n')
    print(json.dumps(value,sort_keys=True))
def check_prepin(build):
    value=read(build/'headers_before.json');need(value['schema']=='mhgp9_survivors_device_gate_dependencies_before_v1' and
        value['build']==str(build),'dependency identity')
    need(value['compile_commands_sha256']==sha(build/'compile_commands.json'),'exact compiler flags')
    need(value['link_command_sha256']==sha(build/'CMakeFiles'/f'{TARGET}.dir/link.txt') and
         value['archives']==link_archives(build),'actual prelinked archives unchanged')
    need(value['response_files']==response_pins(build),'precompiled response files unchanged')
    jobs=dependency_recipe(build);need(len(value['commands'])==len(jobs),'complete dependency inventory');paths=set()
    for row,job in zip(value['commands'],jobs):
        need(all(row[k]==v for k,v in job.items()) and row['exit_code']==0,'exact prepin command')
        paths.update(dependency_paths(row['stdout'],Path(row['cwd'])))
    need(paths==set(value['headers']),'no invented or missing dependencies')
    need(all(sha(path)==digest for path,digest in value['headers'].items()),'precompiled dependencies unchanged')
    return value
def build_pins(state):
    build=Path(state['build']).resolve();before=check_prepin(build);pins=dict(before['headers'])|before['response_files']
    for path in (build/TARGET,build/'CMakeCache.txt',build/'compile_commands.json',build/'headers_before.json',
                 build/'CMakeFiles'/f'{TARGET}.dir/flags.make',build/'CMakeFiles'/f'{TARGET}.dir/link.txt'):
        pins[str(path)]=sha(path)
    deps=list(build.glob(f'CMakeFiles/{TARGET}.dir/**/*.o.d'));need(len(deps)==1,'one compiled dependency file')
    for path in deps:
        pins[str(path)]=sha(path)
        obj=Path(str(path)[:-2]);pins[str(obj)]=sha(obj)
        for dependency in dependency_paths(path.read_text(),build):
            need(dependency in before['headers'],'compiled dependency absent before build')
            need(sha(dependency)==before['headers'][dependency],'compiled dependency changed')
    return pins
def recipe(state):
    build=state['build'];binary=state['binary']
    return [('system',['uname','-a'],0),('compiler',['c++','--version'],0),
        ('nvcc_version',[str(TOOLKIT/'bin/nvcc'),'--version'],0),
        ('provenance',[sys.executable,'-B',str(PRIOR/'provenance.py')],0),
        ('configure',['cmake','-S',str(HERE),'-B',build,'-DCMAKE_BUILD_TYPE=Release','-DCMAKE_EXPORT_COMPILE_COMMANDS=ON',
            '-DCMAKE_CXX_COMPILER=c++','-DCMAKE_CUDA_COMPILER='+str(TOOLKIT/'bin/nvcc'),'-DCMAKE_CUDA_ARCHITECTURES=120'],0),
        ('prepin',[sys.executable,'-B',str(HERE/'run.py'),'--prepin',build],0),
        ('build',['cmake','--build',build,'--target',TARGET,'-j2'],0),
        ('host_gate',[binary,'--host-gate'],0),('refuse_empty',[binary],2),('refuse_unknown',[binary,'--unknown'],2)]
def readback(directory):
    state=read(directory/'capture.json')
    need(state['schema']==SCHEMA and state['status']=='completed' and state['CUDA_executed'] is False and
         state['GCP_used'] is False,'closed local compile/host scope')
    need(str(Path(state['build']).resolve())==state['build'] and state['binary']==str(Path(state['build'])/TARGET),'exact build binding')
    need(state['pins_before']==state['pins_after']==sources(),'LIVE source closure')
    need(state['build_pins']==build_pins(state),'LIVE build and precompiled dependency closure')
    planned=recipe(state);need(len(state['commands'])==len(planned),'complete command inventory');last=None;gate=None
    for row,(name,argv,code) in zip(state['commands'],planned):
        need(row['name']==name and row['argv']==argv and row['exit_code']==code and row['group_closed'] and
             row['ended_epoch']>=row['started_epoch'],'exact command closure')
        need(last is None or row['started_epoch']>=last,'prepin before build chronology');last=row['ended_epoch']
        need(row==read(directory/(name+'.command.json')),'stored command')
        need(all(row.get(k)==v for k,v in read(directory/(name+'.intent.json')).items()),'intent binding')
        for stream in ('stdout','stderr'): need(sha(directory/(name+'.'+stream))==row[stream+'_sha256'],'stream hash')
        stdout=(directory/(name+'.stdout')).read_text();stderr=(directory/(name+'.stderr')).read_text()
        if name=='prepin':
            value=check_prepin(Path(state['build']));need(read(directory/'prepin.stdout')==value and not stderr,'prepin stream binding')
            for path,digest in value['headers'].items():
                if path in state['pins_before']: need(digest==state['pins_before'][path],'precompiled source pin')
        if name=='host_gate':
            gate=read(directory/'host_gate.stdout')
            need(set(gate)==set(GATE) and all(type(gate[key]) is type(value) and gate[key]==value for key,value in GATE.items()) and
                 not stderr,'exact typed host gate not GPU')
        if name.startswith('refuse_'): need(not stdout and stderr.strip()=='device_gate.usage','causal CLI refusal')
        if name=='provenance':
            need(read(directory/'provenance.stdout')==provenance_value() and not stderr,'sealed collector provenance')
    need(gate is not None,'host gate required')
    return dict(status='PASS',commands=len(planned),gate=gate,CUDA_compiled=True,CUDA_executed=False,GCP_used=False,
        scope='CUDA_compile_and_host_fixtures_only_not_GPU_gate',source_pins=len(state['pins_before']),
        compiled_pins=len(state['build_pins']),headers_before=len(check_prepin(Path(state['build']))['headers']))
def capture(directory,build):
    directory=directory.resolve();build=build.resolve()
    need(not directory.exists() and not build.exists(),'fresh capture and build required');directory.mkdir(parents=True)
    state=dict(schema=SCHEMA,status='failed',CUDA_executed=False,GCP_used=False,build=str(build),binary=str(build/TARGET),
        pins_before=sources(),build_pins={})
    spec=importlib.util.spec_from_file_location('device_gate_commands',HELPER);helper=importlib.util.module_from_spec(spec);spec.loader.exec_module(helper)
    commands=helper.Commands(directory,dict(os.environ,PYTHONDONTWRITEBYTECODE='1'))
    def interrupted(signum,_frame): raise InterruptedError('signal '+str(signum))
    for sig in (signal.SIGINT,signal.SIGTERM,signal.SIGHUP): signal.signal(sig,interrupted)
    try:
        for name,argv,code in recipe(state): need(commands.run(name,argv,timeout=None)[0]==code,'command exit '+name)
        state['build_pins']=build_pins(state);state['status']='completed'
    except BaseException as error:
        state['error']=type(error).__name__+': '+str(error);raise
    finally:
        state['commands']=commands.rows;state['pins_after']={path:sha(path) for path in state['pins_before']}
        if state['pins_before']!=state['pins_after']: state['status']='failed'
        save(directory/'capture.json',state)
    value=readback(directory);save(directory/'summary.json',value);print(json.dumps(value,sort_keys=True))
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);actions=parser.add_mutually_exclusive_group(required=True)
    actions.add_argument('--capture',type=Path);actions.add_argument('--readback',type=Path);actions.add_argument('--prepin',type=Path)
    parser.add_argument('--build',type=Path);args=parser.parse_args()
    if args.prepin: prepin(args.prepin)
    elif args.readback: print(json.dumps(readback(args.readback.resolve()),sort_keys=True))
    else:
        need(args.build is not None,'build path required');capture(args.capture,args.build)
