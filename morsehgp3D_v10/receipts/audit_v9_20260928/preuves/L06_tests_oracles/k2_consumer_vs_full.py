"""Le consommateur Python (Kruskal sur cofaces Gabriel, convention gabriel/boundary) a-t-il le meme pi_0 que la
tour FULL native (jugee par T2) ? Compte des composantes a chaque niveau critique, K donne."""
import sys, os, json, subprocess, numpy as np
from fractions import Fraction
sys.path.insert(0, os.path.join(os.getcwd(), 'headclust'))
import cluster as C, measure as M
B='/workspaces/E-HGP/build/v9-weighted-native-20260927-r1/native_weighted_export'
def fr(e): return Fraction(int(e['num']), int(e['den']))
def run(seed, n, k, box):
    rng=np.random.default_rng(seed); s=set()
    while len(s)<n: s.add(tuple(int(v) for v in rng.integers(0,box,3)))
    pts=np.array(sorted(s)); rng.shuffle(pts); open('k2.u32le','wb').write(pts.astype('<u4').tobytes())
    out=subprocess.run([B,'--input','k2.u32le','--k',str(k),'--workers','1'],capture_output=True,text=True)
    if out.returncode: return 'refused '+out.stderr[:120]
    rep=json.loads(out.stdout); nat=rep['native']
    lev={nd['id']:fr(nd['level']) for nd in nat['nodes']}
    succ={nd['id']:nd['successor'] for nd in nat['nodes']}
    cof,gab,size=M.read_export(rep)
    levels=sorted(set(lev.values())|{b for _,b in cof})
    res={}
    for conv in ('gabriel','boundary'):
        births=M.facet_births(cof,gab,conv); keep=None if conv=='boundary' else gab
        bad=0
        for a in levels:
            full=sum(1 for i in lev if lev[i]<=a and (succ[i] is None or lev[succ[i]]>a))
            present=[f for f,b in births.items() if b<=a]
            uf={f:f for f in present}
            def find(x):
                while uf[x]!=x: uf[x]=uf[uf[x]]; x=uf[x]
                return x
            for v,b in cof:
                if b>a: continue
                g=[f for f in M.facets(v) if (keep is None or f in keep) and f in uf]
                for f in g[1:]: uf[find(f)]=find(g[0])
            py=len({find(f) for f in present})
            bad+= (full!=py)
        res[conv]=bad
    return dict(seed=seed,n=n,k=k,box=box,levels=len(levels),bad_levels=res)
for k in (2,3):
  for seed in range(3):
    for box in (40,5000):
      print(json.dumps(run(seed,200,k,box),default=str),flush=True)
