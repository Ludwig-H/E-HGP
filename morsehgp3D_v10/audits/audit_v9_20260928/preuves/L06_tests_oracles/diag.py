import sys, os, json, subprocess, numpy as np
from sklearn.cluster import HDBSCAN
from sklearn.metrics import adjusted_rand_score
variant=sys.argv[1]
sys.path.insert(0, os.path.join(os.getcwd(), variant))
import cluster as C, measure as M
sys.path.insert(0, os.getcwd())
from k1_hdbscan_diff import cloud
B='/workspaces/E-HGP/build/v9-weighted-native-20260927-r1/native_weighted_export'
res=[]
for seed in range(6):
  for mcs in (10,25):
    pts=cloud(seed,1500,3+seed%4); open('diag.u32le','wb').write(pts.astype('<u4').tobytes())
    rep=json.loads(subprocess.run([B,'--input','diag.u32le','--k','1','--workers','1'],capture_output=True,text=True).stdout)
    cof,gab,size=M.read_export(rep); births=M.facet_births(cof,gab,'boundary')
    facets,pl=C.facet_levels(cof,None); nodes,roots=C.merge_tree(facets,pl,births)
    sums,totals,masses,cov=M.measure(cof,gab,1,'boundary')
    cl,order=C.condense(nodes,roots,masses,births,mcs,'radius',1)
    sel=C.select_excess_of_mass(cl,order)
    lab=np.asarray(C.vote(len(pts),C.label_facets(cl,sel),sums,totals,len(sel))[0])
    h=HDBSCAN(min_cluster_size=mcs,min_samples=1).fit(pts.astype(float)).labels_
    ok=adjusted_rand_score(lab,h)==1.0 and ((lab<0)==(h<0)).all()
    res.append(ok)
    if not ok: print(variant,'seed',seed,'mcs',mcs,'hgp',lab.max()+1,(lab<0).sum(),'hdb',h.max()+1,(h<0).sum(), 'ari',round(adjusted_rand_score(lab,h),4))
print(variant,'exact',sum(res),'/',len(res))
