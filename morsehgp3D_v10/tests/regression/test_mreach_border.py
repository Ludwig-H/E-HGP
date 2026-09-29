"""Temoin d'atteignabilite mutuelle, entree « bord » (29 septembre 2026).

Le banc compare la tour a la hierarchie d'HDBSCAN munie de la meme entree et de la meme tete
(mhgp10_mreach_cluster --entry=border). Cette porte exige, sur deux scenes dev de 2 000 points :
  - a K = 1, entree bord = entree coeur (toutes les distances-coeur sont nulles, aucun point ne change d'attache) ;
  - aucun refus ni dendrogramme invalide (code 0) pour K = 2, 3, 5, 8, 10 et alpha = 1, 2 ;
  - l'entree bord ne retire jamais de point a un amas par rapport a l'entree coeur a tete egale : sa couverture
    (points non bruit) est au moins celle de l'entree coeur dans 90 % des cas au moins (la selection peut changer) ;
  - les memes etiquettes a 1 fil et a 4 fils.

  python3 test_mreach_border.py <dossier de build>   -> code 0 si conforme, 1 sinon
"""
import os
import sys

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, '..', '..', 'bench', 'synthetic'))
import methods  # noqa: E402
import scenes  # noqa: E402

SPECS = ({'family': 'spherical', 'n': 2000, 'groups': 8, 'level': 'hard', 'noise_fraction': 0.1, 'seed': 29092027},
         {'family': 'filaments', 'n': 2000, 'groups': 8, 'level': 'hard', 'noise_fraction': 0.0, 'seed': 29092028})


def main():
    build = sys.argv[1]
    failures, checks, cover_ok, cover_n = [], 0, 0, 0
    for spec in SPECS:
        P, L, _ = scenes.generate(spec)
        G, _, _, _ = scenes.quantize18(P, L)
        mcs = int(round(np.sqrt(len(G))))
        cfgs = [(mcs, 1.0, 'eom', False), (mcs, 3.0, 'eom', False), (mcs, 1.0, 'leaf', False)]
        for alpha in (1, 2):
            a = methods.mreach_labels(build, G, 1, alpha, 'border', cfgs)
            b = methods.mreach_labels(build, G, 1, alpha, 'core', cfgs)
            checks += 1
            if not all(np.array_equal(x, y) for x, y in zip(a, b)):
                failures.append('%s alpha=%d K=1 : bord != coeur' % (spec['family'], alpha))
            for k in (2, 3, 5, 8, 10):
                try:
                    bord = methods.mreach_labels(build, G, k, alpha, 'border', cfgs, threads=1)
                    coeur = methods.mreach_labels(build, G, k, alpha, 'core', cfgs, threads=1)
                    bord4 = methods.mreach_labels(build, G, k, alpha, 'border', cfgs, threads=4)
                except RuntimeError as e:
                    failures.append('%s alpha=%d K=%d : %s' % (spec['family'], alpha, k, str(e)[:120]))
                    continue
                checks += 2
                if not all(np.array_equal(x, y) for x, y in zip(bord, bord4)):
                    failures.append('%s alpha=%d K=%d : 4 fils != 1 fil' % (spec['family'], alpha, k))
                for x, y in zip(bord, coeur):
                    cover_n += 1
                    cover_ok += int((x >= 0).mean() >= (y >= 0).mean())
        print('%s : %d points' % (spec['family'], len(G)))
    frac = cover_ok / max(cover_n, 1)
    print('%d controles, %d ecarts ; couverture bord >= coeur dans %.0f %% des cas' % (checks, len(failures), 100 * frac))
    if frac < 0.9:
        failures.append('couverture de l entree bord inferieure a celle de l entree coeur trop souvent (%.2f)' % frac)
    for f in failures:
        print('ECHEC', f)
    return 1 if failures or checks == 0 else 0


if __name__ == '__main__':
    sys.exit(main())
