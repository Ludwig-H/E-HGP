#!/usr/bin/env python3
"""Causal corruptions of private copies, not changes to sealed build origins."""
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
need,sha,read,save=run.need,run.sha,run.read,run.save
QUAL=HERE.parent/'checks/r1'
RUN_SHA='d81fd3c23892ee9a7ff22dc721787050b050e7563fc4c5fe2fdff9b015a958ce'
QUAL_SHA='fcb2afcb7b4627c2e6a9e55d1671111b508fe59273687d8ea56d6f560f05723c'
LABELS=('failed','fake_scope','fake_gate_cuda','float_count','prepin_after_build','forged_response_inventory','missing_object_pin','fake_cli_crash')
def pins():
    need(sha(HERE.parent/'run.py')==RUN_SHA and sha(QUAL/'capture.json')==QUAL_SHA,'frozen origins')
    return {str(p):sha(p) for p in (Path(__file__).resolve(),HERE.parent/'run.py',QUAL/'capture.json',run.HELPER)}
def stream(directory,state,name,which,text):
    path=directory/(name+'.'+which);path.write_text(text)
    row=next(x for x in state['commands'] if x['name']==name);row[which+'_sha256']=sha(path)
    save(directory/(name+'.command.json'),row)
def mutate(directory,label):
    state=read(directory/'capture.json')
    if label=='failed': state['status']='failed';reason='closed local compile/host scope'
    elif label=='fake_scope': state['CUDA_executed']=True;reason='closed local compile/host scope'
    elif label in ('fake_gate_cuda','float_count'):
        value=read(directory/'host_gate.stdout')
        if label=='fake_gate_cuda': value['cuda_executed']=True
        else: value['cases']=24.0
        stream(directory,state,'host_gate','stdout',json.dumps(value)+'\n');reason='exact typed host gate not GPU'
    elif label=='prepin_after_build':
        row=next(x for x in state['commands'] if x['name']=='build')
        row['started_epoch']=next(x for x in state['commands'] if x['name']=='prepin')['started_epoch']
        reason='prepin before build chronology'
    elif label=='forged_response_inventory':
        value=read(directory/'prepin.stdout');value['response_files'].pop(next(iter(value['response_files'])))
        stream(directory,state,'prepin','stdout',json.dumps(value)+'\n');reason='prepin stream binding'
    elif label=='missing_object_pin':
        keys=[key for key in state['build_pins'] if key.endswith('.o')];need(len(keys)==1,'one real object')
        del state['build_pins'][keys[0]];reason='LIVE build and precompiled dependency closure'
    elif label=='fake_cli_crash':
        stream(directory,state,'refuse_unknown','stderr','Segmentation fault\n');reason='causal CLI refusal'
    else: raise ValueError('unknown corruption')
    save(directory/'capture.json',state);return reason
def exercise():
    before=pins();control=run.readback(QUAL);faults=[]
    for label in LABELS:
        with tempfile.TemporaryDirectory(prefix='mhgp9-device-gate-reader-') as temporary:
            copy=Path(temporary)/'receipt';shutil.copytree(QUAL,copy);expected=mutate(copy,label)
            try: run.readback(copy)
            except ValueError as error:
                need(str(error)==expected,'wrong causal refusal '+label+': '+str(error));faults.append(dict(label=label,reason=str(error)))
            else: raise ValueError('accepted corruption '+label)
    need(pins()==before,'origins unchanged');return dict(status='PASS',positive=1,refusals=len(faults),faults=faults,
        CUDA_executed=False,GCP_used=False,control=control,pins=before)
def recipe():
    return [(name,[sys.executable,*flags,'-B',str(Path(__file__).resolve()),'--exercise'])
            for name,flags in (('normal',[]),('optimized',['-O']))]
def readback(directory):
    state=read(directory/'capture.json');need(state['status']=='completed' and state['pins_before']==state['pins_after']==pins(),'selftest closure')
    need(len(state['commands'])==2,'two selftest commands');results=[];end=None
    for row,(name,argv) in zip(state['commands'],recipe()):
        need(row['name']==name and row['argv']==argv and row['exit_code']==0 and row['group_closed'] and
            row['ended_epoch']>=row['started_epoch'] and (end is None or row['started_epoch']>=end),'selftest command closure')
        end=row['ended_epoch'];need(row==read(directory/(name+'.command.json')),'stored command')
        need(all(row.get(k)==v for k,v in read(directory/(name+'.intent.json')).items()),'intent')
        for which in ('stdout','stderr'): need(sha(directory/(name+'.'+which))==row[which+'_sha256'],'stream hash')
        need(not (directory/(name+'.stderr')).read_text(),'no selftest diagnostics');value=read(directory/(name+'.stdout'))
        need(value['status']=='PASS' and type(value['positive']) is int and value['positive']==1 and
            type(value['refusals']) is int and value['refusals']==len(LABELS) and [f['label'] for f in value['faults']]==list(LABELS) and
            value['pins']==pins() and value['CUDA_executed'] is False and value['GCP_used'] is False,'selftest inventory')
        results.append(value)
    need(results[0]==results[1],'normal optimized equality');need(results[0]['control']==run.readback(QUAL),'LIVE valid control')
    return dict(status='PASS',commands=2,positive_per_command=1,refusals_per_command=len(LABELS),pins=pins())
def capture(directory):
    directory=directory.resolve();need(not directory.exists(),'fresh selftest capture');directory.mkdir(parents=True)
    state=dict(status='failed',pins_before=pins())
    need(sha(run.HELPER)==run.FROZEN[run.HELPER],'collector helper')
    spec=importlib.util.spec_from_file_location('gate_reader_commands',run.HELPER);helper=importlib.util.module_from_spec(spec);spec.loader.exec_module(helper)
    commands=helper.Commands(directory,dict(os.environ,PYTHONDONTWRITEBYTECODE='1'))
    try:
        for name,argv in recipe(): need(commands.run(name,argv,timeout=None)[0]==0,'selftest exit '+name)
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
