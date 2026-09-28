"""Porte de CORRECTION de la condensation v10 (pas une reimplementation d'HDBSCAN).

La tete v10 condense la hierarchie de points de la tour ; elle ne depend pas du producteur. On la juge
contre sklearn.cluster.HDBSCAN la ou les deux objets coincident : atteignabilite mutuelle a K = 1, 2 sur
des nuages entiers sans ex aequo notables. A K >= 3 les poids mreach presentent des plateaux (egalites
max(core_a, core_b, d)) que sklearn binarise arbitrairement alors que la v10 fusionne en N-aire : les
partitions peuvent alors differer (ARI moyen 0,97-0,99 mesure le 28 sept.) sans defaut de la tete.
Parametres : min_samples = K in {1, 2}, min_cluster_size in {5, 15, 40}, EOM et feuilles.
Usage : python3 test_hdbscan_equivalence.py BUILD_DIR   (code 0 si egalite partout, 1 sinon, 3 plancher)
"""
import os
import subprocess
import sys
import tempfile

import numpy as np
from sklearn.cluster import HDBSCAN


def same_partition(a, b):
    """Egalite a permutation des etiquettes pres, bruit (-1) compris et fixe."""
    if len(a) != len(b):
        return False
    if not np.array_equal(a == -1, b == -1):
        return False
    mapping = {}
    for u, v in zip(a[a >= 0], b[b >= 0]):
        if mapping.setdefault(u, v) != v:
            return False
    return len(set(mapping.values())) == len(mapping)


def cloud(rng, n):
    k = rng.integers(2, 7)
    centers = rng.uniform(0, 200000, size=(k, 3))
    lab = rng.integers(0, k, n)
    pts = centers[lab] + rng.normal(size=(n, 3)) * rng.uniform(3000, 15000)
    noise = rng.uniform(0, 200000, size=(n // 10, 3))
    pts = np.vstack([pts, noise])
    pts = np.clip(np.rint(pts), 0, 262143).astype(np.uint32)
    return np.unique(pts, axis=0)


def main():
    build = sys.argv[1]
    exe = os.path.join(build, 'mhgp10_mreach_cluster')
    rng = np.random.default_rng(20260928)
    checks = mismatches = 0
    with tempfile.TemporaryDirectory() as tmp:
        for trial in range(30):
            P = cloud(rng, int(rng.integers(300, 1500)))
            rng.shuffle(P)
            src = os.path.join(tmp, 'in.u32le')
            P.astype('<u4').tofile(src)
            for K in (1, 2):
                for mcs in (5, 15, 40):
                    for sel in ('eom', 'leaf'):
                        out = os.path.join(tmp, 'out.i32le')
                        subprocess.run([exe, src, out, '--source=mreach', '--k=%d' % K, '--mcs=%d' % mcs,
                                        '--selection=' + sel, '--threads=2'], check=True, capture_output=True)
                        ours = np.fromfile(out, dtype='<i4')
                        ref = HDBSCAN(min_cluster_size=mcs, min_samples=K, cluster_selection_method=sel,
                                      copy=True).fit(P.astype(np.float64)).labels_
                        checks += 1
                        if not same_partition(ours, ref):
                            mismatches += 1
                            if mismatches <= 5:
                                print('ECART trial=%d K=%d mcs=%d sel=%s ours=%d ref=%d noise %d/%d' % (
                                    trial, K, mcs, sel, ours.max() + 1, ref.max() + 1,
                                    (ours < 0).sum(), (ref < 0).sum()))
    print('hdbscan_checks %d mismatches %d' % (checks, mismatches))
    if checks < 300:  # 30 nuages x 2 x 3 x 2 = 360
        return 3
    return 1 if mismatches else 0


if __name__ == '__main__':
    sys.exit(main())
