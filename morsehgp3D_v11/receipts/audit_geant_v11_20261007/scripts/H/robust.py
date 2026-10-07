"""(1) leviers retires : statistique du juge, sans la prise d'echauffement ; (2) A/A naturels ; (3) puissance."""
import json, math, statistics, sys
sys.path.insert(0, sys.argv[1])
from levers import LEVERS, METRICS, cells, load, FR

def judge_stat(sessions, metric, drop_first=False, drop_rep0=False):
    f = METRICS[metric]
    ratios = []
    for path, new, base, mlab in sessions:
        rep = load(path)
        cs = cells(rep, new, base)
        for fr in FR:
            rows = cs[fr]
            if drop_rep0:
                rows = [r for r in rows if r[0] != 0]
            elif drop_first and mlab == 'gpu' and fr == 'lidar_ng00':
                rows = [r for r in rows if r[0] != 0]
            xs = [f(a) for _, a, b, _ in rows]
            ys = [f(b) for _, a, b, _ in rows]
            ratios.append(statistics.median(xs) / statistics.median(ys))
    return math.exp(statistics.mean(math.log(r) for r in ratios)), ratios

def paired(sessions, metric, drop_rep0=True):
    f = METRICS[metric]
    d_all, cellsd = [], []
    for path, new, base, mlab in sessions:
        rep = load(path)
        cs = cells(rep, new, base)
        for fr in FR:
            rows = [r for r in cs[fr] if not (drop_rep0 and r[0] == 0)]
            d = [math.log(f(a) / f(b)) for _, a, b, _ in rows]
            d_all.extend(d)
            cellsd.append(statistics.stdev(d))
    return d_all, cellsd

print('=== (1) Leviers retires : statistique du juge (gm des medianes), complete / sans 1re prise GPU / sans rep 0')
TARGET = [('G1_AVX2', 'sp+prefix', 0.85), ('G1_AVX2', 'domain', 1.0), ('G1_AVX2', 'wall', None),
          ('frontiere', 'prefix', 0.80), ('frontiere', 'domain', 1.0), ('frontiere', 'wall', None),
          ('graines', 'cells5', 0.85), ('graines', 'forest', 1.0), ('graines', 'wall', None),
          ('combines', 'cells+closes5', 0.90), ('combines', 'forest', 1.0), ('combines', 'wall', None),
          ('annonces', 'pubcpu5', 0.90), ('annonces', 'forest', 1.0), ('annonces', 'wall', None),
          ('cohortes', 'births', 0.80), ('cohortes', 'forest', 1.0), ('cache', 'wall', 1.0), ('tas', 'wall', None), ('thp', 'wall', None)]
for lever, metric, thr in TARGET:
    g0, r0 = judge_stat(LEVERS[lever], metric)
    g1, r1 = judge_stat(LEVERS[lever], metric, drop_first=True)
    g2, r2 = judge_stat(LEVERS[lever], metric, drop_rep0=True)
    d, csd = paired(LEVERS[lever], metric)
    m = statistics.mean(d); se = statistics.stdev(d) / math.sqrt(len(d))
    print('%-10s %-14s seuil %-5s gm=%.3f | sans 1re GPU=%.3f | sans rep0=%.3f | paires(sans rep0) gm=%.3f IC95~[%.3f,%.3f] n=%d neg=%d' % (
        lever, metric, thr, g0, g1, g2, math.exp(m), math.exp(m - 2.01 * se), math.exp(m + 2.01 * se), len(d), sum(1 for v in d if v < 0)))

print()
print('=== (2) A/A : statistique gm(medianes 6 vs 6) sur metriques non touchees par le levier')
NAT = [('G1_AVX2', 'forest'), ('frontiere', 'forest'), ('graines', 'domain'), ('combines', 'domain'), ('annonces', 'domain'),
       ('cohortes', 'domain'), ('o2', 'domain'), ('constantes', 'domain'), ('AA_o1place3', 'wall'), ('AA_o1place3', 'domain'),
       ('AA_o1place3', 'forest'), ('AA_o1place3', 'prefix'), ('AA_o1place3', 'single_pass')]
vals = {}
for lever, metric in NAT:
    g, rr = judge_stat(LEVERS[lever], metric)
    g2, _ = judge_stat(LEVERS[lever], metric, drop_rep0=True)
    vals.setdefault(metric, []).append(math.log(g))
    vals.setdefault('all_stage', []).append(math.log(g)) if metric in ('domain', 'forest', 'wall') else None
    print('  %-12s %-12s gm=%.4f (sans rep0 %.4f) ratios cellule min %.3f max %.3f' % (lever, metric, g, g2, min(rr), max(rr)))
for k, v in vals.items():
    if len(v) > 1:
        print('  %-10s n=%d moyenne log=%.4f sd log=%.4f -> rms=%.4f' % (k, len(v), statistics.mean(v), statistics.stdev(v),
                                                                       math.sqrt(statistics.mean([x * x for x in v]))))

print()
print('=== (3) Bruit par prise (sans rep 0) : sd des log-rapports apparies par cellule, par metrique et voie')
for lever, metric in [('AA_o1place3', 'wall'), ('AA_o1place3', 'domain'), ('AA_o1place3', 'forest'), ('AA_o1place3', 'prefix'),
                      ('AA_o1place3', 'single_pass'),
                      ('G1_AVX2', 'forest'), ('frontiere', 'forest'), ('graines', 'domain'), ('combines', 'domain'),
                      ('annonces', 'domain'), ('graines', 'cells5'), ('combines', 'cells+closes5'), ('annonces', 'pubcpu5'),
                      ('G1_AVX2', 'sp+prefix'), ('frontiere', 'prefix'), ('G1_AVX2', 'wall'), ('annonces', 'wall'),
                      ('graines', 'wall'), ('combines', 'wall'), ('frontiere', 'wall')]:
    d, csd = paired(LEVERS[lever], metric)
    cpu = math.sqrt(statistics.mean([s * s for s in csd[:3]]))
    gpu = math.sqrt(statistics.mean([s * s for s in csd[3:]]))
    print('  %-12s %-14s sd_paire CPU=%.4f GPU=%.4f global=%.4f' % (lever, metric, cpu, gpu, math.sqrt(statistics.mean([s * s for s in csd]))))
