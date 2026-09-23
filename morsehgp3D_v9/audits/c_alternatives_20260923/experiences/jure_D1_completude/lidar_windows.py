#!/usr/bin/env python3
"""Fenetres LiDAR reelles (grille 1 mm, sans sol) : n <= ~150 sites autour d'un site tire,
chaine v9 sur l'image similitude 18 bits, oracle exhaustif entier sur la fenetre."""
import random, struct, subprocess, sys, time
D = "/tmp/claude-1000/-workspaces-E-HGP/71988aca-a49a-4d33-96db-f05468331005/scratchpad/data/"
M = 262143

def load(name):
    b = open(D + name, "rb").read(); v = struct.unpack("<%dI" % (len(b) // 4), b)
    return [tuple(v[i:i + 3]) for i in range(0, len(v), 3)]

def main():
    seed, trials = int(sys.argv[1]), int(sys.argv[2]); rng = random.Random(seed)
    scenes = {s: load("scene_0%d_full.u32le" % s) for s in (0, 1, 2)}
    stats = {"trials": 0, "ok": 0, "refused_ok": 0, "bad": 0, "balls": 0, "n": []}; t0 = time.time()
    for t in range(trials):
        pts = scenes[t % 3]; c = rng.choice(pts); nt = rng.randint(50, 140)
        near = sorted(pts, key=lambda p: max(abs(p[i] - c[i]) for i in range(3)))[:nt]
        lo = [min(p[i] for p in near) for i in range(3)]
        small = sorted(set(tuple(p[i] - lo[i] for i in range(3)) for p in near))
        span = max(max(p) for p in small)
        if span > 1000: continue
        kmax = rng.choice([5, 10]); w = rng.choice([1, 2])
        f = rng.choice([1, M // span]); off = [rng.randint(0, M - f * span) for _ in range(3)]
        txt = "".join("%d %d %d\n" % p for p in small)
        r = subprocess.run(["./med_judge", str(kmax), str(w), str(f)] + [str(o) for o in off], input=txt, capture_output=True, text=True)
        line = (r.stdout.strip().splitlines() or ["CRASH rc=%d" % r.returncode])[-1]
        stats["trials"] += 1; stats["n"].append(len(small))
        if line.startswith("OK"): stats["ok"] += 1; stats["balls"] += int(line.split("balls=")[1].split()[0])
        elif line.startswith("REFUSED_OK"): stats["refused_ok"] += 1
        else: stats["bad"] += 1; print("BAD", kmax, w, f, off, line, small, flush=True)
    stats["n"] = [min(stats["n"]), max(stats["n"])] if stats["n"] else []
    print({"seed": seed, "elapsed_s": round(time.time() - t0, 1), **stats})

main()
