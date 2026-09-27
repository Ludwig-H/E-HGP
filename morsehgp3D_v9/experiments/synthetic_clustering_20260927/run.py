#!/usr/bin/env python3
"""Run the whole predeclared synthetic QUALITY grid, with owned workers.

The 8k/16k/32k inputs remain a separate, explicitly not-run growth plan.
No old scores are reused. No unit is silently truncated, resumed or dropped.
"""
from __future__ import annotations

import argparse
import json
import os
from pathlib import Path
import signal
import subprocess
import sys
import time
import traceback

HERE=Path(__file__).resolve().parent
WEIGHTED=HERE.parent/'weighted_clustering_20260927'
POINT=HERE.parent/'point_dendrogram_20260927'
sys.path[:0]=[str(POINT),str(WEIGHTED)]
from plan import PLAN
from prepare import sha,need,save
from benchmark_point_dendrogram import validate_qualification
from benchmark_full_weighted_parallel_r2 import enable_subreaper,defer_signals,close_group,THREAD_ENV

QUALIFICATION=Path('/tmp/mhgp9-point-dendrogram-qualification-20260927-ltxbaw2x/receipt.json')
NATIVE=Path('/workspaces/E-HGP/build/v9-weighted-attachments-native-20260927-r1/native_weighted_export')
NATIVE_SHA='c7afdae542074ee90bb8c1ba5a7dfe679ef9851f91bf32bbcbc067b15853576e'
SCHEMA='mhgp9_variable_synthetic_quality_capture_v1'


def read(path):
    return json.loads(Path(path).read_text())


def check_pins(pins):
    for path,digest in pins.items():
        need(sha(path)==digest,'LIVE pin changed: '+path)


def write_receipt(path,value):
    # Checkpoints in this newly created capture only.
    with path.open('w') as stream:
        json.dump(value,stream,indent=2,sort_keys=True,allow_nan=False);stream.write('\n')


def sources():
    paths=[HERE/name for name in ('run.py','unit_pipeline.py','prepare.py','plan.py','synthetic_data.py','PLAN.md')]
    for folder in (WEIGHTED,POINT,HERE.parents[1]/'audits/b_point_hierarchy_k_20260927',
                   HERE.parents[1]/'audits/b_gaussian_point_clustering_20260927'):
        paths+=list(folder.glob('*.py'))
    paths.append(Path(sys.executable).resolve())
    return {str(path):sha(path) for path in paths}


def preflight(manifest_path):
    manifest=read(manifest_path);prepared=read(manifest_path.parent/'receipt.json')
    need(manifest['schema']=='mhgp9_synthetic_cluster_input_manifest_v1' and manifest['plan']==PLAN,'fixed synthetic plan')
    need(prepared['status']=='completed' and prepared['manifest_sha256']==sha(manifest_path) and
         prepared['sources_before']==prepared['sources_after'],'closed prepared inputs')
    need([c['id'] for c in manifest['cases']]==[c['id'] for c in PLAN['cases']],'exact input case order')
    pins=validate_qualification(QUALIFICATION)
    additions=dict(prepared['sources_after'])
    additions.update({str(manifest_path):sha(manifest_path),str(manifest_path.parent/'receipt.json'):sha(manifest_path.parent/'receipt.json'),
                      str(manifest_path.parent/'plan.json'):manifest['plan_sha256'],str(NATIVE):NATIVE_SHA})
    for case,planned in zip(manifest['cases'],PLAN['cases']):
        need(all(case.get(key)==value for key,value in planned.items()),'case metadata differs from plan')
        need(case['n']==planned['spec']['n'] and case['groups']==planned['spec']['groups'],'case dimensions differ from plan')
        need(all(case['prepared_sha256'][key]==case['files'][case[key]]
                 for key in ('points_u32le','points_npy','labels_json')),'prepared digest aliases')
        additions.update(case['files'])
        path=manifest_path.parent/case['id']/'case.json'
        need(read(path)==case,'prepared case receipt binding')
        additions[str(path)]=sha(path)
    for path,digest in additions.items():
        need(path not in pins or pins[path]==digest,'conflicting inherited pin')
        pins[path]=digest
    check_pins(pins)
    return manifest,pins


