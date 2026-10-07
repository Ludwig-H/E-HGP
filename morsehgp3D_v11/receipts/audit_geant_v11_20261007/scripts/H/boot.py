"""SE bootstrap (reechantillonnage des prises dans chaque bras x cellule) de log G, statistique du juge ; z, P(garde)."""
import json, math, statistics, sys, random
sys.path.insert(0, sys.argv[1])
from levers import LEVERS, METRICS, cells, load, FR
from perm import data, stat, Phi, NormInv

def boot(cellpairs, B=4000, seed=11):
    rng = random.Random(seed)
    sims = []
    for _ in range(B):
        logs = []
        for pairs in cellpairs:
            xs = [p[0] for p in pairs]; ys = [p[1] for p in pairs]
            bx = [rng.choice(xs) for _ in xs]; by = [rng.choice(ys) for _ in ys]
            logs.append(math.log(statistics.median(bx) / statistics.median(by)))
        sims.append(statistics.mean(logs))
    sims.sort()
    return statistics.pstdev(sims), math.exp(sims[int(0.025 * B)]), math.exp(sims[int(0.975 * B)])

rows = [('G1_AVX2', 'sp+prefix', 0.85), ('G1_AVX2', 'domain', 1.0), ('G1_AVX2', 'wall', None),
        ('frontiere', 'prefix', 0.80), ('frontiere', 'domain', 1.0), ('frontiere', 'wall', None),
        ('graines', 'cells5', 0.85), ('graines', 'forest', 1.0), ('graines', 'wall', None),
        ('combines', 'cells+closes5', 0.90), ('combines', 'forest', 1.0), ('combines', 'wall', None),
        ('annonces', 'pubcpu5', 0.90), ('annonces', 'forest', 1.0), ('annonces', 'wall', None),
        ('cohortes', 'births', 0.80), ('cohortes', 'forest', 1.0), ('cache', 'wall', 1.0), ('O1_place3', 'forest', 0.93),
        ('AA_o1place3', 'wall', None), ('AA_o1place3', 'domain', None), ('AA_o1place3', 'forest', None), ('AA_o1place3', 'prefix', None),
        ('AA_o1place3', 'single_pass', None)]
z95, z80 = NormInv(0.95), NormInv(0.80)
print('%-11s %-14s %5s %7s %8s %17s %6s %9s %8s' % ('levier', 'metrique', 'seuil', 'G', 'SE_logG', 'IC95 bootstrap', 'z', 'P(garde)', 'MDE80'))
out = []
for lever, metric, thr in rows:
    cp = data(LEVERS[lever], metric)
    obs = stat(cp)
    se, lo, hi = boot(cp)
    pk = Phi((math.log(thr) - obs) / se) if thr else float('nan')
    mde = math.exp(-(z95 + z80) * se)
    print('%-11s %-14s %5s %7.3f %8.4f   [%.3f, %.3f] %6.1f %9.3g %8.3f' % (lever, metric, thr, math.exp(obs), se, lo, hi, obs / se, pk, mde))
    out.append(dict(lever=lever, metric=metric, thr=thr, G=math.exp(obs), se=se, lo=lo, hi=hi, z=obs / se, p_keep=pk, mde80=mde))
json.dump(out, open(sys.argv[2], 'w'), indent=1)
