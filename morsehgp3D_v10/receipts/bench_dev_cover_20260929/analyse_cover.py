"""Lecture DEV : entree des points par premiere couverture (tour_cover, tour_coverVote) contre C n X (tower /
tower_cap) et sklearn a min_samples = K, a K apparie. J = moyenne ponderee par cellule de l'ARI_s ; pour chaque
methode et chaque K, meilleure (tete, politique de bruit), puis detail par famille et par taille."""
import collections
import csv
import sys

FILES = sys.argv[1:] or ['kmatch_dev_A.csv', 'kcover_dev_k5cap.csv', 'kcover_dev_cover.csv']
ALIAS = {'tower': 'cap', 'tower_cap': 'cap', 'tower_cover': 'cover', 'tower_coverVote': 'coverVote', 'hdb': 'sklearn'}
cells = collections.defaultdict(lambda: collections.defaultdict(list))
fam = collections.defaultdict(lambda: collections.defaultdict(list))
size = collections.defaultdict(lambda: collections.defaultdict(list))
for path in FILES:
    for r in csv.DictReader(open(path)):
        key = (ALIAS.get(r['method'], r['method']), int(r['k']), r['head'], r['fill'])
        a = float(r['ari_s'])
        cells[key][(r['family'], r['level'], r['noise'], r['n'])].append(a)
        fam[key][r['family']].append(a)
        size[key][r['n']].append(a)
J = {k: sum(sum(v) / len(v) for v in per.values()) / len(per) for k, per in cells.items()}
fams = sorted({f for per in fam.values() for f in per})
print('%-3s %-10s %-14s %-5s %7s  %s' % ('K', 'methode', 'tete', 'bruit', 'J', ' '.join('%6s' % f[:6] for f in fams)))
for K in sorted({k[1] for k in J}):
    for m in ('cap', 'cover', 'coverVote', 'sklearn'):
        c = [(v, k) for k, v in J.items() if k[0] == m and k[1] == K]
        if not c:
            continue
        v, k = max(c)
        fv = [sum(fam[k][f]) / len(fam[k][f]) for f in fams]
        sv = {n: sum(x) / len(x) for n, x in size[k].items()}
        print('%-3d %-10s %-14s %-5s %7.4f  %s   n: %s' % (K, m, k[2], k[3], v, ' '.join('%6.3f' % x for x in fv),
                                                     ' '.join('%s=%.3f' % (n, sv[n]) for n in sorted(sv, key=int))))
    print()
# meme tete et meme politique : ecart cover - cap par tete
print('Ecart couverture - C n X a tete et politique egales (meilleures 5 par K)')
for K in sorted({k[1] for k in J}):
    rows = []
    for k, v in J.items():
        if k[0] == 'cover' and k[1] == K and ('cap', K, k[2], k[3]) in J:
            rows.append((v - J[('cap', K, k[2], k[3])], k[2], k[3], v))
    for d, h, f, v in sorted(rows, reverse=True)[:5]:
        print('  K=%d %-14s %-5s cover=%.4f  delta=%+.4f' % (K, h, f, v, d))
