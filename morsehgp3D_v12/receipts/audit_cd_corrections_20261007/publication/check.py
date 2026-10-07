#!/usr/bin/env python3
"""Témoins temporaires de publication : noms, refus et collision d'anonymisation."""
import contextlib
import hashlib
import importlib.util
import io
import json
from pathlib import Path
import subprocess
import tempfile

HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[3]
PIN='07ee13ef6bebc0b6b85da90207f3f067ffff1755'
REL='morsehgp3D_v12/microbancs/outils/recu_session.py'
ACCOUNT='audit@example.invalid'

def need(value,reason):
    if not value: raise RuntimeError(reason)

def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()

def tree(path):
    if not path.exists():return None
    return {p.relative_to(path).as_posix():sha(p) for p in path.rglob('*') if p.is_file()}

def call(module,session,dest):
    out,err=io.StringIO(),io.StringIO()
    with contextlib.redirect_stdout(out),contextlib.redirect_stderr(err):
        code=module.main(['recu_session','--session',str(session),'--dest',str(dest),'--include','*'])
    return code,out.getvalue(),err.getvalue()

def make_session(path,files):
    results=path/'results/extracted/results';results.mkdir(parents=True)
    (path/'receipt.json').write_text(json.dumps({'recovery_command':ACCOUNT,'status':'synthetic'}))
    (path/'preflight.json').write_text(json.dumps({'gcloud_account':ACCOUNT}))
    for name,data in files.items():(results/name).write_bytes(data)
    return path

def manifest_matches(dest):
    rows=(dest/'SHA256SUMS').read_text().splitlines()
    actual=tree(dest);actual.pop('SHA256SUMS')
    expected={}
    for row in rows:
        digest,name=row.split('  ',1);expected[name]=digest
    need(actual==expected,'manifest mismatch')
    return len(rows)

def main():
    source=ROOT/REL; before=sha(source); own=sha(Path(__file__))
    need(source.read_bytes()==subprocess.check_output(['git','-C',str(ROOT),'show',PIN+':'+REL]),'source differs from pin')
    spec=importlib.util.spec_from_file_location('audit_receipt',source);module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
    rows={}
    with tempfile.TemporaryDirectory(prefix='ehgp-audit-publish-fixes-') as tmp:
        folder=Path(tmp)
        nominal=make_session(folder/'nominal',{ACCOUNT+'.log':b'Anonymous payload.\n'})
        before_input=tree(nominal)
        dest=folder/'nominal_out';code,_,_=call(module,nominal,dest)
        target=dest/'resultats/<compte>.log'
        need(code==0 and target.read_bytes()==b'Anonymous payload.\n','name redaction failed')
        for p in dest.rglob('*'):
            need(ACCOUNT not in str(p.relative_to(dest)),'identity remains in path')
            if p.is_file():need(ACCOUNT.encode() not in p.read_bytes(),'identity remains in content')
        rows['CST0219_fixed']={'exit':code,'renamed_payload_present':True,'no_identity_in_paths_or_contents':True,'manifest_entries':manifest_matches(dest)}
        old=folder/'existing';old.mkdir();(old/'sentinel.bin').write_bytes(b'\0previous /home/exemple')
        (old/'receipt.json').write_bytes(b'previous receipt')
        saved=tree(old);code,_,_=call(module,nominal,old)
        need(code==2 and tree(old)==saved,'preexisting destination modified')
        rows['CST0221_fixed']={'exit':code,'preexisting_directory_and_all_bytes_preserved':True}
        file_dest=folder/'existing_file';file_dest.write_bytes(b'existing file');code,_,_=call(module,nominal,file_dest)
        need(code==2 and file_dest.read_bytes()==b'existing file','preexisting file modified')
        rows['existing_file']={'exit':code,'bytes_preserved':True}
        leak=make_session(folder/'leak',{'leak.bin':b'\0'+ACCOUNT.encode()})
        for existed in [False,True]:
            d=folder/('empty_existing' if existed else 'absent')
            if existed:d.mkdir()
            code,_,_=call(module,leak,d)
            need(code==3 and (tree(d)=={} if existed else not d.exists()),'leak rollback changed')
            rows['binary_leak_'+str(existed)]={'exit':code,'preexisting_empty_directory_preserved':existed,'no_created_file_left':True}
        control=make_session(folder/'two_different',{'first.json':b'{"value":1}\n','second.json':b'{"value":2}\n'})
        d=folder/'two_different_out';code,_,_=call(module,control,d)
        need(code==0 and len(list((d/'resultats').iterdir()))==2,'two-file positive control')
        rows['two_distinct_files']={'exit':code,'selected':2,'published':2,'manifest_entries':manifest_matches(d)}
        collision=make_session(folder/'collision',{'run-'+ACCOUNT+'.json':b'{"value":1}\n','run-<compte>.json':b'{"value":2}\n'})
        original=tree(collision);d=folder/'collision_out';code,out,_=call(module,collision,d)
        payloads=list((d/'resultats').iterdir())
        need(code==0 and len(payloads)==1 and payloads[0].read_bytes()==b'{"value":1}\n','collision behavior changed')
        need('fichiers=3' in out and tree(collision)==original,'collision reporting or input changed')
        rows['CST0224_collision']={'exit':code,'selected':2,'published':1,'reported_files_excluding_manifest':3,'actual_files_excluding_manifest':2,'surviving_payload_value':1,'manifest_entries':manifest_matches(d),'input_files_unchanged':True}
        need(tree(nominal)==before_input,'input session changed')
    need(sha(source)==before and sha(Path(__file__))==own,'source/witness changed')
    print(json.dumps({'pin':PIN,'schema':'ehgp.v12.audit_publication_fixes.v1','source_sha256':{REL:before},'witness_sha256':own,'cases':rows,'before_after_unchanged':True,'gcp_used':False,'scope':'temporary synthetic directories; no historical publication loss demonstrated'},ensure_ascii=False,sort_keys=True,indent=2))

if __name__=='__main__':main()
