#!/usr/bin/env python3
"""Relecture du recu J ; aucun moteur, build ni appel GCP."""
import argparse,collections,hashlib,json,math,re,subprocess,tarfile,types
from pathlib import Path
HERE=Path(__file__).resolve().parent
P='morsehgp3D_v12/'
def need(ok,msg):
    if not ok:raise RuntimeError(msg)
def sha(b):return hashlib.sha256(b).hexdigest()
def selected(path,pilot=True):
    tail=path.removeprefix(P)
    return path.startswith(P) and (tail=='CMakeLists.txt' or tail.startswith(('src/','bench/','cmake/','tests/','reference/')) or (pilot and tail.startswith('microbancs/mes_t2c_g/')))
def gates(text):
    return dict(re.findall(r'^\s*\d+/\d+ Test\s+#\d+:\s+(\S+)\s+\.+(.*)$',text,re.M))
def same_stats(a,b):
    if type(a) is float and type(b) is float:return abs(a-b)<=4*max(math.ulp(a),math.ulp(b))
    if type(a)!=type(b):return False
    if isinstance(a,dict):return a.keys()==b.keys() and all(same_stats(a[k],b[k]) for k in a)
    if isinstance(a,list):return len(a)==len(b) and all(same_stats(x,y) for x,y in zip(a,b))
    return a==b

