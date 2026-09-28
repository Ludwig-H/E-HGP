import sys, json, collections
sys.path.insert(0, '/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v9/experiments/tower_clustering_20260928')
import measure as M, cluster as C

def components_points(facet_sets):
    # union-find over points for a list of point sets
    parent = {}
    def f(x):
        while parent.setdefault(x, x) != x:
            parent[x] = parent[parent[x]]; x = parent[x]
        return x
    for s in facet_sets:
        s = list(s)
        for p in s: f(p)
        for p in s[1:]:
            a, b = f(s[0]), f(p)
            if a != b: parent[a] = b
    return len({f(x) for x in parent})

def run(report):
    cofaces, gabriel, size = M.read_export(report)
    out = {}
    for conv in ('gabriel', 'boundary'):
        births = M.facet_births(cofaces, gabriel, conv)
        keep = None if conv == 'boundary' else gabriel
        facets, plateaus = C.facet_levels(cofaces, keep)
        nodes, roots = C.merge_tree(facets, plateaus, births)
        # facet-graph components (facets connected through shared coface)
        # point coverage of each root
        cover = []
        for r in roots:
            mem = nodes[r]['members'] if r in nodes else {r}
            pts = set()
            for fa in mem: pts.update(fa)
            cover.append(pts)
        allpts = set().union(*cover)
        overl = sum(1 for i in range(len(cover)) for j in range(i+1, len(cover)) if cover[i] & cover[j])
        out[conv] = dict(roots=len(roots), facets=len(facets), covered_points=len(allpts), overlapping_root_pairs=overl,
                         point_components_of_roots=components_points(cover))
    nat = report['native']
    out['native_roots'] = len(nat['roots'])
    out['native_nodes'] = len(nat['nodes'])
    out['n'] = nat['point_count']; out['k'] = nat['k']
    out['gabriel_facets_in_catalogue'] = len(gabriel)
    return out

if __name__ == '__main__':
    for path in sys.argv[1:]:
        r = json.load(open(path))
        print(path.split('/')[-1], json.dumps(run(r)))
