"""Re-analyse statistique des cinq leviers retires (et de quelques leviers gardes) depuis les rapports gpu_ab bruts."""
import json, math, statistics, random, sys, os, glob
R = '/workspaces/E-HGP/morsehgp3D_v11/receipts/'
FR = ('lidar_ng00', 'lidar_ng01', 'lidar_ng02')

def order(s, k, key):
    return [o for o in s['pipeline']['orders'] if o['k'] == k][0][key]

METRICS = {
    'wall': lambda s: s['wall_ms'],
    'domain': lambda s: s['domain_ms'],
    'forest': lambda s: s['forest_ms'],
    'sp+prefix': lambda s: s['single_pass_ms'] + s['prefix_ms'],
    'prefix': lambda s: s['prefix_ms'],
    'single_pass': lambda s: s['single_pass_ms'],
    'cells5': lambda s: order(s, 5, 'publish_cells_est_ms'),
    'cells+closes5': lambda s: order(s, 5, 'publish_cells_est_ms') + order(s, 5, 'publish_closes_est_ms'),
    'pubcpu5': lambda s: order(s, 5, 'publish_cpu_ms'),
    'births': lambda s: s['pipeline']['phases']['births_ns'],
}

def load(path):
    return json.load(open(path))

def cells(report, new, base):
    """rend {(frame): [(rep, x_new, x_base, pos_new)]} avec les resumes."""
    out = {}
    orders = report['orders']
    for f in FR:
        rows = []
        reps = sorted(set(c['rep'] for c in report['cold'] if c['frame'] == f))
        for r in reps:
            a = [c for c in report['cold'] if c['frame'] == f and c['rep'] == r and c['mode'] == new and c['code'] == 0]
            b = [c for c in report['cold'] if c['frame'] == f and c['rep'] == r and c['mode'] == base and c['code'] == 0]
            if len(a) != 1 or len(b) != 1:
                continue
            o = orders[r % len(orders)]
            rows.append((r, a[0]['summary'], b[0]['summary'], o.index(new) < o.index(base)))
        out[f] = rows
    return out

def tq(p, df):
    # quantile t de Student par bissection (bibliotheque standard)
    def cdf(t):
        # integration numerique simple de la densite t
        x = df / (df + t * t)
        # fonction beta incomplete regularisee via fraction continue
        return 1 - 0.5 * betainc(df / 2, 0.5, x) if t > 0 else 0.5 * betainc(df / 2, 0.5, x)
    lo, hi = -50.0, 50.0
    for _ in range(200):
        mid = (lo + hi) / 2
        if cdf(mid) < p:
            lo = mid
        else:
            hi = mid
    return (lo + hi) / 2

def betainc(a, b, x):
    if x <= 0: return 0.0
    if x >= 1: return 1.0
    lbeta = math.lgamma(a + b) - math.lgamma(a) - math.lgamma(b) + a * math.log(x) + b * math.log(1 - x)
    if x < (a + 1) / (a + b + 2):
        return math.exp(lbeta) * cf(a, b, x) / a
    return 1 - math.exp(lbeta) * cf(b, a, 1 - x) / b

def cf(a, b, x):
    qab, qap, qam = a + b, a + 1, a - 1
    c, d = 1.0, 1 - qab * x / qap
    d = 1 / (d if abs(d) > 1e-300 else 1e-300)
    h = d
    for m in range(1, 300):
        m2 = 2 * m
        aa = m * (b - m) * x / ((qam + m2) * (a + m2))
        d = 1 + aa * d; d = 1 / (d if abs(d) > 1e-300 else 1e-300)
        c = 1 + aa / c if abs(c) > 1e-300 else 1e-300
        h *= d * c
        aa = -(a + m) * (qab + m) * x / ((a + m2) * (qap + m2))
        d = 1 + aa * d; d = 1 / (d if abs(d) > 1e-300 else 1e-300)
        c = 1 + aa / c if abs(c) > 1e-300 else 1e-300
        de = d * c; h *= de
        if abs(de - 1) < 1e-12: break
    return h

