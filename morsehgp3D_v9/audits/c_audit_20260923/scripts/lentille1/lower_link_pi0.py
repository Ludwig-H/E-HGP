#!/usr/bin/env python3
"""pi0 du lien inferieur de d_K en un centre critique (Reani-Bobrowski) :
directions h de S^2 avec au moins j = K-p produits h.v_i > delta (marge du lien a rayon epsilon ; v_i = sites de
coquille - centre, c dans l'interieur relatif de conv). Attendu : j=u -> vide
(indice 0, naissance) ; j=u-1 -> u composantes (indice 1, u bras) ; j<=u-2 ->
connexe (indice >= 2, sans effet sur H0)."""
import math, sys
def fib(N):
    g = math.pi * (3 - math.sqrt(5))
    for i in range(N):
        z = 1 - 2 * (i + 0.5) / N; r = math.sqrt(1 - z * z)
        yield (r * math.cos(g * i), r * math.sin(g * i), z)
cases = {
  'u2_paire': [(1, 0, 0), (-1, 0, 0)],
  'u3_triangle_aigu': [(1, 0, 0), (-0.5, 0.8, 0), (-0.4, -0.9, 0)],
  'u4_tetra': [(1, 1, 1), (1, -1, -1), (-1, 1, -1), (-1, -1, 1)],
}
N = 12000
P = list(fib(N)); step = math.sqrt(4 * math.pi / N) * 2.2
cell = {}
for i, p in enumerate(P): cell.setdefault(tuple(int(math.floor(x / step)) for x in p), []).append(i)
def neigh(i):
    c = tuple(int(math.floor(x / step)) for x in P[i])
    for dx in (-1, 0, 1):
        for dy in (-1, 0, 1):
            for dz in (-1, 0, 1):
                for j in cell.get((c[0]+dx, c[1]+dy, c[2]+dz), []):
                    if j != i and sum((a-b)**2 for a, b in zip(P[i], P[j])) < step * step: yield j
rc = 0
for name, V in cases.items():
    u = len(V); out = []
    for j in range(1, u + 1):
        keep = {i for i, h in enumerate(P) if sum(1 for v in V if sum(a*b for a, b in zip(h, v)) > 0.15) >= j}
        seen, comps = set(), 0
        for s in keep:
            if s in seen: continue
            comps += 1; stack = [s]; seen.add(s)
            while stack:
                x = stack.pop()
                for y in neigh(x):
                    if y in keep and y not in seen: seen.add(y); stack.append(y)
        expect = 0 if j == u else (u if j == u - 1 else 1)
        out.append((j, u - j, comps, expect))
        if comps != expect: rc = 1
    print(name, 'j, indice mu=u-j, composantes, attendu :', out)
print('status', 'PASS' if rc == 0 else 'FAIL'); sys.exit(rc)
