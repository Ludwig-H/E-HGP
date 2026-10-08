#!/usr/bin/env python3
"""Application textuelle seule ; reutilise les deux lecteurs publies."""
import argparse, hashlib, importlib.util, json, subprocess, tempfile, types
from pathlib import Path
HERE=Path(__file__).resolve().parent
PIN='45976be8ddc489f14494937b13e43d0ca5fa0a55'
SRC='583d9778d5b8f6c3f8972c7f19f62c3b8174d897f996461392bc6103be28740d'
P='morsehgp3D_v12/'
def need(ok):
    if not ok: raise ValueError('pin, cohorte ou motif different')
def sha(b): return hashlib.sha256(b).hexdigest()
def show(repo,pin,path): return subprocess.check_output(['git','show',pin+':'+path],cwd=repo)
def load(path):
    spec=importlib.util.spec_from_file_location('audit_helper',path);m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);return m
p=argparse.ArgumentParser();p.add_argument('--repo',type=Path,required=True);p.add_argument('--gc',type=Path,required=True);p.add_argument('--tmvr',type=Path,required=True);a=p.parse_args()
r=a.repo/'morsehgp3D_v12/receipts';old=r/'audit_reponses_20261008/d2_memo_mutant';c=r/'composition_gc_tmvr_20261008'
pins=json.loads((old/'pins.json').read_text());cap=json.loads((c/'capture.json').read_text());entry=json.loads((HERE/'mutant.json').read_text())
source=show(a.gc/'git3',PIN,P+'src/tower/resolve.cpp');need(sha(source)==SRC)
patches=[(a.gc/x['name']).read_bytes() for x in cap['patches'][:2]]+[(a.tmvr/cap['patches'][2]['name']).read_bytes()]
need(list(map(sha,patches))==[x['sha256'] for x in cap['patches']]);counts=[]
with tempfile.TemporaryDirectory() as td:
    root=Path(td)
    for item in pins['inputs']:
        if item['path'].endswith(('/run_mutants.py','/mhgp12_gate.py')):
            b=show(a.repo,pins['pin'],item['path']);need(sha(b)==item['sha256']);f=root/item['path'];f.parent.mkdir(parents=True,exist_ok=True);f.write_bytes(b)
    runner=load(root/(P+'tests/mutants/run_mutants.py'))
    def judge(raw,n):
        j=json.loads(raw);need(len(j['mutants'])==n and j['plancher']==n);j['mutants'].append(entry);j['plancher']=n+1
        f=root/'manifest.json';f.write_text(json.dumps(j));parsed=runner.load_manifest(str(f))
        need(len(parsed['mutants'])==n+1);s=root/'src/tower/resolve.cpp';s.parent.mkdir(parents=True,exist_ok=True);s.write_bytes(source)
        changed=runner.mutated_files(str(root),parsed['mutants'][-1]);need(len(runner.edits_of(entry))==4 and list(changed)==['src/tower/resolve.cpp']);counts.append(n+1)
    judge(show(a.gc/'git3',PIN,P+'tests/mutants/tower.json'),18)
    compose=load(c/'check.py')
    class Capture:
        def __init__(self,**kw): self.t=tempfile.TemporaryDirectory(**kw)
        def __enter__(self): self.path=Path(self.t.__enter__());return str(self.path)
        def __exit__(self,*exc):
            if exc[0] is None:
                combined=self.path/'combined';need((combined/(P+'src/tower/resolve.cpp')).read_bytes()==source);judge((combined/(P+'tests/mutants/tower.json')).read_bytes(),27)
            return self.t.__exit__(*exc)
    compose.tempfile=types.SimpleNamespace(TemporaryDirectory=Capture)
    result,_=compose.compose(a.repo,patches,cap['base'],(c/'raccord.patch').read_bytes());need(result==cap['result'])
need(counts==[19,28]);print('date_gc_ok: quatre motifs uniques ; cohortes 19/28 ; aucun natif')
