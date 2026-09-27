#!/usr/bin/env python3
"""Fresh small CPU qualification and CUDA compile-only, no GCP/device run."""
import hashlib
import importlib.util
import json
from pathlib import Path
import shlex
import shutil
import subprocess
import sys
sys.dont_write_bytecode=True
import provenance
HERE=Path(__file__).resolve().parent
BASE=HERE.parent/'b_q34_direct_bands_20260927/run.py'
if hashlib.sha256(BASE.read_bytes()).hexdigest()!='3165f540438340a23792b0c26217b20bc0eec07635c6f661a7acdffa8ff249e5':
    raise RuntimeError('frozen local collector changed')
spec=importlib.util.spec_from_file_location('survivors_local_collector',BASE)
base=importlib.util.module_from_spec(spec);spec.loader.exec_module(base)
need,sha,read=base.need,base.sha,base.read
base.HERE=HERE;base.TARGET='mhgp9_resident_survivors';base.SCHEMA='mhgp9_resident_survivors_capture_v1'
base.PROBE='mhgp9_resident_survivors_v1'
base.LIBS['cuda']=base.LIBS['release']
TOOLKIT=Path('/workspaces/E-HGP/build/cuda-12.9-local')
MUTANTS={'LOW32':'survivors.duplicate_or_unsorted','PAYLOAD':'survivors_gate.key_payload'}

def sources():
    paths=[p for p in (base.ROOT/'morsehgp3D_v9/src').rglob('*') if p.suffix in ('.h','.hpp','.cpp','.cu')]
    paths += [p for p in HERE.iterdir() if p.suffix in ('.hpp','.cpp','.cu','.py') or p.name=='CMakeLists.txt']
    for folder in ('b_q34_filtered_resident_20260927','b_q34_cuda_waves_20260927','b_q34_arena_waves_20260927',
                   'b_q34_collective_arena_20260927','b_q34_direct_bands_20260927','b_q34_factor_plan_20260926','b_q34_bands_20260927'):
        paths += [p for p in (HERE.parent/folder).iterdir() if p.suffix in ('.hpp','.cpp','.cu','.py') or p.name=='CMakeLists.txt']
    # Includes plus actual CUDA compiler internals/libdevice and linked archives,
    # all BEFORE compilation/linking (not merely a post-build link.txt hash).
    for folder in ('include','bin','nvvm'):
        paths += [p for p in (TOOLKIT/folder).rglob('*') if p.is_file()]
    paths += [TOOLKIT/'bin/nvcc',TOOLKIT/'bin/ptxas',BASE,base.HELPER,*base.LIBS.values(),
              TOOLKIT/'lib/libcudart_static.a',TOOLKIT/'lib/libcudadevrt.a',Path('/usr/lib/x86_64-linux-gnu/librt.a'),
              base.ROOT/'morsehgp3D_v9/tests/gen/front_fixtures.hpp']
    paths += [Path(shutil.which(name)).resolve() for name in ('c++','clang++','cmake')]
    return {str(p):sha(p) for p in sorted({p.resolve() for p in paths})}

def mutant_compile(build,label):
    return ['c++','-std=c++20','-O2','-Wall','-Wextra','-Wpedantic','-Werror','-DMHGP9_SURVIVORS_MUTANT_'+label,
        '-I'+str(base.ROOT/'morsehgp3D_v9/src'),'-I'+str(base.ROOT/'morsehgp3D_v9/src/gen'),
        '-I'+str(base.ROOT/'morsehgp3D_v9/tests/gen'),str(HERE/'probe.cpp'),str(HERE/'device_cpu.cpp'),str(HERE/'device_stub.cpp'),
        str(provenance.OLD/'device_cpu.cpp'),str(provenance.OLD/'device_stub.cpp'),str(base.LIBS['release']),'-pthread',
        '-o',str(Path(build)/('mutant_'+label.lower()))]

