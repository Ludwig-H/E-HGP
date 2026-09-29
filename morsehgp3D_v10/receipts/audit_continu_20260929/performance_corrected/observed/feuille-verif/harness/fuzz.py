#!/usr/bin/env python3
"""Fuzzer differentiel adverse J3 : base a10605a06 contre base + patch J3, empreinte complete (niveaux compris).

usage : fuzz.py GRAINE NCAS SORTIE_TSV
Familles : grilles (pas 1 a 30000, decalages aux bords du cube u18), spheres entieres, plans, droites, doublons
ponderes, etendues autour du seuil de la voie etroite (2^12 en coordonnees entieres), coins du cube, amas eloignes.
Nuages ecrits dans le dossier temporaire (u32le x y z). Chaque cas : K dans une liste, M dans une liste.
"""
import os
import random
import subprocess
import sys
import tempfile

LIM = (1 << 18) - 1
H = os.path.dirname(os.path.abspath(__file__))
BASE = os.path.join(H, 'cathash_base')
J3 = os.environ.get('J3_BIN', os.path.join(H, 'cathash_j3'))


def clamp(v):
    return max(0, min(LIM, v))


def shift_into(pts, rnd):
    """Translate le nuage : origine, bord haut du cube ou decalage aleatoire."""
    mx = [max(p[a] for p in pts) for a in range(3)]
    mn = [min(p[a] for p in pts) for a in range(3)]
    ext = [mx[a] - mn[a] for a in range(3)]
    if max(ext) > LIM:
        return [tuple(clamp(c) for c in p) for p in pts]
    mode = rnd.randrange(3)
    off = []
    for a in range(3):
        room = LIM - ext[a]
        o = 0 if mode == 0 else (room if mode == 1 else rnd.randint(0, room))
        off.append(o - mn[a])
    return [(p[0] + off[0], p[1] + off[1], p[2] + off[2]) for p in pts]


def fam_grid(rnd):
    g = rnd.randint(2, 7)
    s = rnd.choice([1, 1, 2, 3, 5, 7, 64, 1000, 4095, 4096, 4097, 8191, 30000])
    sx, sy, sz = s, rnd.choice([s, s, s * 2, s + 1]), rnd.choice([s, s, s * 3, max(1, s - 1)])
    p = rnd.choice([1.0, 0.8, 0.5])
    pts = [(x * sx, y * sy, z * sz) for x in range(g) for y in range(g) for z in range(g) if rnd.random() < p]
    return pts


def sphere_points(r2):
    r = int(r2 ** 0.5) + 1
    return [(x, y, z) for x in range(-r, r + 1) for y in range(-r, r + 1) for z in range(-r, r + 1)
            if x * x + y * y + z * z == r2]


