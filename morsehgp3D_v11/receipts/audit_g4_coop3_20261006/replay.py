#!/usr/bin/env python3
"""Read local metadata and Git only. Never build, run HGP, connect or read cloud payloads."""
import argparse
from collections import Counter
import hashlib,io,json,subprocess,tarfile
from pathlib import Path
PIN='9eee2ed4bcef1e960cdf2456012b84416854dc20'
MODES={'cpu':'16379','gpu':'81915','gpu_coop':'212987'}
EXPECTED={5:{'lidar_ng00':'3a2bfb4f9f48b4b0cc5b0318d9fcf4887906638e034b2c97dda1918e3170a6fe','lidar_ng01':'5212a2ced81bf69bd2abfb935a3b14a35158df25339285a0d25a9d5d5c09e091','lidar_ng02':'78feb765e21c8e4582762a25bd0dd36a455747d3f0df80523e19f7ba4be0e207'},10:{'lidar_ng00':'61a4245b91d9a4fdad012f0a2e26a63c3e48db1d46a180db756f2c4e4aa77295','lidar_ng01':'838a447e0b92e13e42f7f69a84fd536d5d46d26463d3f4cc7c688475878fe0de','lidar_ng02':'81f89995eaccb497cfe92abb81213bebd0d4ce5076210570d86aa9456cf7ff2e'}}

def need(ok,msg):
    if not ok:raise ValueError(msg)
def sha(b):return hashlib.sha256(b).hexdigest()
def sha_file(p):
    h=hashlib.sha256()
    with p.open('rb') as f:
        for b in iter(lambda:f.read(1024*1024),b''):h.update(b)
    return h.hexdigest()
def member(t,n):
    m=t.getmember(n);need(m.isfile() and m.size<=16*1024*1024,'bounded metadata member')
    return t.extractfile(m).read()
def git(repo,path,pin=PIN):return subprocess.check_output(['git','-C',str(repo),'show',pin+':'+path])
def source_tree(repo,package):
    paths=['morsehgp3D_v11/'+p for p in ['src','tests','cmake','bench','reference','tools','cli']]
    exact=['morsehgp3D_v11/CMakeLists.txt','gcp-migration/v11_worker.sh']
    raw=subprocess.check_output(['git','-C',str(repo),'archive',PIN,'--',*paths,*exact])
    prefixes=tuple(p+'/' for p in paths);hashes=[]
    with tarfile.open(fileobj=io.BytesIO(raw)) as g,tarfile.open(package) as a:
        want={m.name for m in g.getmembers() if m.isfile()}
        got={m.name for m in a.getmembers() if m.isfile() and (m.name.startswith(prefixes) or m.name in exact)}
        need(want==got,'source inventory')
        for name in sorted(want):
            b=member(g,name);need(b==member(a,name),'source bytes '+name)
            hashes.append(sha(b)+'  '+name+'\n')
    return dict(files=len(want),manifest_sha256=sha(''.join(hashes).encode()),equal_to_git=True)

