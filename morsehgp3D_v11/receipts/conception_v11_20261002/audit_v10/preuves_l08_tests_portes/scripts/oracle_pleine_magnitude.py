"""Strate absente des portes HEAD : les deux oracles exacts (catalogue brut, Gamma_k) joues sur des nuages a
coordonnees de PLEINE magnitude u18 (les portes n'utilisent que des coordonnees <= 1000).
Reutilise tels quels check() de tests/oracle/test_catalogue_oracle.py et de tests/oracle/test_tower_oracle.py.
Usage : python3 oracle_pleine_magnitude.py BUILD SRC_V10 [graine]
"""
import itertools
import json
import os
import random
import sys
import tempfile
from collections import Counter

build, src = sys.argv[1], sys.argv[2]
seed = int(sys.argv[3]) if len(sys.argv) > 3 else 20261002
sys.path.insert(0, os.path.join(src, 'reference'))
sys.path.insert(0, os.path.join(src, 'tests', 'oracle'))
import hgp10_ref as R  # noqa: E402
import test_catalogue_oracle as C  # noqa: E402
import test_tower_oracle as T  # noqa: E402

TOP = (1 << 18) - 1


def sphere_points(rnd, count):
    """Points entiers EXACTEMENT cospheriques de grande magnitude : (+-a, +-b, +-c) et permutations, translates."""
    while True:
        a, b, c = sorted(rnd.sample(range(20000, 120000), 3))
        pts = set()
        for perm in itertools.permutations((a, b, c)):
            for s in itertools.product((1, -1), repeat=3):
                pts.add(tuple(131072 + s[i] * perm[i] for i in range(3)))
        pts = [p for p in pts if all(0 <= v <= TOP for v in p)]
        if len(pts) >= count:
            return rnd.sample(pts, count)


def families(rnd):
    # (a) uniforme sur tout le domaine u18
    for _ in range(10):
        n = rnd.randint(6, 10)
        yield 'uniforme_u18', list({tuple(rnd.randint(0, TOP) for _ in range(3)) for _ in range(n)})
    # (b) coin superieur : grandes coordonnees absolues, petites differences
    for _ in range(8):
        n = rnd.randint(6, 10)
        yield 'coin_superieur', list({tuple(TOP - rnd.randint(0, 40) for _ in range(3)) for _ in range(n)})
    # (c) deux amas eloignes de toute la diagonale, structure fine dans chacun
    for _ in range(8):
        n = rnd.randint(6, 10)
        pts = set()
        for i in range(n):
            base = 0 if i % 2 == 0 else TOP - 60
            pts.add(tuple(base + rnd.randint(0, 60) for _ in range(3)))
        yield 'deux_amas_diagonale', list(pts)
    # (d) coquilles etendues exactes a grande magnitude : 5 a 7 points cospheriques + points interieurs/exterieurs
    for _ in range(8):
        shell = sphere_points(rnd, rnd.randint(5, 7))
        extra = {tuple(131072 + rnd.randint(-15000, 15000) for _ in range(3)) for _ in range(rnd.randint(1, 3))}
        yield 'cospherique_grand_rayon', list(set(shell) | extra)
    # (e) presque cospherique : un point de la coquille deplace d'une unite de grille
    for _ in range(8):
        shell = sphere_points(rnd, 6)
        i = rnd.randrange(6)
        moved = list(shell[i])
        moved[rnd.randrange(3)] += rnd.choice((-1, 1))
        shell[i] = tuple(min(TOP, max(0, v)) for v in moved)
        extra = {tuple(131072 + rnd.randint(-15000, 15000) for _ in range(2)) + (131072,) for _ in range(2)}
        yield 'presque_cospherique', list(set(shell) | extra)
    # (f) coplanaire et aligne a grande magnitude (triplets alignes, q3 degeneres)
    for _ in range(6):
        d = (rnd.randint(1, 9000), rnd.randint(1, 9000), rnd.randint(1, 9000))
        o = tuple(rnd.randint(0, 20000) for _ in range(3))
        line = [tuple(o[j] + t * d[j] for j in range(3)) for t in rnd.sample(range(0, 26), 5)]
        extra = {tuple(rnd.randint(0, TOP) for _ in range(3)) for _ in range(3)}
        yield 'aligne_grand_pas', list(set(p for p in line if max(p) <= TOP) | extra)


def main():
    rnd = random.Random(seed)
    stats = Counter()
    fails = []
    refus = []
    with tempfile.TemporaryDirectory() as tmp:
        exe_c = os.path.join(build, 'mhgp10_catalogue')
        exe_t = os.path.join(build, 'mhgp10_tower')
        for fam, P in families(rnd):
            P = sorted(set(P))
            rnd.shuffle(P)
            if len(P) < 5:
                continue
            for K in (1, 2, 3, 5):
                err = C.check(exe_c, P, K, tmp)
                stats[fam + ':catalogue'] += 1
                if err and err.startswith('refus'):
                    stats[fam + ':catalogue_refus'] += 1
                    refus.append((fam, 'catalogue', K, err, P))
                elif err:
                    fails.append((fam, 'catalogue', K, err, P))
            for K in (1, 3, 5):
                err, cuts = T.check(exe_t, P, K, tmp)
                stats[fam + ':tour'] += 1
                stats[fam + ':coupes'] += cuts
                if err and err.startswith('code 2'):
                    stats[fam + ':tour_refus'] += 1
                    refus.append((fam, 'tour', K, err, P))
                elif err:
                    fails.append((fam, 'tour', K, err, P))
            print(fam, len(P), 'ok' if not fails else 'ECARTS %d' % len(fails), flush=True)
    for f in fails[:10]:
        print('ECART', f)
    for f in refus[:10]:
        print('REFUS', f)
    print(json.dumps({'graine': seed, 'controles': dict(stats), 'ecarts': len(fails), 'refus': len(refus)}, indent=1, sort_keys=True))
    return 1 if fails else 0


sys.exit(main())
