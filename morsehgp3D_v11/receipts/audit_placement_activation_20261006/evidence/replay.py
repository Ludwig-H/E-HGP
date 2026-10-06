#!/usr/bin/env python3
"""Metadata replay of three closed G4 sessions and integrated C; Git/local archives, stdlib only."""
import argparse,hashlib,io,json,subprocess,tarfile
from collections import Counter
from datetime import datetime,timezone
from pathlib import Path
def need(ok, detail):
    if not ok:
        raise RuntimeError(detail)

def sha(data):
    return hashlib.sha256(data).hexdigest()

def sha_file(path):
    out = hashlib.sha256()
    with path.open('rb') as handle:
        for block in iter(lambda: handle.read(1024 * 1024), b''):
            out.update(block)
    return out.hexdigest()

def read_member(archive, path, maximum=16 * 1024 * 1024):
    member = archive.getmember(path)
    need(member.isfile() and 0 <= member.size <= maximum, 'membre non borne : ' + path)
    return archive.extractfile(member).read()

def kv(data):
    return dict(line.split('=', 1) for line in data.decode().splitlines() if '=' in line)

def source_tree(package, repo, pin):
    """Une archive Git selective ; egalite exacte de tous les fichiers utiles du paquet."""
    prefixes = ('morsehgp3D_v11/src/', 'morsehgp3D_v11/include/', 'morsehgp3D_v11/tests/',
                'morsehgp3D_v11/cmake/', 'morsehgp3D_v11/bench/', 'morsehgp3D_v11/reference/',
                'morsehgp3D_v11/tools/', 'morsehgp3D_v11/cli/', 'morsehgp3D_v11/python/')
    # Le controleur et les gardes s'executent localement ; seul le worker est livre au paquet.
    exact = {'morsehgp3D_v11/CMakeLists.txt', 'gcp-migration/v11_worker.sh'}
    tracked = subprocess.check_output(['git', '-C', str(repo), 'ls-tree', '-r', '--name-only', pin],
                                      text=True).splitlines()
    selected = {name for name in tracked if name in exact or name.startswith(prefixes)}
    # Un repertoire absent (include/, python/, etc.) n'est pas transmis comme pathspec inexistant.
    paths = sorted(exact & selected) + [prefix.rstrip('/') for prefix in prefixes
                                      if any(name.startswith(prefix) for name in selected)]
    git_tar = subprocess.check_output(['git', '-C', str(repo), 'archive', '--format=tar', pin, '--', *paths])
    manifest = {}
    with tarfile.open(fileobj=io.BytesIO(git_tar), mode='r:') as expected, tarfile.open(package, 'r:gz') as actual:
        wanted = {entry.name for entry in expected.getmembers() if entry.isfile()}
        need(wanted == selected, 'source Git selective incomplete ou non reguliere')
        # L'archive inclut egalement ses entrees de repertoires.
        actual_files = {entry.name for entry in actual.getmembers() if entry.isfile() and
                        (entry.name in exact or entry.name.startswith(prefixes))}
        need(actual_files == wanted, 'sources utiles ajoutees/manquantes dans le paquet')
        for name in sorted(wanted):
            original = read_member(expected, name)
            delivered = read_member(actual, name)
            need(original == delivered, 'source du paquet/Git divergent : ' + name)
            manifest[name] = sha(original)
    manifest_bytes = ''.join(digest + '  ' + name + '\n' for name, digest in sorted(manifest.items())).encode()
    return dict(method='git_archive_selectif_egalite_exacte', source_commit=pin,
                files_verified=len(manifest), manifest_sha256=sha(manifest_bytes),
                paths=paths, extra_or_missing_files=False,
                includes_docs_or_receipts=False)

K5_CANONICAL={'lidar_ng00':'3a2bfb4f9f48b4b0cc5b0318d9fcf4887906638e034b2c97dda1918e3170a6fe',
 'lidar_ng01':'5212a2ced81bf69bd2abfb935a3b14a35158df25339285a0d25a9d5d5c09e091',
 'lidar_ng02':'78feb765e21c8e4582762a25bd0dd36a455747d3f0df80523e19f7ba4be0e207'}
