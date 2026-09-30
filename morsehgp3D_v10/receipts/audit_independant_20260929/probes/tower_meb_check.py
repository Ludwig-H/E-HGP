"""Sonde bornee du repli Welzl exact et de la proposition certifiee contre MEB Fraction brute."""
import itertools
import json
import os
import random
import subprocess
import sys
from fractions import Fraction as Fr

sys.path.insert(0, os.path.join(sys.argv[2], "reference"))
import hgp10_ref as R  # noqa: E402


def main():
    rnd = random.Random(2026092902)
    bases = [[(0, 0, 0), (225077, 1, 0), (225068, 1, 0), (152369, 7, 1)],
             list(itertools.product((0, 2), repeat=3)),
             [(x, 0, 0) for x in (0, 1, 3, 8, 13)],
             [(0, 0, 0), (3, 0, 0), (1, 5, 0), (4, 4, 0), (8, 1, 0)]]
    for t in range(16):
        n = rnd.randint(4, 8)
        pts = set()
        while len(pts) < n:
            lim = 3 if t % 4 == 0 else 262143
            p = tuple(rnd.randint(0, lim) for _ in range(3))
            if t % 4 == 1:
                p = p[:2] + (0,)
            pts.add(p)
        bases.append(sorted(pts))
    cases, want = [], []
    for P in bases:
        rad, cen = R.meb(P, tuple(range(len(P))))
        for _ in range(3):
            Q = P.copy()
            rnd.shuffle(Q)
            cases.append(Q)
            want.append((rad, cen))
    payload = "".join(str(len(P)) + " " + " ".join(str(v) for p in P for v in p) + "\n" for P in cases)
    r = subprocess.run([sys.argv[1]], input=payload, text=True, capture_output=True, timeout=30)
    if r.returncode:
        raise RuntimeError(r.stderr)
    lines = r.stdout.splitlines()
    if len(lines) != len(cases):
        raise RuntimeError("wrong result count")
    fallbacks = checks = 0
    for i, (line, w) in enumerate(zip(lines, want)):
        t = list(map(int, line.split()))
        for j in (0, 9):
            rad = Fr(t[j], t[j + 1])
            cen = tuple(Fr(t[j + 2 + a]) + Fr(t[j + 5 + a], t[j + 8]) for a in range(3))
            if (rad, cen) != w:
                raise RuntimeError("case %d %s: got %s want %s points=%s" %
                                   (i, "welzl" if j == 0 else "certified", (rad, cen), w, cases[i]))
            checks += 1
        fallbacks += t[-1]
    print(json.dumps(dict(status="ok", bases=len(bases), permuted_cases=len(cases),
                          exact_spheres=checks, meb_fallbacks=fallbacks), sort_keys=True))


if __name__ == "__main__":
    main()