def fam_sphere(rnd):
    r2 = rnd.choice([2, 3, 5, 6, 9, 11, 14, 17, 25, 27, 50, 81, 101, 125])
    pts = sphere_points(r2)
    s = rnd.choice([1, 1, 2, 64, 1500])
    pts = [(x * s, y * s, z * s) for (x, y, z) in pts if rnd.random() < rnd.choice([1.0, 0.7])]
    if rnd.random() < 0.5:  # centre et quelques sites interieurs / exterieurs
        R = int(r2 ** 0.5) * s + 1
        pts.append((0, 0, 0))
        for _ in range(rnd.randint(0, 6)):
            pts.append(tuple(rnd.randint(-2 * R, 2 * R) for _ in range(3)))
    if rnd.random() < 0.3:  # deux spheres concentriques
        pts += [(2 * x, 2 * y, 2 * z) for (x, y, z) in pts[: len(pts) // 2]]
    return pts


def fam_plane(rnd):
    n = rnd.randint(8, 120)
    L = rnd.choice([4, 10, 50, 5000, 100000])
    ax = rnd.randrange(3)
    pts = []
    for _ in range(n):
        c = [rnd.randint(0, L), rnd.randint(0, L), rnd.randint(0, L)]
        c[ax] = 0
        pts.append(tuple(c))
    if rnd.random() < 0.4:  # quelques sites hors du plan
        for _ in range(rnd.randint(1, 5)):
            pts.append((rnd.randint(0, L), rnd.randint(0, L), rnd.randint(1, L)))
    return pts


def fam_line(rnd):
    n = rnd.randint(4, 40)
    d = (rnd.randint(-3, 3), rnd.randint(-3, 3), rnd.randint(1, 3))
    s = rnd.choice([1, 7, 900])
    pts = [(t * d[0] * s, t * d[1] * s, t * d[2] * s) for t in range(n)]
    for _ in range(rnd.randint(0, 6)):
        pts.append(tuple(rnd.randint(-50 * s, 50 * s) for _ in range(3)))
    return pts


def fam_small_random(rnd):
    n = rnd.randint(10, 400)
    L = rnd.choice([3, 5, 8, 12, 20, 40])
    return [tuple(rnd.randint(0, L) for _ in range(3)) for _ in range(n)]


def fam_extent(rnd):
    # etendue locale autour du seuil 2^18 en repere T (2^12 = 4096 en coordonnees entieres)
    n = rnd.randint(10, 300)
    L = rnd.choice([3000, 4000, 4095, 4096, 4097, 4200, 6000, 8192, 12000, 20000])
    pts = [tuple(rnd.randint(0, L) for _ in range(3)) for _ in range(n)]
    if rnd.random() < 0.5:  # amas dense + sites epars a ~4096
        c = rnd.randint(0, L)
        pts += [(c + rnd.randint(0, 6), c + rnd.randint(0, 6), c + rnd.randint(0, 6)) for _ in range(rnd.randint(5, 60))]
    return pts


def fam_wide(rnd):
    # voie large : coins du cube u18 et sites epars sur tout le cube
    pts = []
    for x in (0, LIM):
        for y in (0, LIM):
            for z in (0, LIM):
                if rnd.random() < 0.8:
                    pts.append((x, y, z))
    for _ in range(rnd.randint(3, 60)):
        pts.append(tuple(rnd.randint(0, LIM) for _ in range(3)))
    for _ in range(rnd.randint(0, 20)):  # pres des coins
        c = rnd.choice(pts)
        pts.append(tuple(clamp(c[a] + rnd.randint(-3, 3)) for a in range(3)))
    return pts


def fam_clusters(rnd):
    k = rnd.randint(2, 5)
    pts = []
    for _ in range(k):
        c = tuple(rnd.randint(0, LIM) for _ in range(3))
        r = rnd.choice([2, 5, 30, 400, 4096])
        for _ in range(rnd.randint(3, 60)):
            pts.append(tuple(clamp(c[a] + rnd.randint(-r, r)) for a in range(3)))
    return pts


FAMILIES = [fam_grid, fam_sphere, fam_plane, fam_line, fam_small_random, fam_extent, fam_wide, fam_clusters]


def make_cloud(rnd):
    fam = rnd.choice(FAMILIES)
    pts = fam(rnd)
    if len(pts) < 2:
        pts += [(0, 0, 0), (1, 2, 3)]
    pts = shift_into(pts, rnd)
    pts = [tuple(clamp(c) for c in p) for p in pts]
    if rnd.random() < 0.3:  # doublons : sites ponderes
        for _ in range(rnd.randint(1, max(1, len(pts) // 4))):
            p = rnd.choice(pts)
            pts += [p] * rnd.randint(1, 12)
    rnd.shuffle(pts)
    return fam.__name__, pts


def run(binary, path, W, Ks, Ms):
    r = subprocess.run([binary, path, str(W), Ks, Ms], capture_output=True, text=True, timeout=60)
    return r.returncode, r.stdout.strip(), r.stderr.strip()


def main():
    seed, ncas, out = int(sys.argv[1]), int(sys.argv[2]), sys.argv[3]
    rnd = random.Random(seed)
    tmp = tempfile.mkdtemp(prefix='j3fuzz_', dir='/tmp')
    path = os.path.join(tmp, 'c.u32le')
    bad = 0
    with open(out, 'a') as f:
        for cas in range(ncas):
            fam, pts = make_cloud(rnd)
            with open(path, 'wb') as g:
                for p in pts:
                    g.write(int(p[0]).to_bytes(4, 'little') + int(p[1]).to_bytes(4, 'little') + int(p[2]).to_bytes(4, 'little'))
            W = rnd.choice([1, 1, 3])
            same = True
            timeout = False
            outs = []
            for K in sorted(rnd.sample([1, 2, 3, 4, 5, 6, 7, 8, 10, 12], 3)):
                Mc = [0, 40, 63, 64]
                Ms = ','.join(str(m) for m in rnd.sample(Mc, 2))
                try:
                    rb = run(BASE, path, 1, str(K), Ms)
                    rj = run(J3, path, W, str(K), Ms)
                except subprocess.TimeoutExpired:
                    timeout = True
                    continue
                outs.append(rb)
                same = same and rb[0] == rj[0] and rb[1] == rj[1] and rb[0] == 0 and not rj[2]
            Ks = 'timeout' if timeout else 'ok'
            Ms = ''
            rb = (0, '\n'.join(o[1] for o in outs), '')
            if not same:
                bad += 1
                keep = os.path.join(os.path.dirname(out), 'contre_exemple_%d_%d.u32le' % (seed, cas))
                with open(path, 'rb') as g, open(keep, 'wb') as h:
                    h.write(g.read())
            nref = sum(1 for l in rb[1].splitlines() if ' refus ' in l)
            f.write('%d\t%d\t%s\t%d\t%s\t%s\t%d\t%s\t%d\n' % (seed, cas, fam, len(pts), Ks, Ms, W,
                                                            'IDENTIQUE' if same else 'DIFFERENT', nref))
            f.flush()
    print('graine', seed, 'cas', ncas, 'differents', bad)


if __name__ == '__main__':
    main()
