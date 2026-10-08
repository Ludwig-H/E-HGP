#!/usr/bin/env python3
"""D6 livré : lecteurs et plans simulés seulement ; aucune exécution HGP."""
import argparse,copy,hashlib,importlib.util,json,subprocess,tempfile,types
from pathlib import Path
H=Path(__file__).resolve().parent
P='morsehgp3D_v12/'
def need(ok,msg):
    if not ok: raise ValueError(msg)
def sha(b): return hashlib.sha256(b).hexdigest()
def module(path):
    s=importlib.util.spec_from_file_location('audit_plan',path);m=importlib.util.module_from_spec(s);s.loader.exec_module(m);return m
def load(source):
    m=types.ModuleType('audit_d6');exec(compile(source,'pilote_d6.py','exec'),m.__dict__);return m

def replay(repo,local=None):
    cap=json.loads((H/'capture.json').read_text());rel=cap['source_path']
    old=subprocess.check_output(['git','show',cap['base_commit']+':'+rel],cwd=repo)
    new=subprocess.check_output(['git','show',cap['commit']+':'+rel],cwd=repo)
    need(sha(old)==cap['base_sha256'] and sha(new)==cap['source_sha256'],'source différente')
    for path,h in cap['dependencies'].items():need(sha((repo/path).read_bytes())==h,'dépendance différente '+path)
    with tempfile.TemporaryDirectory() as temp:
        dst=Path(temp)/rel;dst.parent.mkdir(parents=True);dst.write_bytes(old)
        for path in cap['patches']:
            subprocess.run(['git','apply',str(repo/path)],cwd=temp,check=True,capture_output=True)
        need(dst.read_bytes()==new,'livraison différente des deux patches')
    m=load(new);fixture=repo/(P+'receipts/audit_reponses_20261007/t2c_pilote_proposition')
    before,after=([json.loads(l) for l in (fixture/(name+'.jsonl')).read_text().splitlines()] for name in ('before','after'))
    expected=dict(passes=2,coord_bits=21,kmax=5,threads=1,leaf=24)
    def admits(rows,config=expected):return m.summarize(0,rows,'tour_g','resolution_sha256',config)['ok']
    need(admits(before) and admits(after),'format réel refusé')
    names=set(after[0]['diagnostics'])-set(before[0]['diagnostics']);refused=[]
    def reject(name,rows):need(not admits(rows),'corruption admise '+name);refused.append(name)
    for key in sorted(names):
        rows=copy.deepcopy(after);rows[0]['diagnostics'].pop(key);reject('missing_'+key,rows)
        rows=copy.deepcopy(after);v=rows[0]['diagnostics'][key]
        if type(v) is list:v[0]=False
        else:rows[0]['diagnostics'][key]=False
        reject('bool_'+key,rows)
        if type(after[0]['diagnostics'][key]) is list:
            rows=copy.deepcopy(after);rows[0]['diagnostics'][key].pop();reject('length_'+key,rows)
    rows=copy.deepcopy(after);rows[0]['diagnostics']['extra']=0;reject('unknown',rows)
    rows=copy.deepcopy(after);rows[1]['diagnostics']=copy.deepcopy(before[1]['diagnostics']);reject('mix_after_before',rows)
    rows=copy.deepcopy(before);rows[1]['diagnostics']=copy.deepcopy(after[1]['diagnostics']);reject('mix_before_after',rows)
    for label,v in [('negative',-1),('float',0.0),('overflow',1<<64)]:
        rows=copy.deepcopy(after);rows[0]['diagnostics']['prepare_ns']=v;reject(label,rows)
    k1=[r for r in copy.deepcopy(after) if r['phase']!='ordre' or r['k']==1]
    for row in k1:
        if row['phase']=='tour_g':
            row['kmax']=1
            for key in ('order_ns','pass_ns'):row['diagnostics'][key]=row['diagnostics'][key][:1]
            for key in ('table_ns','join_ns'):row['diagnostics'][key]=[]
    need(admits(k1,dict(expected,kmax=1)),'K1 de forme refusé')
    plan=module(repo/(P+'receipts/audit_d6_20261007/pilotage/check.py'))
    specs=[dict(name='reference_present',largest=1),dict(name='u21_absent_u24_present',largest=1<<21),
           dict(name='all_combinations_absent',largest=1<<21,profiles='21'),
           dict(name='duplicate_profile',largest=1,profiles='21,21,24'),dict(name='duplicate_case',largest=1,cases='test,test')]
    plans=[dict(case=s['name'],result=plan.witness(new.decode(),s)) for s in specs]
    need([p['result']['code'] for p in plans]==[0,2,2,2,2],'codes plan')
    need([p['result']['takes'] for p in plans]==[6,0,0,0,0],'prises plan')
    out={'patches_byte_identical_to_delivery':True,'native_formats_admitted':['G','Gc'],
         'corruptions_refused':refused,'synthetic_K1_admitted':True,'plans':plans,'native_executed':False}
    need(len(refused)==27,'cohorte 27')
    if local is not None:
        for path,h in cap['local_files'].items():need(sha((local/path).read_bytes())==h,'trace locale modifiée '+path)
        report=json.loads((local/'mes_d6.json').read_text());taken=[]
        need(report['controles']==[] and len(report['prises'])==4,'bilan local')
        need({(t['cas'],t['profil'],t['facteur'],t['tour']) for t in report['prises']}==
             {('ng00',p,f,0) for p in (21,24) for f in (1,8)},'cohorte locale')
        for t in report['prises']:
            tag=f"ng00_p{t['profil']}_x{t['facteur']}_t0"
            for phase,suffix,digest in [('catalogue','catalogue','catalogue_sha256'),('tour_g','tour','resolution_sha256')]:
                saved=t[phase];cfg=dict(passes=2,coord_bits=t['profil'],kmax=3,threads=3,leaf=24)
                need(saved['configuration']==cfg,'configuration locale')
                lines=[json.loads(l) for l in (local/'brut'/f'{tag}_{suffix}.jsonl').read_text().splitlines()]
                got=m.summarize(saved['code'],lines,phase,digest,cfg,exported=phase=='catalogue' and t['facteur']==1)
                need(got['ok'],'journal local refusé')
                # Le rapport JSON transforme notamment les clés d’ordre entières en chaînes.
                stored=json.loads(json.dumps(got))
                need(all(saved[k]==v for k,v in stored.items()),'résumé local différent')
                taken.append([tag,phase,len(got['passes_ms'])])
        for path,h in cap['local_files'].items():need(sha((local/path).read_bytes())==h,'trace locale modifiée pendant lecture')
        out['local']={'report_controls':[],'journals_admitted':len(taken),'stage_passes':sum(t[2] for t in taken),
                      'process_combinations':4,'profiles':[21,24],'factors':[1,8],'kmax':3,'threads':3,'passes':2,
                      'summaries_recomputed':True,'payloads_read':False,'external_return_codes_verified':False}
    return out
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--repo',type=Path,required=True);p.add_argument('--local',type=Path);p.add_argument('--check',action='store_true');a=p.parse_args()
    try:out=replay(a.repo.resolve(),a.local)
    except (ValueError,KeyError,OSError,subprocess.CalledProcessError) as e:p.exit(2,'REFUS: '+str(e)+'\n')
    if a.check:
        expected=json.loads((H/'resultats.json').read_text())
        if a.local is None:expected.pop('local',None)
        need(out==expected,'résultats différents')
    print(json.dumps(out,ensure_ascii=False,sort_keys=True,indent=2))
