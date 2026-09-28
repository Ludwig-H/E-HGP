import sys, json, os, math
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import cluster_head as C, cluster_wt as W, measure_head as M
for path in sys.argv[1:]:
    rep = json.load(open(path))
    cof, gab, size = M.read_export(rep)
    conv = 'gabriel'
    keep = gab
    facets, plateaus = C.facet_levels(cof, keep)
    births = M.facet_births(cof, gab, conv)
    nodes, roots = C.merge_tree(facets, plateaus, births)
    sums, totals, masses, covered = M.measure(cof, gab, 1, conv)
    thr = math.sqrt(2000)
    out = []
    for tag, mod, sel in (('HEAD_eom', C, lambda cl, o: C.select_excess_of_mass(cl, o)), ('WT_eom', W, lambda cl, o: W.select(cl, o, 'eom')), ('WT_leaf', W, lambda cl, o: W.select(cl, o, 'leaf'))):
        cl, order = mod.condense(nodes, roots, masses, births, thr, 'radius', 1)
        s = sel(cl, order)
        # mass of selected cluster = mass of all facets falling in it or descendants
        def tot(name):
            st, m = [name], 0.0
            while st:
                c = st.pop(); m += cl[c]['mass']; st.extend(cl[c]['children'])
            return m
        tiny = [round(tot(x), 2) for x in s if tot(x) < thr]
        out.append('%s sel=%d tiny=%d %s' % (tag, len(s), len(tiny), tiny[:6]))
    print(os.path.basename(path)[:28], 'roots', len(roots), ' | '.join(out), flush=True)
