#!/usr/bin/env python3
"""Closed hash-first replay of seven private filesystem/CSV/JSON cases."""
import base64
import hashlib
import json
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parent
FILES = {'README.md','PROTOCOL.txt','check.py','verify.py','receipt.json',
         'normal.stdout','normal.stderr','optimized.stdout','optimized.stderr',
         'source/g4/merge_sessions.py','source/synthetic/decide.py','source/synthetic/scenes.py'}
MODES = ['distinct','directory_alias','symlink_csv','hardlink_csv','symlink_json','hardlink_json','cross_format']


def need(c,m):
    if not c:
        raise ValueError(m)


def digest(b):
    return hashlib.sha256(b).hexdigest()


def run():
    inv = {str(p.relative_to(ROOT)) for p in ROOT.rglob('*') if p.is_file()}
    need(inv==FILES|{'SHA256SUMS'},'file inventory')
    need(not any(p.is_symlink() for p in ROOT.rglob('*')),'archive symlink')
    pins = {}
    for line in (ROOT/'SHA256SUMS').read_text().splitlines():
        pin,name = line.split('  ',1)
        need(len(pin)==64 and all(c in '0123456789abcdef' for c in pin),'pin syntax')
        need(name in FILES and name not in pins,'manifest name')
        pins[name]=pin
    need(set(pins)==FILES,'manifest closure')
    for name,pin in pins.items():
        need(digest((ROOT/name).read_bytes())==pin,'hash '+name)
    receipt=json.loads((ROOT/'receipt.json').read_text())
    need(receipt['source_before']==receipt['source_after'],'shared source drift')
    for name,pin in receipt['source_before'].items():
        need(pins['source/'+name]==pin,'copied source pin')
    need(len(receipt['calls'])==2,'two captures')
    previous=None
    for i,call in enumerate(receipt['calls']):
        mode='optimized' if i else 'normal'
        opts=['-O'] if i else []
        need(call['command']==['python3','-B']+opts+[str(Path(call['cwd'])/'check.py')],'capture command')
        need(call['code']==0 and call['stderr']=='','terminal capture')
        stdout=(ROOT/(mode+'.stdout')).read_bytes()
        need(stdout==call['stdout'].encode() and (ROOT/(mode+'.stderr')).read_bytes()==b'','capture bytes')
        result=json.loads(stdout)
        need([c['mode'] for c in result['cases']]==MODES,'seven cases')
        for c in result['cases']:
            case=c['mode']
            need(c['merge']['stderr']=='','CLI stderr')
            expected=[] if case in ('distinct','directory_alias') else [
                's0/results.csv' if case.endswith('csv') else 's0/run.json']
            need(c['changed_source_files']==expected,'changed files')
            changed=[]
            for name,before in c['sources_before'].items():
                after=c['sources_after'][name]
                b=base64.b64decode(before['base64'],validate=True)
                a=base64.b64decode(after['base64'],validate=True)
                need(digest(b)==before['sha256'] and digest(a)==after['sha256'],'payload hash')
                if b!=a:
                    changed.append(name)
            need(sorted(changed)==expected,'byte causal difference')
            if case=='directory_alias':
                need(c['merge']['code']==2 and c['merged_check_only'] is None and c['same_directory'] is True,
                     'directory positive guard')
            else:
                need(c['merge']['code']==0 and c['merged_check_only']['code']==0 and c['same_directory'] is False,
                     'accepted distinct output')
                if case!='distinct':
                    need(c['same_file'] is True,'stable file alias')
            if case=='cross_format':
                a=base64.b64decode(c['sources_after']['s0/run.json']['base64'])
                need(a.startswith(b'unit,family,level,'),'JSON source became CSV')
        p=subprocess.run([sys.executable,'-B']+opts+[str(ROOT/'check.py')],cwd=ROOT,
                         stdout=subprocess.PIPE,stderr=subprocess.PIPE,check=False,timeout=30)
        need(p.returncode==0 and p.stderr==b'' and p.stdout==stdout,'bounded replay')
        need(previous is None or previous==stdout,'normal/-O equality')
        previous=stdout
    return {'code':0,'cases':7,'replays':2,'scope':'stable private file aliases, no engine/GCP'}


if __name__=='__main__':
    print(json.dumps(run(),sort_keys=True))
