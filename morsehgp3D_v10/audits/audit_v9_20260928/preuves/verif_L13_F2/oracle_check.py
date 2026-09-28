import json, subprocess, sys, itertools
def run(args):
    out = subprocess.run(['nice','-n','19','./cble']+args, capture_output=True, text=True, check=True).stdout
    return json.loads(out)
fails = 0; total = 0; rows=[]
cases = []
for fam, n, rng in (('uniform',40,64),('uniform',60,16),('plane',40,8),('plane',60,12),('grid',40,5),('grid',60,5)):
    for seed in (11,12,13):
        for K in (2,3,5,8):
            cases.append((fam,n,rng,seed,K))
for fam,n,rng,seed,K in cases:
    base = [f'--gen={n}', f'--family={fam}', f'--seed={seed}', f'--range={rng}', f'--K={K}']
    o = run(base+['--oracle'])
    for extra in (['--M=6'], ['--M=6','--dom=3'], ['--M=4','--dom=2','--threads=2'], ['--M=6','--noprune']):
        b = run(base+extra)
        total += 1
        same = (o['by_q_p']==b['by_q_p'] and o['balls']==b['balls'] and o['extra_shell']==b['extra_shell'])
        if not same:
            fails += 1
            print('MISMATCH', fam,n,rng,seed,K,extra, o['by_q_p'], b['by_q_p'], o['extra_shell'], b['extra_shell'])
    rows.append((fam,n,rng,seed,K,o['balls'],o['extra_shell'],o['max_shell']))
for r in rows: print(r)
print('total', total, 'fails', fails)
