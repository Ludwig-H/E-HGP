"""Extension directe des deux oracles de HEAD aux ordres que ses portes ne jouent jamais (K = 7, 8, 10).
Memes fonctions check() que les portes, memes generateurs et graines ; seul K change.
Usage : python3 oracle_k10.py BUILD SRC_V10 [nb_nuages_catalogue] [nb_nuages_tour]
"""
import json
import os
import random
import sys
import tempfile
import time
from collections import Counter

build, src = sys.argv[1], sys.argv[2]
NC = int(sys.argv[3]) if len(sys.argv) > 3 else 40
NT = int(sys.argv[4]) if len(sys.argv) > 4 else 8
sys.path.insert(0, os.path.join(src, 'reference'))
sys.path.insert(0, os.path.join(src, 'tests', 'oracle'))
import hgp10_ref as R  # noqa: E402
import test_catalogue_oracle as C  # noqa: E402
import test_tower_oracle as T  # noqa: E402

stats = Counter()
fails = []
t0 = time.time()
with tempfile.TemporaryDirectory() as tmp:
    rnd = random.Random(20260928)
    for t, P in enumerate(C.clouds(NC, rnd)):
        for K in (8, 10):
            err = C.check(os.path.join(build, 'mhgp10_catalogue'), P, K, tmp)
            stats['catalogue_K%d' % K] += 1
            if err:
                fails.append(('catalogue', t, len(P), K, err))
            else:
                stats['catalogue_boules_K%d' % K] += sum(1 for b in R.catalogue(P, K) if b.qmin >= 2)
    print('catalogue fini en %.0f s' % (time.time() - t0), dict(stats), 'ecarts', len(fails), flush=True)
    rnd = random.Random(20260929)
    for t, P in enumerate(C.clouds(NT, rnd)):
        P = P[:12]
        for K in (7, 10):
            err, cuts = T.check(os.path.join(build, 'mhgp10_tower'), P, K, tmp)
            stats['tour_K%d' % K] += 1
            stats['tour_coupes_K%d' % K] += cuts
            if err:
                fails.append(('tour', t, len(P), K, err))
        print('tour nuage %d n=%d : %s ecarts %d (%.0f s)' % (t, len(P), dict(stats), len(fails), time.time() - t0), flush=True)
for f in fails[:10]:
    print('ECART', f)
print(json.dumps({'controles': dict(stats), 'ecarts': len(fails)}, sort_keys=True))
sys.exit(1 if fails else 0)
