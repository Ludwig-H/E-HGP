# Replique exacte de cluster.merge_tree, mais `members` remplace par sa TAILLE
# (entier). Mesure la somme des tailles que la version a ensembles materialise.
import sys, json, time, collections, resource
sys.dont_write_bytecode=True
T='/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v9/experiments/tower_clustering_20260928'
sys.path.insert(0,T)
import cluster as C, measure as M
rep=json.load(open(sys.argv[1])); nat=rep['native']['roots']
cofaces,gab,size=M.read_export(rep); del rep
births=M.facet_births(cofaces,gab,'gabriel')
facets,plateaus=C.facet_levels(cofaces,gab)
t=time.monotonic()
union=C.Union(facets); top={f:f for f in facets}; nodes={}; counter=0
for beta,groups in plateaus:
    merged=collections.defaultdict(set)
    for group in groups:
        roots={union.find(f) for f in group}
        if len(roots)<2: continue
        anchor=min(roots,key=lambda r:str(r))
        for r in roots: union.union(anchor,r)
        merged[union.find(anchor)].update(roots)
    for root,roots in merged.items():
        children=sorted({top[r] for r in roots},key=str)
        if len(children)<2: continue
        name='n%d'%counter; counter+=1
        cnt=sum(nodes[c] if c in nodes else 1 for c in children)
        nodes[name]=cnt
        for r in roots: top[r]=name
        top[root]=name
roots=sorted({top[union.find(f)] for f in facets},key=str)
s=sum(nodes.values())
depth_max=max(nodes.values())
print('facets',len(facets),'nodes',len(nodes),'roots',len(roots),'sum_members',s,'ratio_sum/facets %.0f'%(s/len(facets)),
      'root sizes',sorted(nodes[r] if r in nodes else 1 for r in roots)[-3:], 'singletons', sum(1 for r in roots if r not in nodes),
      'native_roots',nat,'secs %.1f'%(time.monotonic()-t),'rss %.2fGB'%(resource.getrusage(resource.RUSAGE_SELF).ru_maxrss/1e6),flush=True)
