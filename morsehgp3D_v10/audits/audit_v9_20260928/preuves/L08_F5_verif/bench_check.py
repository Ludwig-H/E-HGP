import sys, os, json, glob
H='/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v9/experiments/tower_clustering_20260928'
sys.path.insert(0, H)
import cluster as C, measure as M
E='/tmp/claude-1000/-workspaces-E-HGP/497c67a8-5e59-434d-a8a7-2dc2144d8af4/scratchpad/audit_v9/L08_clustering_code/exports'
def reach(nodes, roots):
    s=set(); st=list(roots)
    while st:
        x=st.pop()
        if x in s: continue
        s.add(x)
        if x in nodes: st.extend(nodes[x]['children'])
    return s
for p in sorted(glob.glob(E+'/*_2000_*.json')):
    rep=json.load(open(p))
    cof, gab, size = M.read_export(rep)
    for conv in ('gabriel','boundary'):
        births = M.facet_births(cof, gab, conv)
        keep = None if conv=='boundary' else gab
        facets, plateaus = C.facet_levels(cof, keep)
        nodes, roots = C.merge_tree(facets, plateaus, births)
        r = reach(nodes, roots)
        lost = len(facets - r)
        orphan = sum(1 for n in nodes if n not in r)
        nested = [(n,c) for n,d in nodes.items() for c in d['children'] if c in nodes and nodes[c]['level']==d['level']]
        # max group count per plateau
        multi = sum(1 for b,g in plateaus if len(g)>1)
        # root members union vs facets
        cov=set()
        for x in roots: cov |= nodes[x]['members'] if x in nodes else {x}
        print(os.path.basename(p)[:30], conv, 'facets',len(facets),'nodes',len(nodes),'multi',multi,'lost',lost,'orphan',orphan,'nested',len(nested), 'cov_missing', len(set(facets)-cov), flush=True)
