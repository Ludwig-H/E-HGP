import sys, os, json, math, collections
H='/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v9/experiments'
sys.path.insert(0, H+'/tower_clustering_20260928'); sys.path.insert(0, H+'/synthetic_bench_20260928')
import cluster as C, measure as M, run_tower as R
orig = C.merge_tree
def fixed_merge_tree(facets, plateaus, births):
    union = C.Union(facets); top = {f: f for f in facets}; nodes = {}; counter=[0]
    for beta, groups in plateaus:
        previous = {}
        for g in groups:
            for f in g:
                r = union.find(f); previous[r] = top[r]
        for g in groups:
            rs = {union.find(f) for f in g}
            a = min(rs, key=str)
            for r in rs: union.union(a, r)
        grp = collections.defaultdict(set)
        for old, node in previous.items():
            grp[union.find(old)].add(node)
        for root, kids in sorted(grp.items(), key=lambda it: str(min(it[1], key=str))):
            if len(kids) < 2: continue
            children = sorted(kids, key=str)
            name='n%d'%counter[0]; counter[0]+=1
            members=set()
            for ch in children: members.update(nodes[ch]['members'] if ch in nodes else {ch})
            nodes[name]=dict(children=children, level=beta, members=members)
            top[root]=name
    roots = sorted({top[union.find(f)] for f in facets}, key=str)
    return nodes, roots
# sanity on the cx
cof=[((0,1,3),4),((0,2,4),4),((0,3,4),4)]
fa, pl = C.facet_levels(cof, None)
n, r = fixed_merge_tree(fa, pl, {f:0 for f in fa})
print('fixed cx:', {k:(v['children']) for k,v in n.items()}, r)
E='/tmp/claude-1000/-workspaces-E-HGP/497c67a8-5e59-434d-a8a7-2dc2144d8af4/scratchpad/audit_v9/L08_clustering_code/exports'
for fam in sys.argv[1:]:
    rep=json.load(open(E+'/%s_medium_2000_8_0.0_s2026092800_k2.json'%fam))
    cof, gab, size = M.read_export(rep)
    npts = 1 + max(v for c,_ in cof for v in c)
    for conv in ('gabriel','boundary'):
        res={}
        for tag, fn in (('orig',orig),('fixed',fixed_merge_tree)):
            C.merge_tree = fn
            out, det = R.tower_variants(rep, npts, 2, 1, conv, math.sqrt(2000), 'radius')
            res[tag]=(out,det)
        C.merge_tree = orig
        for v in sorted(res['orig'][0]):
            a=res['orig'][0][v][0]; b=res['fixed'][0][v][0]
            diff=sum(1 for x,y in zip(a,b) if x!=y)
            print(fam, conv, v, 'nodes', res['orig'][1]['nodes'], res['fixed'][1]['nodes'], 'labels differing', diff, 'nclusters', len(set(a)), len(set(b)), flush=True)