def api_compile():
    return ['c++','-std=c++17','-Wall','-Wextra','-Werror','-fsyntax-only',
        '-I'+str(base.ROOT/'morsehgp3D_v9/src'),'-include',str(HERE/'device.hpp'),'-x','c++','/dev/null']

def dependency_argv(argv):
    result=[];skip=False
    for token in argv:
        if skip: skip=False;continue
        if token in ('-o','-MF','-MT','-MQ'): skip=True;continue
        if token in ('-c','-MD','-MMD','-MP','-fsyntax-only'): continue
        result.append(token)
    need(not skip,'complete compiler options')
    return result+['-M']

def dependency_paths(text,cwd):
    text=text.replace('\\\n',' ')
    need(':' in text,'dependency file form')
    return sorted({str((Path(item) if Path(item).is_absolute() else cwd/item).resolve())
                   for item in shlex.split(text.split(':',1)[1]) if item!='/dev/null'})

def dependency_jobs(build,kind):
    jobs=[]
    for entry in read(build/'compile_commands.json'):
        argv=entry.get('arguments') or shlex.split(entry['command'])
        if not any('CMakeFiles/'+base.TARGET+'.dir/' in part for part in argv): continue
        jobs.append(dict(name='target_'+str(len(jobs)),cwd=str(Path(entry['directory']).resolve()),argv=dependency_argv(argv)))
    need(len(jobs)==5,'five candidate and baseline translation units')
    if kind=='release':
        for label in MUTANTS:
            argv=mutant_compile(build,label)
            tus=[a for a in argv if a.endswith('.cpp')]
            flags=[a for a in argv if a not in tus and not a.endswith('.a')]
            for i,tu in enumerate(tus):
                jobs.append(dict(name='mutant_'+label+'_'+str(i),cwd=str(base.ROOT),argv=dependency_argv(flags)+[tu]))
        jobs.append(dict(name='api_cpp17',cwd=str(base.ROOT),argv=dependency_argv(api_compile())))
    return jobs

def prepin(build,kind):
    build=build.resolve();need(kind in base.LIBS,'dependency configuration')
    target=build/'headers_before.json';need(not target.exists(),'fresh dependency capture')
    rows=[];pins={};compile_sha=sha(build/'compile_commands.json')
    for job in dependency_jobs(build,kind):
        result=subprocess.run(job['argv'],cwd=job['cwd'],text=True,stdout=subprocess.PIPE,stderr=subprocess.PIPE,check=False)
        row=job|dict(exit_code=result.returncode,stdout=result.stdout,stderr=result.stderr)
        rows.append(row)
        need(result.returncode==0,'dependency discovery '+job['name']+': '+result.stderr)
        for path in dependency_paths(result.stdout,Path(job['cwd'])):
            digest=sha(Path(path));need(path not in pins or pins[path]==digest,'dependency changed while discovering')
            pins[path]=digest
    need(pins and all(sha(Path(path))==digest for path,digest in pins.items()),'dependency discovery closure')
    value=dict(schema='mhgp9_survivors_dependencies_before_v1',kind=kind,build=str(build),
        compile_commands_sha256=compile_sha,commands=rows,headers=dict(sorted(pins.items())))
    with target.open('x') as stream: json.dump(value,stream,sort_keys=True,indent=2);stream.write('\n')
    print(json.dumps(value,sort_keys=True))

def check_prepin(build,kind):
    value=read(build/'headers_before.json')
    need(value['schema']=='mhgp9_survivors_dependencies_before_v1' and value['kind']==kind and value['build']==str(build),
         'before dependency identity')
    need(value['compile_commands_sha256']==sha(build/'compile_commands.json'),'same actual compiler flags')
    jobs=dependency_jobs(build,kind);need(len(jobs)==len(value['commands']),'complete dependency recipe')
    paths=set()
    for row,job in zip(value['commands'],jobs):
        need(all(row[k]==v for k,v in job.items()) and row['exit_code']==0,'exact before dependency recipe')
        paths.update(dependency_paths(row['stdout'],Path(row['cwd'])))
    need(paths==set(value['headers']),'no invented or omitted dependencies')
    need(all(sha(Path(path))==digest for path,digest in value['headers'].items()),'precompiled headers unchanged')
    return value

