#!/usr/bin/env python3
"""Hierarchies rendues sur les quatre fixtures soumises a l'utilisateur en v10 (QUESTIONS_UTILISATEUR.md),
pour la fermeture (m = 1 et m = k + 1), H_1, H_{k+1}, la fermeture exclusive EC_{k+1}, first_{k+1} et core.

    python3 cibles_utilisateur.py > resultats/cibles_utilisateur.txt

Reponses de l'utilisateur (citees dans le contexte de la mission) : Q1 (b) ABC|DEF avant la fusion ; Q2 (a) x avec a ;
Q3 (a) x dans le filament ; Q4 (a) attendre, puis la chaine CmD. Ce sont des preferences, pas un oracle.
Affichage : a chaque niveau ou la hierarchie change, les blocs d'au moins deux sites (rayons en flottant).
"""
import math
import sys

sys.dont_write_bytecode = True
import lib_fermeture as lf  # noqa: E402


def dendro(u, names):
    n = len(u)
    levels = sorted(set(u[i][j] for i in range(n) for j in range(n)))
    out, last = [], None
    for lev in levels:
        blocks = [b for b in lf.pr.blocks_at(u, lev) if len(b) >= 2]
        text = ' | '.join(''.join(names[i] for i in sorted(b)) if all(len(names[i]) == 1 for i in b)
                          else '{' + ','.join(names[i] for i in sorted(b)) + '}' for b in blocks)
        if text != last:
            out.append('r=%.3f : %s' % (math.sqrt(lev), text or '-'))
            last = text
    return out


def show(title, pts, names, k):
    n = len(pts)
    defn = lf.Definition(pts)
    res = defn.order(k)
    print('\n## %s (n=%d, k=%d) ; fusion FULL racine r=%.3f' % (title, n, k,
                                                               math.sqrt(max(nd.level for nd in res.nodes))))
    rules = []
    for m in (1, k + 1):
        rules.append(('fermeture m=%d' % m, lf.closure(res, n, m)['u']))
    ref1, tree = lf.faithful_rules(res, n, 1)
    refq, _ = lf.faithful_rules(res, n, k + 1)
    rules.append(('H_1', lf.ultrametric_of(ref1['margin'], tree)))
    rules.append(('H_%d' % (k + 1), lf.ultrametric_of(refq['margin'], tree)))
    rules.append(('EC_%d' % (k + 1), lf.ultrametric_of(lf.ec_hanging(res, n, k + 1), tree)))
    rules.append(('first_%d' % (k + 1), lf.ultrametric_of(refq['first'], tree)))
    rules.append(('core', lf.ultrametric_of(ref1['core'], tree)))
    for name, u in rules:
        print('  %s' % name)
        for line in dendro(u, names):
            print('     ' + line)


def main():
    q1 = [(268, 3000, 0), (268, 1000, 0), (2000, 2000, 0), (3700, 2000, 0), (5432, 3000, 0), (5432, 1000, 0)]
    show('Q1 pont 1700 ; reponse (b) ABC|DEF avant la fusion', q1, 'ABCDEF', 2)
    q2 = [(1000, 1000, 1000), (1100, 1000, 1000), (1010, 1120, 1000), (1010, 1119, 1016)]
    show('Q2 ; reponse (a) x avec a (x=x, a=a, b1=b, b2=c)', q2, 'xabc', 2)
    O = (10000, 10000, 10000)

    def off(dx, dy, dz):
        return (O[0] + dx, O[1] + dy, O[2] + dz)
    q3 = [off(0, 0, 0), off(700, 3, 0), off(1401, -2, 0), off(2100, 4, 0), off(2802, 0, 0), off(3500, -3, 0),
          off(-900, 0, 0), off(-880, 200, 0), off(-880, -200, 0), off(-880, 0, 200), off(-880, 0, -200),
          off(-1100, 0, 0), off(-1080, 150, 100), off(-1080, -150, -100)]
    show('Q3 ; reponse (a) x dans le filament (x, filament 1-5, amas a-h)', q3, 'x12345abcdefgh', 2)
    q4 = [(3000, 3000, 3000), (4000, 4000, 3000), (4000, 3000, 4000), (3000, 4000, 4000), (2600, 2600, 2600),
          (2200, 2200, 2200), (1200, 1200, 2200), (1200, 2200, 1200), (2200, 1200, 1200)]
    show('Q4 ; reponse (a) attendre, puis la chaine CmD (C, PQR, m, D, P2Q2R2 = STU)', q4, 'CPQRmDSTU', 3)
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
