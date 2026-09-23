#!/usr/bin/env python3
import random, subprocess, sys, time
M = 262143

def walls(rng):
    pts = set(); X0 = rng.randint(20, 60)
    for zr in range(rng.randint(3, 6)):
        z = 10 + zr * rng.randint(2, 4); y = rng.randint(0, 5)
        while y < 60:
            pts.add((X0 + rng.choice([0, 0, 0, 1]), y, z)); y += rng.randint(2, 6)
    Y0 = rng.randint(70, 90)
    for zr in range(rng.randint(2, 5)):
        z = 10 + zr * 3; x = rng.randint(0, 5)
        while x < 50:
            pts.add((x, Y0, z)); x += rng.randint(2, 7)
    for _ in range(rng.randint(0, 10)): pts.add((rng.randint(0, 90), rng.randint(0, 90), rng.randint(0, 40)))
    return sorted(pts)

def uniform(rng):
    n = rng.randint(30, 110); s = rng.choice([12, 25, 60, 200]); pts = set()
    while len(pts) < min(n, (s + 1) ** 3 // 2): pts.add(tuple(rng.randint(0, s) for _ in range(3)))
    return sorted(pts)

def sparse_clusters(rng):
    pts = set()
    for _ in range(rng.randint(8, 25)):
        c = [rng.randint(3, 197) for _ in range(3)]
        for _ in range(rng.randint(1, 4)): pts.add(tuple(c[t] + rng.randint(-2, 2) for t in range(3)))
    return sorted(pts)

def blob_far(rng):
    pts = set()
    while len(pts) < rng.randint(25, 60): pts.add(tuple(90 + rng.randint(0, 10) for _ in range(3)))
    for _ in range(rng.randint(5, 15)): pts.add(tuple(rng.randint(0, 200) for _ in range(3)))
    return sorted(pts)

def lattice(rng):
    a, b, c = rng.randint(3, 7), rng.randint(3, 7), rng.randint(1, 3); pr = rng.uniform(0.3, 0.7)
    step = rng.choice([1, 2, 3])
    return sorted((x * step, y * step, z * step) for x in range(a) for y in range(b) for z in range(c) if rng.random() < pr)

def lines(rng):
    pts = set()
    for _ in range(rng.randint(3, 8)):
        o = [rng.randint(20, 180) for _ in range(3)]; d = [rng.randint(-4, 4) for _ in range(3)]
        if d == [0, 0, 0]: d = [0, 0, 1]
        for j in range(rng.randint(4, 12)):
            p = tuple(o[t] + j * d[t] for t in range(3))
            if all(0 <= v <= 200 for v in p): pts.add(p)
    return sorted(pts)

FAMS = {"walls": walls, "uniform": uniform, "sparse_clusters": sparse_clusters, "blob_far": blob_far,
        "lattice": lattice, "lines": lines}

def main():
    seed, trials = int(sys.argv[1]), int(sys.argv[2]); fams = sys.argv[3].split(",") if len(sys.argv) > 3 else list(FAMS)
    rng = random.Random(seed); t0 = time.time(); stats = {}
    for t in range(trials):
        fam = fams[t % len(fams)]; pts = FAMS[fam](rng)
        if len(pts) < 3: continue
        kmax = rng.choice([3, 5, 5, 10, 10]); w = rng.choice([1, 2])
        mx = max(max(p) for p in pts); f = rng.choice([1, rng.randint(1, M // mx), M // mx])
        off = [rng.randint(0, M - f * mx) for _ in range(3)]
        txt = "".join("%d %d %d\n" % p for p in pts)
        r = subprocess.run(["./med_judge", str(kmax), str(w), str(f)] + [str(o) for o in off], input=txt, capture_output=True, text=True)
        line = (r.stdout.strip().splitlines() or ["CRASH rc=%d %s" % (r.returncode, r.stderr[-200:])])[-1]
        st = stats.setdefault(fam, {"trials": 0, "ok": 0, "refused_ok": 0, "bad": 0, "balls": 0, "maxn": 0})
        st["trials"] += 1; st["maxn"] = max(st["maxn"], len(pts))
        if line.startswith("OK"): st["ok"] += 1; st["balls"] += int(line.split("balls=")[1].split()[0])
        elif line.startswith("REFUSED_OK"): st["refused_ok"] += 1
        else:
            st["bad"] += 1; print("BAD", fam, kmax, w, f, off, line, pts, flush=True)
    print({"seed": seed, "elapsed_s": round(time.time() - t0, 1), "stats": stats})

main()
