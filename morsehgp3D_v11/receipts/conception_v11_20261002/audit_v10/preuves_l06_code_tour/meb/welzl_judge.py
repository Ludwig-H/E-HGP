"""Audit L06 : MEB de tower.cpp (repli Welzl exact et chemin produit) contre la force brute exacte de la reference
(Fraction), sur des ensembles degeneres : grille {0..3}^3, points cospheriques de reseau, cocycliques, alignes,
generiques. Compare les niveaux (rayons carres) exacts.
Usage : python3 welzl_judge.py [nombre d'ensembles] [graine]
"""
import itertools
import os
import random
import subprocess
import sys
from fractions import Fraction

sys.path.insert(0, '/tmp/v11-audit/l06_code_tour/instr/reference')
import hgp10_ref as R  # noqa: E402

N = int(sys.argv[1]) if len(sys.argv) > 1 else 2000
rnd = random.Random(int(sys.argv[2]) if len(sys.argv) > 2 else 20261002)
sphere24 = sorted({tuple(10 + sg[i] * perm[i] for i in range(3)) for perm in itertools.permutations((2, 1, 0))
                   for sg in itertools.product((1, -1), repeat=3)})
circle = [(30 + x, 30 + y, 5) for x in range(-18, 19) for y in range(-18, 19) if x * x + y * y == 325]
grid = [(x, y, z) for x in range(4) for y in range(4) for z in range(4)]
plane = [(x, y, 0) for x in range(7) for y in range(7)]
line = [(3 * i, 2 * i, i) for i in range(12)]
sets = []
kinds = {}
for t in range(N):
    kind = ('grille', 'sphere24', 'cercle325', 'plan', 'ligne', 'generique', 'sphere24+interieur')[t % 7]
    n = rnd.randint(2, 10)
    if kind == 'grille':
        P = rnd.sample(grid, n)
    elif kind == 'sphere24':
        P = rnd.sample(sphere24, n)
    elif kind == 'cercle325':
        P = rnd.sample(circle, n)
    elif kind == 'plan':
        P = rnd.sample(plane, n)
    elif kind == 'ligne':
        P = rnd.sample(line, n)
    elif kind == 'generique':
        P = list({tuple(rnd.randint(0, 262143) for _ in range(3)) for _ in range(n)})
    else:
        P = rnd.sample(sphere24, max(2, n - 2)) + [(10, 10, 10), (11, 10, 10)][:min(2, n)]
        P = list(dict.fromkeys(P))
    if len(P) < 2:
        continue
    sets.append((kind, P))
inp = '\n'.join('%d %s' % (len(P), ' '.join('%d %d %d' % p for p in P)) for _, P in sets) + '\n'
out = subprocess.run([os.path.join(os.path.dirname(os.path.abspath(__file__)), 'welzl_harness')], input=inp,
                     capture_output=True, text=True)
assert out.returncode == 0, out.stderr[:300]
lines = out.stdout.strip().split('\n')
assert len(lines) == len(sets), (len(lines), len(sets))
bad_w = bad_m = fallbacks = 0
for (kind, P), l in zip(sets, lines):
    w, m, fb = l.split()
    fw = Fraction(int(w.split('/')[0]), int(w.split('/')[1]))
    fm = Fraction(int(m.split('/')[0]), int(m.split('/')[1]))
    want = R.meb(P, tuple(range(len(P))))[0]
    k = kinds.setdefault(kind, [0, 0, 0, 0])
    k[0] += 1
    k[3] += int(fb)
    fallbacks += int(fb)
    if fw != want:
        bad_w += 1
        k[1] += 1
        if bad_w <= 5:
            print('ECART repli Welzl exact', kind, P, 'rend', fw, 'attendu', want)
    if fm != want:
        bad_m += 1
        k[2] += 1
        if bad_m <= 5:
            print('ECART chemin produit meb()', kind, P, 'rend', fm, 'attendu', want)
print('ensembles %d ; ecarts du repli Welzl exact %d ; ecarts du chemin produit meb() %d ; replis du chemin produit %d'
      % (len(sets), bad_w, bad_m, fallbacks))
for kind, (n, bw, bm, fb) in sorted(kinds.items()):
    print('  %-20s ensembles %4d  ecarts repli %3d  ecarts produit %3d  replis %4d' % (kind, n, bw, bm, fb))
sys.exit(1 if (bad_w or bad_m) else 0)