def replay(repo,external=None):
    pins=json.loads((HERE/'pins.json').read_text());root=repo/pins['receipt']
    def git(*args):return subprocess.check_output(['git',*args],cwd=repo)
    for name,h in pins['files'].items():need(sha((root/name).read_bytes())==h,'recu modifie: '+name)
    tracked=set(git('ls-tree','-r','--name-only',pins['publication'],'--',pins['receipt']).decode().splitlines())
    files=[]
    for line in (root/'SHA256SUMS').read_text().splitlines():
        h,name=line.split(maxsplit=1);name=name.lstrip('*');need((root/name).is_file(),'fichier absent')
        need(sha((root/name).read_bytes())==h,'hash different: '+name);need(pins['receipt']+'/'+name in tracked,'fichier non livre');files.append(name)
    r=json.loads((root/'receipt.json').read_text())
    need(r['closure']=='stopped' and r['status']=='completed' and r['worker_exit_code']==0,'cloture')
    need(r['targeted_shutdown_certified'] and r['stop_exit_code']==0 and r['observed_after']['status']=='TERMINATED','arret')
    need(r['start_certified'] and r['guest_guard_intact'] and r['results_verified'] and not r['errors'] and not r['warnings'],'gardes')
    need(all(x['status']=='ok' and x['exit_code']=='0' for x in r['commands']) and len(r['commands'])==4,'commandes')
    need(int(r['remote_summary']['overflow_files'])==0 and int(r['remote_summary']['truncated_streams'])==0,'troncature')
    manifest={x['path']:x for x in r['source']['manifest']};source_paths=[x for x in manifest if selected(x)]
    expected={x for x in git('ls-tree','-r','--name-only',pins['source']).decode().splitlines() if selected(x)}
    need(set(source_paths)==expected,'ensemble source incomplet')
    for path in source_paths:
        data=git('show',pins['source']+':'+path);item=manifest[path]
        need(len(data)==item['size'] and sha(data)==item['sha256'],'source differente: '+path)
    data=git('show',pins['source']+':'+pins['pilot']);need(sha(data)==pins['pilot_sha256'],'pilote different')
    m=types.ModuleType('t2c_livre');m.__file__=str(repo/pins['pilot']);exec(compile(data,m.__file__,'exec'),m.__dict__)
    folder=root/'resultats/cmd/001_t2c_pilote/files/t2c';report=json.loads((folder/'rapport_t2c.json').read_text())
    judgment=json.loads(json.dumps(m.juger(report,str(folder))))
    need(same_stats(judgment,report['jugement']),'jugement different')
    need(not judgment['refus'],'admission refusee')
    counts=collections.Counter();warm=collections.Counter();records=[]
    def check(take,k,w,p,arm,group):
        path=folder/take['journal'];need(sha(path.read_bytes())==take['journal_sha256'],'journal')
        fresh=m.lire_prise(str(path),take['code'],k,w,p,m.schema_bras(arm),arm=='profil')
        need(fresh['valide'] and all(take.get(key)==value for key,value in fresh.items()),'resume different')
        counts[group]+=1;warm[group]+=p-1;records.append(take['journal'])
    camp=report['campagne_k5'];need((camp['fils'],camp['passes'],camp['tours_demandes'])==(48,10,10),'configuration')
    for frame,turns in camp['trames'].items():
        for turn in turns:
            for arm,take in turn.items():check(take,5,48,10,arm,'k5')
    for group,cfg in report['informations'].items():
        if not isinstance(cfg,dict):continue
        for turns in cfg['tours'].values():
            for turn in turns:
                for arm,take in turn.items():check(take,cfg['k'],cfg['fils'],cfg['passes'],arm,group)
    need(len(records)==len(set(records))==188,'journaux incomplets')
    positions={b:[0]*5 for b in m.BRAS_JUGES}
    for t in range(10):
        order=[(t+i)%5 for i in range(5)]
        if t%2:order.reverse()
        for at,i in enumerate(order):positions[m.BRAS_JUGES[i]][at]+=1
    need(all(x==[2]*5 for x in positions.values()),'positions')
    need(report['construction']['ablations']==json.loads(json.dumps(m.ABLATIONS)),'ablations')
    gc={}
    for name,total in [('000_socle_ctest',675),('002_lidar_ctest',6),('003_mutants_tour',1)]:
        parsed=gates((root/'resultats/cmd'/name/'stdout').read_text());need(len(parsed)==total and all('Passed' in x for x in parsed.values()),'portes');gc[name]=len(parsed)
    # L'archive source externe est du code ; elle n'est pas copiee dans le recu d'audit.
    if external:
        archive=external/'v12_tour_Gc/v12_src_avant_t2c.tar.gz';need(sha(archive.read_bytes())==pins['external_before_archive_sha256'],'archive avant')
        before=set()
        with tarfile.open(archive) as tar:
            for item in tar.getmembers():
                path=item.name.removeprefix('./')
                if item.isfile() and selected(path,False):
                    need(tar.extractfile(item).read()==git('show',pins['before']+':'+path),'source avant');before.add(path)
        expected={x for x in git('ls-tree','-r','--name-only',pins['before']).decode().splitlines() if selected(x,False)}
        need(before==expected and len(before)==338,'archive incomplete')
        need(sha((external/'v12_tour_Gc/plan_t2c_g.json').read_bytes())==pins['external_plan_sha256'],'plan')
        local=external/'build_v12_u21.ctest_gc2.log';need(sha(local.read_bytes())==pins['local_ctest_sha256'],'journal local')
        l=gates(local.read_text());v=gates((root/'resultats/cmd/000_socle_ctest/stdout').read_text())
        need(set(v)<=set(l) and len(set(l)-set(v))==26 and all(n.startswith('mhgp12_reference_diff_v10') for n in set(l)-set(v)),'perimetre')
    return dict(source_code_test_files=len(source_paths),checksummed_committed_files=len(files),shutdown=r['observed_after']['status'],
        last_stop=r['observed_after']['lastStopTimestamp'],worker_exit_code=r['worker_exit_code'],commands_ok=4,ctest=gc,
        journal_groups=dict(counts),warm_passes_by_group=dict(warm),journals=len(records),verdicts=judgment['verdicts'],
        statistics_agree_within_4_ulp=True,strict_summaries_equal=True,positions_per_arm=[2]*5,source_before_files_captured=338,
        native_runs_by_auditor=0,gcp_calls_by_auditor=0)
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--repo',type=Path,default=HERE.parents[3]);p.add_argument('--external',type=Path);p.add_argument('--check',action='store_true');a=p.parse_args();out=replay(a.repo.resolve(),a.external)
    if a.check:need(out==json.loads((HERE/'preuves.json').read_text()),'resultat different');print('session_j_preuves_ok: sources, 188 journaux, jugement et arret archive ; aucun moteur')
    else:print(json.dumps(out,sort_keys=True,indent=1))
