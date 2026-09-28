import csv, statistics, collections
R='/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v9/receipts/synthetic_bench_20260928/r1/baselines.csv'
B='/tmp/claude-1000/-workspaces-E-HGP/b64b3f68-b0cf-4b1f-9ec0-8ffa4358295a/scratchpad/bench/'
base={(r['scene'],r['seed'],r['method']):r for r in csv.DictReader(open(R))}
def load(f): return {(r['scene'],r['seed']):r for r in csv.DictReader(open(B+f))}
smoke=load('tour_smoke.csv'); pre=load('tour_z1.csv'); post=load('tour_z1b.csv')
keys=list(smoke)
print('smoke pairs',len(keys),'seeds',collections.Counter(k[1] for k in keys))
print('scenes',len({k[0] for k in keys}))
same=sum(1 for k in keys if k in pre and abs(float(pre[k]['ari'])-float(smoke[k]['ari']))<1e-12)
print('smoke==tour_z1 (pre-fix) ARI identical on',same,'/',len(keys))
same2=sum(1 for k in keys if k in post and abs(float(post[k]['ari'])-float(smoke[k]['ari']))<1e-12)
print('smoke==tour_z1b (post-fix) ARI identical on',same2,'/',len(keys))
for name,src in [('smoke(pre)',smoke),('tour_z1b(post)',post)]:
    for ref in ['single_linkage','hdbscan_default','hdbscan_oracle']:
        d=[float(src[k]['ari'])-float(base[(k[0],k[1],ref)]['ari']) for k in keys]
        w=sum(x>1e-12 for x in d); l=sum(x<-1e-12 for x in d); t=len(d)-w-l
        print(f'{name:15s} vs {ref:16s} mean={statistics.mean(d):+.3f} W-L-T={w}-{l}-{t}')
    for lvl in ['easy','medium','hard','extreme']:
        d=[float(src[k]['ari'])-float(base[(k[0],k[1],'hdbscan_oracle')]['ari']) for k in keys if src[k]['level']==lvl]
        print('   ',name,lvl,len(d),f'{statistics.mean(d):+.3f}')
    print('   mean coverage',statistics.mean(float(src[k]['coverage']) for k in keys),'oracle cov',statistics.mean(float(base[(k[0],k[1],'hdbscan_oracle')]['coverage']) for k in keys))
