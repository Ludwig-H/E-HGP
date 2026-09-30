import csv, sys, statistics
R='/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v9/receipts/synthetic_bench_20260928/r1/baselines.csv'
B='/tmp/claude-1000/-workspaces-E-HGP/b64b3f68-b0cf-4b1f-9ec0-8ffa4358295a/scratchpad/bench/'
base={}
for r in csv.DictReader(open(R)):
    base[(r['scene'],r['seed'],r['method'])]=r
def rows(f):
    return list(csv.DictReader(open(B+f)))
for f in ['tour_z1.csv','tour_z1b.csv']:
    t=rows(f)
    sph=[r for r in t if r['family']=='spherical']
    print('==',f,'spherical rows',len(sph),'scenes',len({r['scene'] for r in sph}))
    for subset_name,sub in [('all',sph),('first34',sph[:34])]:
        for ref in ['single_linkage','hdbscan_default','hdbscan_oracle']:
            d=[];w=l=e=0
            for r in sub:
                b=base.get((r['scene'],r['seed'],ref))
                if b is None: continue
                x=float(r['ari'])-float(b['ari']); d.append(x)
                if x>1e-12: w+=1
                elif x<-1e-12: l+=1
                else: e+=1
            print(f'  {subset_name:8s} vs {ref:16s} pairs={len(d)} mean={statistics.mean(d):+.3f} W-L-T={w}-{l}-{e}')
    # by level on first34
    for lvl in ['easy','medium','hard','extreme']:
        d=[float(r['ari'])-float(base[(r['scene'],r['seed'],'hdbscan_oracle')]['ari']) for r in sph[:34] if r['level']==lvl]
        if d: print('   first34 level',lvl,len(d),f'{statistics.mean(d):+.3f}')