def expected_grid():
    result=set()
    for case in PLAN['cases']:
        if case['phase']!='quality':continue
        for m in (20,50):
            for z in (1,2):
                for method in PLAN['quality']['methods']:
                    if method!='hdbscan_standard' or z==1:
                        result.add((case['id'],5,m,z,method))
    return result


def validate_unit(unit,case):
    need(unit.get('schema')=='mhgp9_synthetic_clustering_unit_v1' and
         unit.get('status')=='completed' and unit.get('case')==case['id'] and
         unit.get('n')==case['n'] and unit.get('k')==5,'complete bound unit')
    keys=[tuple(row[key] for key in ('case','k','min_cluster_size','exp_z','method'))
          for row in unit['rows']]
    need(len(keys)==18 and set(keys)=={key for key in expected_grid() if key[0]==case['id']},
         'exact unique unit 18-row grid')


def worker(spec_path):
    from threadpoolctl import threadpool_limits
    from unit_pipeline import run_unit
    spec=read(spec_path)
    check_pins(spec['source_pins']);check_pins(spec['case']['files'])
    need(sha(spec['native_binary'])==NATIVE_SHA,'qualified native worker')
    with threadpool_limits(limits=1):
        result=run_unit(spec['case'],Path(spec['output']),Path(spec['native_binary']))
    validate_unit(result,spec['case'])
    check_pins(spec['source_pins']);check_pins(spec['case']['files'])


