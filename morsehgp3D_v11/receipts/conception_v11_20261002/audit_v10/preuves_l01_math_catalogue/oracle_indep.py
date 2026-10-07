#!/usr/bin/env python3
"""Oracle independant (audit L01, 2 oct. 2026) du catalogue critique de morsehgp3D_v10.

Aucun code commun avec reference/hgp10_ref.py ni avec src/catalogue : enumeration brute des 2-, 3- et
4-sous-ensembles de positions, centre circonscrit par systeme de Gram resolu par Cramer en Fraction,
recensement sur toutes les positions avec poids, q_min et S* par minimum sur toutes les presentations,
admission telle que le code l'applique (generator.cpp:262) :
    coquille avec un site de poids > 1 : p_w <= K - 1 ; sinon p_w + q_min <= K + 1.
Le dump attendu est reconstruit ligne par ligne (rang dense, q, p_w, u_w, drapeaux, S*, I, U) dans l'ordre
canonique (niveau exact, S* en indices de Morton completes par 0xFFFFFFFF) et compare TEXTUELLEMENT au dump
du binaire mhgp10_catalogue.

Usage : oracle_indep.py BIN [graine] [nb_par_famille]   (0 conforme, 1 ecart, 3 plancher)
"""
import itertools
import os
import random
import struct
import subprocess
import sys
import tempfile
from fractions import Fraction as Fr

NONE = 0xFFFFFFFF


def morton(p):
    k = 0
    for b in range(21):
        k |= ((p[0] >> b) & 1) << (3 * b)
        k |= ((p[1] >> b) & 1) << (3 * b + 1)
        k |= ((p[2] >> b) & 1) << (3 * b + 2)
    return k


def det3(m):
    return (m[0][0] * (m[1][1] * m[2][2] - m[1][2] * m[2][1])
            - m[0][1] * (m[1][0] * m[2][2] - m[1][2] * m[2][0])
            + m[0][2] * (m[1][0] * m[2][1] - m[1][1] * m[2][0]))


def gram_solve(D):
    """lam tel que sum lam_i D_i soit le centre circonscrit relatif (Cramer) ; None si Gram singuliere."""
    q = len(D)
    G = [[sum(a * b for a, b in zip(D[i], D[j])) for j in range(q)] for i in range(q)]
    rhs = [Fr(G[i][i], 2) for i in range(q)]
    if q == 1:
        return [Fr(1, 2)] if G[0][0] != 0 else None
    if q == 2:
        d = G[0][0] * G[1][1] - G[0][1] * G[1][0]
        if d == 0:
            return None
        return [(rhs[0] * G[1][1] - G[0][1] * rhs[1]) / d, (G[0][0] * rhs[1] - rhs[0] * G[1][0]) / d]
    d = det3(G)
    if d == 0:
        return None
    out = []
    for c in range(3):
        M = [row[:] for row in G]
        for r in range(3):
            M[r][c] = rhs[r]
        out.append(det3(M) / d)
    return out


def ball_of(T, pos):
    """Boule dont T (indices) est un support : (centre relatif a pos[T[0]], r2) ou None."""
    a = pos[T[0]]
    D = [tuple(x - y for x, y in zip(pos[t], a)) for t in T[1:]]
    lam = gram_solve(D)
    if lam is None:
        return None
    if any(l <= 0 for l in lam) or 1 - sum(lam) <= 0:
        return None  # centre hors de l'interieur relatif
    rel = tuple(sum(l * d[j] for l, d in zip(lam, D)) for j in range(3))
    c = tuple(Fr(a[j]) + rel[j] for j in range(3))
    r2 = sum(x * x for x in rel)
    return c, r2


def critical_balls(pos, w):
    """dict (c, r2) -> [qmin, S*, I, U] (indices de sites), par minimum sur toutes les presentations."""
    n = len(pos)
    balls = {}
    for q in (2, 3, 4):
        for T in itertools.combinations(range(n), q):
            b = ball_of(T, pos)
            if b is None:
                continue
            got = balls.get(b)
            if got is not None:
                if (q, T) < (got[0], got[1]):
                    got[0], got[1] = q, T
                continue
            c, r2 = b
            I, U = [], []
            for s in range(n):
                d2 = sum((Fr(pos[s][j]) - c[j]) ** 2 for j in range(3))
                if d2 < r2:
                    I.append(s)
                elif d2 == r2:
                    U.append(s)
            balls[b] = [q, T, I, U]
    return balls


