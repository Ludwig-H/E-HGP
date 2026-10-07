import math
import natif as N
O = (10000, 10000, 10000)
def add(o, d): return tuple(a + b for a, b in zip(o, d))
FIX = {
    'Q2 (K=2) x a b1 b2': (2, 'xab1', ['x', 'a', 'b1', 'b2'], [(1000, 1000, 1000), (1100, 1000, 1000), (1010, 1120, 1000), (1010, 1119, 1016)]),
    'Q3 (K=2) filament/amas': (2, None, ['x', 'f1', 'f2', 'f3', 'f4', 'f5', 'c0', 'c1', 'c2', 'c3', 'c4', 'c5', 'c6', 'c7'],
        [O, add(O, (700, 3, 0)), add(O, (1401, -2, 0)), add(O, (2100, 4, 0)), add(O, (2802, 0, 0)), add(O, (3500, -3, 0)),
         add(O, (-900, 0, 0)), add(O, (-880, 200, 0)), add(O, (-880, -200, 0)), add(O, (-880, 0, 200)), add(O, (-880, 0, -200)),
         add(O, (-1100, 0, 0)), add(O, (-1080, 150, 100)), add(O, (-1080, -150, -100))]),
    'Q4 (K=3) chaine/tetraedres': (3, None, ['C', 'P', 'Q', 'R', 'm', 'D', 'P2', 'Q2', 'R2'],
        [(3000, 3000, 3000), (4000, 4000, 3000), (4000, 3000, 4000), (3000, 4000, 4000), (2600, 2600, 2600), (2200, 2200, 2200),
         (1200, 1200, 2200), (1200, 2200, 1200), (2200, 1200, 1200)]),
}
for nom, (K, _, names, P) in FIX.items():
    for entry in ('core', 'cover', 'cover1'):
        res = N.run_cluster(P, K, entry, mcs=2)
        if res['code'] != 0:
            print(nom, entry, 'code', res['code'], res['stdout'], res['stderr'])
            continue
        print('%s --entry=%s : hierarchie de points (blocs >= 2)' % (nom, entry))
        for a, p in N.suite(res['tree'], 2):
            print('    r = %10.4f : %s' % (math.sqrt(a), ' | '.join('{' + ','.join(names[i] for i in b) + '}' for b in p) or '(aucun)'))
