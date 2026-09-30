import sys, json, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import cluster_head as C, measure_head as M
def reach(nodes, roots):
    seen, st = set(), list(roots)
    while st:
        c = st.pop()
        if c in seen: continue
        seen.add(c)
        if c in nodes: st.extend(nodes[c]['children'])
    return seen
for path in sys.argv[1:]:
    rep = json.load(open(path))
    cof, gab, size = M.read_export(rep)
    for conv in ('gabriel', 'boundary'):
        keep = None if conv == 'boundary' else gab
        facets, plateaus = C.facet_levels(cof, keep)
        births = M.facet_births(cof, gab, conv)
        nodes, roots = C.merge_tree(facets, plateaus, births)
        lost = len(facets - reach(nodes, roots))
        nested = sum(1 for n in nodes.values() for ch in n['children'] if ch in nodes and nodes[ch]['level'] == n['level'])
        sums, totals, masses, covered = M.measure(cof, gab, 1, conv)
        rm = sorted(float(sum(float(masses[f]) for f in (nodes[r]['members'] if r in nodes else {r}))) for r in roots)
        print(os.path.basename(path)[:30], conv, 'facets', len(facets), 'multi', sum(1 for b,g in plateaus if len(g)>1), 'roots', len(roots), 'lost', lost, 'nested', nested, 'root masses<45', sum(1 for m in rm if m < 44.72), 'min', round(rm[0],3), flush=True)
