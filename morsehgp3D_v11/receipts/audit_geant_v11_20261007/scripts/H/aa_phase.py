"""A/A naturels de la statistique du juge (gm des rapports de medianes de 6) sur les metriques de phase."""
import math, statistics, sys
sys.path.insert(0, sys.argv[1])
from levers import LEVERS, METRICS, cells, load, FR
PAIRS = {
    'prefix': ['graines', 'combines', 'annonces', 'cohortes', 'o2', 'constantes'],
    'sp+prefix': ['graines', 'combines', 'annonces', 'cohortes', 'o2', 'constantes'],
    'pubcpu5': ['G1_AVX2', 'frontiere'],
    'cells5': ['G1_AVX2'],
    'cells+closes5': ['G1_AVX2'],
    'domain': ['graines', 'combines', 'annonces', 'cohortes', 'o2', 'constantes', 'AA_o1place3'],
    'forest': ['G1_AVX2', 'frontiere', 'AA_o1place3'],
    'wall': ['AA_o1place3'],
}
for m, sess in PAIRS.items():
    logs = []
    for s in sess:
        try:
            rat = []
            for path, new, base, ml in LEVERS[s]:
                rep = load(path); cs = cells(rep, new, base)
                for fr in FR:
                    xs = [METRICS[m](a) for _, a, b, _ in cs[fr]]; ys = [METRICS[m](b) for _, a, b, _ in cs[fr]]
                    rat.append(math.log(statistics.median(xs) / statistics.median(ys)))
            logs.append((s, statistics.mean(rat), statistics.stdev(rat)))
        except (KeyError, TypeError, IndexError):
            continue
    gl = [x[1] for x in logs]
    rms = math.sqrt(statistics.mean([g * g for g in gl]))
    # SD attendue de log G a partir de la dispersion entre cellules : sd_cellule / sqrt(6)
    pred = math.sqrt(statistics.mean([x[2] ** 2 for x in logs])) / math.sqrt(6)
    print('%-14s n=%d  logG: %s | rms %.4f ; sd entre cellules/sqrt(6) %.4f' % (m, len(logs), ' '.join('%s:%+.3f' % (x[0][:5], x[1]) for x in logs), rms, pred))
