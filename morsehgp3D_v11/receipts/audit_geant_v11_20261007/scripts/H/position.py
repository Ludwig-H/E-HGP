"""Effet de position dans l'ordre a froid (premier contre second d'une paire), rep 0 exclue."""
import math, statistics, sys
sys.path.insert(0, sys.argv[1])
from levers import LEVERS, METRICS, cells, load, FR
SESS = ['G1_AVX2', 'frontiere', 'graines', 'combines', 'annonces', 'cohortes', 'cache', 'tas', 'thp', 'o2', 'constantes', 'v3']
for m in ('wall', 'domain', 'forest'):
    for lab in ('cpu', 'gpu'):
        first, second = [], []
        for s in SESS:
            for path, new, base, ml in LEVERS[s]:
                if ml != lab: continue
                rep = load(path)
                cs = cells(rep, new, base)
                for fr in FR:
                    rows = [r for r in cs[fr] if r[0] != 0]
                    dm = statistics.mean(math.log(METRICS[m](a) / METRICS[m](b)) for _, a, b, _ in rows)
                    for _, a, b, newfirst in rows:
                        d = math.log(METRICS[m](a) / METRICS[m](b)) - dm
                        (first if newfirst else second).append(d)
        # si 'new' passe en premier, d = log(new/base) ; effet de position = (moy(first) - moy(second)) / 2 = log(premier/second)
        eff = (statistics.mean(first) - statistics.mean(second)) / 2
        se = math.sqrt(statistics.variance(first) / len(first) + statistics.variance(second) / len(second)) / 2
        print('%-6s %s : log(premier/second) = %+.4f (se %.4f, z %.1f), n = %d + %d' % (m, lab, eff, se, eff / se, len(first), len(second)))
