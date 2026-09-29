"""Methodes du banc : la tete v10 sur la tour (CLI mhgp10_cluster) et sklearn.cluster.HDBSCAN appele tel quel."""
import os
import subprocess
import tempfile
import warnings

import numpy as np
from scipy.spatial import cKDTree

warnings.filterwarnings('ignore')


def tower_labels(build, grid, k, mcs, z=1.0, selection='eom', allow_single=False, threads=4, entry='core'):
    exe = os.path.join(build, 'mhgp10_cluster')
    with tempfile.TemporaryDirectory() as tmp:
        src, out = os.path.join(tmp, 'in.u32le'), os.path.join(tmp, 'out.i32le')
        np.ascontiguousarray(grid, dtype='<u4').tofile(src)
        cmd = [exe, src, out, '--k=%d' % k, '--mcs=%d' % mcs, '--z=%r' % float(z), '--selection=' + selection,
               '--threads=%d' % threads, '--entry=' + entry]
        if allow_single:
            cmd.append('--allow-single')
        r = subprocess.run(cmd, capture_output=True, text=True)
        if r.returncode != 0:
            raise RuntimeError('mhgp10_cluster code %d : %s %s' % (r.returncode, r.stdout, r.stderr))
        return np.fromfile(out, dtype='<i4').astype(np.int64)


def tower_labels_batch(build, grid, ks, entries, configs, threads=1):
    """Un seul appel de mhgp10_cluster : un catalogue a l'ordre max(ks), puis chaque (entree, K) de la tour et chaque
    configuration de tete (mcs, z, selection, allow_single). Rend {(entree, K, i): etiquettes}. Leve RuntimeError si
    le binaire refuse (code non nul) : l'appelant retombe alors sur des appels separes."""
    exe = os.path.join(build, 'mhgp10_cluster')
    ks, entries = sorted(set(int(k) for k in ks)), list(entries)
    with tempfile.TemporaryDirectory() as tmp:
        src, out, cfg = os.path.join(tmp, 'in.u32le'), os.path.join(tmp, 'out.i32le'), os.path.join(tmp, 'cfg')
        np.ascontiguousarray(grid, dtype='<u4').tofile(src)
        with open(cfg, 'w') as f:
            for mcs, z, selection, single in configs:
                f.write('%d %r %s %d\n' % (int(mcs), float(z), selection, 1 if single else 0))
        cmd = [exe, src, out, '--k-list=' + ','.join(str(k) for k in ks), '--configs=' + cfg,
               '--entry=' + ','.join(entries), '--threads=%d' % threads]
        r = subprocess.run(cmd, capture_output=True, text=True)
        if r.returncode != 0:
            raise RuntimeError('mhgp10_cluster code %d : %s %s' % (r.returncode, r.stdout, r.stderr))
        labels = {}
        for e in entries:
            for k in ks:
                for i in range(len(configs)):
                    tag = ('.' + e) if len(entries) > 1 else ''
                    labels[(e, k, i)] = np.fromfile('%s%s.k%d.%d' % (out, tag, k, i), dtype='<i4').astype(np.int64)
        return labels


def hdbscan_labels(grid, min_samples, mcs, selection='eom', alpha=1.0, allow_single=False):
    from sklearn.cluster import HDBSCAN
    model = HDBSCAN(min_cluster_size=int(mcs), min_samples=int(min_samples), cluster_selection_method=selection,
                    alpha=float(alpha), allow_single_cluster=bool(allow_single), algorithm='kd_tree', copy=True)
    return model.fit(np.asarray(grid, dtype=np.float64)).labels_.astype(np.int64)


def fill_noise(grid, labels):
    """Remplissage complet : chaque point de bruit recoit l'etiquette du point classe le plus proche."""
    labels = labels.copy()
    noise = labels < 0
    if noise.all() or not noise.any():
        return labels
    tree = cKDTree(np.asarray(grid[~noise], dtype=np.float64))
    _, j = tree.query(np.asarray(grid[noise], dtype=np.float64))
    labels[noise] = labels[~noise][j]
    return labels


def bounded_fill(grid, labels, k, rho):
    """Remplissage borne b(rho) (EVAL_v2 § 2.6) : un point de bruit p recoit l'amas c du point classe le plus proche
    si core(p) <= rho * Q95_c, ou core = distance au k-ieme voisin (point compris) et Q95_c le quantile 95 %
    (type 7) de core sur les membres de c ; sinon p reste du bruit. Egalites de plus proche : ordre de cKDTree."""
    labels = labels.copy()
    noise = labels < 0
    if noise.all() or not noise.any():
        return labels
    X = np.asarray(grid, dtype=np.float64)
    core = cKDTree(X).query(X, k=int(k))[0]
    core = core[:, -1] if core.ndim == 2 else core
    q95 = {c: float(np.quantile(core[labels == c], 0.95)) for c in np.unique(labels[~noise])}
    _, j = cKDTree(X[~noise]).query(X[noise])
    cand = labels[~noise][j]
    idx = np.flatnonzero(noise)
    ok = core[idx] <= rho * np.array([q95[c] for c in cand])
    labels[idx[ok]] = cand[ok]
    return labels


def zhat(grid, k=10):
    """Dimension intrinseque, MLE de Levina-Bickel moyennee (MacKay-Ghahramani), point exclu."""
    X = np.asarray(grid, dtype=np.float64)
    d, _ = cKDTree(X).query(X, k=k + 1)
    d = d[:, 1:]
    d = np.maximum(d, 1e-12)
    inv = np.log(d[:, -1:] / d[:, :-1]).sum(axis=1) / (k - 1)
    inv = inv[np.isfinite(inv) & (inv > 0)]
    return float(1.0 / inv.mean()) if len(inv) else 3.0
