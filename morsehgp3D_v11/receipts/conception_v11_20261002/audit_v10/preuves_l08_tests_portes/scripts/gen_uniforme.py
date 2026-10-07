"""Nuage uniforme u18 a positions distinctes, Python nu (random.Random : flux stable entre versions)."""
import random
import sys

n, seed, out = int(sys.argv[1]), int(sys.argv[2]), sys.argv[3]
rnd = random.Random(seed)
pts = set()
while len(pts) < n:
    pts.add((rnd.randrange(1 << 18), rnd.randrange(1 << 18), rnd.randrange(1 << 18)))
pts = sorted(pts)
rnd.shuffle(pts)
with open(out, 'wb') as f:
    for p in pts:
        for v in p:
            f.write(int(v).to_bytes(4, 'little'))
