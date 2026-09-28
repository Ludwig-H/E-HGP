"""Porte T2 du generateur : catalogue C++ == oracle brut exact (reference/hgp10_ref.py), petits nuages.

Pour chaque nuage (generiques et grilles degenerees : cospheriques, coplanaires, alignes) et chaque K :
  - meme multiensemble de boules (q_min, p, I, U) (une boule critique est determinee par (I, U)) ;
  - S* est un support valide de cardinal q_min inclus dans U ;
  - rangs coherents avec les niveaux exacts (egalite de rang <=> egalite de niveau, ordre croissant).
Usage : python3 test_catalogue_oracle.py BUILD_DIR [nuages]   (0 conforme, 1 desaccord, 3 plancher)
"""
import os
import random
import subprocess
import sys
import tempfile
from fractions import Fraction

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, '..', '..', 'reference'))
import hgp10_ref as R  # noqa: E402


def parse_dump(path):
    balls = []
    for line in open(path):
        head, sup, inner, shell = line.rstrip('\n').split('|')
        rank, q, p, u, flags = (int(t) for t in head.split())
        conv = lambda s: [tuple(int(v) for v in t.split(',')) for t in s.split()]  # noqa: E731
        balls.append(dict(rank=rank, q=q, p=p, u=u, flags=flags, sup=conv(sup), I=conv(inner), U=conv(shell)))
    return balls


def is_support(S, center, radius2):
    if not R.affinely_independent(S):
        return False
    cc = R.circumcenter(S)
    if cc is None:
        return False
    c, lam = cc
    return c == center and all(l > 0 for l in lam) and R.d2(c, S[0]) == radius2


def check(exe, P, K, tmp):
    src = os.path.join(tmp, 'in.u32le')
    with open(src, 'wb') as f:
        for p in P:
            for v in p:
                f.write(int(v).to_bytes(4, 'little'))
    dump = os.path.join(tmp, 'dump.txt')
    r = subprocess.run([exe, src, '--k=%d' % K, '--threads=2', '--dump=' + dump], capture_output=True, text=True)
    if r.returncode != 0:
        return 'refus %s' % r.stdout.strip()
    got = parse_dump(dump)
    want = [b for b in R.catalogue(P, K) if b.qmin >= 2]  # rayon nul : table des sites, pas le catalogue
    idx = {p: i for i, p in enumerate(P)}
    key = lambda q, p, I, U: (q, p, frozenset(I), frozenset(U))  # noqa: E731
    W = {}
    for b in want:
        W[key(b.qmin, b.p, [P[i] for i in b.I], [P[i] for i in b.U])] = b
    G = {}
    for b in got:
        k = key(b['q'], b['p'], b['I'], b['U'])
        if k in G:
            return 'boule en double %s' % (k,)
        G[k] = b
    if set(W) != set(G):
        miss = [k for k in W if k not in G][:3]
        extra = [k for k in G if k not in W][:3]
        return 'manquantes %d en trop %d ex %s %s' % (len(set(W) - set(G)), len(set(G) - set(W)), miss, extra)
    prev = None
    for b in sorted(got, key=lambda t: t['rank']):
        ref = W[key(b['q'], b['p'], b['I'], b['U'])]
        if len(b['sup']) != b['q'] or not set(b['sup']) <= set(b['U']):
            return 'support hors coquille'
        if not is_support(b['sup'], ref.center, ref.level):
            return 'support invalide'
        if b['u'] != len(b['U']):
            return 'poids de coquille'
        if prev is not None:
            if (prev[0] == b['rank']) != (prev[1] == ref.level) or prev[1] > ref.level:
                return 'rangs incoherents'
        prev = (b['rank'], ref.level)
    return None


def clouds(count, rnd):
    for t in range(count):
        n = rnd.randint(5, 22)
        kind = t % 4
        pts = set()
        while len(pts) < n:
            if kind == 0:
                pts.add(tuple(rnd.randint(0, 1000) for _ in range(3)))
            elif kind == 1:
                pts.add(tuple(rnd.randint(0, 3) for _ in range(3)))
            elif kind == 2:
                pts.add((rnd.randint(0, 6), rnd.randint(0, 6), 0))  # coplanaire
            else:
                pts.add(tuple(rnd.choice((0, 2, 4)) + rnd.randint(0, 1) for _ in range(3)))
        P = sorted(pts)
        rnd.shuffle(P)
        yield P


def main():
    exe = os.path.join(sys.argv[1], 'mhgp10_catalogue')
    count = int(sys.argv[2]) if len(sys.argv) > 2 else 40
    rnd = random.Random(20260928)
    checks = fails = balls = 0
    with tempfile.TemporaryDirectory() as tmp:
        for P in clouds(count, rnd):
            for K in (1, 2, 3, 5):
                err = check(exe, P, K, tmp)
                checks += 1
                if err:
                    fails += 1
                    if fails <= 5:
                        print('ECART K=%d n=%d : %s\n  %s' % (K, len(P), err, P))
                else:
                    balls += sum(1 for b in R.catalogue(P, K) if b.qmin >= 2)
    print('catalogue_oracle_checks %d fails %d balls %d' % (checks, fails, balls))
    if fails:
        return 1
    return 3 if checks < 4 * count or balls < 1000 else 0


if __name__ == '__main__':
    sys.exit(main())