def expected_dump(pos, w, K, rule='code'):
    rows = []
    for (c, r2), (q, S, I, U) in critical_balls(pos, w).items():
        pw = sum(w[s] for s in I)
        uw = sum(w[s] for s in U)
        weighted = any(w[s] > 1 for s in U)
        if rule == 'code':
            admit = (pw <= K - 1) if weighted else (pw + q <= K + 1)
        else:  # regle positionnelle unique de GEN_v2
            admit = pw + q <= K + 1
        if not admit:
            continue
        flags = (1 if len(U) > q else 0) | (2 if weighted else 0)
        key = (r2, tuple(S) + (NONE,) * (4 - q))
        rows.append((key, q, pw, uw, flags, S, I, U, r2))
    rows.sort(key=lambda t: t[0])
    lines = []
    rank = -1
    prev = None
    for key, q, pw, uw, flags, S, I, U, r2 in rows:
        if prev is None or r2 != prev:
            rank += 1
            prev = r2
        pt = lambda s: ' %d,%d,%d' % pos[s]  # noqa: E731
        lines.append('%d %d %d %d %d |%s |%s |%s' % (rank, q, pw, uw, flags, ''.join(pt(s) for s in S),
                                                   ''.join(pt(s) for s in I), ''.join(pt(s) for s in U)))
    return lines


def sites_of(points):
    cnt = {}
    for p in points:
        cnt[p] = cnt.get(p, 0) + 1
    pos = sorted(cnt, key=morton)
    return pos, [cnt[p] for p in pos]


def run_native(exe, points, K, tmp, extra=()):
    src = os.path.join(tmp, 'in.u32le')
    with open(src, 'wb') as f:
        for p in points:
            f.write(struct.pack('<3I', *p))
    dump = os.path.join(tmp, 'dump.txt')
    if os.path.exists(dump):
        os.remove(dump)
    try:
        r = subprocess.run([exe, src, '--k=%d' % K, '--dump=' + dump] + list(extra), capture_output=True, text=True,
                           timeout=float(os.environ.get('L01_DELAI', '40')))
    except subprocess.TimeoutExpired:
        return -9, 'delai', None
    if r.returncode != 0:
        return r.returncode, r.stdout.strip(), None
    with open(dump) as f:
        return 0, r.stdout.strip(), [l.rstrip('\n') for l in f]


# ------------------------------------------------------------------ familles

def sphere_points(R2):
    m = int(R2 ** 0.5) + 1
    return [(x, y, z) for x in range(-m, m + 1) for y in range(-m, m + 1) for z in range(-m, m + 1)
            if x * x + y * y + z * z == R2]