def run(manifest_path,output,workers):
    need(type(workers)is int and 1<=workers<=2,'one or two whole-scene workers')
    need(not output.exists(),'NEW capture directory required')
    output.mkdir(parents=True)
    receipt=dict(schema=SCHEMA,status='running',argv=sys.argv,cwd=str(Path.cwd()),
        plan=PLAN,manifest=str(manifest_path),manifest_sha256=sha(manifest_path),
        sources_before=sources(),workers=workers,worker_environment=THREAD_ENV,
        native_binary=str(NATIVE),native_binary_sha256=NATIVE_SHA,
        worker_commands=[],events=[],cleanup=[],units=[],rows=[],
        GCP_used=False,GPU_used=False,growth_executed=False,
        timing_scope='diagnostic_CPU_export_Python_pipeline_not_native_FULL_baseline',
        full_observer_workers=2,native_requested_workers=1)
    save(output/'intent.json',receipt)
    started=time.monotonic();active={};jobs=[]
    try:
        manifest,pins=preflight(manifest_path);receipt['inherited_pins']=pins
        jobs=[case for case in manifest['cases'] if case['phase']=='quality']
        cases_by_id={case['id']:case for case in jobs}
        need(len(jobs)==34,'all and only 34 quality cases')
        enable_subreaper();environment=dict(os.environ,**THREAD_ENV,PYTHONDONTWRITEBYTECODE='1')
        cursor=0;done={}
        while cursor<len(jobs) or active:
            while cursor<len(jobs) and len(active)<workers:
                case=jobs[cursor];cursor+=1;name=case['id'];folder=output/name
                spec_path=output/(name+'.spec.json')
                spec=dict(case=case,output=str(folder),native_binary=str(NATIVE),source_pins=receipt['sources_before'])
                save(spec_path,spec)
                argv=[sys.executable,'-B',str(Path(__file__).resolve()),'--worker',str(spec_path)]
                out=(output/(name+'.stdout')).open('xb');err=(output/(name+'.stderr')).open('xb')
                try:
                    with defer_signals():
                        process=subprocess.Popen(argv,stdout=out,stderr=err,cwd=Path.cwd(),env=environment,start_new_session=True)
                        active[name]=dict(process=process,argv=argv,case=name,k=5,pid=process.pid,
                                          started=time.monotonic(),stdout=out,stderr=err)
                        receipt['events'].append(dict(event='start',case=name,k=5,pid=process.pid,argv=argv))
                except BaseException:
                    if name not in active:out.close();err.close()
                    raise
                print('START',name,flush=True)
            for name,job in list(active.items()):
                process=job['process']
                if process.poll() is None:continue
                process.wait();job['stdout'].close();job['stderr'].close()
                command={key:job[key] for key in ('case','k','pid','argv')}
                command.update(returncode=process.returncode,elapsed_seconds=time.monotonic()-job['started'],
                    stdout_sha256=sha(output/(name+'.stdout')),stderr_sha256=sha(output/(name+'.stderr')))
                receipt['worker_commands'].append(command);receipt['events'].append(dict(event='joined',**command))
                cleanup=close_group(process);receipt['cleanup'].append(cleanup);del active[name]
                need(process.returncode==0,'worker failed: '+name)
                unit_path=output/name/'receipt.json'
                unit=read(unit_path)
                validate_unit(unit,cases_by_id[name])
                done[name]=unit
                receipt['units']=[done[c['id']] for c in jobs if c['id'] in done]
                receipt['rows']=[row for unit in receipt['units'] for row in unit['rows']]
                write_receipt(output/'receipt.json',receipt)
                print('DONE',name,'rows',len(receipt['rows']),flush=True)
            if active:time.sleep(.1)
        keys=[tuple(row[key] for key in ('case','k','min_cluster_size','exp_z','method')) for row in receipt['rows']]
        need(len(keys)==612 and set(keys)==expected_grid(),'exact unique whole 612-row grid')
        check_pins(pins);check_pins(receipt['sources_before'])
        receipt['sources_after']=sources();need(receipt['sources_after']==receipt['sources_before'],'source closure')
        receipt['status']='completed'
    except BaseException as error:
        receipt.update(status='failed',error=repr(error),traceback=traceback.format_exc())
        raise
    finally:
        cleanup_errors=[]
        for name,job in list(active.items()):
            try:
                receipt['cleanup'].append(dict(case=name,**close_group(job['process'])))
            except BaseException as error:
                cleanup_errors.append(dict(case=name,error=repr(error)))
            finally:job['stdout'].close();job['stderr'].close()
        if cleanup_errors:
            receipt['cleanup_errors']=cleanup_errors;receipt['status']='failed'
        closure_errors=[]
        try:
            receipt['sources_after']=sources()
            if receipt['sources_after']!=receipt['sources_before']:
                closure_errors.append('source changed')
        except BaseException as error:
            closure_errors.append('source closure: '+repr(error))
        receipt['elapsed_seconds']=time.monotonic()-started
        receipt['artifacts']={}
        try:
            for path in output.rglob('*'):
                if path.is_file() and path!=output/'receipt.json':
                    try:receipt['artifacts'][str(path)]=sha(path)
                    except BaseException as error:closure_errors.append('artifact '+str(path)+': '+repr(error))
        except BaseException as error:
            closure_errors.append('artifact inventory: '+repr(error))
        if closure_errors:
            receipt['closure_errors']=closure_errors;receipt['status']='failed'
        write_receipt(output/'receipt.json',receipt)
    need(receipt['status']=='completed','capture closure failed')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--worker',type=Path)
    parser.add_argument('--manifest',type=Path);parser.add_argument('--output',type=Path)
    parser.add_argument('--workers',type=int,default=2)
    args=parser.parse_args()
    signal.signal(signal.SIGTERM,lambda signum,frame:(_ for _ in ()).throw(InterruptedError('SIGTERM')))
    if args.worker:worker(args.worker.resolve())
    else:
        need(args.manifest is not None and args.output is not None,'manifest/output required')
        run(args.manifest.resolve(),args.output.resolve(),args.workers)
