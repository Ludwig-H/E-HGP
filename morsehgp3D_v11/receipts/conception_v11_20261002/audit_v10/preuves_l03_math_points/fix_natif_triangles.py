import math
import natif as N
names = 'ABCDEF'
FIX = {
    'P1': [(268, 3000, 0), (268, 1000, 0), (2000, 2000, 0), (4000, 2000, 0), (5732, 3000, 0), (5732, 1000, 0)],
    'P2': [(268, 3000, 0), (268, 1000, 0), (2000, 2000, 0), (3998, 2000, 0), (5730, 3000, 0), (5730, 1000, 0)],
    'T1_1700': [(268, 3000, 0), (268, 1000, 0), (2000, 2000, 0), (3700, 2000, 0), (5432, 3000, 0), (5432, 1000, 0)],
}
for nom, P in FIX.items():
    for entry in ('core', 'cover', 'cover1'):
        for mcs in (2, 3):
            res = N.run_cluster(P, 2, entry, mcs=mcs)
            if res['code'] != 0:
                print(nom, entry, 'code', res['code'], res['stdout'], res['stderr'])
                continue
            if mcs == 2:
                print('%s --entry=%s : hierarchie de points (blocs >= 2)' % (nom, entry))
                for a, p in N.suite(res['tree'], 2):
                    print('    r = %10.4f : %s' % (math.sqrt(a), ' | '.join(''.join(names[i] for i in b) for b in p) or '(aucun)'))
            print('    etiquettes EOM z=1 mcs=%d : %s' % (mcs, res['labels']))
