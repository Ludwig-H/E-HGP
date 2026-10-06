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

CANONICAL = {
    5: {'lidar_ng00': '3a2bfb4f9f48b4b0cc5b0318d9fcf4887906638e034b2c97dda1918e3170a6fe',
        'lidar_ng01': '5212a2ced81bf69bd2abfb935a3b14a35158df25339285a0d25a9d5d5c09e091',
        'lidar_ng02': '78feb765e21c8e4582762a25bd0dd36a455747d3f0df80523e19f7ba4be0e207'},
    10: {'lidar_ng00': '61a4245b91d9a4fdad012f0a2e26a63c3e48db1d46a180db756f2c4e4aa77295',
         'lidar_ng01': '838a447e0b92e13e42f7f69a84fd536d5d46d26463d3f4cc7c688475878fe0de',
         'lidar_ng02': '81f89995eaccb497cfe92abb81213bebd0d4ce5076210570d86aa9456cf7ff2e'}}

PUBLICATION='5861c223f31b5d7d6f621522d0ca84d0064c9904'
ROOT=Path(__file__).resolve().parent
SESSIONS=(('claudereservoir2','reservoir_cases_chainees',''),('claudereservoir3','reservoir3_chemin_chaud',''),('claudewfgpu1','wfgpu1_leviers','session/'))
SOURCE_PINS={'claudereservoir2':'59509bbc816646f7bda0c77a3b948f57c79a8b9c',
             'claudereservoir3':'79fa5e9f7ceeb90b4b3de31e1d8b4938fc095016',
             'claudewfgpu1':'05db6f5b78d43a2657e519bc02841738d4dd02c1'}

def git_file(repo,pin,path):
    return subprocess.check_output(['git','-C',str(repo),'show',pin+':'+path])


def report(raw):
    j=json.loads(raw);k=j['kmax'];modes=j['modes']
    need(j['verdict']=='conforme' and not j['refusals'] and j['identity']==CANONICAL[k],'GPU verdict/canonical')
    c=Counter((r['frame'],r['mode'])for r in j['cold']);w=Counter((r['frame'],r['mode'])for r in j['warm'])
    pairs={(f,m)for f in CANONICAL[k]for m in modes}
    need(set(c)==set(w)==pairs and all(n==j['reps']for n in c.values())and all(n==1 for n in w.values()),'GPU inventory')
    need(int(j['workers'])==48,'workers')
    for r in j['cold']+j['warm']:
        need(r['code']==0 and r['summary']['status']=='ok' and r['dump_sha256']==CANONICAL[k][r['frame']],'GPU process/dump')
    for r in j['warm']:
        need([p['pass']for p in r['passes']]==list(range(1,j['warm_passes']+1))and all(p['status']=='ok'for p in r['passes']),'warm pass inventory')
    build=[{key:val for key,val in e.items()if key in ('step','name','code','sha256','archive_sha256')}for e in j['build']]
    return {'report_sha256':sha(raw),'k':k,'leaf':j['leaf'],'modes':modes,'variants':j.get('variants'),
        'bench_sha256':j['bench_sha256'],'canonical':j['identity'],'cold_processes':len(j['cold']),'warm_processes':len(j['warm']),
        'full_passes':len(j['cold'])+len(j['warm'])*j['warm_passes'],'retained_hashes':len(j['cold'])+len(j['warm']),
        'intermediate_passes_without_dump_or_ledger':len(j['warm'])*(j['warm_passes']-1),'refusals':0,'build_steps':build}