def families(rnd, per):
    def uniq(gen, n):
        pts = set()
        guard = 0
        while len(pts) < n and guard < 100000:
            pts.add(gen())
            guard += 1
        return sorted(pts)

    for t in range(per):
        n = rnd.randint(5, 20)
        yield 'generique_1000', uniq(lambda: tuple(rnd.randint(0, 1000) for _ in range(3)), n)
        yield 'generique_u18', uniq(lambda: tuple(rnd.randint(0, 262143) for _ in range(3)), n)
        yield 'grille_0_3', uniq(lambda: tuple(rnd.randint(0, 3) for _ in range(3)), n)
        yield 'coplanaire', uniq(lambda: (rnd.randint(0, 6), rnd.randint(0, 6), 5), n)
        yield 'colineaire_plus', (uniq(lambda: (rnd.randint(0, 12), 7, 7), min(n, 9))
                                  + uniq(lambda: tuple(rnd.randint(0, 12) for _ in range(3)), 4))
        R2 = rnd.choice((9, 14, 17, 25, 26, 29, 41))
        sp = sphere_points(R2)
        rnd.shuffle(sp)
        off = rnd.choice((7, 1000, 262143 - 7))
        sgn = -1 if off > 1000 else 1
        pts = [(off + sgn * x if sgn > 0 else off - 7 + x, off + sgn * y if sgn > 0 else off - 7 + y,
                off + sgn * z if sgn > 0 else off - 7 + z) for x, y, z in sp[:min(len(sp), rnd.randint(6, 16))]]
        base = (off, off, off) if sgn > 0 else (off - 7, off - 7, off - 7)
        intr = [(base[0] + rnd.randint(-2, 2), base[1] + rnd.randint(-2, 2), base[2] + rnd.randint(-2, 2))
                for _ in range(rnd.randint(0, 4))]
        yield 'sphere_R2_%d' % R2, sorted(set(pts + intr))
        ext = (0, 1, 131071, 131072, 262142, 262143)
        yield 'extremes_u18', uniq(lambda: tuple(rnd.choice(ext) for _ in range(3)), min(n, 14))
        c0 = tuple(rnd.choice((0, 262136)) for _ in range(3))
        c1 = tuple(rnd.choice((0, 262136)) for _ in range(3))
        yield 'amas_coins', (uniq(lambda: tuple(c0[j] + rnd.randint(0, 7) for j in range(3)), n // 2 + 2)
                             + [p for p in uniq(lambda: tuple(c1[j] + rnd.randint(0, 7) for j in range(3)), n // 2 + 2)
                                if c1 != c0])
        s = rnd.randint(1, 5000)
        o = rnd.randint(0, 200000)
        cube = [(o + s * a, o + s * b, o + s * c) for a in (0, 2) for b in (0, 2) for c in (0, 2)]
        extra = [(o + s, o + s, o + s)] + [(o + s, o + s, o), (o + s, o, o + s), (o, o + s, o + s)]
        yield 'cube_centres', cube + extra[:rnd.randint(0, 4)]
        yield 'rectangles', uniq(lambda: (rnd.choice((0, 3, 4, 8)) * 5, rnd.choice((0, 3, 4, 8)) * 7,
                                          rnd.choice((0, 6)) * 3), min(n, 14))
        yield 'cellule_unite', uniq(lambda: (100000 + rnd.randint(0, 2), 100000 + rnd.randint(0, 2),
                                             100000 + rnd.randint(0, 2)), min(n, 18))


def with_weights(rnd, pts):
    out = []
    for p in pts:
        out += [p] * rnd.choice((1, 1, 1, 2, 3))
    rnd.shuffle(out)
    return out


def main():
    exe = sys.argv[1]
    seed = int(sys.argv[2]) if len(sys.argv) > 2 else 20261002
    per = int(sys.argv[3]) if len(sys.argv) > 3 else 6
    rnd = random.Random(seed)
    stats = dict(clouds=0, runs=0, records=0, weighted_records=0, extended_records=0, fails=0, refus=0, delais=0)
    by_family = {}
    with tempfile.TemporaryDirectory(dir=os.environ.get('L01_TMP')) as tmp:
        for name, pts in families(rnd, per):
            if len(pts) < 2:
                continue
            for weighted in (False, True):
                points = with_weights(rnd, pts) if weighted else list(pts)
                rnd.shuffle(points)
                pos, w = sites_of(points)
                stats['clouds'] += 1
                for K in (1, 2, 3, 5, 10, 12):
                    want = expected_dump(pos, w, K)
                    for extra in ((), ('--threads=1', '--leaf=%d' % max(8, K + 4)), ('--threads=3', '--leaf=256')):
                        code, out, got = run_native(exe, points, K, tmp, extra)
                        stats['runs'] += 1
                        if code == -9:
                            stats['delais'] += 1
                            print('DELAI %s w=%s K=%d %s\n  %s' % (name, weighted, K, extra, points), flush=True)
                            continue
                        if code != 0:
                            stats['refus'] += 1
                            print('REFUS %s w=%s K=%d %s : %s\n  %s' % (name, weighted, K, extra, out, points))
                            continue
                        if got != want:
                            stats['fails'] += 1
                            if stats['fails'] <= 10:
                                dif = next((i for i, (a, b) in enumerate(zip(got, want)) if a != b), min(len(got), len(want)))
                                print('ECART %s w=%s K=%d %s : %d lignes contre %d attendues, 1er ecart ligne %d\n  natif  : %s\n  oracle : %s\n  points : %s'
                                      % (name, weighted, K, extra, len(got), len(want), dif,
                                         got[dif] if dif < len(got) else None, want[dif] if dif < len(want) else None, points))
                        else:
                            stats['records'] += len(want)
                            stats['weighted_records'] += sum(1 for l in want if int(l.split('|')[0].split()[4]) & 2)
                            stats['extended_records'] += sum(1 for l in want if int(l.split('|')[0].split()[4]) & 1)
                            by_family[name] = by_family.get(name, 0) + len(want)
    print('oracle_indep_L01 %s' % ' '.join('%s=%d' % kv for kv in stats.items()))
    print('par_famille %s' % ' '.join('%s=%d' % kv for kv in sorted(by_family.items())))
    if stats['fails']:
        return 1
    return 3 if stats['records'] < 5000 or stats['weighted_records'] < 500 or stats['extended_records'] < 500 else 0


if __name__ == '__main__':
    sys.exit(main())
