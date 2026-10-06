#!/usr/bin/env python3
"""Lecture de métadonnées/Git, aucun build ni calcul HGP."""
import argparse
from collections import Counter
import hashlib
import io
import json
from pathlib import Path
import subprocess
import tarfile

PIN='ee3eabe5e7aae4d95515f63935e8f9a84adbd468'
PUBLISHED_PIN=None
PUBLISHED='morsehgp3D_v11/receipts/developpement_20261006/coop1_feuille_cooperative/'
EXPECTED = {5: {'lidar_ng00': '3a2bfb4f9f48b4b0cc5b0318d9fcf4887906638e034b2c97dda1918e3170a6fe',
                 'lidar_ng01': '5212a2ced81bf69bd2abfb935a3b14a35158df25339285a0d25a9d5d5c09e091',
                 'lidar_ng02': '78feb765e21c8e4582762a25bd0dd36a455747d3f0df80523e19f7ba4be0e207'},
            10: {'lidar_ng00': '61a4245b91d9a4fdad012f0a2e26a63c3e48db1d46a180db756f2c4e4aa77295',
                 'lidar_ng01': '838a447e0b92e13e42f7f69a84fd536d5d46d26463d3f4cc7c688475878fe0de',
                 'lidar_ng02': '81f89995eaccb497cfe92abb81213bebd0d4ce5076210570d86aa9456cf7ff2e'}}
MODES={'cpu':'16379','gpu':'81915','gpu_coop':'212987'}
def need(ok, why):
    if not ok:
        raise ValueError(why)

def sha(b):
    return hashlib.sha256(b).hexdigest()

def sha_file(p):
    with p.open('rb') as f:
        return hashlib.file_digest(f, 'sha256').hexdigest()

def member(t, n):
    m = t.getmember(n)
    need(m.isfile() and m.size <= 16 * 1024 * 1024, 'membre non borné')
    return t.extractfile(m).read()

def git(repo, path, pin=PIN):
    return subprocess.check_output(['git', '-C', str(repo), 'show', pin + ':' + path])

def source_tree(repo, package):
    paths = ['morsehgp3D_v11/' + p for p in ['src', 'tests', 'cmake', 'bench', 'reference', 'tools', 'cli']]
    exact = ['morsehgp3D_v11/CMakeLists.txt', 'gcp-migration/v11_worker.sh']
    b = subprocess.check_output(['git', '-C', str(repo), 'archive', PIN, '--', *paths, *exact])
    prefixes = tuple(p + '/' for p in paths)
    hashes = []
    with tarfile.open(fileobj=io.BytesIO(b)) as g, tarfile.open(package) as a:
        wanted = {m.name for m in g.getmembers() if m.isfile()}
        actual = {m.name for m in a.getmembers() if m.isfile() and
                  (m.name.startswith(prefixes) or m.name in exact)}
        need(wanted == actual, 'inventaire de source')
        for n in sorted(wanted):
            v = member(g, n)
            need(v == member(a, n), 'source différente')
            hashes.append(sha(v) + '  ' + n + '\n')
    return {'files': len(wanted), 'manifest_sha256': sha(''.join(hashes).encode()), 'equal_to_git': True}


