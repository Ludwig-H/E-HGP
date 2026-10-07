"""Mesure (absente des portes HEAD) : identite bit a bit des dumps du catalogue et de la tour FULL selon le nombre de
fils, et equivariance par permutation de l'entree (le dump imprime des coordonnees de sites, pas des PointId).
Usage : python3 determinisme.py BUILD IN.u32le K [--points] [fils...]
"""
import hashlib
import json
import os
import random
import subprocess
import sys
import tempfile

build, src, K = sys.argv[1], sys.argv[2], int(sys.argv[3])
rest = sys.argv[4:]
points = '--points' in rest
threads = [int(t) for t in rest if t.isdigit()] or [1, 2, 4]


def sha(path):
    h = hashlib.sha256()
    with open(path, 'rb') as f:
        for block in iter(lambda: f.read(1 << 20), b''):
            h.update(block)
    return h.hexdigest()


def run(exe, inp, thr, tmp, extra=()):
    dump = os.path.join(tmp, 'dump.txt')
    if os.path.exists(dump):
        os.remove(dump)
    r = subprocess.run([os.path.join(build, exe), inp, '--k=%d' % K, '--threads=%d' % thr, '--dump=' + dump] + list(extra),
                       capture_output=True, text=True)
    js = [l for l in r.stdout.splitlines() if l.startswith('{')]
    if r.returncode != 0:
        return 'code %d %s' % (r.returncode, js[-1] if js else ''), 0
    size = os.path.getsize(dump)
    d = sha(dump)
    os.remove(dump)
    return d, size


res = {'entree': os.path.basename(src), 'K': K, 'points': points}
with tempfile.TemporaryDirectory(dir='/tmp/v11-audit/l08_tests_portes') as tmp:
    raw = open(src, 'rb').read()
    n = len(raw) // 12
    res['n'] = n
    perm = list(range(n))
    random.Random(7).shuffle(perm)
    psrc = os.path.join(tmp, 'perm.u32le')
    with open(psrc, 'wb') as f:
        for i in perm:
            f.write(raw[12 * i:12 * i + 12])
    extra = () if points else ('--no-points',)
    cat = {t: run('mhgp10_catalogue', src, t, tmp) for t in threads}
    tow = {t: run('mhgp10_tower', src, t, tmp, extra) for t in threads}
    cat_p = run('mhgp10_catalogue', psrc, threads[-1], tmp)
    tow_p = run('mhgp10_tower', psrc, threads[-1], tmp, extra)
    res['catalogue_sha256_par_fils'] = {str(t): v[0] for t, v in cat.items()}
    res['tour_sha256_par_fils'] = {str(t): v[0] for t, v in tow.items()}
    res['catalogue_octets'] = cat[threads[0]][1]
    res['tour_octets'] = tow[threads[0]][1]
    res['catalogue_identique_selon_fils'] = len({v[0] for v in cat.values()}) == 1
    res['tour_identique_selon_fils'] = len({v[0] for v in tow.values()}) == 1
    res['catalogue_permute_identique'] = cat_p[0] == cat[threads[-1]][0]
    res['tour_permutee_identique'] = tow_p[0] == tow[threads[-1]][0]
    res['catalogue_permute_sha256'] = cat_p[0]
    res['tour_permutee_sha256'] = tow_p[0]
print(json.dumps(res, indent=1, sort_keys=True))
