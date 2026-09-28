"""Rejeu independant d'un echantillon : HDBSCAN apparie (mcs=round(sqrt n), ms=3, eom),
oracle etroit du banc (baselines.hdbscan_oracle), compares aux CSV de l'auditeur et au recu r1."""
import sys, csv, math, random, warnings
warnings.filterwarnings('ignore')
B='/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v9/experiments/synthetic_bench_20260928'
sys.path.insert(0, B)
import numpy as np
import bench_datasets as d, plan as P, baselines as BL
from sklearn.cluster import HDBSCAN
from sklearn.metrics import adjusted_rand_score as ari
R='/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v9/receipts/synthetic_bench_20260928/r1/baselines.csv'
M='/tmp/claude-1000/-workspaces-E-HGP/497c67a8-5e59-434d-a8a7-2dc2144d8af4/scratchpad/audit_v9/L09_bench_method/matched.csv'
rec={(r['scene'],r['seed'],r['method']):r for r in csv.DictReader(open(R))}
mat={(r['scene'],r['seed'],r['method']):r for r in csv.DictReader(open(M))}
specs=[s for s in P.specifications() if s['n']<=2000]
random.seed(7); sample=random.sample(specs, 14)
sample += [s for s in specs if s['family']=='spherical' and s['n']==2000 and s['groups']==8 and s['level']=='medium' and s['noise_fraction']==0.0][:2]
for s in sample:
    p,t,_=d.generate({k:s[k] for k in d.SPEC_KEYS})
    n=len(p); mcs=max(2,int(round(math.sqrt(n))))
    m=ari(t, HDBSCAN(min_cluster_size=mcs, min_samples=3, copy=True).fit(p).labels_)
    o=BL.hdbscan_oracle(p,t)
    key=(s['scene'],str(s['seed']))
    print('%-40s %s matched=%.4f (csv %.4f) oracle=%.4f/mcs%d (recu %.4f/%s)' % (s['scene'], s['seed'], m,
          float(mat[key+('hdbscan_matched_eom',)]['ari']), o['ari'], o['parameter'],
          float(rec[key+('hdbscan_oracle',)]['ari']), rec[key+('hdbscan_oracle',)]['parameter']), flush=True)
