# S* d'une boule q2 = premier diametre en ordre lexicographique des SiteIdx (rangs de Morton).
# Cercle de l'audit carrier (n = 3, d = 10) : avant perturbation Q_b = {AC, BD} ; apres, Q_b = {AC, BtCD}.
def spread(v):
    return sum(((v >> i) & 1) << (3 * i) for i in range(24))
def morton(p):
    return spread(p[0]) | (spread(p[1]) << 1) | (spread(p[2]) << 2)
def sstar(points, diameters):
    order = sorted(range(len(points)), key=lambda i: morton(points[i]))
    rank = {i: r for r, i in enumerate(order)}
    return min(diameters, key=lambda e: tuple(sorted(rank[i] for i in e)))
import sys; n = int(sys.argv[1]); d = n * n + 1
A, B, C, D = (2 * d, d, 0), (d, 2 * d, 0), (0, d, 0), (d, 0, 0)
names = 'ABCD'
for t in [(0, 0, 0), (1, 0, 0), (0, 3, 0), (7, 1, 0), (5, 11, 17), (16, 0, 0), (0, 16, 0)]:
    pts = [tuple(c + s for c, s in zip(p, t)) for p in (A, B, C, D)]
    e = sstar(pts, [(0, 2), (1, 3)])
    print(t, 'S* avant perturbation =', ''.join(names[i] for i in e), '; apres : AC (seul q2)')
