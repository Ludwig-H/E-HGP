"""Experience de DEVELOPPEMENT (graines dev seulement) : affectation des points que la tete laisse en bruit.

Motif : sur les melanges separables, la tete v10-b retient 91 % des modes separables
(audits/tete_multik_20260929) ; l'ecart restant au plafond de Bayes vient de l'affectation de la masse sous le col.
Le remplissage borne b(rho) du lot C donne a un point de bruit l'amas du point classe le plus proche ; la partition de
Bayes d'un melange a modes separes suit plutot les bassins d'attraction de la densite. Remplissages compares,
appliques SYMETRIQUEMENT a la tour (tete v10-b du lot C) et a sklearn.cluster.HDBSCAN (configuration du lot C), a
chaque K du lot C :
  - none ; full (point classe le plus proche) ; b<rho> (remplissage borne, EVAL_v2 § 2.6, core a max(K, 5)) ;
  - asc (k = max(K, 5)) et asc20 (k = 20) : montee de densite, a la QuickShift, sur l'estimateur K-NN. Chaque point
    pointe vers le plus dense de ses k plus proches voisins, lui compris (core = distance au k-ieme voisin minimale ;
    egalite : plus petit indice). Un point de bruit suit ces pointeurs jusqu'au premier point classe, dont il prend
    l'amas, ou jusqu'a un maximum local non classe : il reste alors du bruit ;
  - asc*_b<rho> : la meme montee, puis le rejet de b(rho) (core(p) <= rho * Q95 de l'amas atteint).

Un refus (code non nul du binaire, exception de sklearn ou de la generation) vaut ARI_s = 0 a chaque remplissage et
est compte (colonne `refused`) : aucune scene ni aucune methode n'est omise (EVAL_v2 D8).

  python3 alloc_dev.py --build BUILD --out OUT.csv --sizes 8000,16000,32000 --replicates 2 --jobs 46
Codes : 0 ; 1 si une ligne est un refus (toutes les lignes sont ecrites).
"""
import argparse
import csv
import math
import os
import sys
from concurrent.futures import ProcessPoolExecutor, as_completed

import numpy as np
from scipy.spatial import cKDTree

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import methods  # noqa: E402
import metrics  # noqa: E402
import run_campaign  # noqa: E402
import scenes  # noqa: E402

# tetes du lot C (PREREG_V10_COVER_C_20260929) : z de la tour (entree cover, EOM) ; (selection, alpha) de sklearn
TOWER_Z = {1: 6, 2: 4, 3: 5, 5: 6, 8: 6, 10: 6}
SKLEARN = {1: ('eom', 1.0), 2: ('eom', 1.0), 3: ('leaf', 2.0), 5: ('leaf', 2.0), 8: ('leaf', 2.0), 10: ('leaf', 2.0)}
RHOS = (1.5, 2.0, 2.5)
ASCENTS = ('asc', 'asc20')
FILLS = (('none', 'full') + tuple('b%g' % r for r in RHOS)
         + tuple(a + s for a in ASCENTS for s in ('',) + tuple('_b%g' % r for r in RHOS)))
COLS = ('family', 'level', 'noise', 'n', 'seed', 'method', 'k', 'fill', 'ari_s', 'ami_nc', 'coverage', 'clusters',
        'refused')


def refused_rows(spec, method, k):
    return [dict(family=spec['family'], level=spec['level'], noise=spec['noise_fraction'], n=spec['n'],
                 seed=spec['seed'], method=method, k=k, fill=fill, ari_s=0.0, ami_nc=0.0, coverage=0.0, clusters=0,
                 refused=1) for fill in FILLS]


def ascent_parents(core, nbr):
    """Pointeur de chaque point vers le plus dense de ses voisins, lui compris (core minimal ; egalite : plus petit
    indice). Le long d'un chemin, (core, indice) decroit strictement : aucun cycle hors des points fixes."""
    best = np.lexsort((nbr, core[nbr]), axis=1)[:, 0]
    return nbr[np.arange(len(nbr)), best]


def ascent_fill(labels, parent):
    """Etiquette du premier point classe sur le chemin de montee ; -1 si le chemin finit sur un maximum non classe."""
    reach = np.where(labels >= 0, labels, -2)  # -2 : pas encore resolu
    for p in np.flatnonzero(labels < 0):
        path, q = [], p
        while reach[q] == -2:
            path.append(q)
            if parent[q] == q:
                reach[q] = -1
                break
            q = parent[q]
        reach[path] = reach[q]
    return reach


