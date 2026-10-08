#!/usr/bin/env python3
"""Relecture FULLN, sans natif, cloud ou données d'entrée."""
from pathlib import Path
import argparse,hashlib,importlib.util,json,sys
sys.dont_write_bytecode=True
HERE=Path(__file__).resolve().parent

def need(ok,why):
    if not ok:raise ValueError(why)

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--repo',required=True,type=Path)
    p.add_argument('--session',required=True,type=Path)
    a=p.parse_args()
    for line in (HERE/'SHA256SUMS').read_text().splitlines():
        h,n=line.split('  ',1)
        need('/'not in n and hashlib.sha256((HERE/n).read_bytes()).hexdigest()==h,'receipt '+n)
    cap=json.loads((HERE/'capture.json').read_text())
    def inputs():
        for n,expected in cap['inputs'].items():
            b=(a.session/n).read_bytes()
            need(dict(bytes=len(b),sha256=hashlib.sha256(b).hexdigest())==expected,'pinned input '+n)
    inputs()
    spec=importlib.util.spec_from_file_location('fulln_admission_replay',HERE/'replay.py')
    module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
    result=module.run(a.repo,a.session)
    need(result==json.loads((HERE/'results.json').read_text()),'results differ')
    inputs()
    print(json.dumps(dict(source=result['source_git'],sources_exact=result['source_files'],
        full_processes=result['full']['processes'],full_passes=result['full']['passes'],
        full_warm=result['full']['warm_passes'],mes_c_processes=result['mes_c']['processes'],
        mes_c_passes=result['mes_c']['full_passes'],mes_c_warm=result['mes_c']['warm_passes'],
        replay='concordant_conditional',independent_native_codes=False,final_pilot_elf_closure=False,
        native_executed=False,payload_read=False)))

if __name__=='__main__':main()
