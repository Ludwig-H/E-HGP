import sys, json, math, collections
sys.dont_write_bytecode=True
T='/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v9/experiments/tower_clustering_20260928'
sys.path.insert(0,T)
import cluster as C, measure as M
rep=json.load(open(sys.argv[1])); n=int(sys.argv[2])
cofaces,gab,size=M.read_export(rep); del rep
births=M.facet_births(cofaces,gab,'gabriel')
facets,plateaus=C.facet_levels(cofaces,gab)
nodes,roots=C.merge_tree(facets,plateaus,births)
sums,totals,masses,covered=M.measure(cofaces,gab,1,'gabriel')
single=[r for r in roots if r not in nodes]
print('singleton roots',len(single),'their masses',sorted(round(float(masses[r]),3) for r in single))
for scale in C.STABILITY_SCALES:
    clusters,order=C.condense(nodes,roots,masses,births,math.sqrt(n),'radius',1,scale)
    for method in C.SELECTIONS:
        sel=C.select(clusters,order,method)
        sing_sel=[s for s in sel if clusters[s]['node'] in single]
        labels,ties=C.vote(n,C.label_facets(clusters,sel),sums,totals,len(sel))
        idx={s:i for i,s in enumerate(sel)}
        won=collections.Counter(labels)
        pts=sum(won[idx[s]] for s in sing_sel)
        print(scale,method,'selected',len(sel),'singleton roots selected',len(sing_sel),'points won by them',pts,'noise',won[-1])
