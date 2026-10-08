#!/usr/bin/env python3
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
def need(ok,msg):
    if not ok:raise ValueError(msg)
def sha(b):return hashlib.sha256(b).hexdigest()
def functions(b):return {n.name:n for n in ast.parse(b).body if isinstance(n,ast.FunctionDef)}
def dump(n):return ast.dump(n,include_attributes=False)
def apply(root,patch):
    for check in (True,False):
        subprocess.run(['git','apply']+(['--check'] if check else [])+[str(patch)],cwd=root,check=True,capture_output=True)

def main():
    repo=Path(sys.argv[1]);c=json.loads((HERE/'capture.json').read_text());siblings=HERE.parent
    pair=siblings/'apparie_livraison/composition.patch';closure=siblings/'apparie_fermeture/proposition.patch'
    add=HERE/'fermeture.patch'
    for p,k in [(pair,'base_composition_patch_sha256'),(closure,'closure_patch_sha256'),(add,'incremental_patch_sha256')]:
        need(sha(p.read_bytes())==c[k],'patch pin')
    fc=json.loads((siblings/'pilotes_fixtures_recouvert/capture.json').read_text())
    need(fc['source_commit']==c['source_commit'],'fixture source pin')
    with tempfile.TemporaryDirectory(prefix='audit-triple-') as tmp:
        root=Path(tmp)
        for path,h in fc['sources'].items():
            b=subprocess.check_output(['git','show',c['source_commit']+':'+path],cwd=repo)
            need(sha(b)==h,'source pin');p=root/path;p.parent.mkdir(parents=True,exist_ok=True);p.write_bytes(b)
        original=(root/REL).read_bytes();apply(root,closure);standalone=functions((root/REL).read_bytes())
        (root/REL).write_bytes(original);apply(root,pair);base=(root/REL).read_bytes()
        need(sha(base)==c['base_composition_sha256'],'pair postimage');apply(root,add)
        triple=(root/REL).read_bytes();need(sha(triple)==c['triple_sha256'],'triple postimage')
        before,after=functions(base),functions(triple);need(set(before)==set(after),'function inventory')
        for name in before:
            if name not in ('judge','main'):need(dump(before[name])==dump(after[name]),'changed '+name)
        need(dump(after['main'])==dump(standalone['main']),'main differs from closure proposal')
        judge=copy.deepcopy(after['judge'])
        need([dump(n) for n in judge.body[3:5]]==[dump(n) for n in standalone['judge'].body[1:3]],'closure guards')
        del judge.body[3:5];need(dump(judge)==dump(before['judge']),'cohort judge changed')
        globals_of=lambda b:[dump(n) for n in ast.parse(b).body if not isinstance(n,ast.FunctionDef)]
        need(globals_of(base)==globals_of(triple),'global changes')
        for patch,key in [(siblings/'lf_recouvert_gardes/proposition.patch','lf_patch_sha256'),
                          (siblings/'pilotes_fixtures_recouvert/proposition.patch','fixture_patch_sha256')]:
            need(sha(patch.read_bytes())==fc[key],'dependency patch pin');apply(root,patch)
        path=root/'morsehgp3D_v12/microbancs/mes_apparie/test_pilote_apparie.py'
        spec=importlib.util.spec_from_file_location('triple_fixture',path);test=importlib.util.module_from_spec(spec)
        spec.loader.exec_module(test);result={}
        for mutate in (False,True):
            with tempfile.TemporaryDirectory(prefix='triple-fake-') as work:
                test.prepare(work);run=test.pa.bf.run;calls=[]
                def hook(argv,delay,raw_out=None,raw_err=None):
                    out=run(argv,delay,raw_out,raw_err)
                    if Path(argv[0]).name=='sonde_ok.py':
                        calls.append(1)
                        if mutate and len(calls)==1:
                            with open(argv[0],'ab') as f:f.write(b'\n# audit changed bytes\n')
                    return out
                test.pa.bf.run=hook
                try:code,report,folder=test.campaign(work,'ok')
                finally:test.pa.bf.run=run
                wanted='refuse' if mutate else 'juge'
                need(code==0 and report['jugement']['verdict_calcule']==wanted and len(calls)==64,'campaign outcome')
                need(test.pa.judge(report,folder)['verdict']==wanted,'replay outcome')
                result['changed_probe' if mutate else 'positive']=wanted
                if not mutate:
                    for name,change in [('empty_cohort',lambda r:r['parametres'].update(trames=[])),
                                        ('missing_closure',lambda r:r['provenance'].pop('sonde_fin_sha256'))]:
                        r=copy.deepcopy(report);change(r);v=test.pa.judge(r,folder)['verdict']
                        need(v=='refuse',name);result[name]=v
                    paths=list((Path(folder)/'journaux/identite').glob('*.jsonl'));need(len(paths)==12,'identity count')
                    for p in paths:p.unlink()
                    v=test.pa.judge(report,folder)['verdict'];need(v=='refuse','identity absent');result['absent_identity']=v
    print(json.dumps({'triple_sha256':c['triple_sha256'],'ast_composition_exact':True,'results':result,
                      'scope':'two fake Python campaigns, LF guards and repaired fixtures; no native engine'},sort_keys=True,indent=2))
if __name__=='__main__':main()
