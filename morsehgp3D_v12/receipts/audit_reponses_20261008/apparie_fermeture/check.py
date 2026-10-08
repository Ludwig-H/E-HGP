#!/usr/bin/env python3
"""A generated Python probe changes between calls; no native binary or real campaign is executed."""
import copy
import hashlib
import importlib.util
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import types

HERE=Path(__file__).resolve().parent
REL='morsehgp3D_v12/microbancs/mes_apparie/pilote_apparie.py'

def need(ok,message):
    if not ok:
        raise ValueError(message)

def sha(data):
    return hashlib.sha256(data).hexdigest()

def main():
    repo=Path(sys.argv[1]);c=json.loads((HERE/'capture.json').read_text())
    fixture_capture=json.loads((HERE.parent/'pilotes_fixtures_recouvert/capture.json').read_text())
    need(fixture_capture['source_commit']==c['source_commit'],'fixture source pin')
    patch=HERE/'proposition.patch';need(sha(patch.read_bytes())==c['patch_sha256'],'patch pin')
    result={}
    with tempfile.TemporaryDirectory(prefix='audit-probe-closure-') as tmp:
        root=Path(tmp)
        for path,h in fixture_capture['sources'].items():
            data=subprocess.check_output(['git','show',c['source_commit']+':'+path],cwd=repo)
            need(sha(data)==h,'source pin '+path)
            p=root/path;p.parent.mkdir(parents=True,exist_ok=True);p.write_bytes(data)
        original=(root/REL).read_bytes();need(sha(original)==c['source_sha256'],'pilot pin')
        test=root/'morsehgp3D_v12/microbancs/mes_apparie/test_pilote_apparie.py'
        sp=importlib.util.spec_from_file_location('closure_official_test',test)
        fixture=importlib.util.module_from_spec(sp);sp.loader.exec_module(fixture)
        old=fixture.pa
        # Reuse the already proposed clock correction, without changing the product or archived fixture.
        from_clock='[[4, 5, 6, 0, 6]] + [[3, 4, 5, 5, 6]] * (k - 1)'
        to_clock='[[4, 5, 6, 0, 6]] + [[3, 4, 5, 6, 6]] * (k - 1)'
        need(fixture.FAKE.count(from_clock)==1,'official FAKE clock literal')
        fixture.FAKE=fixture.FAKE.replace(from_clock,to_clock)
        for check in (True,False):
            subprocess.run(['git','apply']+(['--check'] if check else [])+[str(patch)],cwd=root,
                           check=True,capture_output=True)
        candidate=(root/REL).read_bytes();need(sha(candidate)==c['candidate_sha256'],'candidate pin')
        proposed=types.ModuleType('proposed_closure');proposed.__file__=str(root/REL)
        exec(compile(candidate,str(root/REL),'exec'),proposed.__dict__)
        for name,module in (('published',old),('proposed',proposed)):
            # Match the file used by the provenance field to the body actually executed in this arm.
            (root/REL).write_bytes(original if name=='published' else candidate)
            fixture.pa=module;result[name]={}
            for mode in ('positive','changed_after_first_identity','deleted_after_last_information'):
                with tempfile.TemporaryDirectory(prefix='fake-probe-') as work:
                    fixture.prepare(work)
                    actual_run=module.bf.run;events=[];probe_calls=[0]
                    def run(argv,delay,raw_out=None,raw_err=None):
                        out=actual_run(argv,delay,raw_out,raw_err)
                        if Path(argv[0]).name=='sonde_ok.py':
                            probe_calls[0]+=1
                            name_out=Path(raw_out).name
                            if mode=='changed_after_first_identity' and probe_calls[0]==1:
                                with open(argv[0],'ab') as handle:handle.write(b'\n# audit: bytes changed between calls\n')
                                events.append('comment appended after first identity call')
                            if mode=='deleted_after_last_information' and name_out=='seq_r0.jsonl':
                                Path(argv[0]).unlink();events.append('probe deleted after last information call')
                        return out
                    module.bf.run=run
                    try:
                        code,report,folder=fixture.campaign(work,'ok')
                    finally:
                        module.bf.run=actual_run
                    need(code==0 and report is not None and probe_calls[0]==64,'fake execution failed')
                    verdict=report['jugement']['verdict_calcule']
                    wanted='juge' if name=='published' or mode=='positive' else 'refuse'
                    need(verdict==wanted,'closure outcome '+name+'/'+mode)
                    if mode!='positive':need(len(events)==1,'mutation did not occur')
                    p=Path(work)/'sonde_ok.py';final=sha(p.read_bytes()) if p.is_file() else None
                    opening=report['provenance']['sonde_sha256']
                    need((opening==final)==(mode=='positive'),'mutation not visible in byte hashes')
                    need(all(not x['refus'] for x in report['session_v12set'].values()),'unrelated information failure')
                    result[name][mode]={'calculated_verdict':verdict,'probe_calls':probe_calls[0],
                        'opening_equals_final_bytes':opening==final,
                        'reported_final_sha256':report['provenance'].get('sonde_fin_sha256'),
                        'final_sha256':final,'events':events,
                        'information_refusals':0}
                    if name=='proposed':
                        need(module.judge(report,folder)['verdict']==wanted,'replay ignores closure')
                        if mode=='positive':
                            forged=copy.deepcopy(report);del forged['provenance']['sonde_fin_sha256']
                            need(module.judge(forged,folder)['verdict']=='refuse','missing closure admitted')
                            result['missing_final_hash']='refuse'
    print(json.dumps({'source_commit':c['source_commit'],'candidate_sha256':c['candidate_sha256'],
                      'result':result,'scope':'fake Python probe only; endpoint byte identity, not continuous immutability'},
                     sort_keys=True,indent=2))

if __name__=='__main__':
    main()
