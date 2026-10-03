import sys, json
sys.dont_write_bytecode = True
import indep_full as I, hier_fast as HF, cells as C
v2 = json.load(open(C.V2)); by = {e['name']: e for e in v2['fixtures']}
n_ok = n = 0
for _c, name, mcss, _m in C.CELLS:
    e = by[name]; pts = e['points']; P = [tuple(pts[x]) for x in pts]; T = I.Full(P, e['K'])
    for rule in ('H1', 'Hk1', 'H1r', 'P2r', 'cover'):
        U, kind = C.build(rule, T, 2); a, b = I.Hier(U, kind), HF.HierFast(U, kind)
        ca, cb = a.rayons_changement(), b.rayons_changement()
        same_chg = len(ca) == len(cb) and all(x.cmp(y) == 0 for x, y in zip(ca, cb))
        for r in ca:
            n += 1; n_ok += a.partition(r) == b.partition(r)
        if not same_chg: print('chg differ', name, rule)
print('partitions egales', n_ok, '/', n)