def sanitizer(raw,variant):
    j=json.loads(raw)
    need(j['verdict']=='conforme'and len(j['runs'])==12 and len(j['sanitizer'])==5,'sanitizer inventory/verdict')
    need(all(r['code']==0 and r['status']=='ok' and r['work']is not None for r in j['runs']),'sanitizer ordinary status/work')
    reference={(r['cloud'],r['kmax']):r for r in j['runs']if r['mode']=='cpu'}
    need(len(reference)==4 and {(r['cloud'],r['kmax'],r['mode'])for r in j['runs']}==
         {(a,k,m)for a in ('A','B')for k in ('5','10')for m in ('cpu','gpu','gpu_rejoue')},'sanitizer ordinary inventory')
    need(all(r['digest']is not None and r['digest']==reference[(r['cloud'],r['kmax'])]['digest']and
             r['work']==reference[(r['cloud'],r['kmax'])]['work']for r in j['runs']),'ordinary identity/work')
    wanted=[['memcheck','A'],['memcheck','B'],['racecheck','A'],['racecheck','B'],['synccheck','A']]
    need([[r['tool'],r['cloud']]for r in j['sanitizer']]==wanted,'instrumented pairs')
    for r in j['sanitizer']:
        marker='RACECHECK SUMMARY: 0 hazards'if r['tool']=='racecheck'else'ERROR SUMMARY: 0 errors'
        need(r['code']==0 and r['clean']and r['same_dump']and marker in r['tail'],'actual tool verdict')
    need(str(j['bench']).endswith('/b_cuda'+('_AC'if variant=='AC'else'')+'/mhgp11_full_bench'),'sanitizer route')
    return {'report_sha256':sha(raw),'verdict':'conforme','ordinary_runs':12,'ordinary_work_absent':0,'pairs':wanted,
        'binary_variant':variant,'instrumented_ledger_and_status_not_preserved':True,
        'source_judge_guard_finding_remains_independent':True}


