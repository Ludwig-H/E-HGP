"""Concordance du lot C : les scenes calculees par l'execution locale interrompue (binaires dynamiques, Python local)
doivent donner les memes lignes que l'execution G4 (binaires statiques, Python portable, memes versions)."""
import csv
import sys

P = '/workspaces/E-HGP/build/v10-persist/'
local = list(csv.DictReader(open(P + 'test_cover_C_INTERRUPTED_local_20260929/results.csv')))
g4 = {(r['unit'], r['method']): r for r in csv.DictReader(open(sys.argv[1]))}
KEYS = ('ari_s', 'ari_nc', 'ami_nc', 'coverage', 'clusters', 'refused', 'points', 'duplicates', 'zhat')
same = diff = missing = 0
units = set()
for r in local:
    k = (r['unit'], r['method'])
    units.add(r['unit'])
    if k not in g4:
        missing += 1
        continue
    if all(r[c] == g4[k][c] for c in KEYS):
        same += 1
    else:
        diff += 1
        if diff <= 5:
            print('ECART', k, {c: (r[c], g4[k][c]) for c in KEYS if r[c] != g4[k][c]})
print('scenes locales %d, lignes %d : identiques %d, differentes %d, absentes de G4 %d'
      % (len(units), len(local), same, diff, missing))
sys.exit(1 if diff else 0)
