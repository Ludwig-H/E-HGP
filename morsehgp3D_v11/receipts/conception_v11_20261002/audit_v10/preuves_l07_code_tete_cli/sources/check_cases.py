"""Audit L07 : confronte, sur de petits nuages reels, (1) le binaire mhgp10_cluster de afb081774, (2) l'oracle par
coupes strictes (oracle_condense.py) lu sur l'export --tree du meme appel, (3) sklearn.cluster.HDBSCAN appele tel
quel sur l'ultrametrique des points (metric='precomputed', min_samples=1 : distances-coeur nulles, donc liaison
simple sur l'ultrametrique ; distance = niveau^(z/2) pour que lambda = 1/distance = niveau^(-z/2)).

  python3 -B check_cases.py BUILD_DIR
"""
import json
import os
import subprocess
import sys
import tempfile

import numpy as np
from sklearn.cluster import HDBSCAN

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import oracle_condense as oc  # noqa: E402

CASES = [
    # (nom, points, K, entree, mcs, z, racine selectionnable)
    ('six_points_z2_racine_permise', [(2, 0, 1), (2, 0, 4), (2, 0, 9), (9, 10, 5), (10, 6, 5), (11, 5, 6)], 2, 'core', 3, 2, True),
    ('six_points_z1_racine_permise', [(0, 10, 3), (1, 9, 2), (3, 11, 0), (7, 5, 8), (7, 5, 9), (11, 4, 9)], 2, 'core', 3, 1, True),
    ('neuf_points_z2_racine_exclue', [(1, 12, 11), (3, 3, 4), (3, 6, 10), (3, 8, 12), (4, 1, 4), (7, 2, 1), (9, 6, 13), (14, 3, 15), (15, 3, 12)], 2, 'core', 3, 2, False),
    ('huit_points_cover_K3_grille_degeneree', [(1, 2, 3), (2, 2, 0), (2, 3, 5), (3, 4, 3), (3, 4, 4), (4, 2, 0), (4, 4, 0), (5, 4, 1)], 3, 'cover', 4, 2, True),
    ('dix_points_z1_racine_exclue', [(0, 2, 6), (0, 3, 10), (0, 6, 3), (0, 12, 0), (1, 13, 10), (5, 13, 11), (8, 10, 5), (8, 15, 14), (13, 8, 2), (14, 2, 1)], 2, 'core', 3, 1, False),
]


def same_partition(a, b):
    a, b = np.asarray(a), np.asarray(b)
    if len(a) != len(b) or not np.array_equal(a == -1, b == -1):
        return False
    m = {}
    for u, v in zip(a[a >= 0], b[b >= 0]):
        if m.setdefault(int(u), int(v)) != int(v):
            return False
    return len(set(m.values())) == len(m)


def run_case(build, name, pts, k, entry, mcs, z, single, tmp):
    src, out, tree = os.path.join(tmp, 'in.u32le'), os.path.join(tmp, 'out.i32le'), os.path.join(tmp, 'tree.txt')
    np.ascontiguousarray(pts, dtype='<u4').tofile(src)
    cmd = [os.path.join(build, 'mhgp10_cluster'), src, out, '--k=%d' % k, '--mcs=%d' % mcs, '--z=%d' % z,
           '--entry=' + entry, '--threads=1', '--tree=' + tree] + (['--allow-single'] if single else [])
    r = subprocess.run(cmd, capture_output=True, text=True)
    if r.returncode != 0:
        raise RuntimeError('%s : code %d %s %s' % (name, r.returncode, r.stdout, r.stderr))
    binary = np.fromfile(out, dtype='<i4').tolist()
    o = oc.run(tree, mcs, z, True, single)
    levels = o['levels']
    D = np.array([[0.0 if i == j else levels[o['D'][i][j]] ** (z / 2.0) for j in range(len(pts))] for i in range(len(pts))])
    sk = HDBSCAN(min_cluster_size=mcs, min_samples=1, metric='precomputed', allow_single_cluster=single,
                 cluster_selection_method='eom', copy=True).fit(D).labels_.tolist()
    return dict(name=name, points=[list(p) for p in pts], k=k, entry=entry, mcs=mcs, z=z, allow_single=single,
                levels=levels, nodes=[list(v) for v in o['nodes']], attaches=[list(p) for p in o['pts']],
                binaire=binary, oracle=o['labels'], sklearn=sk,
                oracle_clusters=[dict(parent=c['parent'], membres=c['members'], stabilite=str(s), stabilite_float=float(s),
                                      retenu=ch) for c, s, ch in zip(o['clusters'], o['S'], o['chosen'])],
                binaire_egale_oracle=same_partition(binary, o['labels']),
                oracle_egale_sklearn=same_partition(o['labels'], sk), argv=cmd[3:])


def main():
    build = sys.argv[1]
    bad = 0
    with tempfile.TemporaryDirectory() as tmp:
        for case in CASES:
            r = run_case(build, *case, tmp)
            print(json.dumps(r, sort_keys=True))
            if not r['oracle_egale_sklearn']:
                bad += 1
    print('cas %d, desaccords oracle/sklearn %d' % (len(CASES), bad))
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main())
