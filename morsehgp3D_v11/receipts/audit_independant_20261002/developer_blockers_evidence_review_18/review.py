#!/usr/bin/env python3
"""Read copied session receipts; toy collectors are fully mocked. No native/build/cloud execution."""
import argparse
import copy
import hashlib
import importlib.util
import json
from pathlib import Path, PurePosixPath
import re
import sys
import tarfile
import tempfile
from types import SimpleNamespace
from unittest.mock import patch

HERE=Path(__file__).resolve().parent

def need(condition,message):
    if not condition:
        raise ValueError(message)

def load(path):
    return json.loads(path.read_bytes())

def sha(data):
    return hashlib.sha256(data).hexdigest()

def fields(data):
    return dict(line.split('=',1) for line in data.decode().splitlines() if '=' in line)

def archive(folder,path):
    receipt=load(folder/'receipt.json')
    raw=path.read_bytes()
    need(len(raw)==receipt['results_bytes'] and sha(raw)==receipt['results_sha256'],'archive receipt hash')
    out={};expanded=0;seen=set()
    with tarfile.open(path,'r:gz') as tar:
        for m in tar:
            p=PurePosixPath(m.name)
            need(not p.is_absolute() and '..' not in p.parts and (m.isfile() or m.isdir()),'tar member')
            need(m.name not in seen,'duplicate tar member');seen.add(m.name)
            if m.isfile():
                out[m.name]=tar.extractfile(m).read();expanded+=m.size
    need(expanded==receipt['results_expanded_bytes'],'expanded archive size')
    names=set()
    for line in out['results/MANIFEST.sha256'].decode().splitlines():
        h,name=line.split('  ',1);name='results/'+name.removeprefix('./')
        need(name in out and name not in names and sha(out[name])==h,'manifest hash');names.add(name)
    need(names==set(out)-{'results/MANIFEST.sha256'},'manifest exhaustive')
    worker=fields(out['results/worker.txt'])
    need(worker['source']=='commit:'+receipt['commit'] and worker['generation']==receipt['generation'] and
         worker['package_sha256']==receipt['package_sha256'],'source worker')
    need(receipt['closure']=='stopped' and receipt['targeted_shutdown_certified'] is True and
         receipt['generation']==receipt['closing_generation']==receipt['observed_after']['lastStartTimestamp'] and
         receipt['observed_after']['status']=='TERMINATED' and receipt['stop_exit_code']==0 and not receipt['errors'],
         'target closure')
    return receipt,worker,out,dict(files=len(out),manifest_entries=len(names),expanded_bytes=expanded,sha256=sha(raw))

def stockout(index):
    folder=HERE/('raw/v11.20261002.parallel%d'%index)
    receipt=load(folder/'receipt.json');external=load(folder/'external_closure.json')
    need(receipt['status']=='shutdown_uncertified' and receipt['closure']=='generation_unknown' and
         receipt['generation'] is None and receipt['targeted_shutdown_certified'] is False and
         receipt['worker_launched'] is False and receipt['results_verified'] is False and
         (folder/'DONE').read_text().strip()=='74','preserve start failure')
    stderr=(folder/'host/logs/002_guarded_start.stderr').read_text()
    need('ZONE_RESOURCE_POOL_EXHAUSTED_WITH_DETAILS' in stderr and 'us-central1-c' in stderr,'stockout cause')
    observations=[]
    for row in external['repeated_observations']:
        if 'instance' in row:
            facts=row['instance'];ops=row['operations']
            value=dict(status=facts['status'],generation=facts['lastStartTimestamp'],
                       pending=sum(o['status']!='DONE' for o in ops),operations=len(ops))
        else:
            value={k:row[k] for k in ('status','generation','pending')}
        need(value['status']=='TERMINATED' and value['generation']==receipt['pre_start_generation'] and
             value['pending']==0,'external observations')
        observations.append(value)
    need(len(observations)==3 and external['session_status_unchanged']==receipt['status'],'external scope')
    return dict(commit=receipt['commit'],cause='zone capacity: G4 unavailable in us-central1-b',
                worker_launched=False,initial_status=receipt['status'],initial_DONE=74,
                generation_certified=False,external_conclusion=external['conclusion'],
                observations=observations,scope='read-only repeated observations, not a certified new generation shutdown')

