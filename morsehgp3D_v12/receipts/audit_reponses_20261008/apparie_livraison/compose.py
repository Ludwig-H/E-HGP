#!/usr/bin/env python3
"""Compose the two proposed paired-judge repairs; only Python fixtures are executed."""
import argparse
import ast
import copy
import hashlib
import importlib.util
import json
from pathlib import Path
import subprocess
import sys
import tempfile

HERE=Path(__file__).resolve().parent
REL='morsehgp3D_v12/microbancs/mes_apparie/pilote_apparie.py'

def need(ok,message):
    if not ok:
        raise ValueError(message)

def sha(data):
    return hashlib.sha256(data).hexdigest()

def functions(data):
    return {n.name:ast.dump(n,include_attributes=False) for n in ast.parse(data).body if isinstance(n,ast.FunctionDef)}

def apply(root,patch):
    for check in (True,False):
        subprocess.run(['git','apply']+(['--check'] if check else [])+[str(patch)],cwd=root,check=True,capture_output=True)

def main():
    parser=argparse.ArgumentParser();parser.add_argument('--repo',type=Path,required=True)
    parser.add_argument('--cohort-receipt',type=Path,default=HERE.parent/'apparie_cohorte')
    args=parser.parse_args();c=json.loads((HERE/'composition.json').read_text())
    patches=[HERE.parent/'apparie_identite/relecture.patch',args.cohort_receipt/'proposition.patch']
    for path,key in zip(patches,('identity_patch_sha256','cohort_patch_sha256')):
        need(sha(path.read_bytes())==c[key],'input patch pin')
    combo=HERE/'composition.patch'
    need(sha(combo.read_bytes())==c['composition_patch_sha256'],'composed patch pin')
    original=subprocess.check_output(['git','show',c['source_commit']+':'+REL],cwd=args.repo)
    originals=functions(original);versions=[];context_conflicts=[]
    for first,second in (patches,list(reversed(patches))):
        with tempfile.TemporaryDirectory() as tmp:
            root=Path(tmp);p=root/REL;p.parent.mkdir(parents=True);p.write_bytes(original)
            apply(root,first);versions.append(p.read_bytes())
            proc=subprocess.run(['git','apply','--check',str(second)],cwd=root,capture_output=True)
            need(proc.returncode!=0,'expected only shared-context conflict')
            context_conflicts.append({'first':first.parent.name,'second':second.parent.name,'apply_check':proc.returncode})
    with tempfile.TemporaryDirectory(prefix='audit-apparie-compose-') as tmp:
        root=Path(tmp)
        fixtures=HERE.parent/'pilotes_fixtures_recouvert'
        fc=json.loads((fixtures/'capture.json').read_text())
        need(fc['source_commit']==c['source_commit'],'fixture base')
        for path,h in fc['sources'].items():
            data=subprocess.check_output(['git','show',c['source_commit']+':'+path],cwd=args.repo)
            need(sha(data)==h,'source pin')
            p=root/path;p.parent.mkdir(parents=True,exist_ok=True);p.write_bytes(data)
        apply(root,combo)
        combined=(root/REL).read_bytes();need(sha(combined)==c['composition_source_sha256'],'composed postimage')
        f=functions(combined);identity=functions(versions[0]);cohort=functions(versions[1])
        need(set(f)==set(originals)|{'validate_campaign'},'function inventory')
        for name in f:
            expected=identity[name] if name=='reread' else cohort[name] if name in ('judge','validate_campaign') else originals[name]
            need(f[name]==expected,'composition changed function '+name)
        globals_of=lambda data:[ast.dump(n,include_attributes=False) for n in ast.parse(data).body if not isinstance(n,ast.FunctionDef)]
        need(globals_of(combined)==globals_of(original),'module-level changes')
        lfpatch=HERE.parent/'lf_recouvert_gardes/proposition.patch'
        need(sha(lfpatch.read_bytes())==fc['lf_patch_sha256'],'LF patch pin')
        need(sha((fixtures/'proposition.patch').read_bytes())==fc['fixture_patch_sha256'],'fixture patch pin')
        apply(root,lfpatch);apply(root,fixtures/'proposition.patch')
        test=root/'morsehgp3D_v12/microbancs/mes_apparie/test_pilote_apparie.py'
        py=[sys.executable,'-B']+(['-O'] if sys.flags.optimize else [])
        proc=subprocess.run(py+[str(test)],cwd=root,capture_output=True,text=True,timeout=120)
        need(proc.returncode==0 and not proc.stderr,'composed official gate: '+proc.stderr[:1000])
        spec=importlib.util.spec_from_file_location('composed_official_fixture',test)
        fixture=importlib.util.module_from_spec(spec);spec.loader.exec_module(fixture)
        with tempfile.TemporaryDirectory(prefix='audit-paired-fake-') as work:
            fixture.prepare(work);code,report,folder=fixture.campaign(work,'ok')
            need(code==0 and report is not None,'positive fake campaign')
            judge=fixture.pa.judge
            positive=judge(report,folder)
            need(positive['verdict']=='juge' and positive['cas']['cache']['verdict']=='adopte','positive composition')
            empty=copy.deepcopy(report);empty['parametres']['trames']=[]
            empty_result=judge(empty,folder)
            need(empty_result['verdict']=='refuse' and empty_result['refus'],'empty cohort admitted')
            paths=list((Path(folder)/'journaux/identite').glob('*.jsonl'))
            need(len(paths)==12,'identity file cohort')
            for path in paths:path.unlink()
            absent=judge(report,folder)
            need(absent['verdict']=='refuse' and absent['refus'],'missing identity admitted')
    print(json.dumps({'source_commit':c['source_commit'],'context_conflicts':context_conflicts,
                      'composition_sha256':c['composition_source_sha256'],
                      'ast_unchanged_except_exact_proposed_functions':True,
                      'official_gate':{'code':proc.returncode,'stdout':proc.stdout.strip(),'stderr':''},
                      'positive':'juge/cache adopte/seq rejete/A-A controle',
                      'empty_cohort':empty_result,'deleted_identity_files':12,
                      'missing_identity':{'verdict':absent['verdict'],'refusal_count':len(absent['refus'])},
                      'scope':'composition proposed only; LF guards and repaired synthetic fixtures; no engine'},
                     sort_keys=True,indent=2))

if __name__=='__main__':
    main()
