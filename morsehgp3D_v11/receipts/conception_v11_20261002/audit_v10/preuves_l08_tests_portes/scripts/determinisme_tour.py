"""Identite du dump de la tour FULL (avec attaches, entree core) selon le nombre de fils et sous permutation.
Usage : python3 determinisme_tour.py BUILD IN.u32le K fils..."""
import hashlib
import json
import os
import random
import subprocess
import sys
import tempfile

build, src, K = sys.argv[1], sys.argv[2], int(sys.argv[3])
threads = [int(t) for t in sys.argv[4:]] or [1, 4]


def run(inp, thr, tmp):
    dump = os.path.join(tmp, 'dump.txt')
    r = subprocess.run([os.path.join(build, 'mhgp10_tower'), inp, '--k=%d' % K, '--threads=%d' % thr, '--dump=' + dump],
                       capture_output=True, text=True)
    if r.returncode != 0:
        return 'code %d' % r.returncode, 0, None
    js = json.loads([l for l in r.stdout.splitlines() if l.startswith('{')][-1])
    h = hashlib.sha256()
    with open(dump, 'rb') as f:
        for block in iter(lambda: f.read(1 << 20), b''):
            h.update(block)
    size = os.path.getsize(dump)
    os.remove(dump)
    st = js['stages']
    cnt = {p: {c: st[p][c] for c in ('level_exact', 'jump_exact')} for p in ('join', 'point', 'vertical')}
    return h.hexdigest(), size, dict(meb_fallbacks=st['meb_fallbacks'], replis=cnt, noeuds=[o['nodes'] for o in js['orders']])


with tempfile.TemporaryDirectory(dir='/tmp/v11-audit/l08_tests_portes') as tmp:
    raw = open(src, 'rb').read()
    n = len(raw) // 12
    perm = list(range(n))
    random.Random(7).shuffle(perm)
    psrc = os.path.join(tmp, 'perm.u32le')
    with open(psrc, 'wb') as f:
        for i in perm:
            f.write(raw[12 * i:12 * i + 12])
    res = {t: run(src, t, tmp) for t in threads}
    p = run(psrc, threads[-1], tmp)
out = {'entree': os.path.basename(src), 'K': K, 'n': n, 'tour_sha256_par_fils': {str(t): v[0] for t, v in res.items()},
       'tour_octets': res[threads[0]][1], 'tour_identique_selon_fils': len({v[0] for v in res.values()}) == 1,
       'tour_permutee_sha256': p[0], 'tour_permutee_identique': p[0] == res[threads[-1]][0],
       'compteurs_a_%d_fil' % threads[0]: res[threads[0]][2]}
print(json.dumps(out, indent=1, sort_keys=True))
