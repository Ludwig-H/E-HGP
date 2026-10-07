"""Juge global INDEPENDANT de l'ordre K = 1 de la tour aux tailles d'interet.

A K = 1, L_1(a) est l'union des boules de rayon carre a : ses composantes fusionnent aux niveaux d^2 / 4 des aretes de
l'arbre couvrant minimal euclidien (lien simple). On compare, en ENTIERS exacts :
  - le multiensemble des (niveau, taille de la composante creee) des fusions de la tour (dump de mhgp10_tower,
    ordre 1 ; une multifusion N-aire compte une fois) ;
  - le meme multiensemble tire d'un Kruskal par plateaux sur un EMST exact (Prim O(n^2) en numpy int64, aucune
    structure spatiale, aucun flottant).
Aucun tableau indexe par paire (Prim garde un vecteur par sommet). Usage : python3 emst_k1.py BUILD IN.u32le fils
"""
import json
import os
import subprocess
import sys
import tempfile
from collections import Counter
from fractions import Fraction

import numpy as np

build, src, threads = sys.argv[1], sys.argv[2], int(sys.argv[3])
P = np.fromfile(src, dtype='<u4').reshape(-1, 3).astype(np.int64)
P = np.unique(P, axis=0)
n = len(P)

# ---- EMST exact (Prim dense, distances carrees entieres)
best = np.full(n, np.iinfo(np.int64).max, dtype=np.int64)
frm = np.zeros(n, dtype=np.int64)
done = np.zeros(n, dtype=bool)
cur = 0
done[0] = True
edges = []
for _ in range(n - 1):
    d = ((P - P[cur]) ** 2).sum(axis=1)
    upd = (d < best) & ~done
    best[upd] = d[upd]
    frm[upd] = cur
    masked = np.where(done, np.iinfo(np.int64).max, best)
    nxt = int(masked.argmin())
    edges.append((int(best[nxt]), int(frm[nxt]), nxt))
    done[nxt] = True
    cur = nxt
edges.sort()
parent = list(range(n))
size = [1] * n


def find(x):
    while parent[x] != x:
        parent[x] = parent[parent[x]]
        x = parent[x]
    return x


want = Counter()
i = 0
while i < len(edges):
    j = i
    while j < len(edges) and edges[j][0] == edges[i][0]:
        j += 1
    touched = set()
    for w, a, b in edges[i:j]:
        ra, rb = find(a), find(b)
        if ra != rb:
            parent[rb] = ra
            size[ra] += size[rb]
        touched.add(a)
    for r in {find(a) for a in touched}:
        want[(Fraction(edges[i][0], 4), size[r])] += 1
    i = j

# ---- ordre 1 de la tour
with tempfile.TemporaryDirectory(dir='/tmp/v11-audit/l08_tests_portes') as tmp:
    dump = os.path.join(tmp, 't.txt')
    r = subprocess.run([os.path.join(build, 'mhgp10_tower'), src, '--k=1', '--threads=%d' % threads, '--dump=' + dump],
                       capture_output=True, text=True)
    if r.returncode != 0:
        print(json.dumps({'refus': r.stdout.strip()[:200]}))
        sys.exit(2)
    par, lev = [], []
    for line in open(dump):
        t = line.split()
        if t[0] == 'node':
            par.append(int(t[2]))
            lev.append(Fraction(int(t[3]), int(t[4])))
nn = len(par)
kids = [0] * nn
leaves = [0] * nn
for v in range(nn):
    if par[v] >= 0:
        kids[par[v]] += 1
for v in range(nn):          # enfants crees avant les parents (indices croissants vers la racine)
    if kids[v] == 0:
        leaves[v] = 1
    if par[v] >= 0:
        leaves[par[v]] += leaves[v]
got = Counter((lev[v], leaves[v]) for v in range(nn) if kids[v] > 0)
ok = got == want and sum(1 for v in range(nn) if kids[v] == 0) == n
print(json.dumps({'entree': os.path.basename(src), 'sites': n, 'noeuds_tour_k1': nn, 'fusions_tour': sum(got.values()),
                  'fusions_emst_par_plateaux': sum(want.values()), 'aretes_emst': len(edges),
                  'multifusions': sum(1 for v in range(nn) if kids[v] >= 3),
                  'verdict': 'EMST_OK' if ok else 'EMST_ECART',
                  'ecarts': len((got - want) + (want - got))}, sort_keys=True))
sys.exit(0 if ok else 1)
