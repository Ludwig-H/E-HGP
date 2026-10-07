"""Audit L06 : oracle Gamma_k exact sur de petits nuages CHOISIS pour que la descente saute aux ordres eleves.

Famille « noyau + halo » (n = 14) : c points serres (cube de cote 6) et 14 - c points epars autour. On tire 600 nuages,
on garde ceux dont la tour fait le plus de sauts K-NN aux deux derniers ordres (compteurs de mhgp10_tower), puis on les
soumet a l'oracle exhaustif (check() de la porte du depot, copie sous /tmp) a tous les ordres 1..K.
Usage : python3 oracle_sauts.py c K [nuages juges] [graine]
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

exe_dir = '/tmp/v11-audit/l06_code_tour/build-release'
exe = os.path.join(exe_dir, 'mhgp10_tower')
c, K = int(sys.argv[1]), int(sys.argv[2])
keep = int(sys.argv[3]) if len(sys.argv) > 3 else 2
rnd = random.Random(int(sys.argv[4]) if len(sys.argv) > 4 else 20261002)
N = 14


def counters(P, tmp):
    src = os.path.join(tmp, 'c.u32le')
    with open(src, 'wb') as f:
        for p in P:
            for v in p:
                f.write(int(v).to_bytes(4, 'little'))
    r = subprocess.run([exe, src, '--k=%d' % K, '--threads=1'], capture_output=True, text=True)
    if r.returncode != 0:
        return None
    j = json.loads(r.stdout.strip().splitlines()[-1])
    return {o['k']: o['join']['knn_jumps'] + o['point']['knn_jumps'] for o in j['orders']}


cands = []
with tempfile.TemporaryDirectory() as tmp:
    for _ in range(600):
        ctr = tuple(rnd.randint(150, 250) for _ in range(3))
        pts = set()
        while len(pts) < c:
            pts.add(tuple(ctr[i] + rnd.randint(-3, 3) for i in range(3)))
        while len(pts) < N:
            pts.add(tuple(max(0, ctr[i] + rnd.randint(-140, 140)) for i in range(3)))
        P = sorted(pts)
        rnd.shuffle(P)
        js = counters(P, tmp)
        if js is None:
            continue
        cands.append((js.get(K, 0) + js.get(K - 1, 0), js, P))
    cands.sort(key=lambda t: -t[0])
    print('famille noyau %d + halo %d, K = %d : %d nuages sur %d sautent aux ordres %d ou %d' %
          (c, N - c, K, sum(1 for x in cands if x[0] > 0), len(cands), K - 1, K), flush=True)
    fails = cuts = 0
    t0 = time.time()
    for hi, js, P in cands[:keep]:
        err, n = T.check(exe_dir, P, K, tmp) if False else T.check(exe, P, K, tmp)
        cuts += n
        print('noyau %d K=%d : %s (coupes %d, sauts par ordre %s, %.0f s)' % (c, K, err or 'conforme', n, js, time.time() - t0),
              flush=True)
        if err:
            fails += 1
            print('  NUAGE', P, flush=True)
    print('oracle_sauts c=%d K=%d juges %d fails %d cuts %d' % (c, K, min(keep, len(cands)), fails, cuts))
sys.exit(1 if fails else 0)
