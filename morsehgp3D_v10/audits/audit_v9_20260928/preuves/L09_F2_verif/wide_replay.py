import sys, csv, itertools, warnings
warnings.filterwarnings('ignore')
sys.path.insert(0,'/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v9/experiments/synthetic_bench_20260928')
import bench_datasets as d, plan as P
from sklearn.cluster import HDBSCAN
from sklearn.metrics import adjusted_rand_score as ari
W='/tmp/claude-1000/-workspaces-E-HGP/497c67a8-5e59-434d-a8a7-2dc2144d8af4/scratchpad/audit_v9/L09_bench_method/wide_oracle.csv'
wide={(r['scene'],r['seed']):r for r in csv.DictReader(open(W))}
MCS=(5,10,20,30,45,75,100,150); MS=(1,2,3,5,10,20,None)
want=[('spherical','medium',2026092802),('filaments','hard',2026092800),('bridge','medium',2026092803)]
for s in P.specifications():
    if s['n']==2000 and s['groups']==8 and s['noise_fraction']==0.0 and (s['family'],s['level'],s['seed']) in want:
        p,t,_=d.generate({k:s[k] for k in d.SPEC_KEYS}); best=(-2,None)
        for mcs,ms,sel in itertools.product(MCS,MS,('eom','leaf')):
            a=ari(t,HDBSCAN(min_cluster_size=mcs,min_samples=ms,cluster_selection_method=sel).fit(p).labels_)
            if a>best[0]: best=(a,'%d/%s/%s'%(mcs,ms,sel))
        w=wide[(s['scene'],str(s['seed']))]
        print(s['scene'],s['seed'],'%.4f %s'%best,'csv %.4f %s'%(float(w['ari']),w['parameter']),flush=True)
