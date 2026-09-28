import sys, itertools
sys.path.insert(0, sys.argv[1])
import weighted_model as W
bad = 0; total = 0; ex = None
for npts in range(4, 7):
    tris = list(itertools.combinations(range(npts), 3))
    for m in range(2, 4):
        for combo in itertools.combinations(tris, m):
            cof = [dict(vertices=list(t), beta=dict(num=4, den=1)) for t in combo]
            mod = W.build_facet_model(npts, 2, cof, exp_z=2, rational_z2=True)
            total += 1
            F = len(mod['facets'])
            # every facet reachable from roots, one node per component, no nested same-level node
            ch = mod['children']
            seen, st = set(), list(mod['roots'])
            while st:
                c = st.pop()
                if c in seen: continue
                seen.add(c); st.extend(ch.get(c, []))
            leaves = {x for x in seen if x < F}
            nested = any(c >= F for row in ch.values() for c in row)
            if leaves != set(range(F)) or nested:
                bad += 1; ex = (combo, ch, mod['roots'])
print('total', total, 'bad', bad, ex)
