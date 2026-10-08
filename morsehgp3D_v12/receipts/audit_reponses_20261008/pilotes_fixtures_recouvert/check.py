#!/usr/bin/env python3
"""Only synthetic Python probes and the four official Python pilot gates; no native engine."""
import argparse
import ast
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import types

HERE=Path(__file__).resolve().parent
LF='morsehgp3D_v12/microbancs/outils/lecteur_full.py'

def need(ok,message):
    if not ok:
        raise ValueError(message)

def sha(data):
    return hashlib.sha256(data).hexdigest()

def reader(data):
    mod=types.ModuleType('clock_reader')
    exec(compile(data,LF,'exec'),mod.__dict__)
    return mod

def fake(source, mode='ok'):
    tree=ast.parse(source)
    node=next(n for n in tree.body if isinstance(n,ast.Assign) and
              any(isinstance(t,ast.Name) and t.id=='FAKE' for t in n.targets))
    body=ast.literal_eval(node.value)
    if 'LINES = %r' in body:
        return body % (mode,{'uniform':(1000,1)},{'petite':'uniform'})
    return body % mode

def main():
    p=argparse.ArgumentParser();p.add_argument('--repo',type=Path,required=True);args=p.parse_args()
    c=json.loads((HERE/'capture.json').read_text())
    fixtures=HERE/'proposition.patch'; guards=HERE.parent/'lf_recouvert_gardes/proposition.patch'
    need(sha(fixtures.read_bytes())==c['fixture_patch_sha256'],'fixture patch pin')
    need(sha(guards.read_bytes())==c['lf_patch_sha256'],'reader patch pin')
    result={'source_commit':c['source_commit'],'fixtures':{},'official_python_gates':{}}
    python=[sys.executable,'-B']+(['-O'] if sys.flags.optimize else [])
    with tempfile.TemporaryDirectory(prefix='audit-fixture-clocks-') as tmp:
        root=Path(tmp);before={}
        for path,expected in c['sources'].items():
            data=subprocess.check_output(['git','show',c['source_commit']+':'+path],cwd=args.repo)
            need(sha(data)==expected,'source pin '+path)
            target=root/path;target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes(data)
            before[path]=data
        old=reader(before[LF])
        for patch in (guards,fixtures):
            for check in (True,False):
                subprocess.run(['git','apply']+(['--check'] if check else [])+[str(patch)],
                               cwd=root,check=True,capture_output=True)
        need(sha((root/LF).read_bytes())==c['lf_candidate_sha256'],'reader postimage')
        new=reader((root/LF).read_bytes())
        xyz=root/'synthetic.u32';xyz.write_bytes(bytes(12*1000))
        ids=root/'synthetic.ids';ids.write_bytes(bytes(4*1000))
        want=dict(voie='cpu',k=5,fils=3,passes=2,empreinte=True,trames=[('petite',1000)],
                  budget_appareil='partage',bits=21,schema='recouvert')
        for path,expected in c['fixture_after'].items():
            after=(root/path).read_bytes();need(sha(after)==expected,'fixture postimage '+path)
            records={}
            for kind,body in (('before',before[path]),('proposed',after)):
                script=root/'fake.py';script.write_text(fake(body))
                command=python+[str(script),f'--trame={xyz},{ids},petite','--passes=2','--k=5','--threads=3','--digest']
                proc=subprocess.run(command,check=True,capture_output=True,text=True,timeout=10)
                need(not proc.stderr,'fake stderr')
                legacy=old.parse_output(0,proc.stdout,want)
                strengthened=new.parse_output(0,proc.stdout,want)
                need(legacy['etat']=='ok','legacy reader must accept each fixture')
                wanted='illisible' if kind=='before' else 'ok'
                need(strengthened['etat']==wanted,'strengthened reader '+kind)
                if kind=='before':
                    need('dependances des fins par ordre' in strengthened['raison'],'causal guard not reached')
                records[kind]={'legacy':legacy['etat'],'strengthened':strengthened['etat'],
                               'reason':strengthened['raison']}
                seq=subprocess.run(command+['--sequentiel'],check=True,capture_output=True,text=True,timeout=10)
                need(not seq.stderr,'sequential fake stderr')
                for rd in (old,new):
                    need(rd.parse_output(0,seq.stdout,dict(want,schema='sequentiel'))['etat']=='ok','sequential compatibility')
                records[kind]['sequential_sha256']=sha(seq.stdout.encode())
            need(records['before']['sequential_sha256']==records['proposed']['sequential_sha256'],
                 'fixture altered sequential output')
            result['fixtures'][path]=records
        for path in c['fixture_after']:
            proc=subprocess.run(python+[str(root/path)],cwd=root,check=False,capture_output=True,text=True,timeout=120)
            need(proc.returncode==0 and not proc.stderr,'official gate failed '+path+': '+proc.stderr[:1200])
            result['official_python_gates'][path]={'code':proc.returncode,'stdout':proc.stdout.strip(),'stderr':''}
        # Restrict the proposal to exactly the reader guards and four fixture lines.
        changed=[]
        for path in c['sources']:
            if (root/path).read_bytes()!=before[path]:
                changed.append(path)
        need(set(changed)==set(c['fixture_after'])|{LF},'unintended source changes')
    result['scope']='simulated Python probes, synthetic zeros only; no engine, CUDA, real data or campaign'
    print(json.dumps(result,indent=2,sort_keys=True))

if __name__=='__main__':
    main()
