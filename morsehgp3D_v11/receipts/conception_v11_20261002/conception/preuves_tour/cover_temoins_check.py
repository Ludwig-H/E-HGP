# Controle de conception (leger) de la proposition T8 : l'entree cover (niveau alpha_k(x)^2 et ENSEMBLE des noeuds
# couvrants) se lit sur les seuls temoins p + q_min <= k <= p + m ; pour une coquille reguliere le temoin est une
# naissance de l'ordre k et son noeud est son noeud de naissance (aucune descente).
# Verite : etage A de la reference v11 (definition par Gamma_k) ; catalogue : etage B (boules critiques brutes).
# Usage : python3 -B cover_temoins_check.py <chemin de morsehgp3D_v11/reference> [graine] [nuages]
import random, sys
sys.path.insert(0, sys.argv[1])
from hgp11_ref import Definition, Reference

def clouds(rng, count):
    out = []
    for i in range(count):
        kind = i % 4
        n = rng.randint(6, 9)
        if kind == 0:
            pts = set()
            while len(pts) < n:
                pts.add((rng.randint(0, 40), rng.randint(0, 40), rng.randint(0, 40)))
        elif kind == 1:   # grille serree : coquilles etendues, egalites
            pts = set()
            while len(pts) < n:
                pts.add((rng.randint(0, 2), rng.randint(0, 2), rng.randint(0, 2)))
        elif kind == 2:   # plan
            pts = set()
            while len(pts) < n:
                pts.add((rng.randint(0, 4), rng.randint(0, 4), 0))
        else:             # droite et quasi-droite : egalites de premiere couverture
            pts = set()
            while len(pts) < n:
                pts.add((2 * rng.randint(0, 9), rng.choice((0, 0, 0, 1)), 0))
        out.append(sorted(pts))
    return out

def main():
    rng = random.Random(int(sys.argv[2]) if len(sys.argv) > 2 else 3)
    count = int(sys.argv[3]) if len(sys.argv) > 3 else 60
    K = 4
    entries = ties = fails = regular_w = ext_w = not_birth = 0
    for pts in clouds(rng, count):
        truth = Definition(pts)
        ref = Reference(pts, K)
        for k in range(2, min(K, len(pts)) + 1):
            res = truth.order(k)
            births = {(nd.level, nd.center): v for v, nd in enumerate(res.nodes) if nd.center is not None}
            best = [None] * len(pts)
            for b in ref.balls:
                if b.qmin < 2 or not (b.p + b.qmin <= k <= b.p + b.m):
                    continue
                pop = sorted(b.inner + b.shell)
                if b.regular:
                    regular_w += 1
                    v = births.get((b.level, b.center))
                    if v is None:        # un temoin regulier doit etre une naissance de l'ordre k
                        not_birth += 1
                        continue
                else:
                    ext_w += 1
                    v = truth.node_at(k, [ref.inp[y] for y in pop[:k]], b.level)
                for y in pop:
                    i = ref.inp[y]
                    if best[i] is None or b.level < best[i][0]:
                        best[i] = (b.level, {v})
                    elif b.level == best[i][0]:
                        best[i][1].add(v)
            for i, e in enumerate(res.cover):
                entries += 1
                ties += len(e.nodes) >= 2
                if best[i] is None or best[i][0] != e.level or frozenset(best[i][1]) != e.nodes:
                    fails += 1
                    if fails <= 3:
                        print('ECART', pts, 'k', k, 'point', i, best[i], e)
    print('cover_temoins_checks entries', entries, 'ties', ties, 'fails', fails, 'regular_witnesses', regular_w,
          'extended_witnesses', ext_w, 'regular_not_birth', not_birth)
    return 1 if fails or not_birth else 0

if __name__ == '__main__':
    sys.exit(main())
