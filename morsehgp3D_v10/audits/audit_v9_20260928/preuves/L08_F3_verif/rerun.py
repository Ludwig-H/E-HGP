import sys, csv, math, random, ast
import numpy as np
sys.path.insert(0, '/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v9/experiments/synthetic_bench_20260928')
import bench_datasets as data, plan as bench_plan, baselines
from sklearn.cluster import HDBSCAN
from sklearn.metrics import adjusted_rand_score as ari
A='/tmp/claude-1000/-workspaces-E-HGP/497c67a8-5e59-434d-a8a7-2dc2144d8af4/scratchpad/audit_v9/L08_clustering_code/'
specs={(s['scene'],str(s['seed'])):s for s in bench_plan.specifications(heavy=False)}
ms1={(r['scene'],r['seed']):r for r in csv.DictReader(open(A+'hdb_ms1_all.csv'))}
ext={(r['scene'],r['seed']):r for r in csv.DictReader(open(A+'oracle_ext.csv'))}
random.seed(7)
pick=random.sample(sorted(k for k in ms1 if ms1[k]['n']=='2000'),4)
for k in pick:
    s=specs[k]; pts,truth,meta=data.generate({kk:s[kk] for kk in data.SPEC_KEYS})
    X=np.asarray(pts,dtype=np.float64)
    lab=HDBSCAN(min_cluster_size=math.ceil(math.sqrt(len(X))),min_samples=1).fit(X).labels_
    print('ms1',k,'recomputed',round(ari(truth,lab),4),'csv',ms1[k]['ari_ms1_sqrt'])
pick2=[k for k in sorted(ext) if 'shells' in k[0]][:1]+random.sample(sorted(k for k in ext if 'spherical' in k[0] and ext[k]['n']=='2000'),2)+random.sample(sorted(k for k in ext if 'filaments' in k[0]),1)
for k in pick2:
    s=specs[k]; pts,truth,meta=data.generate({kk:s[kk] for kk in data.SPEC_KEYS})
    X=np.asarray(pts,dtype=np.float64)
    res={}
    for mcs in baselines.ORACLE_GRID:
        if mcs*2>len(X): continue
        for ms in (1,5,None):
            for sel in ('eom','leaf'):
                res[(mcs,ms,sel)]=ari(truth,HDBSCAN(min_cluster_size=mcs,min_samples=ms,cluster_selection_method=sel).fit(X).labels_)
    best=max(res.values()); restr=max(v for (m,ms,sel),v in res.items() if ms is None and sel=='eom')
    best_ms1=max(v for (m,ms,sel),v in res.items() if ms==1); best_not1=max(v for (m,ms,sel),v in res.items() if ms!=1)
    print('ext',k,'best',round(best,4),'csv',ext[k]['oracle_ext'],'restr',round(restr,4),'csv',ext[k]['oracle_restricted'],'best_ms1',round(best_ms1,4),'best_ms!=1',round(best_not1,4))
