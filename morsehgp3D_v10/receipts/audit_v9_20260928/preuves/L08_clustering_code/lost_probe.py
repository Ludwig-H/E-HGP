"""Replique sans ensembles de merge_tree : facettes perdues et noeuds imbriques de meme niveau, a l'echelle."""
import sys, os, json, collections
sys.path.insert(0, '/tmp/claude-1000/-workspaces-E-HGP/497c67a8-5e59-434d-a8a7-2dc2144d8af4/scratchpad/audit_v9/L08_clustering_code')
import probe as P
C, M = P.C, P.M
def audit_tree(facets, plateaus):
    union = C.Union(facets)
    top = {f: f for f in facets}
    size, level, kids = {}, {}, {}
    count = 0
    for beta, groups in plateaus:
        merged = collections.defaultdict(set)
        for group in groups:
            roots = {union.find(f) for f in group}
            if len(roots) < 2: continue
            anchor = min(roots, key=lambda r: str(r))
            for r in roots: union.union(anchor, r)
            merged[union.find(anchor)].update(roots)
        for root, roots in merged.items():
            children = {top[r] for r in roots}
            if len(children) < 2: continue
            name = ('n', count); count += 1
            size[name] = sum(size.get(ch, 1) for ch in children); level[name] = beta; kids[name] = children
            for r in roots: top[r] = name
            top[root] = name
    roots = {top[union.find(f)] for f in facets}
    covered = sum(size.get(r, 1) for r in roots)
    nested = sum(1 for n, ch in kids.items() for c in ch if c in level and level[c] == level[n])
    # orphan nodes: nodes not reachable from roots
    reach, stack = set(), list(roots)
    while stack:
        x = stack.pop()
        if x in reach: continue
        reach.add(x)
        stack.extend(kids.get(x, ()))
    orphans = sum(1 for n in kids if n not in reach)
    lost = len(facets) - sum(1 for f in facets if f in reach)
    return dict(nodes=count, roots=len(roots), covered_by_roots=covered, facets=len(facets), lost_facets=lost, orphan_nodes=orphans, nested_same_level=nested)
if __name__ == "__main__":
  for path in sys.argv[1:]:
      report = json.load(open(path))
      cofaces, gabriel, size = M.read_export(report)
      for conv in ('gabriel', 'boundary'):
          keep = None if conv == 'boundary' else gabriel
          facets, plateaus = C.facet_levels(cofaces, keep)
          out = audit_tree(facets, plateaus)
          out.update(tag=os.path.basename(path), conv=conv, multi=sum(1 for b, g in plateaus if len(g) > 1), native_roots=len(report['native']['roots']))
          print(json.dumps(out), flush=True)
