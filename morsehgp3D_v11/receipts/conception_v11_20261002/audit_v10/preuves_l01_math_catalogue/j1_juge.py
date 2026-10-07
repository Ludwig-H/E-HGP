#!/usr/bin/env python3
"""Juge d'echantillon J1 (audit L01, 2 oct. 2026) : exactitude des boules EMISES a l'echelle.

Pour chaque boule tiree du dump canonique : centre exact depuis S* (Gram, Fraction), recensement BRUT sur tous les
sites de la trame (filtre flottant large, marge relative 1e-6, puis decision exacte en Fraction), puis controle de
  - I et U (ensembles complets, ordre du dump = ordre de Morton) ;
  - p, u, drapeau de coquille etendue ;
  - q_min et S* (premier support, tailles 2, 3, 4, dans l'ordre lexicographique des indices de Morton) ;
  - admission p + q_min <= K + 1.
Usage : j1_juge.py NUAGE.u32le ECHANTILLON.txt K   (0 conforme, 1 ecart, 3 plancher)"""
import itertools
import os
import sys
from fractions import Fraction as Fr

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import oracle_indep as O  # noqa: E402


def main():
    cloud, sample, K = sys.argv[1], sys.argv[2], int(sys.argv[3])
    P = np.fromfile(cloud, dtype='<u4').reshape(-1, 3).astype(np.int64)
    Pf = P.astype(np.float64)
    conv = lambda s: [tuple(int(v) for v in t.split(',')) for t in s.split()]  # noqa: E731
    n = fails = 0
    stats = dict(q2=0, q3=0, q4=0, etendues=0, sites_recenses=0, pmax=0)
    for line in open(sample):
        head, sup, inner, shell = line.rstrip('\n').split('|')
        rank, q, p, u, flags = (int(t) for t in head.split())
        S, I, U = conv(sup), conv(inner), conv(shell)
        n += 1
        b = O.ball_of(tuple(range(len(S))), S)
        err = None
        if b is None:
            err = 'S* n\'est pas un support'
        else:
            c, r2 = b
            cf = np.array([float(x) for x in c])
            d2 = ((Pf - cf) ** 2).sum(axis=1)
            near = np.nonzero(d2 <= float(r2) * (1 + 1e-6) + 1e-6)[0]
            bI, bU = [], []
            for i in near:
                x = tuple(int(v) for v in P[i])
                e = sum((Fr(x[j]) - c[j]) ** 2 for j in range(3))
                if e < r2:
                    bI.append(x)
                elif e == r2:
                    bU.append(x)
            stats['sites_recenses'] += len(near)
            bI.sort(key=O.morton)
            bU.sort(key=O.morton)
            if bI != I:
                err = 'interieur : %d sites bruts contre %d publies' % (len(bI), len(I))
            elif bU != U:
                err = 'coquille : %d sites bruts contre %d publies' % (len(bU), len(U))
            elif p != len(I) or u != len(U):
                err = 'poids p ou u'
            elif (flags & 1) != (1 if len(U) > q else 0) or flags & 2:
                err = 'drapeaux'
            elif p + q > K + 1:
                err = 'boule hors admission'
            else:
                first = None
                for size in (2, 3, 4):
                    for T in itertools.combinations(range(len(U)), size):
                        bb = O.ball_of(T, U)
                        if bb is not None and bb[0] == c and bb[1] == r2:
                            first = [U[t] for t in T]
                            break
                    if first is not None:
                        break
                if first != S or len(S) != q:
                    err = 'support canonique : %s publie, %s attendu' % (S, first)
        if err:
            fails += 1
            if fails <= 5:
                print('ECART %s\n  %s' % (err, line.rstrip()))
        else:
            stats['q%d' % q] += 1
            stats['etendues'] += flags & 1
            stats['pmax'] = max(stats['pmax'], p)
    print('j1 boules %d ecarts %d %s' % (n, fails, ' '.join('%s=%d' % kv for kv in stats.items())))
    return 1 if fails else (3 if n < 1000 else 0)


if __name__ == '__main__':
    sys.exit(main())