def reject(labels, filled, core, rho):
    """Rejet de b(rho) : un point rempli garde l'amas c atteint si core(p) <= rho * Q95_c, sinon il redevient bruit."""
    out = filled.copy()
    new = np.flatnonzero((labels < 0) & (filled >= 0))
    if new.size:
        q95 = {c: float(np.quantile(core[labels == c], 0.95)) for c in np.unique(labels[labels >= 0])}
        lim = np.array([q95[c] for c in filled[new]])
        out[new[core[new] > rho * lim]] = -1
    return out


def run_unit(spec, build):
    P, L, _ = scenes.generate(spec)
    G, T, _, _ = scenes.quantize18(P, L)
    n = len(G)
    mcs = int(round(math.sqrt(n)))
    X = np.asarray(G, dtype=np.float64)
    tree = cKDTree(X)
    memo = {}

    def knn(kk):
        if kk not in memo:
            d, nbr = tree.query(X, k=kk)
            memo[kk] = (d[:, -1], ascent_parents(d[:, -1], nbr))
        return memo[kk]

    ks = sorted(TOWER_Z)
    zs = sorted(set(TOWER_Z.values()))
    try:
        batch = methods.tower_labels_batch(build, G, ks, ['cover'], [(mcs, z, 'eom', False) for z in zs], threads=1)
        raw = [('tour', k, batch[('cover', k, zs.index(TOWER_Z[k]))]) for k in ks]
    except Exception:  # refus de la tour (code non nul, binaire absent) : ARI_s = 0 (D8)
        raw = [('tour', k, None) for k in ks]
    for k in ks:
        try:
            raw.append(('sklearn', k, methods.hdbscan_labels(G, k, mcs, *SKLEARN[k])))
        except Exception:  # refus de sklearn : ARI_s = 0 (D8)
            raw.append(('sklearn', k, None))
    out = []
    for method, k, lab in raw:
        if lab is None:
            out += refused_rows(spec, method, k)
            continue
        core = knn(max(k, 5))[0]
        for fill in FILLS:
            if fill == 'none':
                pred = lab
            elif fill == 'full':
                pred = methods.fill_noise(G, lab)
            elif fill[0] == 'b':
                pred = methods.bounded_fill(G, lab, max(k, 5), float(fill[1:]))
            else:
                name, _, rho = fill.partition('_b')
                pred = ascent_fill(lab, knn(max(k, 5) if name == 'asc' else 20)[1])
                if rho:
                    pred = reject(lab, pred, core, float(rho))
            s = metrics.scores(T, pred)
            out.append(dict(family=spec['family'], level=spec['level'], noise=spec['noise_fraction'], n=n,
                            seed=spec['seed'], method=method, k=k, fill=fill, ari_s=round(s['ari_s'], 6),
                            ami_nc=round(s['ami_nc'], 6), coverage=round(s['coverage'], 4), clusters=s['clusters'],
                            refused=0))
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--build', required=True)
    ap.add_argument('--out', required=True)
    ap.add_argument('--sizes', default='8000,16000,32000')
    ap.add_argument('--replicates', type=int, default=2)
    ap.add_argument('--jobs', type=int, default=4)
    a = ap.parse_args()
    specs = run_campaign.plan('dev', [int(s) for s in a.sizes.split(',')], a.replicates)
    specs.sort(key=lambda s: -s['n'])
    refused = 0
    with open(a.out, 'w', newline='') as h:
        w = csv.DictWriter(h, fieldnames=COLS)
        w.writeheader()
        with ProcessPoolExecutor(max_workers=a.jobs) as pool:
            futs = {pool.submit(run_unit, s, a.build): s for s in specs}
            for i, fu in enumerate(as_completed(futs)):
                try:
                    rows = fu.result()
                except Exception as e:  # generation ou ouvrier en echec : toutes ses lignes a ARI_s = 0 (D8)
                    print('ECHEC', futs[fu], repr(e)[:300], flush=True)
                    rows = [r for m in ('tour', 'sklearn') for k in sorted(TOWER_Z) for r in refused_rows(futs[fu], m, k)]
                refused += sum(r['refused'] for r in rows)
                w.writerows(rows)
                h.flush()
                print('%d/%d' % (i + 1, len(specs)), flush=True)
    print('SCENES %d LIGNES_REFUSEES %d' % (len(specs), refused), flush=True)
    return 1 if refused else 0


if __name__ == '__main__':
    sys.exit(main())