def recipe(state):
    rows=[('system',['uname','-a'],0),('provenance',[sys.executable,'-B',str(HERE/'provenance.py')],0),
          ('port_diff',[sys.executable,'-B',str(HERE/'provenance.py'),'--diff'],0),
          ('nvcc_version',[str(TOOLKIT/'bin/nvcc'),'--version'],0)]
    for kind in ('release','sanitize','cuda'):
        compiler='clang++' if kind=='sanitize' else 'c++';flags='-Wall -Wextra -Wpedantic -Werror'
        if kind=='sanitize': flags+=' -O1 -g1 -fsanitize=address,undefined -fno-omit-frame-pointer'
        configure=['cmake','-S',str(HERE),'-B',state['builds'][kind],'-DCMAKE_BUILD_TYPE=Release',
            '-DCMAKE_EXPORT_COMPILE_COMMANDS=ON',
            '-DCMAKE_CXX_COMPILER='+compiler,'-DCMAKE_CXX_FLAGS='+flags,'-DMHGP9_GEN_LIBRARY='+str(base.LIBS[kind]),
            '-DMHGP9_SURVIVORS_ENABLE_CUDA='+('ON' if kind=='cuda' else 'OFF')]
        if kind=='cuda': configure += ['-DCMAKE_CUDA_COMPILER='+str(TOOLKIT/'bin/nvcc'),'-DCMAKE_CUDA_ARCHITECTURES=120']
        rows += [('compiler_'+kind,[compiler,'--version'],0),('configure_'+kind,configure,0),
            ('prepin_'+kind,[sys.executable,'-B',str(HERE/'run.py'),'--prepin',state['builds'][kind],kind],0),
            ('build_'+kind,['cmake','--build',state['builds'][kind],'--target',base.TARGET,'-j2'],0),
            ('gate_'+kind,[state['binaries'][kind]],0)]
    rows.append(('api_cpp17',api_compile(),0))
    for label in MUTANTS:
        binary=str(Path(state['builds']['release'])/('mutant_'+label.lower()))
        argv=mutant_compile(state['builds']['release'],label)
        rows += [('compile_mutant_'+label.lower(),argv,0),('mutant_'+label.lower(),[binary],2)]
    rows += [('refuse_usage',[state['binaries']['release'],'--unknown'],2),
             ('refuse_stub',[state['binaries']['release'],'--cuda'],2)]
    return rows

OLD_BUILD_PINS=base.build_pins
def build_pins(state):
    pins=OLD_BUILD_PINS(state)
    for label in MUTANTS:
        path=Path(state['builds']['release'])/('mutant_'+label.lower());pins[str(path)]=sha(path)
    for kind,folder in state['builds'].items():
        build=Path(folder).resolve();before=check_prepin(build,kind)
        pins.update(before['headers'])
        for name in ('compile_commands.json','headers_before.json'): pins[str(build/name)]=sha(build/name)
        depfiles=list(build.glob('CMakeFiles/'+base.TARGET+'.dir/**/*.o.d'))
        need(len(depfiles)==5,'five compiled dependency files')
        for path in depfiles:
            pins[str(path)]=sha(path)
            for dependency in dependency_paths(path.read_text(),build):
                need(dependency in before['headers'],'compiled dependency absent before build: '+dependency)
                need(sha(Path(dependency))==before['headers'][dependency],'compiled dependency changed')
    return pins

