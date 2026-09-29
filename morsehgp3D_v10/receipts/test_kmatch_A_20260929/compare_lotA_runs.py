"""Concordance du lot A : les scenes calculees par le run interrompu (redemarrage de la machine) doivent donner les
memes lignes que la relance complete (meme preenregistrement, meme binaire, calcul deterministe)."""
import csv
import sys

P = '/workspaces/E-HGP/build/v10-persist/'
old = list(csv.DictReader(open(P + 'test_kmatch_A_INTERRUPTED_restart_20260929/results.csv')))
new = list(csv.DictReader(open(P + 'test_kmatch_A/results.csv')))
KEYS = ('ari_s', 'ari_nc', 'ami_nc', 'coverage', 'clusters', 'refused', 'points', 'duplicates', 'zhat')
idx = {(r['unit'], r['method']): r for r in new}
same = diff = missing = 0
for r in old:
    k = (r['unit'], r['method'])
    if k not in idx:
        missing += 1
        continue
    if all(r[c] == idx[k][c] for c in KEYS):
        same += 1
    else:
        diff += 1
        if diff <= 5:
            print('ECART', k, {c: (r[c], idx[k][c]) for c in KEYS if r[c] != idx[k][c]})
print('lignes du run interrompu : %d ; identiques %d ; differentes %d ; absentes de la relance %d'
      % (len(old), same, diff, missing))
sys.exit(1 if diff else 0)