def session(repo,sessions,spec):
    short,folder,prefix=spec;s=sessions/('v11.20261006.'+short);rr=(s/'receipt.json').read_bytes();r=json.loads(rr)
    need(r['commit']==SOURCE_PINS[short],'unexpected source pin')
    need(r['source_kind']=='commit'and r['evidence_grade']=='pushed_commit'and r['worker_source']=='commit:'+r['commit'],'source provenance')
    need(r['closure']=='stopped'and r['targeted_shutdown_certified']and r['start_certified']and
         r['generation']==r['closing_generation']==r['observed_after']['lastStartTimestamp']and
         r['observed_after']['status']=='TERMINATED'and r['observed_after']['name']==r['target']['instance'],'targeted closure')
    need(r['private_key_deleted']and r['oslogin_key_removed']and r['reserve_released'],'closure cleanup')
    need(r['results_verified']and not r['results_skipped_members']and not r['overflow']['truncated_streams']and r['data_verified_remote'],'results/data provenance')
    expected=(1,'failed_remote')if short=='claudereservoir2'else(0,'completed')
    done_code=int((s/'DONE').read_text().strip())
    need((r['worker_exit_code'],r['status'])==expected and done_code==(3 if short=='claudereservoir2' else 0),'worker/session/controller outcome')
    for field,path in [('results_sha256','results/results.tar.gz'),('package_sha256','package/package.tar.gz'),('plan_sha256','package/plan.json')]:
        need(sha_file(s/path)==r[field],'session hash '+field)
    base='morsehgp3D_v11/receipts/developpement_20261006/'+folder+'/'
    published={}
    for line in git_file(repo,PUBLICATION,base+'SHA256SUMS').decode().splitlines():
        h,name=line.split(None,1);name=name.strip().removeprefix('./');raw=git_file(repo,PUBLICATION,base+name)
        need(sha(raw)==h,'published hash '+name);published[name]=raw
    for name,local in [(prefix+'receipt.json','receipt.json'),(prefix+'launch.json','launch.json'),(prefix+'plan.json','package/plan.json')]:
        need(published[name]==(s/local).read_bytes(),'published/session file '+name)
    result={'session':s.name,'pin':r['commit'],'status':r['status'],'worker_exit_code':r['worker_exit_code'],
        'done_controller_code':done_code,'archive_sha256':r['results_sha256'],'package_sha256':r['package_sha256'],'plan_sha256':r['plan_sha256'],
        'receipt_sha256':sha(rr),'targeted_stop_certified':True,
        'stop_utc':datetime.fromisoformat(r['observed_after']['lastStopTimestamp']).astimezone(timezone.utc).isoformat(),
        'published_files_verified':len(published),'source_equality':source_tree(s/'package/package.tar.gz',repo,r['commit']),
        'commands':[],'gpu_reports':[],'overflow_count':len(r['overflow']['evicted'])}
    if short=='claudereservoir2':
        need(len(r['overflow']['evicted'])==1 and r['overflow']['evicted'][0].endswith('cmd/002_sanitizer/files/sanitizer/work/dump'),'eviction')
        h,n,path=r['overflow']['evicted'][0].split('\t');need(int(n)==272195386,'evicted bytes')
        result['eviction']={'sha256':h,'bytes':int(n),'artifact':'sanitizer/work/dump'}
    else:need(not r['overflow']['evicted'],'unexpected eviction')
    with tarfile.open(s/'results/results.tar.gz')as tar:
        members={m.name:m for m in tar.getmembers()if m.isfile()};listed=set();mr=read_member(tar,'results/MANIFEST.sha256')
        for line in mr.decode().splitlines():
            h,name=line.split(None,1);name='results/'+name.strip().removeprefix('./')
            need(name not in listed and sha(read_member(tar,name))==h,'result manifest');listed.add(name)
        need(set(members)==listed|{'results/MANIFEST.sha256'},'result inventory')
        result['result_manifest']={'files':len(listed),'sha256':sha(mr),'exact_inventory':True}
        for m in members.values():
            if m.name.endswith('/meta.txt'):
                meta=kv(read_member(tar,m.name));need(meta['status']=='ok'and meta['exit_code']=='0','command status')
                result['commands'].append({'path':m.name,'status':'ok','exit_code':0})
            if m.name.endswith('gpu_ab_report.json'):
                raw=read_member(tar,m.name);entry=report(raw);k=entry['k'];label=m.name.split('/')[4]
                if short=='claudewfgpu1':name={'k5_16':'gpu_ab_report_k5_16.json','k10_24':'gpu_ab_report_k10_24.json','k5_24':'gpu_ab_report_k5_24.json'}[label]
                elif label=='k5_feuilles24':name='gpu_ab_report_k5_feuilles24.json'
                else:name='gpu_ab_report_k%d.json'%k
                need(published[prefix+name]==raw,'published GPU report');entry['path']=m.name;result['gpu_reports'].append(entry)
            if m.name.endswith('gpu_sanitizer.json'):
                raw=read_member(tar,m.name);name='gpu_sanitizer_ac.json'if short=='claudewfgpu1'else'gpu_sanitizer.json'
                need(published[prefix+name]==raw,'published sanitizer');result['sanitizer']=sanitizer(raw,'AC'if short=='claudewfgpu1'else'base595')
    result['commands'].sort(key=lambda e:e['path']);result['gpu_reports'].sort(key=lambda e:e['path'])
    need(len(result['commands'])==(4 if short=='claudewfgpu1' else 3),'command inventory')
    return result,r,published


