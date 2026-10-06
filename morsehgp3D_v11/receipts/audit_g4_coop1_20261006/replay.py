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

PIN='c3df818052494d62837834d257dbb28b89b4b074'
PUBLISHED_PIN='ee3eabe5e7aae4d95515f63935e8f9a84adbd468'
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
    publication=[]
    for name,value in [('receipt.json',receipt_raw),('launch.json',(session/'launch.json').read_bytes()),('plan.json',plan)]:
        need(git(repo,PUBLISHED+name,PUBLISHED_PIN)==value,'publication '+name)
        publication.append({'file':name,'sha256':sha(value),'equal_to_closed_session':True})
    for line in git(repo,PUBLISHED+'SHA256SUMS',PUBLISHED_PIN).decode().splitlines():
        h,name=line.split(None,1);name=name.lstrip('*').removeprefix('./')
        need(sha(git(repo,PUBLISHED+name,PUBLISHED_PIN))==h,'publication hash')
    raw_judge=git(repo,'morsehgp3D_v11/bench/gpu_ab.py');judge=raw_judge.decode()
    need("work == report['ledger'].get(frame)" in judge and
         "report['verdict'] = 'conforme' if not report['refusals'] else 'refus'" in judge,'ledger judge')
    gate_source=git(repo,'morsehgp3D_v11/bench/coop_g4_gate.py')
    need("'--error-exitcode', '9'" in gate_source.decode(),'sanitizer guard')
    host_names=['mhgp11_catalogue_leaf_coop_'+s for s in ['witness_q3_obtuse_q4','cache_hits','sizes','near_max','inventaire']]+['mhgp11_tower_full_leaf_lanes']
    result=[];perf=[];commands=[];processes=dumps=constructed=intermediate=0
    with tarfile.open(arc) as t:
        for name,index in [('portes_hote',0),('gpu_k5',1),('coop_gate',2),('gpu_k10',3)]:
            meta=member(t,f'results/cmd/{index:03d}_{name}/meta.txt')
            values=dict(line.split('=',1) for line in meta.decode().splitlines() if '=' in line)
            need(values['status']=='ok' and values['exit_code']=='0','commande incomplete')
            commands.append({'name':name,'status':values['status'],'exit_code':0,'metadata_sha256':sha(meta)})
        host=member(t,'results/cmd/000_portes_hote/stdout')
        matches=re.findall(r'\d+/\d+ Test\s+#\d+:\s+(\S+)\s+\.+\s+(Passed|\*\*\*\S+)\s+([0-9.]+) sec',host.decode())
        need(len(matches)==len(host_names) and len({x[0] for x in matches})==len(matches) and
             {x[0] for x in matches}==set(host_names) and all(x[1]=='Passed' for x in matches),'host gates')
        host_summary={'passed':len(matches),'failed':0,'missing':0,'verdicts':[{'name':n,'state':s,'seconds':v} for n,s,v in matches],'stdout_sha256':sha(host)}
        for index,k,reps,passes,leaf in [(1,5,5,12,16),(3,10,3,8,24)]:
            raw=member(t,f'results/cmd/{index:03d}_gpu_k{k}/files/gpu_k{k}/gpu_ab_report.json')
            need(git(repo,PUBLISHED+f'gpu_ab_report_k{k}.json',PUBLISHED_PIN)==raw,'report publication')
            publication.append({'file':f'gpu_ab_report_k{k}.json','sha256':sha(raw),'equal_to_closed_archive':True})
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
            result.append({'k':k,'leaf':leaf,'cold_processes':len(q['cold']),'warm_processes':len(q['warm']),'warm_passes_per_process':passes,
                'canonical':q['identity'],'report_sha256':sha(raw),'bench_sha256':q['bench_sha256'],'verdict':q['verdict'],
                'ledger_reference_sha256':sha(json.dumps(q['ledger'],sort_keys=True).encode()),'scope':q['scope'],
                'ledger_equality_evidence':'Pinned gpu_ab judge checks complete work per retained dump; refusal list empty. Individual native work lines are not retained in report.'})
        need(result[0]['bench_sha256']==result[1]['bench_sha256'],'binary')
        raw=member(t,'results/cmd/002_coop_gate/files/coop_gate/coop_g4_gate.json')
        need(git(repo,PUBLISHED+'coop_g4_gate.json',PUBLISHED_PIN)==raw,'gate publication')
        publication.append({'file':'coop_g4_gate.json','sha256':sha(raw),'equal_to_closed_archive':True})
        g=json.loads(raw);need(g['verdict']=='conforme' and len(g['runs'])==16 and len(g['sanitizer'])==6,'gpu gate')
        rows={(x['cloud'],x['kmax'],x['mode']):x for x in g['runs']}
        wanted={(c,k,m) for c in ['A','B'] for k in ['5','10'] for m in ['cpu','gpu','gpu_coop','lot_coop']}
        need(set(rows)==wanted and len(rows)==len(g['runs']),'gate inventory')
        for c in ['A','B']:
            for k in ['5','10']:
                ref=rows[c,k,'cpu']
                for m in ['cpu','gpu','gpu_coop','lot_coop']:
                    x=rows[c,k,m]
                    need(x['code']==0 and x['status']=='ok' and x['digest']==ref['digest'] and x['work']==ref['work'],'gate result')
                for key in ['jobs','records','population','unresolved']:
                    need(len({rows[c,k,m]['batch'][key] for m in ['gpu','gpu_coop','lot_coop']})==1,'gate batch')
        unresolved=rows['B','10','gpu_coop']['batch']['unresolved'];need(unresolved>0,'unresolved coverage')
        batch=rows['A','10','gpu_coop']['batch'];need(0<batch['fill_jobs']<batch['jobs'] and batch['copied_jobs']>0,'write coverage')
        expected_san={('memcheck','A','5'),('memcheck','B','10'),('racecheck','B','5'),('racecheck','B','10'),('synccheck','B','5'),('synccheck','B','10')}
        need({(x['tool'],x['cloud'],x['kmax']) for x in g['sanitizer']}==expected_san,'san inventory')
        san=[]
        for x in g['sanitizer']:
            marker='RACECHECK SUMMARY: 0 hazards displayed (0 errors, 0 warnings)' if x['tool']=='racecheck' else 'ERROR SUMMARY: 0 errors'
            need(x['code']==0 and x['clean'] and x['same_dump'] and marker in x['tail'],'san result')
            san.append({n:x[n] for n in ['tool','cloud','kmax','code','clean','same_dump']})
        gate={'runs':16,'verdict':'conforme','report_sha256':sha(raw),'unresolved_B_K10':unresolved,
            'A_K10_batch':{k:batch[k] for k in ['jobs','records','population','unresolved','fill_jobs','copied_jobs']},'sanitizer':san,
            'scope':'Synthetic 3000-site16-bit and400-site21-bit inputs; CUDA u21; does not transfer sanitizer coverage to LiDAR or u24.'}
    data={x['name']:x for x in r['data_files']}
    frames=[]
    for frame,n in [('lidar_ng00',39885),('lidar_ng01',35551),('lidar_ng02',45845)]:
        need(data[frame+'.u32le']['size']==12*n and data[frame+'.ids.u32le']['size']==4*n,'full frame sizes')
        frames.append({'frame':frame,'sites':n,'xyz_sha256':data[frame+'.u32le']['sha256'],'ids_sha256':data[frame+'.ids.u32le']['sha256']})
    return {'schema':'audit_g4_coop1_metadata_v1','session':session.name,'source_pin':PIN,'publication_pin':PUBLISHED_PIN,
        'receipt_status':r['status'],'worker_exit_code':0,'closed_certified':True,'last_stop_timestamp':after['lastStopTimestamp'],
        'receipt_sha256':sha(receipt_raw),'archive_sha256':r['results_sha256'],'package_sha256':r['package_sha256'],'plan_sha256':r['plan_sha256'],
        'source_equality':source,'judge_sha256':sha(raw_judge),'gpu_gate_source_sha256':sha(gate_source),'commands':commands,
        'host_gates':host_summary,'gpu_synthetic_gate':gate,'results':result,'whole_frames_metadata':frames,
        'totals':{'lidar_processes':processes,'constructed_lidar_passes':constructed,'canonical_dumps_checked':dumps,'intermediate_passes_without_dump_or_ledger':intermediate},
        'performance_criteria':perf,'developer_publication_pieces':publication,'developer_publication_SHA256SUMS_valid':True,
        'limits':['Receipt qualifies executed c3df source; subsequent ee3 source correction is not natively requalified by coop1.','No mutants run in this session.','No local native/cloud action by auditor.','Coop2 excluded.'],
        'native_or_cloud_runs_by_auditor':0}

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--repo',type=Path,default=Path('/workspaces/E-HGP'))
    p.add_argument('--session',type=Path,default=Path('/workspaces/.ehgp-sessions/v11.20261006.claudecoop1'))
    p.add_argument('--capture',action='store_true')
    a=p.parse_args();result=derive(a.repo,a.session)
    target=Path(__file__).with_name('summary.json')
    text=json.dumps(result,ensure_ascii=False,sort_keys=True,indent=2)+'\n'
    if a.capture:target.write_text(text)
    else:need(json.loads(target.read_text())==result,'summary changed')
    print(text,end='')
if __name__=='__main__':main()