def derive(repo, session):
    import re
    receipt_raw=(session/'receipt.json').read_bytes()
    r=json.loads(receipt_raw); after=r['observed_after']
    need((session/'DONE').read_text().strip()=='0' and r['status']=='completed' and r['closure']=='stopped' and
         r['targeted_shutdown_certified'] and r['start_certified'] and r['worker_exit_code']==0,'cloture')
    need(after['status']=='TERMINATED' and after['name']==r['target']['instance'] and
         r['generation']==r['closing_generation']==after['lastStartTimestamp'],'cible generation')
    need(r['private_key_deleted'] and r['reserve_released'] and r['oslogin_key_removed'],'gardes')
    need(r['commit']==PIN and r['worker_source']=='commit:'+PIN and r['evidence_grade']=='pushed_commit','pin')
    need(r['results_verified'] and r['data_verified_remote'] and not r['results_skipped_members'] and
         not r['overflow']['evicted'] and not r['overflow']['truncated_streams'],'capture')
    arc=session/'results/results.tar.gz';package=session/'package/package.tar.gz'
    plan=(session/'package/plan.json').read_bytes()
    need(sha_file(arc)==r['results_sha256'] and sha_file(package)==r['package_sha256'] and sha(plan)==r['plan_sha256'],'hashes')
    source=source_tree(repo,package)
    raw_judge=git(repo,'morsehgp3D_v11/bench/gpu_ab.py');judge=raw_judge.decode()
    need("work == report['ledger'].get(frame)" in judge and
         "report['verdict'] = 'conforme' if not report['refusals'] else 'refus'" in judge,'ledger judge')
    result=[];perf=[];stages=[];commands=[];processes=dumps=constructed=intermediate=0
    with tarfile.open(arc) as t:
        for name,index in [('gpu_k5',0),('gpu_k10',1),('profil_k10',2)]:
            meta=member(t,f'results/cmd/{index:03d}_{name}/meta.txt')
            values=dict(line.split('=',1) for line in meta.decode().splitlines() if '=' in line)
            need(values['status']=='ok' and values['exit_code']=='0','commande incomplete')
            commands.append({'name':name,'status':values['status'],'exit_code':0,'metadata_sha256':sha(meta)})
        for index,k,reps,passes,leaf in [(0,5,5,12,16),(1,10,3,8,24)]:
            raw=member(t,f'results/cmd/{index:03d}_gpu_k{k}/files/gpu_k{k}/gpu_ab_report.json')
            q=json.loads(raw)
            need(q['verdict']=='conforme' and not q['refusals'] and q['modes']==MODES and q['workers']=='48' and
                 q['reps']==reps and q['warm_passes']==passes and q['kmax']==k and q['leaf']==leaf and q['identity']==EXPECTED[k],'report contract')
            need('derniere passe' in q['scope'],'scope')
            seen=Counter()
            for regime in ['cold','warm']:
                rows=q[regime]
                need(len(rows)==(9*reps if regime=='cold' else 9),'inventory')
                for x in rows:
                    need(x['frame'] in EXPECTED[k] and x['mode'] in MODES and x['code']==0 and
                         x['summary']['status']=='ok' and x['summary']['exit']=='ok' and x['dump_sha256']==EXPECTED[k][x['frame']],'row')
                    need(x['summary']['batch']['unresolved']==0,'whole-frame unresolved')
                    ps=x['passes']
                    need(not ps if regime=='cold' else len(ps)==passes and [z['pass'] for z in ps]==list(range(1,passes+1)) and all(z['status']=='ok' for z in ps),'pass inventory')
                    seen[regime,x['frame'],x['mode']]+=1
                    processes+=1;dumps+=1;constructed+=1 if regime=='cold' else passes
                    intermediate+=0 if regime=='cold' else passes-1
            for frame in EXPECTED[k]:
                for mode in MODES:need(seen['cold',frame,mode]==reps and seen['warm',frame,mode]==1,'mode inventory')
            warm={(x['frame'],x['mode']):x for x in q['warm']}
            for frame in EXPECTED[k]:
                metric='batch_executor_ns' if k==5 else 'domain_ns';reference='gpu' if k==5 else 'cpu';target=500 if k==5 else 850
                ref=[z[metric] for z in warm[frame,reference]['passes'][1:]]
                coop=[z[metric] for z in warm[frame,'gpu_coop']['passes'][1:]]
                need(min(coop)*1000>max(ref)*target,'performance conclusion')
                perf.append({'k':k,'frame':frame,'metric':metric,'reference_mode':reference,'reference_range_ns':[min(ref),max(ref)],
                    'cooperative_range_ns':[min(coop),max(coop)],'target_permille':target,'ratio_lower_bound_permille':min(coop)*1000//max(ref),
                    'criterion_failed_on_all_observed_postfirst_warm_passes':True})
                for mode in ['gpu','gpu_coop']:
                    batch=warm[frame,mode]['summary']['batch']
                    stages.append({'k':k,'frame':frame,'mode':mode,'scope':'last warm call only',**{key:batch[key] for key in ['count_ns','fill_ns','executor_ns','jobs','fill_jobs','copied_jobs']}})
            result.append({'k':k,'leaf':leaf,'cold_processes':len(q['cold']),'warm_processes':len(q['warm']),'warm_passes_per_process':passes,
                'canonical':q['identity'],'report_sha256':sha(raw),'bench_sha256':q['bench_sha256'],'verdict':q['verdict'],
                'ledger_reference_sha256':sha(json.dumps(q['ledger'],sort_keys=True).encode()),'scope':q['scope'],
                'ledger_equality_evidence':'Pinned gpu_ab judge checks complete work per retained dump; refusal list empty. Individual native work lines are not retained in report.'})
        need(result[0]['bench_sha256']==result[1]['bench_sha256'],'binary')
        raw=member(t,'results/cmd/002_profil_k10/files/profil_k10/gpu_profile.json')
        q=json.loads(raw)
        need(q['frame']=='lidar_ng00' and q['mode']=='81915' and q['workers']=='48' and q['passes']==3 and not q['problems'],'profile scope')
        profile_source=git(repo,'morsehgp3D_v11/bench/gpu_profile.py')
        need("'ERR_NVGPUCTRPERM'" in profile_source.decode() and "'ncu_profile_sudo'" in profile_source.decode(),'profile retry contract')
        tool_codes={x['name']:x['code'] for x in q['log']}
        need(tool_codes.get('ncu_profile_sudo')==0 and all(x['code']==0 or x['name']=='ncu_profile' for x in q['log']),'final profile tool result')
        profile={'report_sha256':sha(raw),'frame':q['frame'],'mode':q['mode'],'workers':q['workers'],'passes':q['passes'],
            'problems':q['problems'],'command_verdicts':[{'name':x['name'],'code':x['code']} for x in q['log']],
            'scope':'Separate profiled execution of mono-thread GPU path; instrumentation overhead excluded from performance criterion; not a cooperative profile.',
            'source_sha256':sha(profile_source),'first_attempt_refused_counter_permission_then_sudo_retry_passed':tool_codes.get('ncu_profile')!=0 and tool_codes.get('ncu_profile_sudo')==0,
            'metrics':{name:q['ncu'][name]['metrics'] for name in ['count_kernel','fill_kernel']},'raw_report_hashes':q['raw_reports']}
        profile_members=[]
        for name in ['gpu_full.nsys-rep','nsys_stats.csv','leaf_kernels.ncu-rep','ncu_details.csv','ncu_source.csv']:
            member_name='results/cmd/002_profil_k10/files/profil_k10/'+name
            item=t.getmember(member_name);need(item.isfile() and item.size<64*1024*1024,'profile member bound')
            raw=t.extractfile(item).read()
            digest=sha(raw)
            if name in q['raw_reports']:need(q['raw_reports'][name]['sha256']==digest and q['raw_reports'][name]['bytes']==len(raw),'raw report hash')
            profile_members.append({'name':name,'bytes':len(raw),'sha256':digest})
        profile['archive_member_hashes']=profile_members
        import csv,io
        csv_rows=list(csv.DictReader(io.StringIO(member(t,'results/cmd/002_profil_k10/files/profil_k10/ncu_details.csv').decode())))
        for name,metrics in profile['metrics'].items():
            actual={row['Metric Name']:'%s %s'%(row.get('Metric Value',''),row.get('Metric Unit','')) for row in csv_rows if name in row.get('Kernel Name','')}
            need(all(actual.get(key)==value for key,value in metrics.items()),'NCU metrics JSON/CSV')
        profile['metrics_equal_to_details_csv']=True
    data={x['name']:x for x in r['data_files']}
    frames=[]
    for frame,n in [('lidar_ng00',39885),('lidar_ng01',35551),('lidar_ng02',45845)]:
        need(data[frame+'.u32le']['size']==12*n and data[frame+'.ids.u32le']['size']==4*n,'full frame sizes')
        frames.append({'frame':frame,'sites':n,'xyz_sha256':data[frame+'.u32le']['sha256'],'ids_sha256':data[frame+'.ids.u32le']['sha256']})
    return {'schema':'audit_g4_coop2_metadata_v1','session':session.name,'source_pin':PIN,'publication_pin':PUBLISHED_PIN,
        'receipt_status':r['status'],'worker_exit_code':0,'closed_certified':True,'last_stop_timestamp':after['lastStopTimestamp'],
        'receipt_sha256':sha(receipt_raw),'archive_sha256':r['results_sha256'],'package_sha256':r['package_sha256'],'plan_sha256':r['plan_sha256'],
        'source_equality':source,'judge_sha256':sha(raw_judge),'commands':commands,
        'results':result,'profile':profile,'last_warm_stages':stages,'whole_frames_metadata':frames,
        'totals':{'lidar_processes':processes,'constructed_lidar_passes':constructed,'canonical_dumps_checked':dumps,'intermediate_passes_without_dump_or_ledger':intermediate},
        'performance_criteria':perf,'developer_publication_compared':False,
        'limits':['Receipt qualifies executed ee3 source only; any subsequent WIP is excluded.','No mutants run in this session.','No local native/cloud action by auditor.','No host CTests, mutants, Compute Sanitizer, u24 or profiling of cooperative kernel in this session.'],
        'native_or_cloud_runs_by_auditor':0}

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--repo',type=Path,default=Path('/workspaces/E-HGP'))
    p.add_argument('--session',type=Path,default=Path('/workspaces/.ehgp-sessions/v11.20261006.claudecoop2'))
    p.add_argument('--capture',action='store_true')
    a=p.parse_args();result=derive(a.repo,a.session)
    target=Path(__file__).with_name('summary.json')
    text=json.dumps(result,ensure_ascii=False,sort_keys=True,indent=2)+'\n'
    if a.capture:target.write_text(text)
    else:need(json.loads(target.read_text())==result,'summary changed')
    print(text,end='')
if __name__=='__main__':main()