def parallel_bootstrap():
    folder=HERE/'raw/v11.20261002.parallel3'
    receipt,worker,data,inventory=archive(folder,folder/'results.tar.gz')
    main=load_bytes(data['results/cmd/000_matrice/files/matrix/summary.json'])
    supplement=load_bytes(data['results/cmd/001_asan18/files/matrix/summary.json'])
    configs=main['configurations'];required=[c for c in configs if not c['optional']]
    need(main['complete'] is True and main['conforming'] is False and main['exit_code']==1 and
         len(configs)==9 and len(required)==8 and
         all(c['status']=='requirement_missing' and 'g++' in c['reason'] and c['steps']==[] and
             'tests' not in c for c in required),'missing tool is not native failure')
    clang=next(c for c in configs if c['name']=='clang_release')
    need(clang['status']=='absent' and clang['optional'] is True,'clang optional')
    need(supplement['conforming'] is False and supplement['exit_code']==1 and
         supplement['configurations'][0]['status']=='requirement_missing','supplement blocked')
    need(all(main['host'][k]=='absent' for k in ('gxx','cmake','ctest','clangxx')),'host inventory absent')
    meta=[]
    for name,code in [('000_matrice',1),('001_asan18',1),('002_parallel',2)]:
        row=fields(data['results/cmd/'+name+'/meta.txt'])
        need(row['status']=='failed' and row['exit_code']==str(code) and row['group_closed']=='1' and
             row['residual_group_killed']=='0' and row['streams_truncated']=='0','failed command preserved')
        meta.append(dict(name=name,exit_code=code,wall_seconds=row['wall_seconds'],group_closed=True))
    need(worker['status']=='failed' and worker['commands_ok']=='0' and receipt['worker_exit_code']==1 and
         receipt['status']=='failed_remote' and (folder/'DONE').read_text().strip()=='3','bootstrap campaign verdict')
    need(not any(p.endswith('/parallel.json') for p in data),'no native benchmark report')
    return dict(commit=receipt['commit'],source_kind=receipt['source_kind'],status=receipt['status'],
                cause='new VM image lacks g++, cmake, ctest; no configure/build/native gate ran',
                configurations_blocked=8,supplement_blocked=1,clang_optional_absent=True,
                native_gates_selected=0,native_gates_played=0,benchmarks_played=0,
                commands=meta,archive=inventory,generation=receipt['generation'],
                target=receipt['target'],target_shutdown_certified=True)

def load_bytes(data):
    return json.loads(data)

def tooling():
    folder=HERE/'raw/v11.20261002.tools1'
    receipt,worker,data,inventory=archive(folder,folder/'results/results.tar.gz')
    host=load_bytes(data['results/cmd/000_host_tools/files/host_tools.json'])
    meta=fields(data['results/cmd/000_host_tools/meta.txt'])
    need(receipt['status']=='completed' and receipt['worker_exit_code']==0 and
         (folder/'DONE').read_text().strip()=='0' and worker['status']=='completed','tools session success')
    need(host['schema']=='ehgp.v11.host_tools.v1' and host['status']=='ready' and host['product_executed'] is False,
         'tools scope')
    identity=host['identity'];target=receipt['target']
    need(identity['project/project-id']==target['project'] and identity['instance/name']==target['instance'] and
         identity['instance/zone'].rsplit('/',1)[-1]==target['zone'] and
         identity['instance/machine-type'].rsplit('/',1)[-1]=='g4-standard-48','tools target identity')
    tools=('g++','cmake','ctest','make','/usr/bin/time')
    need(host['packages']==['cmake','g++','make'] and all(host['before'][k]['path'] is None for k in tools[:-1]),
         'missing packages')
    need(all(host['after'][k]['path'] and host['after'][k]['exit_code']==0 for k in tools),'after tool inventory')
    need(len(host['commands'])==2 and all(c['state']=='returned' and c['exit_code']==0 for c in host['commands']),
         'apt outcomes')
    need(meta['status']=='ok' and meta['exit_code']=='0' and meta['group_closed']=='1' and
         meta['residual_group_killed']=='0' and meta['streams_truncated']=='0','tools group closure')
    plan=load(HERE/'sources_git/morsehgp3D_v11/bench/plans/host_tools_g4.json')
    need(plan==load(folder/'package/plan.json') and plan['default_build'] is False and
         plan['commands'][0]['timeout_seconds']==720,'source tool plan')
    package=load(HERE/'tools_package_before.json')
    need(package['sha256']==receipt['package_sha256'] and all(r['exact_captured_git'] for r in package['selected_sources']),
         'selected code packet identity')
    return dict(commit=receipt['commit'],status='ready',product_executed=False,qualification_transferred=False,
                installed_packages=host['packages'],before_missing=list(tools[:-1]),
                after_versions={k:host['after'][k]['version'].splitlines()[0] for k in tools},
                wall_seconds=meta['wall_seconds'],group_closed=True,residual_group_killed=False,
                generation=receipt['generation'],target=receipt['target'],target_shutdown_certified=True,
                archive=inventory,package_hash_only=package['sha256'],selected_packet_sources_exact_git=3)

