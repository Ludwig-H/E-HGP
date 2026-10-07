#!/usr/bin/env python3
"""Small true-CLI counterexample: MES-P's family analyser silently pools 1/4/48 threads."""
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[4]
PIN = 'f601b36ace16bcc8f7ac9bc532e45ab5079f9667'
REL = 'morsehgp3D_v12/microbancs/mes_p_petits/analyse_p.py'


def need(ok, why):
    if not ok:
        raise RuntimeError(why)


def main():
    analyser = ROOT/REL
    before = analyser.read_bytes()
    need(before == subprocess.check_output(['git','show',PIN+':'+REL],cwd=ROOT),'analyser source pin')
    takes = [dict(nuage='bout_n%d'%n,k=5,fils=t,sites=n,code=0,chaud=rate*n)
             for t,rate in [(1,100e-6),(4,60e-6),(48,10e-6)] for n in (100,200)]
    outputs = {}
    with tempfile.TemporaryDirectory(prefix='ehgp-mes-p-audit-') as tmp:
        data = Path(tmp)/'mes_p.json'
        for name,rows in [('f1',[r for r in takes if r['fils']==1]),
                          ('f4',[r for r in takes if r['fils']==4]),
                          ('f48',[r for r in takes if r['fils']==48]),('mixed',takes)]:
            data.write_text(json.dumps({'prises':rows}))
            args = [sys.executable,'-B','-S',*(['-O'] if sys.flags.optimize else []),str(analyser),str(data)]
            proc = subprocess.run(args,capture_output=True,text=True,timeout=5)
            need(proc.returncode == 0 and not proc.stderr,'analyser refused '+name)
            outputs[name] = dict(code=proc.returncode,stdout=proc.stdout)
    for name,rate in [('f1','100.00'),('f4','60.00'),('f48','10.00')]:
        need(rate+' µs par site' in outputs[name]['stdout'],'positive single-thread regime '+name)
    mixed = outputs['mixed']['stdout']
    need('| 100 a 299 sites | 6 |' in mixed,'six takes were not pooled')
    need('56.67 µs par site' in mixed,'expected pooled regression')
    need('fils' not in mixed and 'thread' not in mixed,'unexpected disclosure of pooled thread counts')
    need(analyser.read_bytes()==before,'analyser changed')
    print(json.dumps(dict(schema='ehgp.v12.audit_reprise.mes_p.v1',pin=PIN,
                          analyser_sha256=hashlib.sha256(before).hexdigest(),
                          script_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                          takes=takes,outputs=outputs,all_calls_code_zero=True,
                          mixed_thread_counts_unreported=True,synthetic_timing_data=True,
                          native_engine_executed=False,gcp_used=False),sort_keys=True,indent=2))


if __name__=='__main__':
    main()
