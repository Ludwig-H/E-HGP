#!/usr/bin/env python3
"""Postimages Git et contre-JSON causaux ; --gates ajoute seulement les sondes Python simulées."""
import argparse
import ast
import copy
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import types

sys.dont_write_bytecode = True
HERE = Path(__file__).resolve().parent
PREFIX = 'morsehgp3D_v12/microbancs/'


def need(ok, why):
    if not ok:
        raise ValueError(why)


def sha(b):
    return hashlib.sha256(b).hexdigest()


def module(body, name, filename='pinned.py'):
    m = types.ModuleType(name)
    m.__file__ = filename
    exec(compile(body, filename, 'exec'), m.__dict__)
    return m


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--repo', type=Path, required=True)
    ap.add_argument('--gates', action='store_true')
    a = ap.parse_args()
    c = json.loads((HERE/'capture.json').read_text())
    git = lambda pin, p: subprocess.check_output(['git', '-C', str(a.repo), 'show', pin+':'+p])
    live = {p:git(c['source_commit'], p) for p in c['source_hashes']}
    for p, h in c['source_hashes'].items():
        need(sha(live[p])==h, 'source '+p)
    for p, h in c['patches'].items():
        need(sha((a.repo/p).read_bytes())==h, 'patch '+p)
    result = {'source_commit':c['source_commit'], 'source_files':len(live), 'native_execution':False}
    with tempfile.TemporaryDirectory(prefix='audit-85db-') as temp:
        root = Path(temp)
        for p in live:
            f=root/p; f.parent.mkdir(parents=True, exist_ok=True)
            f.write_bytes(git(c['base_commit'],p))
        for p in c['patches']:
            subprocess.run(['git','apply','--check',str(a.repo/p)], cwd=root, check=True, capture_output=True)
            subprocess.run(['git','apply',str(a.repo/p)], cwd=root, check=True, capture_output=True)
        exact = ['outils/lecteur_full.py','mes_full/pilote_full.py','mes_full/test_pilote_full.py',
                 'mes_b_scenes/test_pilote_b.py','mes_c_petits/test_pilote_c.py']
        for p in exact:
            need((root/(PREFIX+p)).read_bytes()==live[PREFIX+p], 'postimage '+p)
        pair=PREFIX+'mes_apparie/pilote_apparie.py'
        proposal=(root/pair).read_text()
        before="if type(rounds) is not list or len(rounds) < max(cfg['tours'], 1):"
        need(proposal.count(before)==1, 'contrôle de tours')
        normalized=proposal.replace(before,'if type(rounds) is not list:')
        need(ast.dump(ast.parse(normalized))==ast.dump(ast.parse(live[pair])), 'autre delta AST apparié')
        fakepath=PREFIX+'mes_apparie/test_pilote_apparie.py'
        def fake(b):
            return next(ast.literal_eval(n.value) for n in ast.parse(b).body if isinstance(n,ast.Assign)
                        and any(isinstance(t,ast.Name) and t.id=='FAKE' for t in n.targets))
        need(fake((root/fakepath).read_bytes())==fake(live[fakepath]), 'fixture appariée V6 différente')
        result['postimages_exactes']=exact
        result['apparie_ast_delta']='suppression seule du second contrôle du nombre de tours'
        result['fixture_appariee_fake_exacte']=True
        # Le temporaire bascule maintenant sur les seuls objets Python livrés.
        for p,b in live.items():
            (root/p).write_bytes(b)
        tools=root/(PREFIX+'outils');sys.path.insert(0,str(tools))
        test=module(live[PREFIX+'outils/test_lecteur_full.py'],'clock_fixture',str(tools/'test_lecteur_full.py'))
        new=module(live[PREFIX+'outils/lecteur_full.py'],'new_lf')
        old=module(git(c['base_commit'],PREFIX+'outils/lecteur_full.py'),'old_lf')
        tree=ast.parse(live[PREFIX+'outils/test_lecteur_full.py'])
        function=next(n for n in tree.body if isinstance(n,ast.FunctionDef) and n.name=='check_overlapped')
        expr=next(n.value for n in function.body if isinstance(n,ast.Assign)
                  and any(isinstance(t,ast.Name) and t.id=='mutations' for t in n.targets))
        mutations=eval(compile(ast.Expression(expr),'mutations','eval'), {})
        names=['tour_hors_du_mur','ouvertures_differentes','fins_g_avant_ouverture','noyau_avant_g',
               'm_avant_noyau','r_avant_m','v_avant_m','v_avant_m_ordre_inferieur','maximum_g_faux']
        def output(change=None):
            rows=[dict(phase='open',status='ok',reason='none',wall_ns=5,budget_appareil='separe')]
            for i in range(2):
                row=test.overlapped_row(i)
                if change and i==1:change(row)
                rows += [row,dict(phase='liberation',pass_=i,liberation_ns=3)]
            rows.append(dict(phase='exit',status='ok',reason='none'))
            return test.dump(rows)
        expected=dict(test.ATTENDU,schema='recouvert')
        read=lambda reader, change=None:reader.parse_output(0,output(change),expected)['etat']
        need(read(old)==read(new)=='ok','positif refusé')
        result['neuf_horloges']={}
        for n in names:
            states=[read(old,mutations[n]),read(new,mutations[n])]
            need(states==['ok','illisible'],'contre-flux '+n)
            result['neuf_horloges'][n]=states
        mutant_lib=module(live[PREFIX+'outils/mutants_lecteur_full.py'],'clock_mutants',str(tools/'mutants_lecteur_full.py'))
        witnesses={'sans_enveloppe_tour':names[0],'sans_egalite_ouvertures':names[1],
                   'sans_chaine_par_ordre':names[3],'sans_dependance_m_inferieur':names[7],
                   'sans_maximum_g':names[8]}
        result['mutants_horloges_causaux']={}
        for n,w in witnesses.items():
            find,replace=mutant_lib.MUTANTS[n];body=live[PREFIX+'outils/lecteur_full.py'].decode()
            need(body.count(find)==1,'motif '+n)
            bad=module(body.replace(find,replace),'mutant_'+n)
            need(read(bad)=='ok' and read(bad,mutations[w])=='ok','mutant non causal '+n)
            result['mutants_horloges_causaux'][n]=w
        def edge(row):
            q=row['recouvrement'];opening=q['ouverture_ns']
            for e in row['fins_par_ordre_ns']:e[0]=opening
            q['fin_g_ns']=opening+1;row['etapes_ns']['G']=opening+1
            q['queue_ns']=q['fin_ns']-q['fin_g_ns'];row['etapes_ns']['TMVR']=q['queue_ns']
        need(read(new,edge)=='ok','sentinelle1ns')
        result['sentinelle_1ns']='ok'
        # Residuel historique explicitement non fermé par ces gardes temporelles.
        result['code_false_residuel']=new.parse_output(False,output(),expected)['etat']
        # Une vraie campagne du pilote, avec uniquement sa fausse sonde Python,
        # permet de tester les quatre nouvelles gardes par leur effet sur l'admission.
        pairdir=root/(PREFIX+'mes_apparie');sys.path.insert(0,str(pairdir))
        fixture=module(live[fakepath],'paired_fixture',str(root/fakepath))
        mutants=module(live[PREFIX+'mes_apparie/mutants_pilote_apparie.py'],'paired_mutants',
                       str(pairdir/'mutants_pilote_apparie.py')).MUTANTS
        result['mutants_apparies_causaux']={}
        with tempfile.TemporaryDirectory(prefix='audit-85db-fake-') as work:
            fixture.prepare(work);code,report,folder=fixture.campaign(work,'ok')
            need(code==0 and fixture.pa.judge(report,folder)['verdict']=='juge','positif apparié')
            for name in ['sans_fermeture_binaire','resume_non_compare','cohorte_vide_admise','identite_non_relue']:
                body=live[pair].decode();find,replace=mutants[name]
                need(body.count(find)==1,'motif apparié '+name)
                badjudge=module(body.replace(find,replace),'paired_'+name,str(pairdir/'pilote_apparie.py'))
                need(badjudge.judge(report,folder)['verdict']=='juge','mutant refuse le positif '+name)
                forged=copy.deepcopy(report);missing=None
                if name=='sans_fermeture_binaire':forged['provenance']['sonde_fin_sha256']='ab'*32
                elif name=='resume_non_compare':forged['campagne']['ng00'][0]['cache']['cpu_ns']+=1
                elif name=='cohorte_vide_admise':
                    forged['parametres']['trames']=[]
                    for k in ('identite','campagne','trames'):forged[k]={}
                else:
                    missing=Path(folder)/'journaux/identite/ng02_ref.jsonl'
                    saved=missing.read_bytes();missing.unlink()
                try:
                    before=fixture.pa.judge(forged,folder)['verdict']
                    after=badjudge.judge(forged,folder)['verdict']
                    need(before=='refuse' and after=='juge','mutant apparié non causal '+name)
                    result['mutants_apparies_causaux'][name]={'livre':before,'mutant':after}
                finally:
                    if missing:missing.write_bytes(saved)
        result['gates']=[]
        if a.gates:
            env=dict(os.environ)
            if sys.flags.optimize:env['PYTHONOPTIMIZE']='1'
            else:env.pop('PYTHONOPTIMIZE',None)
            scripts=['outils/test_lecteur_full.py','mes_full/test_pilote_full.py',
                     'mes_apparie/test_pilote_apparie.py','mes_b_scenes/test_pilote_b.py',
                     'mes_c_petits/test_pilote_c.py','outils/mutants_lecteur_full.py',
                     'mes_apparie/mutants_pilote_apparie.py']
            for p in scripts:
                run=subprocess.run([sys.executable,'-B']+(['-O'] if sys.flags.optimize else [])+
                    [str(root/(PREFIX+p))],capture_output=True,text=True,timeout=600,env=env)
                need(run.returncode==0 and not run.stderr,'gate '+p+': '+run.stderr[:600])
                result['gates'].append({'script':p,'code':0,'stdout':run.stdout.strip()})
    print(json.dumps(result,sort_keys=True,indent=2))


if __name__=='__main__':main()
