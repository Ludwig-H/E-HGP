"""Audit L07 : --allow-single quand la racine seule est retenue. La tete publiee etiquette tous les points ;
sklearn.cluster.HDBSCAN(allow_single_cluster=True) applique un seuil de lambda et laisse du bruit."""
import json
import subprocess

import numpy as np
from sklearn.cluster import HDBSCAN

rng = np.random.default_rng(3)
B = '/tmp/v11-audit/l07_code_tete_cli/build/'
W = '/tmp/v11-audit/l07_code_tete_cli/single/'
for trial in range(5):
    blob = rng.normal(size=(300, 3)) * 3000 + 100000
    far = rng.uniform(0, 200000, size=(6, 3))
    P = np.unique(np.clip(np.rint(np.vstack([blob, far])), 0, 262143).astype(np.uint32), axis=0)
    P.astype('<u4').tofile(W + 'in.u32le')
    for K in (1, 2):
        subprocess.run([B + 'mhgp10_mreach_cluster', W + 'in.u32le', W + 'mr', '--source=mreach', '--k=%d' % K, '--mcs=20',
                        '--allow-single', '--threads=1'], capture_output=True, text=True, check=True)
        mr = np.fromfile(W + 'mr', dtype='<i4')
        subprocess.run([B + 'mhgp10_cluster', W + 'in.u32le', W + 'tw', '--k=%d' % K, '--mcs=20', '--allow-single',
                        '--threads=1'], capture_output=True, text=True, check=True)
        tw = np.fromfile(W + 'tw', dtype='<i4')
        sk = HDBSCAN(min_cluster_size=20, min_samples=K, allow_single_cluster=True, copy=True).fit(P.astype(np.float64)).labels_
        print(json.dumps(dict(essai=trial, K=K, n=len(P),
                              temoin_mreach=dict(clusters=int(mr.max() + 1), bruit=int((mr < 0).sum())),
                              tour=dict(clusters=int(tw.max() + 1), bruit=int((tw < 0).sum())),
                              sklearn=dict(clusters=int(sk.max() + 1), bruit=int((sk < 0).sum())))))