def analyse(label, sessions, metric, thr=None):
    """sessions : liste de (chemin rapport, nom new, nom base, etiquette mode)."""
    f = METRICS[metric]
    ratios, all_d, cell_means, within_sd, arm_sd = [], [], [], [], []
    detail = []
    for path, new, base, mlab in sessions:
        rep = load(path)
        cs = cells(rep, new, base)
        for fr in FR:
            rows = cs[fr]
            xs = [f(a) for _, a, b, _ in rows]
            ys = [f(b) for _, a, b, _ in rows]
            ratio = statistics.median(xs) / statistics.median(ys)
            ratios.append(ratio)
            d = [math.log(x / y) for x, y in zip(xs, ys)]
            all_d.extend(d)
            cell_means.append(statistics.mean(d))
            within_sd.append(statistics.stdev(d) if len(d) > 1 else float('nan'))
            arm_sd.append(statistics.stdev([math.log(v) for v in xs]))
            arm_sd.append(statistics.stdev([math.log(v) for v in ys]))
            detail.append((mlab, fr, len(xs), statistics.median(ys), statistics.median(xs), ratio,
                           math.exp(statistics.mean(d)), statistics.stdev(d), sum(1 for v in d if v < 0)))
    gm = math.exp(statistics.mean(math.log(r) for r in ratios))
    n = len(all_d)
    mean_d = statistics.mean(all_d)
    # SE par cellule (6 cellules) et SE regroupee (36 paires, interaction nulle supposee)
    se_cells = statistics.stdev(cell_means) / math.sqrt(len(cell_means))
    pooled_sd = math.sqrt(statistics.mean([s * s for s in within_sd]))
    se_pool = pooled_sd / math.sqrt(n)
    t6 = tq(0.975, len(cell_means) - 1)
    tN = tq(0.975, n - len(cell_means))
    ci_cells = (math.exp(statistics.mean(cell_means) - t6 * se_cells), math.exp(statistics.mean(cell_means) + t6 * se_cells))
    ci_pool = (math.exp(mean_d - tN * se_pool), math.exp(mean_d + tN * se_pool))
    neg = sum(1 for v in all_d if v < 0)
    # test des signes bilateral exact
    def binom_tail(k, n):
        return sum(math.comb(n, i) for i in range(0, k + 1)) / 2 ** n
    p_sign = min(1.0, 2 * binom_tail(min(neg, n - neg), n))
    # test de permutation par inversion de signes (Monte Carlo) de la moyenne des d
    rng = random.Random(12345)
    obs = abs(mean_d)
    hits = 0
    B = 20000
    for _ in range(B):
        s = sum(v if rng.random() < 0.5 else -v for v in all_d) / n
        if abs(s) >= obs - 1e-15:
            hits += 1
    p_perm = (hits + 1) / (B + 1)
    arm = math.sqrt(statistics.mean([s * s for s in arm_sd]))
    return dict(label=label, metric=metric, thr=thr, gm_medians=gm, n_pairs=n, gm_paired=math.exp(mean_d),
                ci_cells=ci_cells, ci_pool=ci_pool, sd_pair=pooled_sd, sd_arm=arm, neg=neg, p_sign=p_sign,
                p_perm=p_perm, detail=detail)

def show(res):
    print('--- %s : %s (seuil %s)' % (res['label'], res['metric'], res['thr']))
    for row in res['detail']:
        print('   %-4s %s n=%d base %.2f new %.2f r_med %.3f r_paire %.3f sd_d %.3f neg %d' % row)
    print('   gm(medianes)=%.3f gm(paires)=%.3f IC95 cellules [%.3f, %.3f] IC95 regroupe [%.3f, %.3f]' % (
        res['gm_medians'], res['gm_paired'], *res['ci_cells'], *res['ci_pool']))
    print('   sd paire (log)=%.4f sd bras (log, entre processus)=%.4f  paires<0 : %d/%d  p_signes=%.2g p_perm=%.2g' % (
        res['sd_pair'], res['sd_arm'], res['neg'], res['n_pairs'], res['p_sign'], res['p_perm']))

