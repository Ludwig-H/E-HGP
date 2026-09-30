#!/usr/bin/env python3
"""Closed bounded file-alias counterproof, copied CLI sources; tmp effects only."""
import base64
import csv
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parent
DECIDE = ROOT/'source/synthetic/decide.py'
MERGE = ROOT/'source/g4/merge_sessions.py'
FIELDS = ('unit','family','level','noise','n','seed','method','ari_s','ami_nc','refused')


def need(ok, message):
    if not ok:
        raise ValueError(message)


def sha(b):
    return hashlib.sha256(b).hexdigest()


def json_write(p,value):
    p.write_text(json.dumps(value,sort_keys=True,indent=1)+'\n')


def snapshot(root):
    return {str(p.relative_to(root)):p.read_bytes() for p in sorted(root.glob('s*/**/*')) if p.is_file()}


def encoded(snap):
    return {k:{'sha256':sha(v),'base64':base64.b64encode(v).decode()} for k,v in snap.items()}


def invoke(script,args,sandbox):
    cmd = [sys.executable,'-B','-S']+(['-O'] if sys.flags.optimize else [])+[str(script)]+list(map(str,args))
    p = subprocess.run(cmd,stdout=subprocess.PIPE,stderr=subprocess.PIPE,check=False,timeout=10)
    replace = lambda s:s.replace(str(sandbox),'<private>').replace(str(ROOT),'<archive>')
    return {'argv':[replace(x) for x in cmd], 'code':p.returncode,
            'stdout':replace(p.stdout.decode()),'stderr':replace(p.stderr.decode())}


def run():
    sp = importlib.util.spec_from_file_location('decide_alias_copy',DECIDE)
    d = importlib.util.module_from_spec(sp)
    sp.loader.exec_module(d)
    sources = {str(p.relative_to(ROOT)):sha(p.read_bytes()) for p in (DECIDE,MERGE,ROOT/'source/synthetic/scenes.py')}
    cases = []
    for mode in ('distinct','directory_alias','symlink_csv','hardlink_csv','symlink_json','hardlink_json','cross_format'):
        with tempfile.TemporaryDirectory(prefix='mhgp10-alias-proof-') as tmp:
            p = Path(tmp)
            prereg = {'id':'bounded-alias-control','attribution_statement':'Synthetic IO audit, no clustering claim.',
                'plan':{'split':'dev','families':['spherical'],'levels':['easy'],'sizes':[128],
                        'noises':[0.0],'replicates':2,'manifest_sha256':'pending'},
                'methods':[{'name':'tour'},{'name':'hdb'}],
                'decision':{'alpha':0.05,'delta_min':0.05,'permutations':1,'bootstrap':1,
                            'refusal_cap':0,'pairs':[{'name':'K2','method':'tour','adversary':'hdb'}]}}
            need(d.check_prereg(prereg)==[], 'valid prereg schema')
            specs,digest = d.plan_specs(prereg)
            prereg['plan']['manifest_sha256'] = digest
            pp = p/'prereg.json'
            json_write(pp,prereg)
            psha = sha(pp.read_bytes())
            for i,spec in enumerate(specs):
                sd = p/('s%d'%i)
                sd.mkdir()
                info = {'prereg_sha256':psha,'plan_sha256':digest,'scenes':2,'computed':1,'complete':False,
                        'session_identity':'original-session-%d'%i}
                json_write(sd/'run.json',info)
                with (sd/'results.csv').open('w',newline='') as f:
                    w = csv.DictWriter(f,fieldnames=FIELDS)
                    w.writeheader()
                    for method in ('tour','hdb'):
                        w.writerow({'unit':d.unit_name(spec),'family':spec['family'],'level':spec['level'],
                                    'noise':str(spec['noise_fraction']),'n':str(spec['n']),
                                    'seed':str(spec['seed']),'method':method,'ari_s':'0.5',
                                    'ami_nc':'0.5','refused':'0'})
            out = p/'output'
            out.mkdir()
            (out/'results.csv').write_text('UNRELATED-OUTPUT-CSV-SENTINEL\n')
            (out/'run.json').write_text('UNRELATED-OUTPUT-JSON-SENTINEL\n')
            source = p/'s0'
            alias = target = None
            if mode == 'directory_alias':
                out = source
            elif mode in ('symlink_csv','hardlink_csv'):
                alias,target = out/'results.csv',source/'results.csv'
            elif mode in ('symlink_json','hardlink_json'):
                alias,target = out/'run.json',source/'run.json'
            elif mode == 'cross_format':
                alias,target = out/'results.csv',source/'run.json'
            if alias is not None:
                alias.unlink()  # exact private sentinel only, within TemporaryDirectory
                if mode.startswith('hardlink'):
                    os.link(target,alias)
                else:
                    alias.symlink_to(target)
            before = snapshot(p)
            prereg_before = pp.read_bytes()
            merged = invoke(MERGE,['--prereg',pp,'--out',out,p/'s0',p/'s1'],p)
            after = snapshot(p)
            changed = sorted(k for k in before if after[k]!=before[k])
            need(pp.read_bytes()==prereg_before,'prereg sentinel changed')
            if mode == 'directory_alias':
                need(merged['code']==2 and not changed,'existing directory guard')
                verdict = None
            else:
                need(merged['code']==0 and not merged['stderr'],'merge unexpectedly refused')
                verdict = invoke(DECIDE,['--prereg',pp,'--run',out,'--check-only'],p)
                need(verdict['code']==0 and not verdict['stderr'],'merged output is invalid')
                if mode == 'distinct':
                    need(not changed,'distinct sentinel control')
                else:
                    need(changed==['s0/'+('results.csv' if mode.endswith('csv') else 'run.json')],
                         'wrong causal mutation '+mode)
            cases.append({'mode':mode,'same_directory':os.path.realpath(out)==os.path.realpath(source),
                          'same_file':False if alias is None else os.path.samefile(alias,target),
                          'changed_source_files':changed,'merge':merged,'merged_check_only':verdict,
                          'sources_before':encoded(before),'sources_after':encoded(after)})
    need(sources=={k:sha((ROOT/k).read_bytes()) for k in sources},'copied source drift')
    return {'scope':'seven tiny IO-only cases, no engine/native/GCP, no TOCTOU claim',
            'sources':sources,'cases':cases}


if __name__ == '__main__':
    # Exclude optimization flag only from commands so semantic normal/-O receipts agree.
    r = run()
    for c in r['cases']:
        for k in ('merge','merged_check_only'):
            if c[k] is not None:
                c[k]['argv'] = [x for x in c[k]['argv'] if x != '-O']
    print(json.dumps(r,sort_keys=True,separators=(',',':')))
