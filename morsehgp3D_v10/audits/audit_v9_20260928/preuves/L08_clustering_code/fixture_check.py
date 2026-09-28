import sys, json, collections
sys.path.insert(0, '/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v9/experiments/tower_clustering_20260928')
import cluster as C, measure as M
def reachable(nodes, roots):
    seen, stack = set(), list(roots)
    while stack:
        cur = stack.pop()
        if cur in seen: continue
        seen.add(cur)
        if cur in nodes: stack.extend(nodes[cur]['children'])
    return seen
def analyse(report, conv, label=''):
    cofaces, gabriel, size = M.read_export(report)
    births = M.facet_births(cofaces, gabriel, conv)
    keep = None if conv == 'boundary' else gabriel
    facets, plateaus = C.facet_levels(cofaces, keep)
    nodes, roots = C.merge_tree(facets, plateaus, births)
    reach = reachable(nodes, roots)
    multi = sum(1 for b, g in plateaus if len(g) > 1)
    lvl = collections.Counter(n['level'] for n in nodes.values())
    dup_levels = sum(1 for v in lvl.values() if v > 1)
    # nodes nested at same level
    nested = sum(1 for n in nodes.values() for ch in n['children'] if ch in nodes and nodes[ch]['level'] == n['level'])
    # catalogue balls with more sites than q_min on shell (degenerate)
    cat = report['catalogue']
    degenerate = sum(1 for b in cat if len(b['shell']) > b['q_min'])
    # gabriel facets missing because shell has extra points: K-sets = interior + minimal support subsets with total K
    print(label, conv, 'cofaces', len(cofaces), 'facets', len(facets), 'plateaus', len(plateaus), 'multi-group plateaus', multi,
          'nodes', len(nodes), 'roots', len(roots), 'lost facets', len(facets - reach), 'orphan nodes', len(set(nodes) - reach),
          'nested same-level', nested, 'degenerate balls', degenerate, 'gabriel', len(gabriel))
    return cofaces, gabriel, facets, nodes, roots
if __name__ == '__main__':
    rep = json.load(open(sys.argv[1]))
    for conv in ('gabriel', 'boundary'):
        analyse(rep, conv, sys.argv[1].split('/')[-1])
