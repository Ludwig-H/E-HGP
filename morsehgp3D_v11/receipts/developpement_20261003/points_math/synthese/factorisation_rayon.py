"""Meme controle que factorisation_check.py pour les regles en RAYON de l'oracle (reference_radius_rules) :
margin_r1 (= P_1 o Pi_1) et margin_r (= P_1 o Pi_{k+1} = H^r_{k+1}). Blocs aux coupes fermees L : x entre si
e_x <= sqrt(L) ; x, y reunis si les ancetres vivants a L de leurs proprietaires coincident (tolerance 1e-90)."""
import sys, random, time, json
from decimal import Decimal, localcontext, Context
sys.dont_write_bytecode = True
sys.path.insert(0, '/workspaces/E-HGP/build/v11-claude-20261003/morsehgp3D_v11/bench')
import points_reference as pr
from hgp11_ref import Definition
def popcount(x):
    return bin(x).count('1')
seed, clouds, seconds = int(sys.argv[1]), int(sys.argv[2]), float(sys.argv[3])
rng = random.Random(seed); t0 = time.time()
st = dict(nuages=0, couples=0, blocs=0, blocs_hors_amas=0, egalites_F=0, ecarts_F=0)
tol = Decimal('1e-90')
with localcontext(Context(prec=120)):
    for c in range(clouds):
        if time.time() - t0 > seconds:
            break
        n = rng.randint(4, 8); side = rng.choice([3, 4, 6, 10, 30])
        pts = set()
        while len(pts) < n:
            pts.add((rng.randint(0, side), rng.randint(0, side), rng.randint(0, side // 3 or 1)))
        pts = sorted(pts); st['nuages'] += 1
        D = Definition(pts)
        for k in (2, 3):
            if k + 1 > n:
                continue
            res = D.order(k)
            cov = {}
            for cut in res.cuts:
                for v, coverage, _core in cut.closed:
                    cov[(cut.level, v)] = coverage
            levels = sorted(set(cut.level for cut in res.cuts))
            for m, key in ((1, 'margin_r1'), (k + 1, 'margin_r')):
                out, tree = pr.reference_radius_rules(res, n, m)
                entries = out[key]
                st['couples'] += 1
                for L in levels:
                    r = (Decimal(L.numerator) / Decimal(L.denominator)).sqrt()
                    groups = {}
                    for i in range(n):
                        e, owner = entries[i][0], entries[i][1]
                        if e <= r + tol:
                            v = tree.alive_ancestor(owner, L)
                            groups.setdefault(v, set()).add(i)
                    hb = []
                    for v, B in groups.items():
                        mask = cov.get((L, v), 0)
                        st['blocs'] += 1
                        if not all(mask >> i & 1 for i in B):
                            st['blocs_hors_amas'] += 1
                        hb.append((frozenset(B), popcount(mask)))
                    for mcs in range(2, n + 1):
                        big = set(B for B, _ in hb if len(B) >= mcs)
                        absorb = set(B for B, size in hb if size >= mcs and len(B) >= mcs)
                        st['egalites_F' if big == absorb else 'ecarts_F'] += 1
st['secondes'] = round(time.time() - t0, 1)
print(json.dumps(dict(graine=seed, stats=st), indent=1))
