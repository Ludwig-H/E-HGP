#!/usr/bin/env python3
"""Rejeu de juge Python : copies Git, patch isole et JSON ; aucune sonde."""
import argparse
import ast
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile

HERE=Path(__file__).resolve().parent
ROOT='morsehgp3D_v12/microbancs/mes_g_appareil/'


def need(ok,why):
    if not ok:raise ValueError(why)


def sha(b):return hashlib.sha256(b).hexdigest()


def module(path,name):
    spec=importlib.util.spec_from_file_location(name,path);m=importlib.util.module_from_spec(spec)
    spec.loader.exec_module(m);return m


def function(src,name):
    n=next(n for n in ast.parse(src).body if isinstance(n,ast.FunctionDef) and n.name==name)
    return ''.join(src.splitlines(keepends=True)[n.lineno-1:n.end_lineno])


def main(repo,returned):
    cap=json.loads((HERE/'capture.json').read_text());patch=(HERE/'proposition.patch').read_bytes()
    need(sha(patch)==cap['patch_sha256'],'patch hash')
    for p,h in cap['raw_hashes'].items():need(sha((returned/p).read_bytes())==h,'raw hash '+p)
    with tempfile.TemporaryDirectory() as td:
        td=Path(td);old=td/'old';new=td/'new'
        for p,h in cap['source_hashes'].items():
            b=subprocess.check_output(['git','show',cap['pin']+':'+p],cwd=repo)
            need(sha(b)==h,'Git pin '+p)
            for dest in (old,new):
                q=dest/p;q.parent.mkdir(parents=True,exist_ok=True);q.write_bytes(b)
        subprocess.run(['git','apply','--check','-'],input=patch,cwd=new,check=True,capture_output=True)
        subprocess.run(['git','apply','-'],input=patch,cwd=new,check=True,capture_output=True)
        a=module(old/ROOT/'pilote_g_appareil.py','before')
        b=module(new/ROOT/'pilote_g_appareil.py','after')
        cases=module(new/ROOT/'test_journal_strict.py','cases')
        proof={}
        # Les portes officielles restent des tests de juge Python, sans subprocess de sonde.
        env=dict(os.environ,PYTHONDONTWRITEBYTECODE='1')
        for name,folder,count in [('before',old,14),('after',new,15)]:
            cmd=[sys.executable,'-B']+([] if __debug__ else ['-O'])+[str(folder/ROOT/'test_pilote_g_appareil.py')]
            done=subprocess.run(cmd,capture_output=True,text=True,env=env,timeout=120)
            need(done.returncode==0 and ('Ran %d tests'%count) in done.stderr,'official gate '+name+' '+done.stderr)
            proof[name]={'code':done.returncode,'tests':count}
        before=cases.jouer(a);after=cases.jouer(b)
        need(all(x['ok'] for x in after),'synthetic after failures '+repr([x for x in after if not x['ok']]))
        five={'identite_ecrasee','phase_inconnue','surplus_processus','code_bool','passe_bool'}
        need(all(x['rendu']==cases.ADOPTE for x in before if x['cas'] in five),'five original false adoptions')
        original=(old/ROOT/'pilote_g_appareil.py').read_text();fixed=(new/ROOT/'pilote_g_appareil.py').read_text()
        mutants=[('phases','phase_inconnue',fixed.replace(function(fixed,'lire_journal'),function(original,'lire_journal'))),
                 ('code_type','code_bool',fixed.replace(' or type(prise.get("code")) is not int','')),
                 ('passe_type','passe_bool',fixed.replace(' or any(type(n) is not int for n in numeros)','')),
                 ('cohorte','surplus_processus',fixed.replace('type(prises) is list and len(prises) == n','type(prises) is list').replace('len(valides) != campagne["processus"]','len(valides) < campagne["processus"]')),
                 ('code_identite','code1_identite_vraie',fixed.replace('if prise["code"] != (0 if journal[3]["identite"] else 1):','if False:'))]
        causal=[]
        for name,target,body in mutants:
            need(body!=fixed,'mutant no-op '+name)
            p=td/(name+'.py');p.write_text(body);m=module(p,name)
            case=next(c for n,c,_ in cases.cas(b) if n==target)
            got=m.juger(case)['verdicts'];need(got==cases.ADOPTE,'mutant not exposed '+name)
            causal.append({'garde':name,'temoin':target,'sans_garde':got,'avec_garde':cases.REFUSE})
        camp=json.loads((returned/'campagne.json').read_text())
        native_before=a.juger(camp);native_after=b.juger(camp)
        need(native_before==native_after,'real report changed')
        need(native_after['verdicts']=={'D1':'rejete','D2':'rejete'} and not native_after['refus'],'real judgement')
        matched=0
        for shots in camp['trames'].values():
            for shot in shots:
                raw=(returned/'journaux'/shot['journal']).read_bytes()
                need(raw.decode().splitlines()==shot['lignes'],'journal and campaign mismatch')
                need(sha(raw)==shot['journal_sha256'],'journal report hash')
                a.resumer_processus(a.lire_journal(shot['lignes']),camp['passes'],True)
                b.lire_processus(shot,camp['passes'],True);matched+=1
        need(matched==15,'real cohort')
        real_summary={'rapports_avant_apres_egaux':True,'processus_decisifs':matched,
                      'verdicts':native_after['verdicts'],'refus':native_after['refus'],
                      'mutant_cote_code':camp['mutants']['cote_nul']['code'],
                      'mutant_l4_code':camp['mutants']['l4']['code'],
                      'k10_informatif_code':camp['information_k10']['code']}
        return {'official_python_gates':proof,'synthetiques_avant':before,'synthetiques_apres':after,
                'mutants_causaux':causal,'reel':real_summary,
                'post_pilot_sha256':sha(fixed.encode()),'native_invocations':0}


if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('repo',type=Path);p.add_argument('returned',type=Path)
    p.add_argument('--check',action='store_true');args=p.parse_args()
    result=main(args.repo,args.returned)
    if args.check:need(result==json.loads((HERE/'results.json').read_text()),'stored result differs')
    print(json.dumps(result,indent=2,sort_keys=True))
