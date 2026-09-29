#!/usr/bin/env python3
"""J3 sous ASan + UBSan sur des nuages d'etendue extreme (voie large aux bornes du cube u18) et autour du seuil
de la voie etroite, plus les familles du fuzzer ; dump compare a la base. usage : ubsan_run.py GRAINE NCAS SORTIE"""
import hashlib
import os
import random
import subprocess
import sys
import tempfile

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import fuzz as F  # noqa: E402

ASAN = '/tmp/j3verif_asan/mhgp10_catalogue'
BASE = '/workspaces/E-HGP/build/v10-perf/feuille-verif/build-base/mhgp10_catalogue'


def corners(rnd):
    L = F.LIM
    pts = [(x, y, z) for x in (0, L) for y in (0, L) for z in (0, L)]
    pts += [(rnd.choice((0, L)), rnd.choice((0, L)), rnd.randint(0, L)) for _ in range(rnd.randint(0, 12))]
    pts += [tuple(rnd.randint(0, L) for _ in range(3)) for _ in range(rnd.randint(2, 40))]
    return pts


def threshold(rnd):
    # etendues locales juste sous et juste sur 2^18 en repere T (4096 en coordonnees entieres)
    e = rnd.choice([4094, 4095, 4096, 4097])
    base = [tuple(rnd.randint(0, 3) for _ in range(3)) for _ in range(rnd.randint(4, 20))]
    far = [tuple(rnd.choice((0, e)) + rnd.randint(-1, 1) for _ in range(3)) for _ in range(rnd.randint(2, 12))]
    return [tuple(max(0, c) for c in p) for p in base + far]


def main():
    seed, ncas, out = int(sys.argv[1]), int(sys.argv[2]), sys.argv[3]
    rnd = random.Random(seed)
    tmp = tempfile.mkdtemp(prefix='j3ubsan_', dir='/tmp')
    src = os.path.join(tmp, 'c.u32le')
    bad = 0
    with open(out, 'a') as f:
        for cas in range(ncas):
            kind = rnd.choice(['corners', 'threshold', 'fuzz'])
            pts = corners(rnd) if kind == 'corners' else threshold(rnd) if kind == 'threshold' else F.make_cloud(rnd)[1]
            pts = [tuple(F.clamp(c) for c in p) for p in pts]
            with open(src, 'wb') as g:
                for p in pts:
                    g.write(b''.join(int(c).to_bytes(4, 'little') for c in p))
            for K in rnd.sample([2, 3, 5, 8, 10, 12], 2):
                M = rnd.choice([0, 64])
                res = []
                for exe in (ASAN, BASE):
                    dump = os.path.join(tmp, 'd.txt')
                    if os.path.exists(dump):
                        os.remove(dump)
                    try:
                        r = subprocess.run([exe, src, '--k=%d' % K, '--threads=2', '--leaf=%d' % M, '--dump=' + dump],
                                           capture_output=True, text=True, timeout=120)
                    except subprocess.TimeoutExpired:
                        res.append(('timeout', '', ''))
                        continue
                    h = hashlib.sha256(open(dump, 'rb').read()).hexdigest()[:16] if os.path.exists(dump) else '-'
                    res.append((r.returncode, h, r.stderr.strip()[:300]))
                ok = res[0][0] == res[1][0] and res[0][1] == res[1][1] and not res[0][2]
                if not ok:
                    bad += 1
                    with open(src, 'rb') as g, open(os.path.join(os.path.dirname(out), 'ubsan_cas_%d_%d.u32le' % (seed, cas)), 'wb') as h:
                        h.write(g.read())
                f.write('%d\t%d\t%s\t%d\tK=%d\tM=%d\t%s\t%s\n' % (seed, cas, kind, len(pts), K, M,
                                                                  'OK' if ok else 'ECART', res[0][2].replace('\n', ' ')))
                f.flush()
    print('ubsan graine', seed, 'cas', ncas, 'ecarts', bad)


if __name__ == '__main__':
    main()
