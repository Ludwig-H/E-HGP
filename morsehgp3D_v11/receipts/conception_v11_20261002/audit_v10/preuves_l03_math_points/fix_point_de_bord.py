"""Principe de l'utilisateur (reponse Q-Pi2) : « un point de bord ne fait pas exister un cluster a lui seul ».
Figure SANS filament : amas de 8 points + x a 900, plus un groupe lointain de 4 points ; K = 2, mcs = 9.
(a) binaire v10 : entrees cover, cover1, core ; (b) ER0h(1, 12) du juge final (code du juge, lecture seule)."""
import math, os, sys
os.environ['MHGP10_FIXTURES_SCRATCH'] = '/tmp/v11-audit/l03_math_points/juge/scratch'
sys.dont_write_bytecode = True
import natif as N
from fractions import Fraction
O = (10000, 10000, 10000)
def add(o, d): return tuple(a + b for a, b in zip(o, d))
amas = [add(O, d) for d in [(-900, 0, 0), (-880, 200, 0), (-880, -200, 0), (-880, 0, 200), (-880, 0, -200), (-1100, 0, 0), (-1080, 150, 100), (-1080, -150, -100)]]
x = [O]
loin = [add(O, d) for d in [(60000, 0, 0), (60150, 0, 0), (60000, 150, 0), (60150, 150, 40)]]
P = x + amas + loin
names = ['x'] + ['c%d' % i for i in range(8)] + ['g%d' % i for i in range(4)]
mcs = 9
for entry in ('cover', 'cover1', 'core'):
    res = N.run_cluster(P, 2, entry, mcs=mcs)
    s = [(math.sqrt(a), p) for a, p in N.suite(res['tree'], mcs)]
    print('binaire v10 --entry=%-6s : premier bloc >= %d points : %s' % (entry, mcs, [(round(r, 3), ['{' + ','.join(names[i] for i in b) + '}' for b in p]) for r, p in s if p][:2]))
VER = '/workspaces/E-HGP/build/v10-verrou-points/juge_final/verif_echelle_relative'
sys.path.insert(0, VER)
import vfull, ver, ver_var
T, _ = vfull.full_gamma([tuple(p) for p in P], 2)
hh, rh = ver_var.regle_var(T, Fraction(10 ** 12), Fraction(1), Fraction(12), 1, 'h')
prev = None
for r in hh.rayons_changement():
    part = hh.partition(r)
    gros = [b for b in part if len(b) >= mcs]
    if gros:
        print('ER0h(1,12) du juge, puis condensation a mcs = %d : premier cluster a r = %.3f : %s' % (mcs, float(r), [sorted(names[i] for i in b) for b in gros]))
        break
print('date d entree de x sous ER0h : %.3f' % float(hh.dates[0]))
