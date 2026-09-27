#!/usr/bin/env python3
"""Faults against copies of a closed receipt; never writes its origins/builds."""
import argparse
import importlib.util
import json
import os
from pathlib import Path
import shutil
import sys
import tempfile
sys.dont_write_bytecode=True
HERE=Path(__file__).resolve().parent
sys.path.insert(0,str(HERE.parent))
import run
need,sha,read=run.need,run.sha,run.read
QUAL=HERE.parent/'checks/r1'
RUN_SHA='a9b75fca4c0598b22841b8dd57a91a120dc475f2fb84d558aad75b7d1abc8156'
QUAL_SHA='d2c3c0813ac32bbbfd0b4e1c7d58e207395e7553090b33e40cbc016a39c23585'
def pins():
    need(sha(HERE.parent/'run.py')==RUN_SHA and sha(QUAL/'capture.json')==QUAL_SHA,'frozen reader and qualification')
    return {str(p):sha(p) for p in (Path(__file__).resolve(),HERE.parent/'run.py',QUAL/'capture.json',run.base.HELPER)}
def save(path,value):
    path.write_text(json.dumps(value,sort_keys=True,indent=2,allow_nan=False)+'\n')
def repin_stream(directory,state,name,stream,content):
    path=directory/(name+'.'+stream);path.write_text(content)
    row=next(x for x in state['commands'] if x['name']==name)
    row[stream+'_sha256']=sha(path);save(directory/(name+'.command.json'),row)
def mutate(directory,label):
    state=read(directory/'capture.json')
    if label=='failed': state['status']='failed';reason='closed scope'
    elif label=='missing_profile': del state['builds']['cuda'];reason='exact three build profiles'
    elif label=='wrong_binary': state['binaries']['cuda']+='_invented';reason='binary belongs to build'
    elif label=='source_changed': state['pins_after'][next(iter(state['pins_after']))]='0'*64;reason='LIVE source closure'
    elif label=='missing_command': state['commands'].pop();reason='all commands'
    elif label=='prepin_after_build':
        row=next(x for x in state['commands'] if x['name']=='build_release')
        prior=next(x for x in state['commands'] if x['name']=='prepin_release')
        row['started_epoch']=prior['started_epoch'];reason='commands and prepin precede compilation'
    elif label in ('float_count','extra_field','fake_cuda'):
        value=read(directory/'gate_release.stdout')
        if label=='float_count': value['cases']=85.0;reason='exact typed gate fields'
        elif label=='extra_field': value['invented']=1;reason='exact typed gate fields'
        else: value['cuda_executed']=True;reason='portable gate only'
        repin_stream(directory,state,'gate_release','stdout',json.dumps(value)+'\n')
    elif label=='forged_dependency_inventory':
        value=read(directory/'prepin_release.stdout');value['headers'].pop(next(iter(value['headers'])))
        repin_stream(directory,state,'prepin_release','stdout',json.dumps(value)+'\n');reason='before-build stream binding'
    elif label=='mutant_crash_reason':
        repin_stream(directory,state,'mutant_low32','stderr','Segmentation fault\n');reason='causal mutant'
    else: raise ValueError('unknown mutation')
    save(directory/'capture.json',state);return reason
LABELS=('failed','missing_profile','wrong_binary','source_changed','missing_command','prepin_after_build',
        'float_count','extra_field','fake_cuda','forged_dependency_inventory','mutant_crash_reason')
def exercise():
    before=pins();control=run.readback(QUAL);rejected=[]
    for label in LABELS:
        with tempfile.TemporaryDirectory(prefix='mhgp9-survivors-reader-') as temporary:
            copy=Path(temporary)/'receipt';shutil.copytree(QUAL,copy)
            expected=mutate(copy,label)
            try: run.readback(copy)
            except ValueError as error:
                need(str(error)==expected,'wrong refusal for '+label+': '+str(error))
                rejected.append(dict(mutation=label,reason=str(error)))
            else: raise ValueError('accepted mutation '+label)
    need(pins()==before,'originals unchanged by faults')
    return dict(status='PASS',positive=1,rejections=len(rejected),faults=rejected,CUDA_executed=False,GCP_used=False,
                control_gate=control['gate'],pins=before)
def recipe():
    return [(name,[sys.executable,*flags,'-B',str(Path(__file__).resolve()),'--exercise'])
            for name,flags in (('normal',[]),('optimized',['-O']))]
def readback(directory):
    state=read(directory/'capture.json');need(state['status']=='completed' and state['pins_before']==state['pins_after']==pins(),'reader test closure')
    need(len(state['commands'])==2,'two reader tests');results=[];end=None
    for row,(name,argv) in zip(state['commands'],recipe()):
        need(row['name']==name and row['argv']==argv and row['exit_code']==0 and row['group_closed'] and
             row['ended_epoch']>=row['started_epoch'] and (end is None or row['started_epoch']>=end),'test command closure')
        end=row['ended_epoch'];need(row==read(directory/(name+'.command.json')),'stored command')
        need(all(row.get(k)==v for k,v in read(directory/(name+'.intent.json')).items()),'intent binding')
        for stream in ('stdout','stderr'): need(sha(directory/(name+'.'+stream))==row[stream+'_sha256'],'test stream hash')
        need(not (directory/(name+'.stderr')).read_text(),'test diagnostic')
        value=read(directory/(name+'.stdout'));need(value['status']=='PASS' and value['positive']==1 and
            value['rejections']==len(LABELS) and [r['mutation'] for r in value['faults']]==list(LABELS) and
            value['pins']==pins() and value['CUDA_executed'] is False and value['GCP_used'] is False,'test inventory')
        results.append(value)
    need(results[0]==results[1],'normal and optimized equality')
    need(results[0]['control_gate']==run.readback(QUAL)['gate'],'LIVE control remains valid')
    return dict(status='PASS',commands=2,positive_per_command=1,refusals_per_command=len(LABELS),pins=pins())
def capture(directory):
    directory=directory.resolve();need(not directory.exists(),'fresh reader test capture');directory.mkdir(parents=True)
    before=pins();state=dict(status='failed',pins_before=before)
    need(sha(run.base.HELPER)==run.base.HELPER_SHA,'collector helper')
    spec=importlib.util.spec_from_file_location('survivors_reader_process',run.base.HELPER)
    helper=importlib.util.module_from_spec(spec);spec.loader.exec_module(helper)
    commands=helper.Commands(directory,dict(os.environ,PYTHONDONTWRITEBYTECODE='1'))
    try:
        for name,argv in recipe(): need(commands.run(name,argv,timeout=None)[0]==0,'reader test '+name)
        state['status']='completed'
    finally:
        state['commands']=commands.rows;state['pins_after']=pins();save(directory/'capture.json',state)
    value=readback(directory);save(directory/'summary.json',value);print(json.dumps(value,sort_keys=True))
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);actions=parser.add_mutually_exclusive_group(required=True)
    actions.add_argument('--exercise',action='store_true');actions.add_argument('--capture',type=Path);actions.add_argument('--readback',type=Path)
    args=parser.parse_args()
    if args.exercise: print(json.dumps(exercise(),sort_keys=True))
    elif args.capture: capture(args.capture)
    else: print(json.dumps(readback(args.readback.resolve()),sort_keys=True))
