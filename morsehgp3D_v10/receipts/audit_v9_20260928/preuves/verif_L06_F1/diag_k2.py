import sys, os, json, subprocess, numpy as np, collections
from fractions import Fraction
sys.path.insert(0, os.path.join(os.getcwd(), 'headclust'))
import cluster as C, measure as M
B='/workspaces/E-HGP/build/v9-weighted-native-20260927-r1/native_weighted_export'
def fr(e): return Fraction(int(e['num']), int(e['den']))
def one(seed,n,k,box,conv):
    rng=np.random.default_rng(seed); s=set()
    while len(s)<n: s.add(tuple(int(v) for v in rng.integers(0,box,3)))
    pts=np.array(sorted(s)); rng.shuffle(pts); open('d2.u32le','wb').write(pts.astype('<u4').tobytes())
    rep=json.loads(subprocess.run([B,'--input','d2.u32le','--k',str(k),'--workers','1'],capture_output=True,text=True).stdout)
    nat=rep['native']
    full_merges=sorted((fr(nd['level']),len(nd['children'])-1) for nd in nat['nodes'] if nd['children'])
    full_births=sorted(fr(nd['level']) for nd in nat['nodes'] if not nd['children'])
    cof,gab,size=M.read_export(rep)
    births=M.facet_births(cof,gab,conv); keep=None if conv=='boundary' else gab
    facets,pl=C.facet_levels(cof,keep); nodes,roots=C.merge_tree(facets,pl,births)
    py_merges=sorted((nd['level'],len(nd['children'])-1) for nd in nodes.values())
    py_births=sorted(births[f] for f in facets)
    fm=collections.Counter(); [fm.update({l:c}) for l,c in full_merges]
    pm=collections.Counter(); [pm.update({l:c}) for l,c in py_merges]
    print(f'seed={seed} n={n} K={k} box={box} conv={conv}: FULL births={len(full_births)} merges(sum c-1)={sum(fm.values())} roots={len(nat["roots"])} | '
          f'PY facets={len(facets)} merges={sum(pm.values())} roots={len(roots)} | merge-multiset equal={fm==pm} '
          f'| births multiset equal={collections.Counter(full_births)==collections.Counter(py_births)}')
for k,conv in ((2,'gabriel'),(2,'boundary'),(3,'boundary'),(3,'gabriel')):
    for seed in range(2): one(seed,60,k,5000,conv)
