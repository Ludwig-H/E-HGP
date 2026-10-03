"""Bornes d'entree de H^r_{k+1} (= P_1 o Pi_{k+1}, marge en rayon) : alpha_{k+1}(x) <= e(x) <= alpha_{k+1}(x) + d_k(x)/2,
avec alpha_{k+1} = racine de la premiere couverture qualifiee (m = k+1). Oracle de la definition, nuages aleatoires."""
import sys, random, time, json
from decimal import Decimal, getcontext
sys.dont_write_bytecode = True
sys.path.insert(0, '/workspaces/E-HGP/build/v11-claude-20261003/morsehgp3D_v11/bench')
import points_reference as pr
from hgp11_ref import Definition
getcontext().prec = 60
def rad(level):
    return (Decimal(level.numerator) / Decimal(level.denominator)).sqrt()
def popcount(x):
    return bin(x).count('1')
seed, clouds, seconds = int(sys.argv[1]), int(sys.argv[2]), float(sys.argv[3])
rng = random.Random(seed); t0 = time.time()
st = dict(nuages=0, sites=0, viol_bas=0, viol_haut=0, max_ratio_e_sur_dk=0.0, max_retard_sur_dk=0.0, apres_coeur=0)
tol = Decimal('1e-40')
for c in range(clouds):
    if time.time() - t0 > seconds:
        break
    n = rng.randint(4, 8); side = rng.choice([3, 5, 8, 20, 60])
    pts = set()
    while len(pts) < n:
        pts.add((rng.randint(0, side), rng.randint(0, side), rng.randint(0, max(1, side // 4))))
    pts = sorted(pts); st['nuages'] += 1
    D = Definition(pts)
    for k in (2, 3):
        if k + 1 > n:
            continue
        res = D.order(k)
        out = pr.reference_radius_rules(res, n, k + 1)
        out = out[0] if isinstance(out, tuple) else out
        firstq = [None] * n
        for cut in res.cuts:
            for v, coverage, _core in cut.closed:
                if popcount(coverage) >= k + 1:
                    for i in range(n):
                        if coverage >> i & 1 and (firstq[i] is None or cut.level < firstq[i]):
                            firstq[i] = cut.level
        for i in range(n):
            e = Decimal(out['margin_r'][i][0]); a = rad(firstq[i]); dk = rad(res.core[i].level)
            st['sites'] += 1
            if e < a - tol: st['viol_bas'] += 1
            if e > a + dk / 2 + tol: st['viol_haut'] += 1
            if e > dk + tol: st['apres_coeur'] += 1
            st['max_ratio_e_sur_dk'] = max(st['max_ratio_e_sur_dk'], float(e / dk))
            st['max_retard_sur_dk'] = max(st['max_retard_sur_dk'], float((e - a) / dk))
st['secondes'] = round(time.time() - t0, 1)
print(json.dumps(dict(graine=seed, stats=st), indent=1))
