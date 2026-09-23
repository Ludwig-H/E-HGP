#!/usr/bin/env python3
import random, sys, time
from oracle_cmp import judge, M

def uniq(pts):
    return sorted(set(pts))

def fam_grid(rng):
    n = rng.randint(6, 15); sx, sy, sz = rng.randint(1, 4), rng.randint(1, 4), rng.randint(0, 3)
    pts = set()
    while len(pts) < min(n, (sx + 1) * (sy + 1) * (sz + 1)):
        pts.add((rng.randint(0, sx), rng.randint(0, sy), rng.randint(0, sz)))
    return sorted(pts)

def scale(pts, rng):
    mx = max(max(p) for p in pts) or 1
    f = rng.randint(1, M // mx)
    off = [rng.randint(0, M - f * max(p[t] for p in pts)) for t in range(3)]
    return [tuple(off[t] + f * p[t] for t in range(3)) for p in pts]

def fam_grid_u18(rng):
    return scale(fam_grid(rng), rng)

def fam_scanlines(rng):
    pts = set(); nl = rng.randint(2, 4)
    for _ in range(nl):
        o = [rng.randint(0, 40) for _ in range(3)]
        d = [rng.randint(-3, 3) for _ in range(3)]
        if d == [0, 0, 0]: d = [1, 0, 0]
        for j in range(rng.randint(3, 5)):
            p = tuple(o[t] + j * d[t] for t in range(3))
            if min(p) >= 0: pts.add(p)
    return sorted(pts)[:16]

def fam_planar(rng):
    n = rng.randint(5, 14); pts = set(); s = rng.choice([3, 5, 30])
    while len(pts) < n: pts.add((rng.randint(0, s), rng.randint(0, s), 7))
    return sorted(pts)

def fam_collinear(rng):
    n = rng.randint(3, 12); xs = set()
    while len(xs) < n: xs.add(rng.randint(0, 30))
    d = (rng.randint(1, 3), rng.randint(0, 3), rng.randint(0, 3))
    return sorted((x * d[0], x * d[1], x * d[2]) for x in xs)

def fam_cluster_far(rng):
    pts = set()
    while len(pts) < rng.randint(6, 10): pts.add(tuple(rng.randint(100, 104) for _ in range(3)))
    for _ in range(rng.randint(2, 5)): pts.add(tuple(rng.choice([0, M, rng.randint(0, M)]) for _ in range(3)))
    return sorted(pts)

def fam_sphere(rng):
    R2 = rng.choice([9, 11, 17, 18, 19, 26, 27, 29])
    R = int(R2 ** 0.5) + 1
    sph = [(x, y, z) for x in range(-R, R + 1) for y in range(-R, R + 1) for z in range(-R, R + 1) if x*x+y*y+z*z == R2]
    rng.shuffle(sph); k = rng.randint(4, min(13, len(sph)))
    pts = [tuple(v + 50 for v in p) for p in sph[:k]]
    for _ in range(rng.randint(0, 3)): pts.append(tuple(50 + rng.randint(-R, R) for _ in range(3)))
    return uniq(pts)

def fam_two_planes(rng):
    pts = set()
    for zz in (0, rng.randint(1, 4)):
        for _ in range(rng.randint(3, 7)): pts.add((rng.randint(0, 4), rng.randint(0, 4), zz))
    return sorted(pts)

FAMS = {"grid": fam_grid, "grid_u18": fam_grid_u18, "scanlines": fam_scanlines, "planar": fam_planar,
        "collinear": fam_collinear, "cluster_far": fam_cluster_far, "sphere": fam_sphere, "two_planes": fam_two_planes}

def main():
    seed = int(sys.argv[1]); trials = int(sys.argv[2]); fams = sys.argv[3].split(",") if len(sys.argv) > 3 else list(FAMS)
    rng = random.Random(seed); stats = {}; t0 = time.time(); bad = 0
    for t in range(trials):
        fam = fams[t % len(fams)]
        pts = FAMS[fam](rng)
        if len(pts) < 2: continue
        kmax = rng.choice([2, 3, 5, 10])
        w = rng.choice([1, 2])
        verdict, nexp = judge(pts, kmax, w, 1)
        s = stats.setdefault(fam, {"trials": 0, "balls": 0, "refused_ok": 0, "bad": 0})
        s["trials"] += 1; s["balls"] += nexp
        if verdict == "refused_ok": s["refused_ok"] += 1
        elif verdict != "ok":
            s["bad"] += 1; bad += 1
            print("BAD", fam, "kmax", kmax, "w", w, verdict, "pts", pts, flush=True)
    print({"seed": seed, "elapsed_s": round(time.time() - t0, 1), "stats": stats, "bad": bad})

main()