D7 = R + 'developpement_20261007/'
D6 = R + 'developpement_20261006/'
LEVERS = {
    'G1_AVX2': [(D7 + 'filtre_g1_avx2/claudeg1/gpu_ab_report_ab_k5_16_cpu.json', 'new:cpu', 'base:cpu', 'cpu'),
                (D7 + 'filtre_g1_avx2/claudeg1/gpu_ab_report_ab_k5_24_gpu.json', 'new:gpu', 'base:gpu', 'gpu')],
    'frontiere': [(D7 + 'frontiere_tranches/claudefront1/gpu_ab_report_ab_k5_16_cpu.json', 'new:cpu', 'base:cpu', 'cpu'),
                  (D7 + 'frontiere_tranches/claudefront1/gpu_ab_report_ab_k5_24_gpu.json', 'new:gpu', 'base:gpu', 'gpu')],
    'graines': [(D7 + 'prechargement_graines/claudepref1/gpu_ab_report_ab_k5_16_cpu.json', 'new:cpu', 'base:cpu', 'cpu'),
                (D7 + 'prechargement_graines/claudepref1/gpu_ab_report_ab_k5_24_gpu.json', 'new:gpu', 'base:gpu', 'gpu')],
    'combines': [(D7 + 'prechargements_publieurs/claudepref2/gpu_ab_report_ab_k5_16_cpu.json', 'new:cpu', 'base:cpu', 'cpu'),
                 (D7 + 'prechargements_publieurs/claudepref2/gpu_ab_report_ab_k5_24_gpu.json', 'new:gpu', 'base:gpu', 'gpu')],
    'annonces': [(D7 + 'annonces_publieurs/claudeann1/gpu_ab_report_ab_k5_16_cpu.json', 'new:cpu', 'base:cpu', 'cpu'),
                 (D7 + 'annonces_publieurs/claudeann1/gpu_ab_report_ab_k5_24_gpu.json', 'new:gpu', 'base:gpu', 'gpu')],
    'cohortes': [(D7 + 'cohortes_tranches/claudebirths1/gpu_ab_report_ab_k5_16_cpu.json', 'new:cpu', 'base:cpu', 'cpu'),
                 (D7 + 'cohortes_tranches/claudebirths1/gpu_ab_report_ab_k5_24_gpu.json', 'new:gpu', 'base:gpu', 'gpu')],
    'cache': [(D7 + 'cache_blocs/claudecache1/gpu_ab_report_ab_k5_16_cpu.json', 'cpu_cache', 'cpu', 'cpu'),
              (D7 + 'cache_blocs/claudecache1/gpu_ab_report_ab_k5_24_gpu.json', 'gpu_cache', 'gpu', 'gpu')],
    'thp': [(D7 + 'thp_exploration/claudethp1/gpu_ab_report_ab_k5_16_cpu.json', 'thp:cpu', 'new:cpu', 'cpu'),
            (D7 + 'thp_exploration/claudethp1/gpu_ab_report_ab_k5_24_gpu.json', 'thp:gpu', 'new:gpu', 'gpu')],
    'tas': [(D7 + 'retention_tas/claudetas1/gpu_ab_report_ab_k5_16_cpu.json', 'cpu_tas', 'cpu', 'cpu'),
            (D7 + 'retention_tas/claudetas1/gpu_ab_report_ab_k5_24_gpu.json', 'gpu_tas', 'gpu', 'gpu')],
    'o2': [(D6 + 'o2_publieur/claudeo2a/gpu_ab_report_k5_16_cpu.json', 'new:cpu', 'cst:cpu', 'cpu'),
           (D6 + 'o2_publieur/claudeo2a/gpu_ab_report_k5_24_gpu.json', 'new:gpu', 'cst:gpu', 'gpu')],
    'constantes': [(D6 + 'constantes_pas_descente/claudev3c/gpu_ab_report_k5_16_cpu.json', 'new:cpu', 'v3:cpu', 'cpu'),
                   (D6 + 'constantes_pas_descente/claudev3c/gpu_ab_report_k5_24_gpu.json', 'new:gpu', 'v3:gpu', 'gpu')],
    'v3': [(D6 + 'v3_census/claudev3ab/gpu_ab_report_k5_16_cpu.json', 'new:cpu', 'base:cpu', 'cpu'),
           (D6 + 'v3_census/claudev3ab/gpu_ab_report_k5_24_gpu.json', 'new:gpu', 'base:gpu', 'gpu')],
    'AA_o1place3': [(D6 + 'o1_placement/claudeo1place3/gpu_ab_report_k5_16_cpu.json', 'cpu_aa', 'cpu', 'cpu'),
                    (D6 + 'o1_placement/claudeo1place3/gpu_ab_report_k5_24_gpu.json', 'gpu_aa', 'gpu', 'gpu')],
    'O1_place3': [(D6 + 'o1_placement/claudeo1place3/gpu_ab_report_k5_16_cpu.json', 'cpu_place', 'cpu', 'cpu'),
                  (D6 + 'o1_placement/claudeo1place3/gpu_ab_report_k5_24_gpu.json', 'gpu_place', 'gpu', 'gpu')],
}
PLAN = [
    ('G1_AVX2', 'sp+prefix', 0.85), ('G1_AVX2', 'domain', 1.00), ('G1_AVX2', 'wall', None), ('G1_AVX2', 'forest', None),
    ('frontiere', 'prefix', 0.80), ('frontiere', 'domain', 1.00), ('frontiere', 'wall', None), ('frontiere', 'forest', None),
    ('graines', 'cells5', 0.85), ('graines', 'forest', 1.00), ('graines', 'wall', None), ('graines', 'domain', None),
    ('combines', 'cells+closes5', 0.90), ('combines', 'forest', 1.00), ('combines', 'wall', None), ('combines', 'domain', None),
    ('annonces', 'pubcpu5', 0.90), ('annonces', 'forest', 1.00), ('annonces', 'wall', None), ('annonces', 'domain', None),
    ('cohortes', 'births', 0.80), ('cohortes', 'forest', 1.00), ('cohortes', 'wall', None), ('cohortes', 'domain', None),
    ('cache', 'wall', 1.00), ('thp', 'wall', None), ('tas', 'wall', None),
    ('o2', 'forest', None), ('o2', 'wall', None), ('o2', 'domain', None),
    ('constantes', 'forest', None), ('constantes', 'wall', None), ('constantes', 'domain', None),
    ('v3', 'forest', None), ('v3', 'wall', None), ('v3', 'domain', None),
    ('AA_o1place3', 'wall', None), ('AA_o1place3', 'domain', None), ('AA_o1place3', 'forest', None),
    ('AA_o1place3', 'single_pass', None), ('AA_o1place3', 'prefix', None), ('AA_o1place3', 'pubcpu5', None),
    ('AA_o1place3', 'cells5', None), ('AA_o1place3', 'births', None),
    ('O1_place3', 'wall', None), ('O1_place3', 'forest', None),
]
if __name__ == '__main__':
    out = []
    for lever, metric, thr in PLAN:
        try:
            res = analyse(lever, LEVERS[lever], metric, thr)
        except Exception as e:
            print('ERREUR', lever, metric, repr(e)); continue
        show(res)
        out.append({k: v for k, v in res.items()})
    json.dump(out, open(sys.argv[1], 'w'), indent=1, default=str)