def variants(repo,data,receipt,published,wfgpu):
    expected={p['name']:p for p in receipt['data_files']if p['name'].endswith('_src.tar.gz')};proof=[]
    for line in published['session/archives_variantes.sha256'].decode().splitlines():
        h,name=line.split(None,1);name=Path(name.strip()).name;path=data/name
        need(name in expected and expected[name]['sha256']==h and sha_file(path)==h and path.stat().st_size==expected[name]['size'],'data/uploaded/archive identity')
        proof.append({'name':name,'sha256':h,'bytes':expected[name]['size']})
    need(len(proof)==4 and len(expected)==4,'variant source inventory')
    sources={p['name'].split('_src.')[0]:p['sha256']for p in proof}
    benches=None
    for report in wfgpu['gpu_reports']:
        need(report['variants']==['new','sync','A','C','AC']and set(report['modes'])=={n+':gpu'for n in report['variants']}and set(report['modes'].values())=={'81915'},'workflow modes')
        logged={e['name']:e['archive_sha256']for e in report['build_steps']if e['step']=='variant'}
        need(logged==sources,'logged source archives')
        need(benches is None or benches==report['bench_sha256'],'workflow binary hashes changed');benches=report['bench_sha256']
    first=wfgpu['gpu_reports'][0]['build_steps']
    need(sum(e['step']=='built'for e in first)==5 and not any(e['step']=='reuse'for e in first),'workflow first builds')
    for e in first:
        if e['step']in ('configure','build'):need(e['code']==0,'workflow build status')
    # Exact equality of all integrated product src files, not just the two changed headers.
    product='morsehgp3D_v11/src';archive=subprocess.check_output(['git','-C',str(repo),'archive','--format=tar',PUBLICATION,'--',product])
    manifest={}
    with tarfile.open(fileobj=io.BytesIO(archive),mode='r:')as git,tarfile.open(data/'C_src.tar.gz')as actual:
        mapping={m.name.removeprefix('./'):m.name for m in actual.getmembers()if m.isfile()}
        wanted={m.name for m in git.getmembers()if m.isfile()}
        need(wanted=={n for n in mapping if n.startswith(product+'/')},'integratedC product inventory')
        for name in sorted(wanted):
            original=read_member(git,name);delivered=read_member(actual,mapping[name]);need(original==delivered,'integratedC product mismatch '+name);manifest[name]=sha(original)
    manifest_bytes=''.join(h+'  '+n+'\n'for n,h in sorted(manifest.items())).encode()
    return {'archive_sources':sorted(proof,key=lambda p:p['name']),'binary_sha256':benches,'first_command_built_all_five':True,
        'archive_hashes_stable_across_three_reports':True,'integrated_C_product_source':{'files_verified':len(manifest),'manifest_sha256':sha(manifest_bytes),'equal_to_variant_C':True,
        'changed_headers_sha256':{n:manifest[n]for n in ('morsehgp3D_v11/src/catalogue/leaf_device.hpp','morsehgp3D_v11/src/catalogue/leaf_device_predicates.hpp')}},
        'qualification_scope':'GPU u21 LiDAR diffs of C; Compute Sanitizer AC only; no integrated C CTest/mutant/u18/u24 execution in this session.'}


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo',type=Path,default=Path('/workspaces/E-HGP'));parser.add_argument('--sessions',type=Path,default=Path('/workspaces/.ehgp-sessions'))
    parser.add_argument('--variants-dir',type=Path,default=Path('/workspaces/E-HGP/build/v11-persist/wf_gpu/data_g4'));parser.add_argument('--capture',action='store_true');args=parser.parse_args()
    entries=[];wf=None
    for spec in SESSIONS:
        entry,receipt,published=session(args.repo,args.sessions,spec);entries.append(entry)
        if spec[0]=='claudewfgpu1':wf=variants(args.repo,args.variants_dir,receipt,published,entry)
    tool_hashes={}
    for name in ('gpu_ab.py','ab_g4.py'):
        snapshot=(ROOT.parent/'cache'/name).read_bytes()
        path='morsehgp3D_v11/bench/'+name
        need(snapshot==git_file(args.repo,SOURCE_PINS['claudewfgpu1'],path)==git_file(args.repo,PUBLICATION,path),'judge snapshot/publication differs')
        tool_hashes[path]=sha(snapshot)
    result={'judge_sources_identical_at_session_and_publication':tool_hashes,'schema':'audit_closed_reservoir_wfgpu_metadata_v1','publication_pin':PUBLICATION,'sessions':entries,'workflow_variants':wf,
        'native_executions':0,'cloud_actions':0,'limits':['No native binary dump or LiDAR bytes retained/reread; six canonical hash references checked.',
         'Intermediate warm passes have success status but only cold and last warm hashes/ledger judge results preserved.',
         'Reservoir2 worker failure is retained separately from three successful commands and its explicit dump eviction.',
         'No timing statistics derived; no sanitizer/CTest/mutant qualification transferred from AC to integrated C.',
         'Sanitizer source guard finding remains independent; actual preserved ordinary registers are present and equal.']}
    text=json.dumps(result,sort_keys=True,indent=2)+'\n'
    if args.capture:(ROOT/'summary.json').write_text(text)
    else:need((ROOT/'summary.json').read_text()==text,'summary differs')
    print(text,end='')