def derive(repo,session):
    raw=(session/'receipt.json').read_bytes();r=json.loads(raw);after=r['observed_after']
    need((session/'DONE').read_text().strip()=='0' and r['status']=='completed' and r['worker_exit_code']==0,'session result')
    need(r['closure']=='stopped' and r['targeted_shutdown_certified'] and r['start_certified'] and after['status']=='TERMINATED' and after['name']==r['target']['instance'] and r['generation']==r['closing_generation']==after['lastStartTimestamp'],'targeted generation closure')
    need(r['private_key_deleted'] and r['reserve_released'] and r['oslogin_key_removed'] and r['guest_guard_intact'],'session guards')
    need(r['commit']==PIN and r['worker_source']=='commit:'+PIN and r['source_kind']=='commit' and r['evidence_grade']=='pushed_commit','source pin')
    need(r['results_verified'] and r['data_verified_remote'] and not r['results_skipped_members'] and not r['overflow']['evicted'] and not r['overflow']['truncated_streams'] and not r['errors'],'complete capture')
    arc=session/'results/results.tar.gz';package=session/'package/package.tar.gz';planraw=(session/'package/plan.json').read_bytes();plan=json.loads(planraw)
    need(sha_file(arc)==r['results_sha256'] and sha_file(package)==r['package_sha256'] and sha(planraw)==r['plan_sha256'],'outer hashes')
    need(plan['default_build'] is False and plan['python_packages']=='none' and len(plan['commands'])==2,'plan scope')
    judge=git(repo,'morsehgp3D_v11/bench/gpu_ab.py');text=judge.decode()
    need("work == report['ledger'].get(frame)" in text and "report['verdict'] = 'conforme' if not report['refusals'] else 'refus'" in text and '-DMHGP11_COORD_BITS=21' in text and '-DCMAKE_BUILD_TYPE=Release' in text and '-DMHGP11_ENABLE_CUDA=ON' in text,'pinned judge/build scope')
    source=source_tree(repo,package);commands=[];reports=[];ranges=[];last=[];identities=[];totals=dict(lidar_processes=0,canonical_dumps_checked=0,constructed_lidar_passes=0,intermediate_passes_without_dump_or_ledger=0)
    with tarfile.open(arc) as t:
        need(sha(member(t,'results/plan.sh'))==r['worker_plan_sha256'],'worker plan bytes')
        manifest=member(t,'results/MANIFEST.sha256');entries=manifest.decode().splitlines()
        for line in entries:
            digest,name=line.split('  ',1);need(sha(member(t,'results/'+name.removeprefix('./')))==digest,'member hash')
        for index,k,reps,passes,leaf in [(0,5,5,12,16),(1,10,3,8,24)]:
            cmd=plan['commands'][index];argv=cmd['argv'];need(cmd['name']=='gpu_k'+str(k) and cmd['timeout_seconds']==1100,'command plan')
            for flag,val in [('--modes','cpu=16379,gpu=81915,gpu_coop=212987'),('--reps',str(reps)),('--workers','48'),('--warm-passes',str(passes)),('--kmax',str(k)),('--leaf',str(leaf))]:need(argv[argv.index(flag)+1]==val,'argv '+flag)
            meta=member(t,f'results/cmd/{index:03d}_gpu_k{k}/meta.txt');kv=dict(x.split('=',1) for x in meta.decode().splitlines() if '=' in x)
            need(kv['status']=='ok' and kv['exit_code']=='0' and kv['group_closed']=='1' and kv['streams_truncated']=='0','command closed')
            commands.append(dict(name=cmd['name'],exit_code=0,status='ok',wall_seconds=kv['wall_seconds'],metadata_sha256=sha(meta)))
            report_raw=member(t,f'results/cmd/{index:03d}_gpu_k{k}/files/gpu_k{k}/gpu_ab_report.json');q=json.loads(report_raw)
            need(q['modes']==MODES and q['workers']=='48' and q['reps']==reps and q['warm_passes']==passes and q['kmax']==k and q['leaf']==leaf and q['verdict']=='conforme' and not q['refusals'] and q['identity']==EXPECTED[k],'report contract')
            seen=Counter();repetitions={}
            for regime in ['cold','warm']:
                rows=q[regime];need(len(rows)==(9*reps if regime=='cold' else 9),'process inventory')
                for row in rows:
                    need(row['frame'] in EXPECTED[k] and row['mode'] in MODES and row['code']==0 and row['summary']['status']=='ok' and row['summary']['exit']=='ok' and row['dump_sha256']==EXPECTED[k][row['frame']],'raw canonical row')
                    need(row['summary']['batch']['unresolved']==0,'whole frame fallback')
                    ps=row['passes'];need(not ps if regime=='cold' else len(ps)==passes and [p['pass'] for p in ps]==list(range(1,passes+1)) and all(p['status']=='ok' for p in ps),'warm pass coverage')
                    need(row['workers']=='48','row workers')
                    if regime=='cold':repetitions.setdefault((row['frame'],row['mode']),[]).append(row['rep'])
                    seen[regime,row['frame'],row['mode']]+=1
                    totals['lidar_processes']+=1;totals['canonical_dumps_checked']+=1;totals['constructed_lidar_passes']+=1 if regime=='cold' else passes;totals['intermediate_passes_without_dump_or_ledger']+=0 if regime=='cold' else passes-1
                    identities.append(dict(k=k,regime=regime,frame=row['frame'],mode=row['mode'],rep=row.get('rep'),dump_sha256=row['dump_sha256']))
                    if regime=='warm':
                        later=ps[1:];metrics=['wall_ns','domain_ns','forest_ns','batch_executor_ns','batch_count_ns','batch_fill_ns']
                        ranges.append(dict(k=k,leaf=leaf,frame=row['frame'],mode=row['mode'],workers=48,scope='retained raw passes 2..P; no median',passes=len(later),raw_ranges_ns={m:[min(z[m] for z in later),max(z[m] for z in later)] for m in metrics}))
                        if row['mode']!='cpu':
                            b=row['summary']['batch'];last.append(dict(k=k,frame=row['frame'],mode=row['mode'],scope='last warm call only',**{m:b[m] for m in ('jobs','fill_jobs','copied_jobs','count_ns','fill_ns','executor_ns')}))
            for frame in EXPECTED[k]:
                for mode in MODES:
                    need(seen['cold',frame,mode]==reps and seen['warm',frame,mode]==1,'exact frame/mode occurrences')
                    need(sorted(repetitions[frame,mode])==list(range(reps)),'exact cold repetitions')
            reports.append(dict(k=k,leaf=leaf,cold_processes=len(q['cold']),warm_processes=len(q['warm']),warm_passes_per_process=passes,report_sha256=sha(report_raw),bench_sha256=q['bench_sha256'],canonical=q['identity'],ledger_reference_sha256=sha(json.dumps(q['ledger'],sort_keys=True).encode()),verdict=q['verdict'],scope=q['scope'],ledger_equality_evidence='Pinned judge checks complete catalogue_work for every retained dump; refusal list empty. Individual work lines are not retained.'))
        need(reports[0]['bench_sha256']==reports[1]['bench_sha256'],'binary reuse')
        configure=member(t,'results/cmd/000_gpu_k5/files/gpu_k5/build_configure.log');need(b'bits = 21' in configure and b'NVIDIA 12.9.41' in configure,'actual build profile')
        need(not member(t,'results/overflow.txt') and not member(t,'results/truncated.txt'),'streams complete')
    need(totals==dict(lidar_processes=90,canonical_dumps_checked=90,constructed_lidar_passes=252,intermediate_passes_without_dump_or_ledger=162),'totals')
    data={x['name']:x for x in r['data_files']};frames=[]
    for name in EXPECTED[5]:
        xyz=data[name+'.u32le'];ids=data[name+'.ids.u32le'];need(xyz['size']%12==0 and xyz['size']//12==ids['size']//4,'data metadata shape')
        frames.append(dict(frame=name,sites=xyz['size']//12,xyz_sha256=xyz['sha256'],ids_sha256=ids['sha256']))
    old_raw=git(repo,'morsehgp3D_v11/receipts/audit_g4_coop2_20261006/summary.json','1be8048654ca382fabecc99993dd0334afd18087')
    old=json.loads(old_raw)
    need(old['source_pin']=='ee3eabe5e7aae4d95515f63935e8f9a84adbd468' and old['closed_certified'] and old['whole_frames_metadata']==frames,'comparison closed scope/data')
    changed=subprocess.check_output(['git','-C',str(repo),'diff','--name-only',old['source_pin'],PIN,'--','morsehgp3D_v11/src'],text=True).splitlines()
    need(changed==['morsehgp3D_v11/src/catalogue/leaf_device.hpp'],'comparison source delta')
    old_reports={x['k']:x for x in old['results']}
    for q in reports:need(old_reports[q['k']]['ledger_reference_sha256']==q['ledger_reference_sha256'] and old_reports[q['k']]['leaf']==q['leaf'] and old_reports[q['k']]['canonical']==q['canonical'],'comparison ledger/K/leaf')
    old_stages={(x['k'],x['frame'],x['mode']):x for x in old['last_warm_stages']};comparison=[]
    for row in last:
        if row['mode']!='gpu':continue
        prev=old_stages[row['k'],row['frame'],row['mode']]
        need(all(prev[k]==row[k] for k in ('jobs','fill_jobs','copied_jobs')),'comparison workload counters')
        comparison.append(dict(k=row['k'],frame=row['frame'],mode='gpu',workers=48,mask=81915,leaf=16 if row['k']==5 else 24,scope='last warm call only; same data/K/leaf/W48/mask and workload; source extend changed; no FULL gain inferred',previous_pin=old['source_pin'],current_pin=PIN,previous_fill_ns=prev['fill_ns'],current_fill_ns=row['fill_ns'],previous_executor_ns=prev['executor_ns'],current_executor_ns=row['executor_ns']))
    return dict(schema='audit_g4_coop3_metadata_v1',session=session.name,source_pin=PIN,closed_certified=True,receipt_status='completed',worker_exit_code=0,closing_generation=r['closing_generation'],last_stop_timestamp=after['lastStopTimestamp'],receipt_sha256=sha(raw),archive_sha256=r['results_sha256'],package_sha256=r['package_sha256'],plan_sha256=r['plan_sha256'],worker_plan_sha256=r['worker_plan_sha256'],source_equality=source,judge_sha256=sha(judge),results_manifest_entries=len(entries),results_manifest_sha256=sha(manifest),commands=commands,results=reports,totals=totals,raw_identity_rows=identities,warm_raw_ranges=ranges,last_warm_stages=last,whole_frames_metadata=frames,comparison_previous_summary_sha256=sha(old_raw),last_warm_gpu_comparison= comparison,source_change_scope='Only src/catalogue/leaf_device.hpp changed versus ee3 in product source; explicit extend loop restored',native_or_cloud_runs_by_auditor=0,limits=['Only executed pin9eee and u21 Release CUDA, W48 and masks in this plan. Active MST/v2 WIP excluded.','No sanitizer, unit test, mutant, profile or coherent-warp gate selected. Previous guards are not closed by this measurement.','90 dump hashes checked; complete catalogue ledger equality delegated to pinned judge. No per-row work lines retained.','252 FULL constructions reported successful, but162 intermediate warm passes serialize neither dump nor catalogue ledger.','No median, no gain FULL inferred from a fill time, and no100ms contract obtained.'])

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--repo',type=Path,default=Path('/workspaces/E-HGP'));p.add_argument('--session',type=Path,default=Path('/workspaces/.ehgp-sessions/v11.20261006.claudecoop3'));p.add_argument('--check',type=Path);a=p.parse_args();result=derive(a.repo,a.session)
    if a.check:
        need(json.loads(a.check.read_text())==result,'frozen summary differs')
        print('coop3 closed metadata PASS: Git source, 2 commands, 90 canonical dumps, 252 passes; 162 intermediate passes unjudged; native0 cloud0')
    else:print(json.dumps(result,ensure_ascii=False,sort_keys=True,indent=2))
