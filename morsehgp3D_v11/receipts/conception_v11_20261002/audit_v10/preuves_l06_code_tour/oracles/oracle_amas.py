"""Audit L06 : oracle Gamma_k sur des nuages a deux amas serres de 7 points (n = 14), K = 6, pour exercer le saut K-NN
aux ordres 5 et 6 (a 12 points generiques, la descente ne saute presque jamais aux ordres >= 6).
Usage : python3 oracle_amas.py BUILD_DIR [nuages] [graine]
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


def cloud(rnd):
    pts = set()
    c1 = tuple(rnd.randint(50, 80) for _ in range(3))
    c2 = tuple(rnd.randint(300, 400) for _ in range(3))
    for c in (c1, c2):
        got = set()
        while len(got) < 7:
            got.add(tuple(c[i] + rnd.randint(-6, 6) for i in range(3)))
        pts |= got
    P = sorted(pts)
    rnd.shuffle(P)
    return P


def jumps(exe, P, K, tmp):
    src = os.path.join(tmp, 'c.u32le')
    with open(src, 'wb') as f:
        for p in P:
            for v in p:
                f.write(int(v).to_bytes(4, 'little'))
    r = subprocess.run([exe, src, '--k=%d' % K, '--threads=1'], capture_output=True, text=True)
    j = json.loads(r.stdout.strip().splitlines()[-1])
    return {o['k']: (o['join']['knn_jumps'] + o['point']['knn_jumps'], o['join']['steps']) for o in j['orders']}


def main():
    exe = os.path.join(sys.argv[1], 'mhgp10_tower')
    count = int(sys.argv[2]) if len(sys.argv) > 2 else 3
    rnd = random.Random(int(sys.argv[3]) if len(sys.argv) > 3 else 20261002)
    fails = cuts = 0
    tot = {}
    t0 = time.time()
    with tempfile.TemporaryDirectory() as tmp:
        for _ in range(count):
            P = cloud(rnd)
            if len(P) != 14:
                continue
            err, c = T.check(exe, P, 6, tmp)
            cuts += c
            js = jumps(exe, P, 6, tmp)
            for k, (a, b) in js.items():
                t = tot.setdefault(k, [0, 0])
                t[0] += a
                t[1] += b
            print('deux amas n=%d K=6 : %s (coupes %d, sauts par ordre %s, %.0f s)' %
                  (len(P), err or 'conforme', c, {k: v[0] for k, v in js.items()}, time.time() - t0), flush=True)
            if err:
                fails += 1
                print('  NUAGE', P, flush=True)
    print('oracle_amas fails %d cuts %d sauts par ordre %s' % (fails, cuts, {k: v[0] for k, v in sorted(tot.items())}))
    return 1 if fails else 0


if __name__ == '__main__':
    sys.exit(main())
