#!/usr/bin/env python3
"""Raccord de sources et application de patch en copie ; aucune compilation."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import tempfile

HERE=Path(__file__).resolve().parent


def need(ok,why):
    if not ok:raise ValueError(why)


def main(repo,snapshot):
    c=json.loads((HERE/'capture.json').read_text())
    def blob(pin,path):return subprocess.check_output(['git','show',pin+':'+path],cwd=repo)
    def sha(b):return hashlib.sha256(b).hexdigest()
    delta=subprocess.check_output(['git','diff','--name-only',c['base'],c['pre'],'--','morsehgp3D_v12/src'],cwd=repo)
    need(not delta,'unexpected native delta before integration')
    for path,h in c['witness_sources'].items():need(sha(blob(c['pin'],'morsehgp3D_v12/'+path))==h,'witness source '+path)
    patch=(snapshot/'a6_bdfca8fb1.patch').read_bytes()
    need(sha(patch)==c['patch_sha256'],'delivery patch')
    need(sha((snapshot/'CONCEPTION_A6.md').read_bytes())==c['conception_sha256'],'design')
    names=subprocess.check_output(['git','diff','--name-only',c['pre'],c['pin']],cwd=repo,text=True).splitlines()
    need(names==[r['path'] for r in c['postimages']],'integration inventory')
    with tempfile.TemporaryDirectory() as td:
        t=Path(td)
        for name in names:
            b=subprocess.run(['git','show',c['base']+':'+name],cwd=repo,capture_output=True)
            if b.returncode==0:
                q=t/name;q.parent.mkdir(parents=True,exist_ok=True);q.write_bytes(b.stdout)
        subprocess.run(['git','apply','--check','-'],input=patch,cwd=t,capture_output=True,check=True)
        subprocess.run(['git','apply','-'],input=patch,cwd=t,capture_output=True,check=True)
        for row in c['postimages']:
            b=blob(c['pin'],row['path'])
            need(sha(b)==row['sha256'] and (t/row['path']).read_bytes()==b,'postimage '+row['path'])
        bridge=blob(c['pin'],c['bridge_path']);need(sha(bridge)==c['bridge_sha256'],'bridge hash')
        subprocess.run(['git','apply','--check','-'],input=bridge,cwd=t,capture_output=True,check=True)
        subprocess.run(['git','apply','-'],input=bridge,cwd=t,capture_output=True,check=True)
        post=sha((t/'morsehgp3D_v12/src/tower/forest_kernel.cpp').read_bytes())
    return {'native_delta_base_to_pre':0,'integration_postimages_equal':len(names),
            'prefix_witness_sources_equal':len(c['witness_sources']),'bridge_applies':True,
            'bridge_post_kernel_sha256':post,'native_invocations':0,'CST_0242_closed':False}


if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('repo',type=Path);p.add_argument('snapshot',type=Path)
    p.add_argument('--check',action='store_true');a=p.parse_args();r=main(a.repo,a.snapshot)
    if a.check:need(r==json.loads((HERE/'results.json').read_text()),'stored result')
    print(json.dumps(r,indent=2,sort_keys=True))