PIN1='86b3cbf14247f2cff523757ceb38168d365df82e'
PIN2='9d10de21341fee138c15c3caf9cb30925e030cb9'
SPECS=[('claudeo1place',PIN1,0),('claudeo1place2',PIN2,72),('claudeo1place3',PIN2,None)]
ROOT=Path(__file__).resolve().parent

def git_file(repo,pin,name):
    return subprocess.check_output(['git','-C',str(repo),'show',pin+':'+name])

def judge_source(repo,pin):
    paths=['src/tower/forest_parallel.hpp','src/tower/forest_pipeline.cpp','bench/full_probe.cpp',
           'bench/gpu_ab.py','tests/tower/forest_pipeline_test.cpp','tests/tower/tests.cmake']
    result={}
    for path in paths:
        raw=git_file(repo,pin,'morsehgp3D_v11/'+path);lines=raw.decode().splitlines()
        result[path]={'sha256':sha(raw)}
        needles={'src/tower/forest_parallel.hpp':['place_pipeline_(std::exchange'],
                 'src/tower/forest_pipeline.cpp':['if (parallel.place_pipeline())','pipeline_placement_cores =','pipeline_placement_requested ='],
                 'bench/full_probe.cpp':['full_params.place_pipeline =','pipeline_tasks'],
                 'bench/gpu_ab.py':['def take_summary','summary=take_summary(got)','work is not None'],
                 'tests/tower/forest_pipeline_test.cpp':['placements +=','pipeline_placement_requested','pipeline_equivalence orders='],
                 'tests/tower/tests.cmake':['mhgp11_add_unit(mhgp11_tower_pipeline','mhgp11_add_unit(mhgp11_tower_placement']}
        result[path]['anchors']=[{'line':i+1,'text':line}for i,line in enumerate(lines)if any(n in line for n in needles[path])]
    gpu=git_file(repo,pin,'morsehgp3D_v11/bench/gpu_ab.py').decode()
    summary=gpu.split('def take_summary(got):')[1].split('def find_nvcc')[0]
    need('pipeline_tasks' not in summary and 'placement_cores' not in summary,'summary unexpectedly preserves placement')
    move=git_file(repo,pin,'morsehgp3D_v11/src/tower/forest_parallel.hpp').decode()
    need(('place_pipeline_(std::exchange(other.place_pipeline_, false))' in move)==(pin==PIN2),'move correction differs')
    return result

