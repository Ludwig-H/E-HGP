#!/usr/bin/env python3
"""Rejoue uniquement du Python/JSON existant ; applique le patch dans un temporaire."""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import subprocess
import sys
import tempfile

HERE=Path(__file__).resolve().parent


def need(ok,raison):
    if not ok: raise ValueError(raison)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def obj(path):
    return json.loads(path.read_text())


def module(path,name):
    spec=importlib.util.spec_from_file_location(name,path)
    m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);return m


def blob(repo,pin,path):
    return subprocess.check_output(['git','-C',str(repo),'show',pin+':'+path])


def inventory(folder):
    paths=[folder/'campagne.json',folder/'rapport_g_appareil.json',*sorted((folder/'journaux').glob('*.jsonl'))]
    values={str(p.relative_to(folder)):sha(p.read_bytes()) for p in paths}
    return len(paths),sha(json.dumps(values,sort_keys=True,separators=(',',':')).encode())


def main(repo,folder):
    cap=obj(HERE/'capture.json');pin=cap['source_pin']
    before=blob(repo,pin,cap['pilot']);patch=(HERE/'proposition.patch').read_bytes()
    need(sha(before)==cap['pilot_before_sha256'],'source pin')
    need(sha(patch)==cap['patch_sha256'],'patch pin')
    need(sha(blob(repo,pin,'morsehgp3D_v12/microbancs/mes_g_appareil/mes_g_app.cpp'))==cap['emitter_sha256'],'emitter pin')
    need(inventory(folder)==(cap['returned_files'],cap['returned_inventory_sha256']),'returned inventory')
    with tempfile.TemporaryDirectory(prefix='audit-gapp-json-') as td:
        root=Path(td);a=root/'before.py';a.write_bytes(before)
        b=root/cap['pilot'];b.parent.mkdir(parents=True);b.write_bytes(before)
        official=root/cap['official_test'];official_before=blob(repo,pin,cap['official_test'])
        need(sha(official_before)==cap['official_test_before_sha256'],'official gate pin')
        official.write_bytes(official_before)
        legacy=root/'legacy';legacy.mkdir()
        (legacy/'pilote_g_appareil.py').write_bytes(before)
        (legacy/'test_pilote_g_appareil.py').write_bytes(official_before)
        subprocess.run(['git','apply','--check',str(HERE/'proposition.patch')],cwd=root,check=True,capture_output=True)
        subprocess.run(['git','apply',str(HERE/'proposition.patch')],cwd=root,check=True,capture_output=True)
        after=b.read_text();test=root/cap['test']
        need(sha(official.read_bytes())==cap['official_test_after_sha256'],'official gate postimage')
        need(sha(after.encode())==cap['pilot_after_sha256'] and sha(test.read_bytes())==cap['test_after_sha256'],'postimages')
        old=module(a,'gapp_before');new=module(b,'gapp_after');gate=module(test,'gapp_gate')
        old_gate=gate.jouer(old);new_gate=gate.jouer(new)
        need(all(x['ok'] for x in new_gate),'permanent JSON gate')
        need(old.auto_test()==[] and new.auto_test()==[],'official existing auto-tests')
        for path in (legacy/'test_pilote_g_appareil.py',official):
            argv=[sys.executable]+(['-O'] if sys.flags.optimize else [])+['-S',str(path)]
            done=subprocess.run(argv,capture_output=True,text=True)
            need(done.returncode==0,'official gate failed: '+done.stderr)
        by_name={x['cas']:x for x in old_gate}
        primary=('identite_ecrasee','phase_inconnue','surplus_processus','code_bool','passe_bool','code1_identite_vraie')
        need(all(by_name[n]['rendu']=='adopte' for n in primary),'six causal historical witnesses')
        # Chaque mutant retire uniquement la garde qui porte son temoin. Aucun executable natif.
        mutations=[
          ('phases', [('if not passes or phases != attendues:', 'if not passes:')], 'identite_ecrasee'),
          ('cle_unique', [('if cle in d:', 'if False:')], 'cle_json_repetee'),
          ('code_entier', [('type(prise) is not dict or type(prise.get("code")) is not int or prise["code"] not in (0, 1)',
                            'type(prise) is not dict or prise["code"] not in (0, 1)')], 'code_bool'),
          ('passe_entiere', [('any(type(n) is not int for n in numeros) or numeros !=', 'numeros !=')], 'passe_bool'),
          ('cohorte_exacte', [('len(prises) == n', 'len(prises) >= n'),('if len(valides) != campagne["processus"]:', 'if len(valides) < campagne["processus"]:')], 'surplus_processus'),
          ('code_identite', [('if prise["code"] != (0 if resume["identite"] else 1):', 'if False:')], 'code1_identite_vraie'),
          ('drapeau_identite', [('if type(s.get("coherentes")) is not bool or identite.get("identite") is not sortie["identite"]:', 'if False:')], 'drapeau_incoherent'),
          ('mutant_entier', [('type(mutant.get("code")) is not int or mutant.get("code") != 1', 'mutant.get("code") != 1')], 'mutant_code_bool')]
        killed=[]
        for name,replacements,witness in mutations:
            source=after
            for old_text,new_text in replacements:
                need(source.count(old_text)==1,'mutation pattern '+name)
                source=source.replace(old_text,new_text)
            path=root/('mutant_'+name+'.py');path.write_text(source)
            mutant=module(path,'mutant_'+name)
            case=next(c for n,c,_ in gate.cas(mutant) if n==witness)
            result=mutant.juger(case)['verdict']
            need(result=='adopte','causal guard removal '+name)
            killed.append({'mutant':name,'temoin':witness,'sans_garde':result,'avec_garde':'refuse'})
        campaign=obj(folder/'campagne.json');published=obj(folder/'rapport_g_appareil.json')
        need(sha((folder/'campagne.json').read_bytes())==cap['campaign_sha256'] and
             sha((folder/'rapport_g_appareil.json').read_bytes())==cap['report_sha256'],'returned report pins')
        # Les lignes embarquees et les prises originales doivent etre les memes octets/logs.
        total=0
        for takes in campaign['trames'].values():
            for take in takes:
                raw=(folder/'journaux'/take['journal']).read_bytes()
                need(sha(raw)==take['journal_sha256'],'native journal hash')
                need(raw.decode().splitlines()==take['lignes'],'embedded lines differ')
                total+=1
        first=old.juger(campaign);second=new.juger(campaign)
        need(first==second,'real judgment changed')
        differences=[]
        def compare(a,b,path=''):
            need(type(a) is type(b),'published type '+path)
            if type(a) is dict:
                need(set(a)==set(b),'published keys '+path)
                for k in a: compare(a[k],b[k],path+'/'+k)
            elif type(a) is list:
                need(len(a)==len(b),'published list '+path)
                for i,(v,w) in enumerate(zip(a,b)): compare(v,w,path+'/'+str(i))
            elif a!=b:
                need(type(a) is float,'published nonfloat changed '+path)
                differences.append({'path':path,'local':a,'published':b})
        compare(second,{k:published[k] for k in second})
        # Exceptions exactes observees, aucune tolerance generale ni changement des seuils/verdicts.
        need(differences==cap['published_float_differences'],'published arithmetic differs beyond pins')
        need(second['verdict']=='rejete' and not second['refus'],'real performance rejection')
        output={'synthetiques':{'avant':old_gate,'apres':new_gate,'portes_passees':len(new_gate)},
                'auto_tests_officiels_avant_apres':True,'mutants_causaux':killed,
                'reel':{'prises':total,'passes_par_prise':campaign['passes']+1,'verdict':second['verdict'],
                        'jugement_strictement_identique':True,'ecarts_arithmetiques_rapport':differences,'refus':second['refus'],'rejets':second['rejets'],
                        'jugement_sha256':sha(json.dumps(second,sort_keys=True,separators=(',',':')).encode())},
                'patch_applique_seulement_en_copie':True,'moteur_execute':False}
    need(inventory(folder)==(cap['returned_files'],cap['returned_inventory_sha256']),'returned changed during replay')
    return output


if __name__=='__main__':
    ap=argparse.ArgumentParser();ap.add_argument('--repo',type=Path,required=True)
    ap.add_argument('--returned',type=Path,required=True);ap.add_argument('--check',action='store_true');args=ap.parse_args()
    result=main(args.repo,args.returned)
    if args.check: need(result==obj(HERE/'results.json'),'stored results mismatch')
    print(json.dumps(result,indent=2,sort_keys=True,ensure_ascii=False))
