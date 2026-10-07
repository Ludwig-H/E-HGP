#!/usr/bin/env python3
"""Correction CST0224 : fichiers homonymes et conflit fichier/repertoire, sans donnee reelle."""
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
PIN='c3de9d73d8999f2f1e31a0f592b829efc6e7a4da'
REL='morsehgp3D_v12/microbancs/outils/recu_session.py'
ACCOUNT='audit@example.invalid'
def need(ok,why):
    if not ok:raise RuntimeError(why)
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def tree(p):return {f.relative_to(p).as_posix():sha(f) for f in p.rglob('*') if f.is_file()} if p.exists() else None
def run(module,session,dest):
    out,err=io.StringIO(),io.StringIO();code=exception=None
    with contextlib.redirect_stdout(out),contextlib.redirect_stderr(err):
        try:code=module.main(['recu_session','--session',str(session),'--dest',str(dest),'--include','*'])
        except OSError as e:exception=type(e).__name__
    return code,exception

def main():
    source=ROOT/REL; before=sha(source); own=sha(Path(__file__))
    need(source.read_bytes()==subprocess.check_output(['git','-C',str(ROOT),'show',PIN+':'+REL]),'source differs from pin')
    spec=importlib.util.spec_from_file_location('publication',source);module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
    rows={}
    filesets={
      'distinct':{'one.json':b'1','two.json':b'2'},
      'redacted_name':{ACCOUNT+'.json':b'1'},
      'flat_collision':{'run-'+ACCOUNT+'.json':b'1','run-<compte>.json':b'2'},
      'nested_collision':{ACCOUNT+'/one.json':b'1','<compte>/one.json':b'2'},
      'file_directory_collision':{ACCOUNT:b'1','<compte>/child.json':b'2'},
    }
    with tempfile.TemporaryDirectory(prefix='ehgp-u32-publication-') as tmp:
        base=Path(tmp)
        for name,files in filesets.items():
            session=base/name;results=session/'results/extracted/results';results.mkdir(parents=True)
            (session/'receipt.json').write_text(json.dumps({'recovery_command':ACCOUNT}))
            (session/'preflight.json').write_text(json.dumps({'gcloud_account':ACCOUNT}))
            for filename,data in files.items():
                f=results/filename;f.parent.mkdir(parents=True,exist_ok=True);f.write_bytes(data)
            original=tree(session)
            for existing in [False,True]:
                dest=base/(name+('_empty' if existing else '_absent'))
                if existing:dest.mkdir()
                code,error=run(module,session,dest)
                remaining=tree(dest)
                if name in ['flat_collision','nested_collision']:
                    need(code==3 and error is None and remaining==({} if existing else None),'collision rollback')
                elif name=='file_directory_collision':
                    need(code is None and error=='FileExistsError','prefix conflict changed')
                    need(sorted(remaining)==['receipt.json','resultats/<compte>'],'partial tree changed')
                else:
                    need(code==0 and error is None,'valid publication rejected')
                    manifest={line.split('  ',1)[1]:line.split('  ',1)[0] for line in (dest/'SHA256SUMS').read_text().splitlines()}
                    need({k:v for k,v in remaining.items() if k!='SHA256SUMS'}==manifest,'manifest mismatch')
                    need(all(ACCOUNT not in f and ACCOUNT.encode() not in (dest/f).read_bytes() for f in remaining),'identity left')
                need(tree(session)==original,'input altered')
                rows[name+('_existing_empty' if existing else '_absent')]={'return_code':code,'exception':error,
                    'destination_exists':dest.exists(),'remaining_files':sorted(remaining) if remaining is not None else None,
                    'input_unchanged':True,'has_manifest':(dest/'SHA256SUMS').exists()}
            occupied=base/(name+'_occupied');occupied.mkdir();(occupied/'sentinel').write_bytes(b'preserved')
            saved=tree(occupied);code,error=run(module,session,occupied)
            need(code==2 and error is None and tree(occupied)==saved,'existing destination regression')
    need(sha(source)==before and sha(Path(__file__))==own,'changed source/witness')
    print(json.dumps({'schema':'audit.u32.publication.v1','pin':PIN,'source_sha256':{REL:before},
        'witness_sha256':own,'cases':rows,'preexisting_nonempty_refusals':len(filesets),'before_after_unchanged':True,
        'gcp_used':False,'scope':'temporary synthetic trees only; no historical partial receipt alleged'},sort_keys=True,indent=2))
if __name__=='__main__':main()