def metadata_session(repo,sessions,short,pin,placements,source_cache):
    import re
    root=sessions/('v11.20261006.'+short);raw=(root/'receipt.json').read_bytes();r=json.loads(raw)
    need(r['commit']==pin and r['worker_source']=='commit:'+pin and r['source_kind']=='commit' and r['evidence_grade']=='pushed_commit','source identity')
    need(r['status']=='completed' and r['worker_exit_code']==0 and (root/'DONE').read_text().strip()=='0','session outcome')
    need(r['closure']=='stopped' and r['targeted_shutdown_certified'] and r['start_certified'] and
         r['generation']==r['closing_generation']==r['observed_after']['lastStartTimestamp'] and
         r['observed_after']['status']=='TERMINATED' and r['observed_after']['name']==r['target']['instance'],'certified stop')
    need(r['private_key_deleted'] and r['oslogin_key_removed'] and r['reserve_released'],'cleanup')
    need(r['results_verified'] and not r['results_skipped_members'] and r['data_verified_remote'] and
         not r['overflow']['evicted'] and not r['overflow']['truncated_streams'],'integrity/refused overflow')
    for key,name in [('results_sha256','results/results.tar.gz'),('package_sha256','package/package.tar.gz'),('plan_sha256','package/plan.json')]:
        need(sha_file(root/name)==r[key],'session SHA '+key)
    if r['package_sha256'] not in source_cache:
        source_cache[r['package_sha256']]=source_tree(root/'package/package.tar.gz',repo,pin)
    plan=json.loads((root/'package/plan.json').read_text())
    expected_commands=['portes','k5_16_cpu','k5_24_gpu'] if placements is not None else ['k5_16_cpu','k5_24_gpu']
    need([c['name']for c in plan['commands']]==expected_commands,'plan command inventory')
    need(plan['python_packages']=='none' and plan['default_build'] is True,'plan route')
    if short=='claudeo1place3':
        need('claudeo1place' in plan['note'] and 'exclues de la decision' in plan['note'],'exclusion declaration')
    commands=[];reports=[];unit=None;manifest=None
    with tarfile.open(root/'results/results.tar.gz')as archive:
        members={m.name:m for m in archive.getmembers()if m.isfile()}
        need(len(members)==len([m for m in archive.getmembers()if m.isfile()]),'duplicate result names')
        mr=read_member(archive,'results/MANIFEST.sha256');listed=set()
        for line in mr.decode().splitlines():
            h,name=line.split(None,1);name='results/'+name.strip().removeprefix('./')
            need(name not in listed and sha(read_member(archive,name))==h,'manifest identity')
            listed.add(name)
        need(set(members)==listed|{'results/MANIFEST.sha256'},'manifest inventory')
        manifest={'files':len(listed),'sha256':sha(mr),'exact_inventory':True}
        for name in sorted(members):
            if name.startswith('results/cmd/') and name.endswith('/meta.txt'):
                m=kv(read_member(archive,name));need(m['status']=='ok' and m['exit_code']=='0','command refused')
                commands.append({'name':name.split('/')[2],'exit_code':0,'status':'ok'})
            if name=='results/cmd/000_portes/stdout':
                stdout=read_member(archive,name).decode()
                passed=re.findall(r'Test\s+#\d+:\s+(mhgp11_[A-Za-z0-9_]+)\s+\.+\s+Passed',stdout)
                expected={'mhgp11_tower_pipeline_'+n for n in ('decisions','equivalence','abandon','inventaire')}|{'mhgp11_tower_placement_'+n for n in ('parse','cores','plan','affinity','inventaire')}
                need(set(passed)==expected and len(passed)==9 and 'Failed' not in stdout,'unit verdicts')
                match=re.findall(r'pipeline_equivalence orders=(\d+) pipelines=(\d+) placements=(\d+)',stdout)
                need(match==[('960','192',str(placements))],'activation witness')
                unit={'passed':sorted(passed),'failed':[],'missing':[],'stdout_sha256':sha(stdout.encode()),
                      'witness':{'orders':960,'pipelines':192,'calls_with_nonzero_placement_cores':placements}}
            if name.endswith('/gpu_ab_report.json'):
                data=read_member(archive,name);j=json.loads(data)
                arm='cpu' if j['leaf']==16 else 'gpu';base=16379 if arm=='cpu' else 81915
                expected_modes={arm:str(base),arm+'_place':str(base+262144)}
                if short=='claudeo1place3':expected_modes[arm+'_aa']=str(base)
                need(j['verdict']=='conforme' and not j['refusals'] and j['kmax']==5 and j['workers']=='48' and
                     j['modes']==expected_modes and j['variants']==['new'] and j['identity']==K5_CANONICAL,'GPU report identity/modes')
                need(j['reps']==(6 if short=='claudeo1place3' else 5) and j['warm_passes']==10,'take inventory')
                pairs={(f,m)for f in K5_CANONICAL for m in j['modes']}
                cold=Counter((q['frame'],q['mode'])for q in j['cold']);warm=Counter((q['frame'],q['mode'])for q in j['warm'])
                need(set(cold)==set(warm)==pairs and all(x==j['reps']for x in cold.values()) and all(x==1 for x in warm.values()),'take coverage')
                for q in j['cold']+j['warm']:
                    need(q['code']==0 and q['summary']['status']=='ok' and q['summary']['exit']=='ok' and q['dump_sha256']==K5_CANONICAL[q['frame']],'take success/hash')
                    need('pipeline_tasks' not in q['summary'] and 'placement_cores' not in q['summary'] and 'pipeline_placement_requested' not in q['summary'],'unexpected activation observation')
                for q in j['warm']:
                    need([v['pass']for v in q['passes']]==list(range(1,11)) and all(v['status']=='ok'for v in q['passes']),'warm pass success')
                need(set(j['ledger'])==set(K5_CANONICAL) and all(isinstance(x,dict) and x for x in j['ledger'].values()),'preserved reference ledger')
                reports.append({'path':name,'sha256':sha(data),'arm':arm,'k':5,'leaf':j['leaf'],'modes':j['modes'],
                    'cold_processes':len(j['cold']),'warm_processes':len(j['warm']),
                    'full_passes':len(j['cold'])+len(j['warm'])*10,'retained_hashes':len(j['cold'])+len(j['warm']),
                    'canonical':j['identity'],'reference_ledger_sha256':sha(json.dumps(j['ledger'],sort_keys=True).encode()),
                    'verdict':'conforme','refusals':0,'bench_sha256':j['bench_sha256'],
                    'build_steps':[{k:v for k,v in e.items()if k in ('step','sha256','code')}for e in j['build']],
                    'placement_cores_observed_per_lidar_take':False,'requested_boolean_observed_per_lidar_take':False})
    need(len(reports)==2 and len(commands)==len(expected_commands),'command/result inventory')
    need((unit is not None)==(placements is not None),'unit scope')
    need(reports[0]['bench_sha256']==reports[1]['bench_sha256'],'same native bench within session')
    need(any(e['step']=='built'for e in reports[0]['build_steps'])and not any(e['step']=='reuse'for e in reports[0]['build_steps']),'first bench newly built')
    return {'session':root.name,'source_pin':pin,'status':'completed','worker_exit_code':0,'done_code':0,
            'targeted_stop_certified':True,'stop_utc':datetime.fromisoformat(r['observed_after']['lastStopTimestamp']).astimezone(timezone.utc).isoformat(),
            'receipt_sha256':sha(raw),'plan_sha256':r['plan_sha256'],'package_sha256':r['package_sha256'],'archive_sha256':r['results_sha256'],
            'source_equality':source_cache[r['package_sha256']],'manifest':manifest,'commands':commands,'unit_gate':unit,'reports':reports,
            'new_sanitizer_or_mutant_execution':False,'capture_kind':'local_closed_session_not_developer_published_receipt'}

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo',type=Path,default=Path('/workspaces/E-HGP'))
    parser.add_argument('--sessions',type=Path,default=Path('/workspaces/.ehgp-sessions'))
    parser.add_argument('--capture',action='store_true');args=parser.parse_args()
    cache={};entries=[metadata_session(args.repo,args.sessions,*spec,cache)for spec in SPECS]
    result={'schema':'audit_placement_activation_v1','source_anchors':{pin:judge_source(args.repo,pin)for pin in [PIN1,PIN2]},
            'sessions':entries,'native_executions':0,'cloud_actions':0,
            'limits':['No timing statistics or performance adoption derived.',
                      'First session has zero activated plans and is explicitly excluded by the third plan.',
                      'Nonzero placement_cores is observed on the place2 unit gate only, not on each LiDAR take.',
                      'The requested option is checked by source and PASS of the place2 unit gate; report does not preserve it per LiDAR take.',
                      'No new sanitizer/mutant result or API-default qualification is transferred from these probe sessions.',
                      'Only cold and last warm outputs/registers are judged; intermediate pass success is preserved separately.',
                      'No LiDAR data, native dump or account identifiers copied.']}
    output=json.dumps(result,sort_keys=True,indent=2)+'\n'
    if args.capture:(ROOT/'summary.json').write_text(output)
    else:need((ROOT/'summary.json').read_text()==output,'summary mismatch')
    print(output,end='')
