"""Audit L07 : le vote de couverture (--label=vote) change-t-il une etiquette non bruit de l'arbre ?
Lecture du code (cli/mhgp10_cluster.cpp:203-206, src/tower/tower.cpp:1584) : la premiere boule couvrante de x porte
son noeud d'attache, donc le premier candidat du vote est l'etiquette de l'arbre ; le vote ne peut que remplir du
bruit. Controle par execution sur quatre scenes dev de 2 000 points."""
import glob
import json
import os
import subprocess
import sys

import numpy as np

B = '/tmp/v11-audit/l07_code_tete_cli/build/mhgp10_cluster'
W = '/tmp/v11-audit/l07_code_tete_cli/vote'
os.makedirs(W, exist_ok=True)
rows = []
for prefix in ('s0000', 's0041', 's0100', 's0120'):
    src = glob.glob('/tmp/v11-audit/l07_code_tete_cli/dev/n2000/%s*.u32le' % prefix)[0]
    for K in (2, 3, 5):
        for mcs, z in ((45, 3.0), (15, 1.0), (5, 1.0)):
            out = os.path.join(W, 'o')
            r = subprocess.run([B, src, out, '--k=%d' % K, '--mcs=%d' % mcs, '--z=%r' % z, '--entry=cover', '--label=vote',
                                '--threads=1'], capture_output=True, text=True)
            if r.returncode != 0:
                print('REFUS', r.stdout)
                sys.exit(1)
            lab, vote = np.fromfile(out, dtype='<i4'), np.fromfile(out + '.vote', dtype='<i4')
            rows.append(dict(scene=os.path.basename(src)[:-6], K=K, mcs=mcs, z=z, n=len(lab), bruit_arbre=int((lab < 0).sum()),
                             bruit_vote=int((vote < 0).sum()), etiquettes_non_bruit_changees=int(((lab >= 0) & (vote != lab)).sum()),
                             bruits_remplis=int(((lab < 0) & (vote >= 0)).sum())))
for r in rows:
    print(json.dumps(r))
print('cas %d ; etiquettes non bruit modifiees par le vote : %d ; bruits remplis : %d sur %d' % (
    len(rows), sum(r['etiquettes_non_bruit_changees'] for r in rows), sum(r['bruits_remplis'] for r in rows),
    sum(r['bruit_arbre'] for r in rows)))