def readback(directory):
    state=read(directory/'capture.json');need(state['schema']==base.SCHEMA and state['status']=='completed' and state['GCP_used'] is False,'closed scope')
    need(set(state['builds'])==set(state['binaries'])=={'release','sanitize','cuda'},'exact three build profiles')
    need(all(state['binaries'][kind]==str(Path(folder)/base.TARGET) for kind,folder in state['builds'].items()),'binary belongs to build')
    need(state['cases']==[] and state['partitions']==[],'no input campaign');need(state['pins_before']==state['pins_after']==sources(),'LIVE source closure')
    need(state['build_pins']==build_pins(state),'LIVE binaries and compiled headers')
    planned=recipe(state);need(len(planned)==len(state['commands']),'all commands')
    gates=[];previous_end=None
    for row,(name,argv,exitcode) in zip(state['commands'],planned):
        need(row['name']==name and row['argv']==argv and row['exit_code']==exitcode and row['group_closed'] and
             row['ended_epoch']>=row['started_epoch'],'exact command closure')
        need(previous_end is None or row['started_epoch']>=previous_end,'commands and prepin precede compilation')
        previous_end=row['ended_epoch']
        need(row==read(directory/(name+'.command.json')),'stored command')
        need(all(row.get(k)==v for k,v in read(directory/(name+'.intent.json')).items()),'intent')
        for stream in ('stdout','stderr'): need(sha(directory/(name+'.'+stream))==row[stream+'_sha256'],'stream pin')
        stdout=(directory/(name+'.stdout')).read_text();stderr=(directory/(name+'.stderr')).read_text()
        if name.startswith('gate_'):
            value=read(directory/(name+'.stdout'))
            need(value['schema']==base.PROBE and value['status']=='passed' and value['cuda_executed'] is False and not stderr,'portable gate only')
            counters=('collector_cases','refused','cases','calls','queries','S','reordered','empty','zero_output',
                      'sparse_waves','high_keys','growths','max_array_bytes','planned','fallbacks')
            need(set(value)==set(counters)|{'schema','status','cuda_executed'} and
                 all(type(value[field]) is int and value[field]>0 for field in counters),'exact typed gate fields')
            need(value['cases']==85 and value['calls']==1360 and value['collector_cases']==18 and value['refused']==33,'gate matrix')
            for field in ('queries','S','reordered','empty','zero_output','sparse_waves','high_keys','growths','max_array_bytes','planned','fallbacks'):
                need(type(value[field]) is int and value[field]>0,'nonvacuity '+field)
            gates.append(value)
        if name=='provenance': need(read(directory/(name+'.stdout'))==provenance.check() and not stderr,'explicit source port')
        if name=='port_diff': need(stdout==provenance.diff_text() and not stderr,'complete copy diff')
        if name.startswith('prepin_'):
            kind=name.removeprefix('prepin_');before=check_prepin(Path(state['builds'][kind]),kind)
            need(read(directory/(name+'.stdout'))==before and not stderr,'before-build stream binding')
            for path,digest in before['headers'].items():
                if path in state['pins_before']: need(digest==state['pins_before'][path],'precompiled source identity')
        if name.startswith('mutant_'): need(not stdout and stderr.strip()==MUTANTS[name.removeprefix('mutant_').upper()],'causal mutant')
        if name=='refuse_usage': need(not stdout and stderr.strip()=='survivors_probe.usage','CLI refusal')
        if name=='refuse_stub': need(not stdout and stderr.strip()=='resident.CUDA_not_compiled','no fake GPU')
    need(len(gates)==3 and gates[0]==gates[1]==gates[2],'Release/San/CUDA-linked portable identity')
    return dict(status='PASS',commands=len(planned),gate=gates[0],CUDA_compiled=True,CUDA_executed=False,GCP_used=False,
        scope='output_only_portable_gate_and_CUDA_compile_no_GPU_no_FULL',mutants=2,source_port=provenance.check())

base.sources,base.recipe,base.build_pins,base.readback=sources,recipe,build_pins,readback
base.cases=lambda:([],{},[])
if __name__=='__main__':
    if len(sys.argv)==4 and sys.argv[1]=='--prepin': prepin(Path(sys.argv[2]),sys.argv[3])
    else: base.main()
