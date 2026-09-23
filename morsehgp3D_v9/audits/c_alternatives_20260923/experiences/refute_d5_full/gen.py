#!/usr/bin/env python3
"""Nuages degeneres pour attaquer D5 (auditeur C, jure). Triplets u32 little-endian."""
import random, struct, sys

def write(path, pts):
    with open(path, "wb") as f:
        for p in pts:
            f.write(struct.pack("<3I", *p))

def lattice(L, density, scale, seed, off=1000):
    rng = random.Random(seed)
    pts = [(off + scale * x, off + scale * y, off + scale * z)
           for x in range(L) for y in range(L) for z in range(L) if rng.random() < density]
    return pts

def planar_lattice(L, density, scale, seed, off=1000):
    # grille plane + une seconde couche : cocirculaires en masse
    rng = random.Random(seed)
    pts = []
    for x in range(L):
        for y in range(L):
            for z in range(2):
                if rng.random() < density:
                    pts.append((off + scale * x, off + scale * y, off + scale * z * 2))
    return pts

def squares(nsq, seed, scale=10, off=5000):
    # carres cocirculaires (coquille u=4, q_min=2) avec interieurs, disperses
    rng = random.Random(seed)
    pts = set()
    for i in range(nsq):
        cx, cy, cz = (off + rng.randrange(0, 400) * scale for _ in range(3))
        r = rng.randrange(3, 9) * scale
        # carre dans le plan xy centre en c
        for dx, dy in ((r, 0), (0, r), (-r, 0), (0, -r)):
            pts.add((cx + dx, cy + dy, cz))
        for _ in range(rng.randrange(0, 4)):
            pts.add((cx + rng.randrange(-r // 2, r // 2 + 1), cy + rng.randrange(-r // 2, r // 2 + 1),
                     cz + rng.randrange(-r // 2, r // 2 + 1)))
    return sorted(pts)

if __name__ == "__main__":
    kind, out = sys.argv[1], sys.argv[2]
    seed = int(sys.argv[3]) if len(sys.argv) > 3 else 1
    if kind == "lat4":
        pts = lattice(4, 0.55, 1000, seed)
    elif kind == "lat5":
        pts = lattice(5, 0.35, 1000, seed)
    elif kind == "lat6":
        pts = lattice(6, 0.25, 1000, seed)
    elif kind == "plan":
        pts = planar_lattice(7, 0.45, 1000, seed)
    elif kind == "sq":
        pts = squares(25, seed)
    else:
        raise SystemExit("kind")
    write(out, pts)
    print(len(pts))
