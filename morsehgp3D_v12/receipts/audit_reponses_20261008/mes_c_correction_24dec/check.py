#!/usr/bin/env python3
"""Rejeu Git MES-C 24dec : seulement portes Python et sondes fictives."""
import argparse
import ast
import hashlib
import itertools
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
from types import SimpleNamespace

HERE = Path(__file__).resolve().parent
BASE = 'morsehgp3D_v12/microbancs/'
PILOT = BASE+'mes_c_petits/pilote_c.py'
TEST = BASE+'mes_c_petits/test_pilote_c.py'
MUTANTS = BASE+'mes_c_petits/mutants_pilote_c.py'
PATCH = 'morsehgp3D_v12/receipts/audit_reponses_20261008/mes_c_livraison/cohorte_et_porte.patch'


def need(ok, why):
    if not ok:
        raise ValueError(why)


def tree_function(body, name):
    return next(n for n in ast.parse(body).body if isinstance(n, ast.FunctionDef) and n.name==name)


def same(a,b):
    return ast.dump(a,include_attributes=False)==ast.dump(b,include_attributes=False)


def main():
    ap=argparse.ArgumentParser();ap.add_argument('--repo',type=Path,required=True);args=ap.parse_args()
    cap=json.loads((HERE/'capture.json').read_text());blobs={}
    for version,sources in cap['sources'].items():
        blobs[version]={}
        for name,digest in sources.items():
            raw=subprocess.check_output(['git','-C',str(args.repo),'show',cap['pins'][version]+':'+name])
            need(hashlib.sha256(raw).hexdigest()==digest,'source différente')
            blobs[version][name]=raw
    out={}
    # Les subprocess des portes sont la sonde Python fabriquée et cmake --version (lecture d'environnement).
    # --essai empêche toute construction, nvcc et nvidia-smi. Aucun binaire moteur n'est fourni.
    env=dict(os.environ,PYTHONDONTWRITEBYTECODE='1')
    def run(argv,folder):
        r=subprocess.run(argv,cwd=folder,env=env,capture_output=True,text=True,timeout=60)
        return dict(code=r.returncode,stdout=r.stdout.strip(),stderr=r.stderr.strip())
    with tempfile.TemporaryDirectory(prefix='audit-mesc24-') as tmp:
        root=Path(tmp);old=root/'proposition';new=root/'livre'
        for folder,version in ((old,'avant'),(new,'livre')):
            for name,raw in blobs[version].items():
                target=folder/name;target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes(raw)
        patch=root/'proposition.patch';patch.write_bytes(blobs['livre'][PATCH])
        for opts in (['--check'],[]):
            r=run(['git','apply',*opts,str(patch)],old);need(r['code']==0,'application proposition')
        proposed=(old/PILOT).read_bytes();delivered=(new/PILOT).read_bytes()
        need(same(tree_function(proposed,'verdicts'),tree_function(delivered,'verdicts')),'cohorte différente')
        unchanged=('fit','session_values','fits_of','probe_argv','expected')
        for name in unchanged:
            need(same(tree_function(blobs['avant'][PILOT],name),tree_function(delivered,name)),name+' changé')
        constants=lambda body:{n.targets[0].id:ast.dump(n.value,include_attributes=False)
            for n in ast.parse(body).body if isinstance(n,ast.Assign) and isinstance(n.targets[0],ast.Name)
            and n.targets[0].id in ('MAIN_REGIME_NS_PER_SITE','FIXED_LIMIT_NS')}
        need(constants(blobs['avant'][PILOT])==constants(delivered),'seuil déplacé')
        # Compare les 108 combinaisons de l'ancien bloc proposé au helper livré.
        main_node=tree_function(proposed,'main')
        branch=next(n for n in main_node.body if isinstance(n,ast.If) and
                    isinstance(n.test,ast.Attribute) and n.test.attr=='essai' and
                    any(isinstance(s,ast.Subscript) for s in ast.walk(n)))
        ns={};exec(compile(ast.Module(body=[tree_function(delivered,'overall')],type_ignores=[]),'overall','exec'),ns)
        cases=0
        for states in itertools.product(('tenu','non tenu','non evalue'),repeat=3):
            crit={c:dict(etat=s) for c,s in zip(('C1','C2','C3'),states)}
            for essai,controlled in itertools.product((False,True),repeat=2):
                controls=['x'] if controlled else [];scope=dict(args=SimpleNamespace(essai=essai),controls=controls,
                                                              crit=crit,report={})
                exec(compile(ast.Module(body=[branch],type_ignores=[]),'ancien_verdict','exec'),scope)
                need(ns['overall'](essai,controls,crit)==scope['report']['verdict'],'refactor verdict')
                cases+=1
        for key,flags in (('porte_normal',[]),('porte_optimized',['-O'])):
            r=run([sys.executable,'-B','-S',*flags,str(new/TEST)],new)
            need(r['code']==0 and not r['stderr'],'porte');out[key]=r
        r=run([sys.executable,'-B','-S',str(new/MUTANTS)],new)
        need(r['code']==0 and not r['stderr'],'runner mutants');out['runner_officiel']=r
        assign=next(n for n in ast.parse(blobs['livre'][MUTANTS]).body if isinstance(n,ast.Assign)
                    and isinstance(n.targets[0],ast.Name) and n.targets[0].id=='MUTANTS')
        mutations=ast.literal_eval(assign.value);need(len(mutations)==11,'mutants incomplets')
        causal={}
        for name,(before,after) in mutations.items():
            text=delivered.decode();need(text.count(before)==1,'motif mutant')
            text=text.replace(before,after);ast.parse(text);(new/PILOT).write_text(text)
            r=run([sys.executable,'-B','-S',str(new/TEST)],new)
            need(r['code']==1 and r['stderr'] and 'Traceback' not in r['stderr'],'mort non causale '+name)
            causal[name]=r['stderr'].splitlines()[0]
        (new/PILOT).write_bytes(delivered)
    out.update(cohorte_ast_proposition_identique=True,refactor_verdict_cas=cases,seuils_et_fonctions_inchanges=list(unchanged),
               mutants_diagnostics=causal,native_execution=False,remote_execution=False)
    print(json.dumps(out,indent=2,sort_keys=True,ensure_ascii=False))


if __name__=='__main__':
    main()
