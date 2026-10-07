#!/usr/bin/env python3
"""Rejeux ciblés des corrections, sans données ni cloud ni matrice native."""
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[3]
V12=ROOT/'morsehgp3D_v12'
PIN='274592a30f6961cb7702125dcd2f031ff22b7df2'
def need(ok,msg):
    if not ok:raise RuntimeError(msg)
def sha(data):return hashlib.sha256(data).hexdigest()
def closure():
    raw=subprocess.check_output(['git','-C',str(ROOT),'ls-tree','-r','-z',PIN,'--','morsehgp3D_v12'])
    rows=[]
    for record in raw.split(b'\0'):
        if not record:continue
        meta,path=record.split(b'\t',1);mode,kind,oid=meta.split();rel=path.decode()
        need(kind==b'blob','unexpected tree entry')
        data=(ROOT/rel).read_bytes()
        object_hash=hashlib.sha1(b'blob '+str(len(data)).encode()+b'\0'+data).hexdigest()
        need(object_hash==oid.decode(),'source differs from pin: '+rel)
        rows.append(rel+' '+sha(data))
    return {'tracked_files':len(rows),'content_manifest_sha256':sha(('\n'.join(rows)+'\n').encode()),
            'git_tree':subprocess.check_output(['git','-C',str(ROOT),'rev-parse',PIN+':morsehgp3D_v12'],text=True).strip()}
def main():
    before=closure(); own={p.name:sha(p.read_bytes()) for p in HERE.iterdir() if p.suffix in ['.py','.cpp']}
    flags=['-B','-S']+(['-O'] if sys.flags.optimize else [])
    env=dict(os.environ,PYTHONDONTWRITEBYTECODE='1');rows={}
    def run(name,args,expected=None):
        proc=subprocess.run([sys.executable]+flags+args,cwd=ROOT,env=env,text=True,capture_output=True,timeout=180)
        need(proc.returncode==0 and not proc.stderr,'failed '+name+': '+proc.stdout+proc.stderr)
        if expected is not None:need(proc.stdout.strip()==expected,'verdict '+name)
        rows[name]={'exit':proc.returncode,'stdout':proc.stdout.strip()} if expected is not None else json.loads(proc.stdout)
    run('format',[str(HERE/'format.py')])
    run('publication',[str(HERE/'publication.py')])
    with tempfile.TemporaryDirectory(prefix='ehgp-t2-support-') as tmp:
        run('gate_properties',[str(V12/'tests/support/test_gate_properties.py'),shutil.which('cmake'),shutil.which('ctest'),str(V12),tmp], 'gate_properties_ok controles=69')
    run('g4_matrix',[str(V12/'tests/support/test_g4_matrix.py'),str(V12/'tools/g4_matrix.py')], 'g4_matrix_ok controles=39')
    run('check_style_gate',[str(V12/'tests/support/test_check_style.py'),str(V12/'tools/check_style.py')], 'check_style_ok controles=145')
    proc=subprocess.run([sys.executable]+flags+[str(V12/'tools/check_style.py')],cwd=ROOT,env=env,text=True,capture_output=True,timeout=180)
    need(proc.returncode==0 and not proc.stderr,'style source tree failed')
    rows['style']={'exit':0,'stdout':proc.stdout.strip()}
    prov=(V12/'docs/PROVENANCE.md').read_text();ports=(V12/'docs/PORTS.md').read_text()
    need('**profil 32 admis**' in prov and 'a0091e2b7' in ports and 'sans mise à jour de leur ligne' in ports,'provenance not historical')
    need('THREADS = (1,)' in (V12/'reference/test_dump_v10.py').read_text(),'frozen not single threaded')
    need(closure()==before and own=={p.name:sha(p.read_bytes()) for p in HERE.iterdir() if p.suffix in ['.py','.cpp']},'changed during audit')
    print(json.dumps({'schema':'audit.t2.corrections.v1','pin':PIN,'sources_before_after':before,'witness_sha256':own,'runs':rows,
        'provenance_historical_port_explicit':True,'frozen_v10_single_thread':True,'gcp_used':False,'gpu_used':False,
        'heavy_native_tests_run':False,'before_after_unchanged':True},sort_keys=True,indent=2))
if __name__=='__main__':main()
