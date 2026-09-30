#!/usr/bin/env python3
"""Hash-first, closed private archive reader; pure Fraction subprocesses only."""
import hashlib
import json
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parent
FILES = {'README.md','PROTOCOL.txt','check.py','reference_functions.py',
         'receipt.json','normal.stdout','normal.stderr','optimized.stdout',
         'optimized.stderr','verify.py'}


def need(ok, message):
    if not ok:
        raise ValueError(message)


def run():
    need({p.name for p in ROOT.iterdir()} == FILES | {'SHA256SUMS'}, 'inventory')
    entries = {}
    for line in (ROOT/'SHA256SUMS').read_text().splitlines():
        pin,name = line.split('  ',1)
        need(len(pin)==64 and all(c in '0123456789abcdef' for c in pin), 'pin syntax')
        need(name in FILES and name not in entries, 'manifest name')
        entries[name] = pin
    need(set(entries) == FILES, 'full manifest closure')
    for name,pin in entries.items():
        need(hashlib.sha256((ROOT/name).read_bytes()).hexdigest() == pin, 'hash '+name)
    receipt = json.loads((ROOT/'receipt.json').read_text())
    need(receipt['sources_before'] == receipt['sources_after'], 'shared source drift during capture')
    need(len(receipt['calls']) == 2, 'two captures')
    previous = None
    for i,c in enumerate(receipt['calls']):
        kind = 'normal' if i == 0 else 'optimized'
        opts = [] if i == 0 else ['-O']
        need(Path(c['cwd']).is_absolute() and
             c['command'] == ['python3','-B']+opts+[str(Path(c['cwd'])/'check.py')],
             'capture command')
        need(c['code']==0 and c['stderr']=='', 'terminal capture')
        expected = (ROOT/(kind+'.stdout')).read_bytes()
        need(c['stdout'].encode()==expected and (ROOT/(kind+'.stderr')).read_bytes()==b'', 'capture channels')
        p = subprocess.run([sys.executable,'-B']+opts+[str(ROOT/'check.py')],
                           cwd=ROOT,stdout=subprocess.PIPE,stderr=subprocess.PIPE,check=False)
        need(p.returncode==0 and p.stderr==b'' and p.stdout==expected, 'bounded replay')
        need(previous is None or p.stdout==previous, 'normal/-O equality')
        previous = p.stdout
    result = json.loads(previous)
    need(len(result['median_cases'])==5, 'five toys')
    need(result['finite_bound']['exact_Bt']=='99' and
         result['finite_bound']['announced_simplified']=='35' and
         result['finite_bound']['not_a_stability_counterexample'] is True, 'finite-bound scope')
    return {'code':0,'replays':2,'toy_cases':5,'scope':'mathematical stream only, no native/GCP'}


if __name__ == '__main__':
    print(json.dumps(run(),sort_keys=True))
