"""Audit L06 : oracle Gamma_k etendu aux ordres 6..10 (la porte mhgp10_tower_oracle s'arrete a K = 5).

Reutilise check() de la porte du depot (copie sous /tmp, jamais l'arbre de lecture) : pour chaque nuage de n = 12 points
et chaque ordre k <= 10, a chaque niveau critique et aux deux coupes, nombre de composantes et partition C n X contre
Gamma_k exhaustif, plus la naturalite des verticales. Familles : generique, grille {0..3}^3, coplanaire, grille a
grappes, amas serres + points epars (descentes longues).
Usage : python3 oracle_k10.py BUILD_DIR [nuages par famille] [graine]
"""
import json
import os
import random
import subprocess
import sys
import tempfile
import time

COPY = '/tmp/v11-audit/l06_code_tour/instr'
sys.path.insert(0, os.path.join(COPY, 'reference'))
sys.path.insert(0, os.path.join(COPY, 'tests', 'oracle'))
import test_tower_oracle as T  # noqa: E402


def cloud(kind, n, rnd):
    pts = set()
    if kind == 'amas':
        centers = [tuple(rnd.randint(20, 200) for _ in range(3)) for _ in range(rnd.choice((2, 3)))]
        while len(pts) < n:
            if rnd.random() < 0.3:
                pts.add(tuple(rnd.randint(0, 220) for _ in range(3)))
            else:
                c = rnd.choice(centers)
                pts.add(tuple(c[i] + rnd.randint(-3, 3) for i in range(3)))
    else:
        while len(pts) < n:
            if kind == 'generique':
                pts.add(tuple(rnd.randint(0, 1000) for _ in range(3)))
            elif kind == 'grille':
                pts.add(tuple(rnd.randint(0, 3) for _ in range(3)))
            elif kind == 'coplanaire':
                pts.add((rnd.randint(0, 6), rnd.randint(0, 6), 0))
            else:
                pts.add(tuple(rnd.choice((0, 2, 4)) + rnd.randint(0, 1) for _ in range(3)))
    P = sorted(pts)
    rnd.shuffle(P)
    return P


def counters(exe, P, K, tmp):
    src = os.path.join(tmp, 'c.u32le')
    with open(src, 'wb') as f:
        for p in P:
            for v in p:
                f.write(int(v).to_bytes(4, 'little'))
    r = subprocess.run([exe, src, '--k=%d' % K, '--threads=1'], capture_output=True, text=True)
    if r.returncode != 0:
        return None
    j = json.loads(r.stdout.strip().splitlines()[-1])
    out = {}
    for o in j['orders']:
        c = o['join']
        out[o['k']] = dict(resolves=c['resolves'], steps=c['steps'], jumps=c['knn_jumps'], merges=o['merges'],
                           births=o['births'], joins=o['joins'], cells=o['local_cells'])
    return out


def main():
    exe = os.path.join(sys.argv[1], 'mhgp10_tower')
    per = int(sys.argv[2]) if len(sys.argv) > 2 else 2
    seed = int(sys.argv[3]) if len(sys.argv) > 3 else 20261002
    n = int(sys.argv[4]) if len(sys.argv) > 4 else 12
    rnd = random.Random(seed)
    checks = fails = cuts = 0
    tot = {}
    t0 = time.time()
    with tempfile.TemporaryDirectory() as tmp:
        for kind in ('generique', 'grille', 'coplanaire', 'grappes', 'amas'):
            for _ in range(per):
                P = cloud(kind, n, rnd)
                K = min(10, len(P))
                err, c = T.check(exe, P, K, tmp)
                checks += 1
                cuts += c
                cs = counters(exe, P, K, tmp)
                if cs:
                    for k, v in cs.items():
                        d = tot.setdefault(k, dict(resolves=0, steps=0, jumps=0, merges=0, births=0, joins=0, cells=0))
                        for a in d:
                            d[a] += v[a]
                print('%s n=%d K=%d : %s (coupes %d, %.0f s)' % (kind, len(P), K, err or 'conforme', c, time.time() - t0),
                      flush=True)
                if err:
                    fails += 1
                    print('  NUAGE', P, flush=True)
    print('oracle_k10 checks %d fails %d cuts %d' % (checks, fails, cuts))
    for k in sorted(tot):
        print('ordre %d : %s' % (k, tot[k]))
    return 1 if fails else 0


if __name__ == '__main__':
    sys.exit(main())