def intent_probe():
    bench=HERE/'sources_git/morsehgp3D_v11/bench';sys.path.insert(0,str(bench))
    import catalogue_profiles as profiles
    import catalogue_parallel as parallel
    cases=[dict(name=name,count=count,coordinates=name+'.xyz',point_ids=name+'.ids',
                sha256='1'*64,ids_sha256='2'*64) for name,count in profiles.COUNTS.items()]
    manifest=dict(cases=cases);results=[]
    for name,driver,filename in [('profiles',profiles,'profiles.json'),('parallel',parallel,'parallel.json')]:
        entered=[]
        def interrupt(argv,**_kwargs):
            entered.append(argv);raise KeyboardInterrupt('toy only: no native process launched')
        with tempfile.TemporaryDirectory(prefix='mhgp11-intent-review18-') as temp:
            root=Path(temp);args=SimpleNamespace(out=root/'out',work=root/'work',data=root/'data',
                                               builds=root/'builds',qualification=root/'q.json',
                                               supplement=root/'s.json',budget_seconds=750)
            builds={bits:dict(coord_bits=bits,path=str(root/('never_launch_b%d'%bits))) for bits in profiles.PROFILES}
            with patch.object(profiles,'checked_builds',return_value=builds), \
                 patch.object(profiles,'checked_supplement',return_value='0'*64), \
                 patch.object(profiles,'inputs',return_value=(manifest,'3'*64)), \
                 patch.object(profiles.base,'digest',return_value='0'*64), \
                 patch.object(profiles.subprocess,'run',side_effect=interrupt), \
                 patch.object(profiles.subprocess,'Popen',side_effect=RuntimeError('native forbidden')):
                try:driver.run(args)
                except KeyboardInterrupt:pass
                else:raise ValueError('toy interruption absent')
            report=load(args.out/filename);intents=report['launch_intents']
            need(len(entered)==1 and len(intents)==1 and report['runs']==[] and report['complete'] is False,
                 'new intent checkpoint')
            intent=intents[0]
            need(intent['argv']==entered[0] and intent['input_sha256']=='1'*64 and intent['ids_sha256']=='2'*64 and
                 'does not prove child spawned' in intent['scope'],'honest launch intention')
            results.append(dict(collector=name,persisted_intents=1,persisted_results=0,native_processes_launched=0,
                                input_hashes_present=True,argv_present=True,identity={k:intent[k] for k in
                                ('case','coord_bits','kmax','workers','repetition')},scope=intent['scope']))
    return results

def copies():
    metadata=('sources_before.json','guard_logs_before.json','tools_before.json','build_contract_before.json',
              'probe_dependencies_before.json')
    for name in metadata:
        for row in load(HERE/name)['files']:
            if row.get('copy') is None:continue
            data=(HERE/row['copy']).read_bytes()
            need(len(data)==row['bytes'] and sha(data)==row['sha256'],'source copy changed')

def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();copies()
    result=dict(scope='pure Python copied evidence and mocked collector only; zero native/build/GCP calls',
                stockout=[stockout(i) for i in (1,2)],parallel3=parallel_bootstrap(),tools1=tooling(),
                intent_checkpoint_after_fix=intent_probe())
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(json.dumps(result,indent=2,sort_keys=True)+'\n')
    print('blockers_review_ok stockout2 bootstrap_fail1 tools_ready1 product_runs0 intent_probes2')

if __name__=='__main__':main()
