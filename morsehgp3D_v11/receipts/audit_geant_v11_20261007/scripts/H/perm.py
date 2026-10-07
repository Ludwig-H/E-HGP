"""Test de randomisation (inversion des etiquettes dans chaque paire rep x cellule) de la statistique du juge,
SD de log G sous H0, MDE, probabilite de garder au seuil ecrit si l'effet vrai = l'effet observe."""
import json, math, statistics, sys, random
sys.path.insert(0, sys.argv[1])
from levers import LEVERS, METRICS, cells, load, FR

def NormInv(p):
    # inverse de la loi normale (Acklam)
    a = [-3.969683028665376e+01, 2.209460984245205e+02, -2.759285104469687e+02, 1.383577518672690e+02, -3.066479806614716e+01, 2.506628277459239e+00]
    b = [-5.447609879822406e+01, 1.615858368580409e+02, -1.556989798598866e+02, 6.680131188771972e+01, -1.328068155288572e+01]
    c = [-7.784894002430293e-03, -3.223964580411365e-01, -2.400758277161838e+00, -2.549732539343734e+00, 4.374664141464968e+00, 2.938163982698783e+00]
    d = [7.784695709041462e-03, 3.224671290700398e-01, 2.445134137142996e+00, 3.754408661907416e+00]
    pl = 0.02425
    if p < pl:
        q = math.sqrt(-2 * math.log(p)); return (((((c[0]*q+c[1])*q+c[2])*q+c[3])*q+c[4])*q+c[5]) / ((((d[0]*q+d[1])*q+d[2])*q+d[3])*q+1)
    if p > 1 - pl:
        q = math.sqrt(-2 * math.log(1 - p)); return -(((((c[0]*q+c[1])*q+c[2])*q+c[3])*q+c[4])*q+c[5]) / ((((d[0]*q+d[1])*q+d[2])*q+d[3])*q+1)
    q = p - 0.5; r = q * q
    return (((((a[0]*r+a[1])*r+a[2])*r+a[3])*r+a[4])*r+a[5])*q / (((((b[0]*r+b[1])*r+b[2])*r+b[3])*r+b[4])*r+1)

def Phi(x):
    return 0.5 * (1 + math.erf(x / math.sqrt(2)))

def data(sessions, metric):
    f = METRICS[metric]
    out = []
    for path, new, base, mlab in sessions:
        rep = load(path)
        cs = cells(rep, new, base)
        for fr in FR:
            out.append([(f(a), f(b)) for _, a, b, _ in cs[fr]])
    return out

def stat(cellpairs):
    logs = []
    for pairs in cellpairs:
        xs = [p[0] for p in pairs]; ys = [p[1] for p in pairs]
        logs.append(math.log(statistics.median(xs) / statistics.median(ys)))
    return statistics.mean(logs)

def perm(cellpairs, B=20000, seed=7):
    rng = random.Random(seed)
    obs = stat(cellpairs)
    sims = []
    for _ in range(B):
        cp = [[(a, b) if rng.random() < 0.5 else (b, a) for a, b in pairs] for pairs in cellpairs]
        sims.append(stat(cp))
    sd = statistics.pstdev(sims)
    p2 = (sum(1 for s in sims if abs(s) >= abs(obs) - 1e-15) + 1) / (B + 1)
    return obs, sd, p2

z95 = NormInv(0.95); z80 = NormInv(0.80); z975 = NormInv(0.975)
rows = [('G1_AVX2', 'sp+prefix', 0.85), ('G1_AVX2', 'domain', 1.0), ('G1_AVX2', 'wall', None), ('G1_AVX2', 'forest', None),
        ('frontiere', 'prefix', 0.80), ('frontiere', 'domain', 1.0), ('frontiere', 'wall', None), ('frontiere', 'forest', None),
        ('graines', 'cells5', 0.85), ('graines', 'forest', 1.0), ('graines', 'wall', None), ('graines', 'domain', None),
        ('combines', 'cells+closes5', 0.90), ('combines', 'forest', 1.0), ('combines', 'wall', None), ('combines', 'domain', None),
        ('annonces', 'pubcpu5', 0.90), ('annonces', 'forest', 1.0), ('annonces', 'wall', None), ('annonces', 'domain', None),
        ('cohortes', 'births', 0.80), ('cohortes', 'forest', 1.0), ('cohortes', 'wall', None),
        ('cache', 'wall', 1.0), ('tas', 'wall', None), ('thp', 'wall', None), ('o2', 'forest', None), ('o2', 'wall', None),
        ('constantes', 'forest', None), ('constantes', 'wall', None), ('v3', 'forest', None), ('v3', 'wall', None),
        ('O1_place3', 'forest', 0.93), ('O1_place3', 'wall', None),
        ('AA_o1place3', 'wall', None), ('AA_o1place3', 'domain', None), ('AA_o1place3', 'forest', None), ('AA_o1place3', 'prefix', None)]
res = []
print('%-11s %-14s %6s %7s %8s %8s %8s %9s %9s %9s' % ('levier', 'metrique', 'seuil', 'G', 'sd_logG', 'p_perm', 'MDE80', 'seuil5%', 'P(garde)', 'z'))
for lever, metric, thr in rows:
    cp = data(LEVERS[lever], metric)
    obs, sd, p2 = perm(cp)
    mde = math.exp(-(z95 + z80) * sd)
    thr5 = math.exp(-z95 * sd)
    pk = Phi((math.log(thr) - obs) / sd) if thr is not None and thr < 1 else (Phi((math.log(thr) - obs) / sd) if thr else float('nan'))
    print('%-11s %-14s %6s %7.3f %8.4f %8.2g %8.3f %9.3f %9.3g %9.1f' % (lever, metric, thr, math.exp(obs), sd, p2, mde, thr5, pk, obs / sd))
    res.append(dict(lever=lever, metric=metric, thr=thr, G=math.exp(obs), sd_logG=sd, p_perm=p2, mde80=mde, thr5=thr5, p_keep=pk))
json.dump(res, open(sys.argv[2], 'w'), indent=1)
