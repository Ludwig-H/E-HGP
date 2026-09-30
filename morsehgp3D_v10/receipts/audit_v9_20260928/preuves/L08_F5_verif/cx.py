import sys, itertools
sys.path.insert(0, '/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v9/experiments/tower_clustering_20260928')
import cluster
def run(cofaces, label):
    facets, plateaus = cluster.facet_levels(cofaces, None)
    births = {f: 0 for f in facets}
    nodes, roots = cluster.merge_tree(facets, plateaus, births)
    print('==', label)
    for n, d in nodes.items():
        print(' ', n, 'level', d['level'], 'children', d['children'], 'members', sorted(d['members']))
    print('  roots', roots)
    covered = set()
    for r in roots:
        covered |= nodes[r]['members'] if r in nodes else {r}
    print('  missing from roots:', sorted(set(facets) - covered))
    # nested same level
    for n, d in nodes.items():
        for c in d['children']:
            if c in nodes and nodes[c]['level'] == d['level']:
                print('  NESTED same level', n, '>', c)
    # orphans: nodes not reachable from roots
    reach=set(); st=list(roots)
    while st:
        x=st.pop()
        if x in reach: continue
        reach.add(x)
        if x in nodes: st.extend(nodes[x]['children'])
    print('  orphan nodes:', [n for n in nodes if n not in reach])
run([((0,1,3),4),((0,2,4),4),((0,3,4),4)], 'claimed cx')
# all permutations of coface order
for perm in itertools.permutations([(0,1,3),(0,2,4),(0,3,4)]):
    run([(c,4) for c in perm], 'perm %s' % (perm,))
