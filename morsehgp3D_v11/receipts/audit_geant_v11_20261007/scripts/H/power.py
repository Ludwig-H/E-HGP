"""Ecart-type des log-rapports apparies (prise d'echauffement exclue) par metrique et voie, puis effectifs requis."""
import json, math, statistics, sys
sys.path.insert(0, sys.argv[1])
from levers import LEVERS, METRICS, cells, load, FR
from statistics import NormalDist
NormInv = NormalDist().inv_cdf
# Sessions de 7 oct. (bras apparies new/base), metriques non visees ou visees : on prend la dispersion des d apres
# soustraction de la moyenne de cellule (l'effet eventuel est retire), rep 0 exclu.
SESS = ['G1_AVX2', 'frontiere', 'graines', 'combines', 'annonces', 'cohortes', 'cache', 'tas', 'thp', 'o2', 'constantes', 'v3', 'AA_o1place3']
METS = ['wall', 'domain', 'forest', 'prefix', 'sp+prefix', 'cells5', 'cells+closes5', 'pubcpu5']
acc = {}
for s in SESS:
    for path, new, base, mlab in LEVERS[s]:
        rep = load(path)
        cs = cells(rep, new, base)
        for fr in FR:
            rows = [r for r in cs[fr] if r[0] != 0]
            for m in METS:
                try:
                    d = [math.log(METRICS[m](a) / METRICS[m](b)) for _, a, b, _ in rows]
                except (KeyError, TypeError, ZeroDivisionError):
                    continue
                mu = statistics.mean(d)
                acc.setdefault((m, mlab), []).extend([x - mu for x in d])
                acc.setdefault((m, 'n_cells'), []).append(len(d))
z = {'a2': NormInv(0.975), 'a1': NormInv(0.95), 'b': NormInv(0.80)}
print('%-14s %-4s %8s %6s | n paires totales (bilateral 5 %%, puissance 80 %%) pour un effet de 1 / 2 / 3 / 5 %%' % ('metrique', 'voie', 'sd_d', 'ddl'))
out = {}
for (m, lab), res in sorted(acc.items()):
    if lab == 'n_cells':
        continue
    # ddl : chaque cellule de 5 paires perd 1 ddl
    ncell = len(res) // 5
    df = len(res) - ncell
    sd = math.sqrt(sum(x * x for x in res) / df)
    need = [math.ceil(((z['a2'] + z['b']) * sd / math.log(1 / (1 - e))) ** 2) for e in (0.01, 0.02, 0.03, 0.05)]
    out[(m, lab)] = (sd, need)
    print('%-14s %-4s %8.4f %6d | %s' % (m, lab, sd, df, ' / '.join(str(n) for n in need)))
