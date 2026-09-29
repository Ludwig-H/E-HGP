"""Equivalence des appels groupes du banc (29 septembre 2026).

Le banc de test calcule toutes les methodes de la tour d'une scene en un seul appel de mhgp10_cluster : un catalogue a
l'ordre maximal de la liste, puis chaque (entree, K) et chaque tete (`--k-list`, `--entry=core,cover`, `--configs`).
Cette porte exige, sur deux scenes dev de 2 000 points :
  - les memes etiquettes que des appels separes (un catalogue a l'ordre K seulement, une seule tete) ;
  - les memes etiquettes avec `--label=vote`, qui resout toutes les boules couvrantes, que sans (seules les premieres
    boules couvrantes sont resolues) : les deux chemins de l'attache par premiere couverture concordent ;
  - les memes etiquettes a 1 fil et a 4 fils.

  python3 test_batch_equivalence.py <dossier de build>   -> code 0 si conforme, 1 sinon
"""
import os
import subprocess
import sys
import tempfile

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, '..', '..', 'bench', 'synthetic'))
import methods  # noqa: E402
import scenes  # noqa: E402

SPECS = ({'family': 'filaments', 'n': 2000, 'groups': 8, 'level': 'hard', 'noise_fraction': 0.1, 'seed': 29092026},
         {'family': 'anisotropic', 'n': 2000, 'groups': 8, 'level': 'medium', 'noise_fraction': 0.0, 'seed': 2909})
KS = (1, 2, 3, 5, 8)
ENTRIES = ('core', 'cover')


def vote_call(build, G, k, cfg, threads):
    with tempfile.TemporaryDirectory() as tmp:
        src, out = os.path.join(tmp, 'in.u32le'), os.path.join(tmp, 'out')
        np.ascontiguousarray(G, dtype='<u4').tofile(src)
        mcs, z, sel, _ = cfg
        r = subprocess.run([os.path.join(build, 'mhgp10_cluster'), src, out, '--k=%d' % k, '--mcs=%d' % mcs,
                            '--z=%r' % z, '--selection=' + sel, '--threads=%d' % threads, '--entry=cover',
                            '--label=vote'], capture_output=True, text=True)
        if r.returncode != 0:
            raise RuntimeError('code %d : %s' % (r.returncode, r.stdout))
        return np.fromfile(out, dtype='<i4').astype(np.int64)


def main():
    build = sys.argv[1]
    failures, checks = [], 0
    for spec in SPECS:
        P, L, _ = scenes.generate(spec)
        G, _, _, _ = scenes.quantize18(P, L)
        n = len(G)
        mcs = int(round(np.sqrt(n)))
        configs = [(mcs, methods.zhat(G), 'eom', False), (mcs, 1.0, 'leaf', False)]
        batch = methods.tower_labels_batch(build, G, KS, ENTRIES, configs, threads=1)
        batch4 = methods.tower_labels_batch(build, G, KS, ENTRIES, configs, threads=4)
        for e in ENTRIES:
            for k in KS:
                for i, (m, z, sel, single) in enumerate(configs):
                    alone = methods.tower_labels(build, G, k, m, z, sel, single, threads=1, entry=e)
                    checks += 3
                    if not np.array_equal(alone, batch[(e, k, i)]):
                        failures.append('%s %s K=%d tete %d : groupe != separe' % (spec['family'], e, k, i))
                    if not np.array_equal(batch4[(e, k, i)], batch[(e, k, i)]):
                        failures.append('%s %s K=%d tete %d : 4 fils != 1 fil' % (spec['family'], e, k, i))
                    if e == 'cover' and k > 1:
                        if not np.array_equal(vote_call(build, G, k, configs[i], 2), alone):
                            failures.append('%s K=%d tete %d : --label=vote change l etiquette de l arbre'
                                            % (spec['family'], k, i))
                    else:
                        checks -= 1
        print('%s : %d points, %d amas (couverture, K = 3, tete 0)'
              % (spec['family'], n, len(set(batch[('cover', 3, 0)].tolist()) - {-1})))
    print('%d controles, %d ecarts' % (checks, len(failures)))
    for f in failures:
        print('ECHEC', f)
    return 1 if failures or checks == 0 else 0


if __name__ == '__main__':
    sys.exit(main())
