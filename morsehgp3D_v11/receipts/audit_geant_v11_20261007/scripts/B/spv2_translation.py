# Verifie que la selection de Kruskal de SPv2 (ordre BallIdx = (niveau, S* lex en SiteIdx = rang de Morton))
# n'est pas equivariante par translation entiere. Triangle equilateral K=1, trois boules diametrales au meme niveau.
from fractions import Fraction
from itertools import combinations

def spread(v, bits=21):
    out = 0
    for i in range(bits):
        if (v >> i) & 1:
            out |= 1 << (3 * i)
    return out

def morton(p):
    x, y, z = p
    return spread(x) | (spread(y) << 1) | (spread(z) << 2)

def d2(a, b):
    return sum((x - y) ** 2 for x, y in zip(a, b))

def kruskal_kept(points):
    order = sorted(range(len(points)), key=lambda i: morton(points[i]))
    site = {i: r for r, i in enumerate(order)}            # SiteIdx = rang de Morton
    balls = []
    for i, j in combinations(range(len(points)), 2):
        a, b = points[i], points[j]
        c = tuple(Fraction(x + y, 2) for x, y in zip(a, b))
        lam = Fraction(d2(a, b), 4)
        others = [k for k in range(len(points)) if k not in (i, j)]
        assert all(sum((x - y) ** 2 for x, y in zip(points[k], c)) > lam for k in others)  # p = 0, m = 2
        balls.append((lam, tuple(sorted((site[i], site[j]))), (i, j)))
    balls.sort()                                          # BallIdx : (niveau, S*)
    parent = list(range(len(points)))
    def find(x):
        while parent[x] != x:
            x = parent[x]
        return x
    kept = []
    for lam, sstar, (i, j) in balls:
        ri, rj = find(i), find(j)
        if ri != rj:
            parent[ri] = rj
            kept.append(frozenset((i, j)))
    return set(kept), [lam for lam, _, _ in balls]

base = [(0, 1, 1), (1, 0, 1), (1, 1, 0)]
levels = None
results = {}
for t in [(0, 0, 0), (1, 0, 0), (0, 1, 0), (0, 0, 1), (5, 11, 17)]:
    pts = [tuple(c + d for c, d in zip(p, t)) for p in base]
    kept, lv = kruskal_kept(pts)
    results[t] = sorted(tuple(sorted(e)) for e in kept)    # indices d'entree, donc comparables
    print(t, 'niveaux', [str(x) for x in lv], 'aretes gardees (indices d entree)', results[t])
print('equivariant' if len({tuple(v) for v in results.values()}) == 1 else 'NON equivariant par translation')
