#!/usr/bin/env python3
"""Complément ciblé au pin 76adb8fa9 : huit identités de trame et deux faits T2."""
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[3]
V12=ROOT/'morsehgp3D_v12'
PIN='76adb8fa9eb10894b6a363672e7f28cc3f12e75a'
OLD_AUDIT=V12/'receipts/audit_t2_20261007/transition'

def need(ok,msg):
    if not ok:raise RuntimeError(msg)
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def load(name,path):
    spec=importlib.util.spec_from_file_location(name,path);module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module);return module

def sources():
    paths=[V12/'reference/transition_catalogue.py',V12/'reference/test_resolution_v12.py',
           V12/'docs/CONTRAT_TOUR.md',V12/'docs/CONTRAT_NUMERIQUE.md',V12/'docs/OBJET_ET_CONTRAT_MATHEMATIQUE.md',
           ROOT/'docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md',OLD_AUDIT/'check.py',OLD_AUDIT/'normal.json']
    paths+=list((V12/'reference/hgp12_ref').glob('*.py'))
    out={}
    for path in paths:
        rel=path.relative_to(ROOT).as_posix();raw=path.read_bytes()
        need(raw==subprocess.check_output(['git','-C',str(ROOT),'show',PIN+':'+rel]),'source differs from pin: '+rel)
        out[rel]=sha(path)
    return out

def main():
    before=sources();own=sha(Path(__file__))
    witness=load('independent_old_witness',OLD_AUDIT/'check.py')
    points=[(1,0,0),(2,1,0),(1,2,0),(0,1,0),(1,1,0)];balls=witness.exhaustive(points,2)
    need(len(balls)==9,'historical geometric witness drift')
    flags=[sys.executable,'-B','-S']+(['-O'] if sys.flags.optimize else [])
    env=dict(os.environ,PYTHONDONTWRITEBYTECODE='1');cases=[]
    rows=[('ascii_valid',b'audit',b'audit',0),('ascii_printable_edges',b' ~',b' ~',0),
          ('ascii_full_header',b'A'*24,b'A'*24,0),('ascii_distinct',b'audita',b'auditb',2),
          ('historical_ff_fe',b'audit\xff',b'audit\xfe',2),('same_invalid_ff',b'audit\xff',b'audit\xff',2),
          ('ascii_control',b'audit\x1f',b'audit\x1f',2),('ascii_del',b'audit\x7f',b'audit\x7f',2)]
    old=json.loads((OLD_AUDIT/'normal.json').read_text())
    old_row=next(x for x in old['cli_cases'] if x['case']=='different_invalid_frames_accepted')
    with tempfile.TemporaryDirectory(prefix='ehgp-t2fix-') as tmp:
        root=Path(tmp)
        for name,left,right,expected in rows:
            paths=[root/(name+'_'+side+'.bin') for side in ['left','right']]
            for path,convention,frame in zip(paths,['v11','v12'],[left,right]):
                path.write_bytes(witness.dump(points,balls,2,convention,frame=frame))
            hashes=[sha(path) for path in paths]
            if name=='historical_ff_fe':
                need(hashes==old_row['inputs_sha256'] and old_row['code']==0,'historical inputs differ')
            p=subprocess.run(flags+[str(V12/'reference/transition_catalogue.py'),*map(str,paths)],
                             capture_output=True,text=True,env=env,timeout=20)
            need(p.returncode==expected,'reader result '+name)
            if expected==0:need('transition_catalogue_conforme' in p.stdout and p.stderr.startswith('transition_catalogue_temps secondes='),'positive diagnostic')
            else:need('transition_catalogue_refus' in p.stderr,'refusal diagnostic')
            cases.append({'case':name,'exit':p.returncode,'input_bytes':[path.stat().st_size for path in paths],
                          'input_sha256':hashes})
    sys.path.insert(0,str(V12/'reference'))
    facts=load('targeted_resolution_facts',V12/'reference/test_resolution_v12.py')
    import hgp12_ref
    results={}
    for name in ['fact_lemma_needs_two_sites','fact_saturated_in_catalogue']:
        gaps=getattr(facts,name)(hgp12_ref);need(gaps==[],'fact failed '+name);results[name]={'gaps':gaps,'passed':True}
    need(sources()==before and sha(Path(__file__))==own,'source changed')
    print(json.dumps({'schema':'audit.juges.t2fix.v1','pin':PIN,'parent_tranche_pin':'1f7642e105aebd76632c58c63fdfd5b5c0824779',
          'source_sha256':before,'witness_sha256':own,'header_cases':cases,'historical_payloads_byte_identical':True,
          'targeted_facts':results,'full_reference_suite_run':False,'before_after_unchanged':True,
          'native_qualified':False,'gcp_used':False,'real_data_used':False},sort_keys=True,indent=2))
if __name__=='__main__':main()
