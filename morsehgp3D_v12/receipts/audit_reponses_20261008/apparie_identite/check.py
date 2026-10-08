#!/usr/bin/env python3
"""Usage python [-O] check.py SNAPSHOT. Fixtures officielles : fausses sondes Python seulement."""
import copy
import hashlib
import importlib.util
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import types
sys.dont_write_bytecode=True
HERE=Path(__file__).resolve().parent
C=json.loads((HERE/'capture.json').read_text())
S=Path(sys.argv[1])
def need(ok,msg):
    if not ok: raise ValueError(msg)
def sha(b):return hashlib.sha256(b).hexdigest()
for p,h in C['files'].items():need(sha((S/p).read_bytes())==h,'capture modifiée')
rel='morsehgp3D_v12/microbancs/mes_apparie/pilote_apparie.py';source=S/rel
need(sha((HERE/'relecture.patch').read_bytes())==C['patch_sha256'],'patch modifié')
with tempfile.TemporaryDirectory() as tmp:
    target=Path(tmp)/rel;target.parent.mkdir(parents=True);target.write_bytes(source.read_bytes())
    subprocess.run(['git','apply','--check',str(HERE/'relecture.patch')],cwd=tmp,check=True,capture_output=True)
    subprocess.run(['git','apply',str(HERE/'relecture.patch')],cwd=tmp,check=True,capture_output=True)
    candidate=target.read_bytes();need(sha(candidate)==C['candidate_sha256'],'postimage modifiée')
p=source.parent/'test_pilote_apparie.py';sp=importlib.util.spec_from_file_location('test_apparie_capture',p)
T=importlib.util.module_from_spec(sp);sp.loader.exec_module(T);old=T.pa
new=types.ModuleType('pilote_apparie_propose');new.__file__=str(source);exec(compile(candidate,str(source),'exec'),new.__dict__)
with tempfile.TemporaryDirectory() as tmp:
    T.prepare(tmp);code,report,folder=T.campaign(tmp,'ok')
    need(code==0 and report is not None,'fixture officielle non produite')
    baseline=old.judge(report,folder);need(baseline['verdict']=='juge' and baseline['cas']['cache']['verdict']=='adopte','positif ancien')
    need(new.judge(report,folder)==baseline,'positif refusé par proposition')
    result={};folder=Path(folder)
    def judge_case(name,r):
        before=old.judge(r,str(folder));after=new.judge(r,str(folder))
        need(before==baseline,'ancien ne reproduit pas le témoin '+name)
        need(after['verdict']=='refuse' and after['refus'],'nouveau admet '+name)
        result[name]={'ancien':'juge/cache adopte','propose':after['verdict'],'refus':after['refus']}
    # Supprimer toutes les preuves d'identité, sans toucher aux chronos.
    originals={p:p.read_bytes() for p in (folder/'journaux/identite').glob('*.jsonl')}
    for p in originals:p.unlink()
    judge_case('douze_identites_absentes',copy.deepcopy(report))
    for p,b in originals.items():p.write_bytes(b)
    # Changer les FUL1 réels d'un bras et mettre son SHA à jour ; résumé de l'identité inchangé.
    r=copy.deepcopy(report);take=r['identite']['ng00']['cache'];p=folder/'journaux/identite/ng00_cache.jsonl'
    rows=[json.loads(x) for x in p.read_text().splitlines()]
    for row in rows:
        if row.get('phase')=='full':row['full_sha256']='cd'*32
    p.write_text(''.join(json.dumps(x)+'\n' for x in rows));take['journal_sha256']=sha(p.read_bytes())
    judge_case('empreinte_brute_modifiee_rehachee',r);p.write_bytes(originals[p])
    r=copy.deepcopy(report);r['identite']['ng00']['cache']['empreintes']=[]
    judge_case('empreinte_bras_vide_union_unique',r)
    r=copy.deepcopy(report)
    for tour in r['campagne']['ng00']:
        take=tour['cache'];take['etapes_ns']['P']=999_000_000;take['cpu_ns']=888_000_000;take['pic_octets']=777*2**20
    judge_case('tableaux_ressources_et_etapes_forges',r)
    old_lines=old.tables(report,baseline).splitlines();bad_lines=old.tables(r,old.judge(r,str(folder))).splitlines()
    find=lambda lines:next(x for x in lines if x.startswith('| ng00 | `cache` |'))
    result['tableaux_ressources_et_etapes_forges']['table_avant']=find(old_lines)
    result['tableaux_ressources_et_etapes_forges']['table_fausse_admise']=find(bad_lines)
    # Témoin positif inchangé après restauration de toutes les sources de preuve.
    need(new.judge(report,str(folder))==baseline,'restauration incomplète')
print(json.dumps({'positif':'juge/cache adopte, seq rejete, A/A controle', 'contre_exemples':result,
                  'candidate_sha256':C['candidate_sha256'],
                  'scope':'fixtures Python officielles, cadre essai ; aucun résultat réel ni moteur'},
                 ensure_ascii=False,sort_keys=True,indent=2))
