#!/usr/bin/env python3
"""Usage: python [-O] check.py SNAPSHOT SOURCE_PILOTE_PRECEDENT DEPOT. Python pur."""
import ast
import contextlib
import copy
import hashlib
import importlib.util
import io
import json
from pathlib import Path
import sys
import subprocess
import tempfile
sys.dont_write_bytecode = True
C=json.loads(Path(__file__).with_name('capture.json').read_text())
S, OLD, REPO = map(Path, sys.argv[1:4])
def need(ok,msg):
    if not ok: raise ValueError(msg)
def sha(b): return hashlib.sha256(b).hexdigest()
for p,h in C['files'].items(): need(sha((S/p).read_bytes())==h,'source modifiée: '+p)
need(sha(OLD.read_bytes())==C['prior_pilote_sha256'],'ancienne source modifiée')
P=S/'morsehgp3D_v12/microbancs/mes_t2d_b/pilote_t2d_b.py'
def functions(p):
    return {x.name:ast.dump(x) for x in ast.parse(p.read_text()).body if isinstance(x,ast.FunctionDef)}
published={}
for path,h in C['published_files'].items():
    content=subprocess.check_output(['git','-C',str(REPO),'show',C['published_commit']+':'+path])
    need(sha(content)==h,'source Git publiée modifiée')
    published[path]=content
changed_published=[p for p in C['files'] if C['files'][p]!=C['published_files'][p]]
need(changed_published==['morsehgp3D_v12/microbancs/mes_t2d_b/pilote_t2d_b.py'], 'autre delta de livraison')
a0=ast.parse(P.read_text());b0=ast.parse(published[changed_published[0]])
need(ast.dump(ast.Module(body=a0.body[1:],type_ignores=[]))==ast.dump(ast.Module(body=b0.body[1:],type_ignores=[])),
     'changement autre que docstring module')
a,b=functions(OLD),functions(P)
changed=sorted(k for k in a.keys()|b.keys() if a.get(k)!=b.get(k))
need(changed==['auto_test_lecture','etape_informations','journal_full_synthetique','lire_full','prise_full'], 'autre delta de fonction')
m=json.loads((P.parent/'bras_t2d_b.json').read_text());postimages={}
for f,v in m['bras']['apres']['fichiers'].items():
    need(sha((S/'morsehgp3D_v12'/f).read_bytes())==v['sha256_apres'],'produit différent du bras après')
    postimages[f]=v['sha256_apres']
sp=importlib.util.spec_from_file_location('pilote_b_capture',P);mod=importlib.util.module_from_spec(sp);sp.loader.exec_module(mod)
output=io.StringIO()
with contextlib.redirect_stdout(output): need(mod.etape_auto_test()==0,'auto-test développeur')
nominal=mod.journal_full_synthetique()
refused={}
mutations={'blocs_ignores':lambda x:x[1].update(c_ns=None),
           'usage_sup_pic':lambda x:x[1]['memoire_octets'].update(C=[10,9]),
           'sous_etages_hors_tmvr':lambda x:x[1]['etapes_ns'].update(T=41),
           'tables_hors_g':lambda x:x[1]['g_ns'].update(tables=11)}
with tempfile.TemporaryDirectory() as tmp:
    path=Path(tmp)/'trace.jsonl'
    def read(rows):
        path.write_text(''.join(json.dumps(x)+'\n' for x in rows))
        return mod.lire_full(str(path),0,2,'ng00',3,8)
    good=read(nominal);need(good['valide'] is True,'positif refusé')
    for label,mut in mutations.items():
        rows=copy.deepcopy(nominal);mut(rows)
        try: read(rows)
        except ValueError as e: refused[label]=str(e)
        else: raise ValueError('résidu encore admis: '+label)
print(json.dumps({'postimages_identiques':postimages,'fonctions_modifiees':changed,
                  'auto_test':output.getvalue().strip(),'nominal_FULL':good,'residus_FULL_refuses':refused,
                  'published_commit':C['published_commit'],'delta_livraison':'docstring module seulement',
                  'scope':'raccord produit livré ; ni moteur ni mesure'},ensure_ascii=False,indent=2,sort_keys=True))
