"""Audit L07 : recoupement de la condensation « a cohortes » par quatre chemins independants, sur de petits nuages
reels (grilles etroites : egalites exactes frequentes ; et grilles larges : position quasi generale).

  V  : binaire publie mhgp10_cluster (afb081774)                       -> etiquettes et export --tree
  N  : sonde C++ de cet audit (l07_probe, cohortes sur dendrogramme normalise)
  A  : binaire lie a la tete de l'auditeur independant (overlay cohort_repair, f79850f8...)
  O  : oracle Python par coupes strictes sur l'export --tree (oracle_condense.py)
  S  : sklearn.cluster.HDBSCAN, metric='precomputed' sur l'ultrametrique des points (seulement quand la
       racine est exclue : la regle d'etiquetage de la racine unique de sklearn differe, hors sujet ici)

Compte les cas ou N, A, O (et S) rendent la meme partition, et ceux ou V en differe.
  python3 -B cross_check.py N_CLOUDS SEED
"""
import json
import os
import subprocess
import sys
import tempfile

import numpy as np
from sklearn.cluster import HDBSCAN

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import oracle_condense as oc  # noqa: E402

ROOT = '/tmp/v11-audit/l07_code_tete_cli'
CLI = os.path.join(ROOT, 'build', 'mhgp10_cluster')
OVERLAY = os.path.join(ROOT, 'probe', 'mhgp10_cluster_overlay_auditeur')
PROBE = os.path.join(ROOT, 'probe', 'l07_probe')


def same_partition(a, b):
    a, b = np.asarray(a), np.asarray(b)
    if len(a) != len(b) or not np.array_equal(a == -1, b == -1):
        return False
    m = {}
    for u, v in zip(a[a >= 0], b[b >= 0]):
        if m.setdefault(int(u), int(v)) != int(v):
            return False
    return len(set(m.values())) == len(m)


def main():
    clouds, seed = int(sys.argv[1]), int(sys.argv[2])
    rng = np.random.default_rng(seed)
    stats = dict(cas=0, refus=0, N_eq_O=0, A_eq_O=0, S_eq_O=0, S_cas=0, V_ne_O=0, ambigus=0, hors_domaine=0,
                 racine_seule_A_ne_O=0)
    bad = []
    with tempfile.TemporaryDirectory() as tmp:
        for t in range(clouds):
            n = int(rng.integers(8, 41))
            grid = int(rng.choice([6, 12, 40, 1000]))
            P = np.unique(rng.integers(0, grid, size=(n, 3)), axis=0)
            rng.shuffle(P)
            if len(P) < 8:
                continue
            src = os.path.join(tmp, 'in.u32le')
            np.ascontiguousarray(P, dtype='<u4').tofile(src)
            K = int(rng.choice([1, 2, 3, 5]))
            entry = str(rng.choice(['core', 'cover']))
            mcs = int(rng.choice([2, 3, 5]))
            z = int(rng.choice([1, 2]))
            single = bool(rng.integers(0, 2))
            if K > len(P):
                continue
            out, tree = os.path.join(tmp, 'v'), os.path.join(tmp, 'tree')
            args = ['--k=%d' % K, '--mcs=%d' % mcs, '--z=%d' % z, '--entry=' + entry, '--threads=1'] + (
                ['--allow-single'] if single else [])
            r = subprocess.run([CLI, src, out] + args + ['--tree=' + tree], capture_output=True, text=True)
            if r.returncode != 0:
                stats['refus'] += 1
                continue
            V = np.fromfile(out, dtype='<i4')
            outa = os.path.join(tmp, 'a')
            ra = subprocess.run([OVERLAY, src, outa] + args, capture_output=True, text=True)
            A = np.fromfile(outa, dtype='<i4') if ra.returncode == 0 else None
            cfg = os.path.join(tmp, 'cfg')
            with open(cfg, 'w') as f:
                f.write('%d %d eom %d\n' % (mcs, z, 1 if single else 0))
            pre = os.path.join(tmp, 'p')
            rp = subprocess.run([PROBE, 'run', src, pre, '--k-list=%d' % K, '--entries=' + entry, '--configs=' + cfg,
                                 '--threads=1', '--dump=all'], capture_output=True, text=True)
            N = np.fromfile('%s.%s.k%d.0.n' % (pre, entry, K), dtype='<i4')
            try:
                o = oc.run(tree, mcs, z, True, single)
            except oc.Ambiguous:
                stats['ambigus'] += 1
                continue
            except ValueError:
                stats['hors_domaine'] += 1  # niveau nul (K = 1 : lambda infini) ou point lourd
                continue
            O = np.array(o['labels'])
            stats['cas'] += 1
            ok_n = same_partition(N, O)
            ok_a = A is not None and same_partition(A, O)
            stats['N_eq_O'] += ok_n
            stats['A_eq_O'] += ok_a
            stats['V_ne_O'] += not same_partition(V, O)
            if not single:
                levels = o['levels']
                D = np.array([[0.0 if i == j else levels[o['D'][i][j]] ** (z / 2.0) for j in range(len(P))]
                              for i in range(len(P))])
                S = HDBSCAN(min_cluster_size=mcs, min_samples=1, metric='precomputed', allow_single_cluster=False,
                            cluster_selection_method='eom', copy=True).fit(D).labels_
                stats['S_cas'] += 1
                ok_s = same_partition(S, O)
                stats['S_eq_O'] += ok_s
            else:
                ok_s = True
            if not ok_a and single and len(o['clusters']) == 1:
                stats['racine_seule_A_ne_O'] += 1
            if not (ok_n and ok_a and ok_s) and len(bad) < 12:
                bad.append(dict(points=P.tolist(), K=K, entry=entry, mcs=mcs, z=z, single=single, V=V.tolist(),
                                N=N.tolist(), A=None if A is None else A.tolist(), O=O.tolist(),
                                S=None if single else S.tolist(), N_eq_O=bool(ok_n), A_eq_O=bool(ok_a), S_eq_O=bool(ok_s)))
    print(json.dumps(stats))
    for b in bad:
        print(json.dumps(b))


if __name__ == '__main__':
    main()
