"""Audit L10 : sklearn.cluster.HDBSCAN (1.9.1) n'est pas invariant par permutation des lignes des que K >= 3 sur la
grille entiere : les aretes a egalite exacte de l'arbre couvrant sont triees par un argsort non stable
(sklearn/cluster/_hdbscan/hdbscan.py:165). Mesure : meme scene, meme machine, ordre des lignes permute.
Usage : python3 sklearn_egalites.py <dossier bench/synthetic epingle>
"""
import math
import sys

import numpy as np
from sklearn.cluster import HDBSCAN
from sklearn.metrics import adjusted_rand_score

sys.path.insert(0, sys.argv[1])
import metrics  # noqa: E402
import run_campaign  # noqa: E402
import scenes  # noqa: E402


def labels(G, k, mcs, sel, alpha):
    m = HDBSCAN(min_cluster_size=mcs, min_samples=k, cluster_selection_method=sel, alpha=alpha, algorithm='kd_tree',
                copy=True).fit(np.asarray(G, dtype=np.float64))
    w = np.sort(m._single_linkage_tree_['value'])
    ties = int(np.sum(w[1:] == w[:-1]))
    return m.labels_.astype(np.int64), ties, len(w)


def main():
    rng = np.random.default_rng(20261002)
    print('famille niveau n K selection alpha | aretes a egalite / aretes | ARI entre ordres (min) | ARI_s contre la verite : identite, min, max sur 4 ordres')
    for family, level in (('spherical', 'hard'), ('shells', 'medium'), ('filaments', 'hard')):
        base = dict(family=family, n=2000, groups=8, level=level, noise_fraction=0.1)
        spec = dict(base, seed=run_campaign.seed_of('dev', base, 0))
        P, L, _ = scenes.generate(spec)
        G, T, dups, _ = scenes.quantize18(P, L)
        n = len(G)
        mcs = int(round(math.sqrt(n)))
        for k in (1, 2, 3, 5, 10):
            for sel, alpha in (('eom', 1.0), ('leaf', 2.0)):
                ref, ties, edges = labels(G, k, mcs, sel, alpha)
                s0 = metrics.scores(T, ref)['ari_s']
                aris, ss = [], [s0]
                for _ in range(3):
                    p = rng.permutation(n)
                    lab_p, _, _ = labels(G[p], k, mcs, sel, alpha)
                    lab = np.empty(n, dtype=np.int64)
                    lab[p] = lab_p
                    aris.append(adjusted_rand_score(metrics.singletons(ref), metrics.singletons(lab)))
                    ss.append(metrics.scores(T, lab)['ari_s'])
                print('%-10s %-6s %d K=%-2d %-4s a=%g | %5d / %d | %.6f | %.6f %.6f %.6f' % (
                    family, level, n, k, sel, alpha, ties, edges, min(aris), s0, min(ss), max(ss)))


if __name__ == '__main__':
    main()
