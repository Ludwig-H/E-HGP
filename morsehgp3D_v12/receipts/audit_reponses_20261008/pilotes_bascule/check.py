#!/usr/bin/env python3
"""Usage: python [-O] check.py DEPOT SNAPSHOT [--gates]. Pas de sonde native."""
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
REPO,SNAPSHOT=map(Path,sys.argv[1:3])
def need(ok,msg):
    if not ok:raise ValueError(msg)
def sha(b):return hashlib.sha256(b).hexdigest()
for p,h in C['files'].items():
    need(sha((SNAPSHOT/p).read_bytes())==h,'capture modifiée')
    need(sha(subprocess.check_output(['git','-C',str(REPO),'show',C['commit']+':'+p]))==h,'Git différent')
p='morsehgp3D_v12/microbancs/mes_full/pilote_full.py';source=(SNAPSHOT/p).read_bytes()
need(sha((HERE/'tableau.patch').read_bytes())==C['patch_sha256'],'patch modifié')
with tempfile.TemporaryDirectory() as t:
    f=Path(t)/p;f.parent.mkdir(parents=True);f.write_bytes(source)
    subprocess.run(['git','apply','--check',str(HERE/'tableau.patch')],cwd=t,check=True,capture_output=True)
    subprocess.run(['git','apply',str(HERE/'tableau.patch')],cwd=t,check=True,capture_output=True)
    candidate=f.read_bytes();need(sha(candidate)==C['postimage_sha256'],'postimage incorrecte')
def mod(text):
    m=types.ModuleType('pilote_full_capture');m.__file__=str(SNAPSHOT/p)
    exec(compile(text,m.__file__,'exec'),m.__dict__);return m
before,after=mod(source),mod(candidate)
results={}
for mode in ('recouvert','sequentiel'):
    e={k:1 for k in ('P','C','G','raccord','TMVR','T','M','V','R')
       if mode=='sequentiel' or k in ('P','C','G','raccord','TMVR')}
    v=dict(sites=8,mediane_ns=10,max_medianes_ns=11,max_ns=12,premiere_ns=13,
           etapes_ns=e,c_ns={'transferts':1},pic_octets=1000)
    tables=[]
    for m in (before,after):
        m.MODE['schema']=mode
        tables.append([line for line in m.table('temoin',{'ok':v,'absent':None}) if line.startswith('|')])
    counts=[[line.count('|')-1 for line in table] for table in tables]
    need(counts[0][1]==counts[0][0]+1 and counts[0][2]==counts[0][0]+1,'témoin changé')
    need(len(set(counts[1]))==1 and tables[0][0]==tables[1][0] and tables[0][-1]==tables[1][-1],
         'correctif ne conserve pas les valeurs/en-têtes')
    results[mode]={'avant':counts[0],'propose':counts[1],'valeurs_identiques':True}
if '--gates' in sys.argv:
    for name,expected in C['official_tests'].items():
        cmd=[sys.executable,'-B']+(['-O'] if sys.flags.optimize else [])
        cmd.append(str(SNAPSHOT/'morsehgp3D_v12/microbancs'/name))
        run=subprocess.run(cmd,text=True,capture_output=True)
        need(run.returncode==0 and run.stdout.strip()==expected and not run.stderr,'porte Python '+name)
print(json.dumps({'tableaux':results,'portes_officielles':C['official_tests'],
                  'scope':'orchestration Python fictive ; aucune qualification moteur ou temporelle'},
                 ensure_ascii=False,sort_keys=True,indent=2))
