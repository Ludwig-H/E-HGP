#!/usr/bin/env python3
"""Audit L03 : temoin d'entree interne de `cover` (auditeur continu, `internal_k3`, K = 3, six sites non generiques).
Rejeu independant : (1) oracle exact de cet audit (alpha_3, niveaux de FULL_3) ; (2) binaire v10 de afb081774
(`--entry=cover --tree`) ; (3) condensation publiee contre cohortes sur l'arbre exporte (mcs = 6, z = 1 et 2)."""
import sys, os, tempfile
from fractions import Fraction
sys.dont_write_bytecode = True
HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import numpy as np
import oracle_l03 as O
import natif as N
import impact_condensation as IC

P = [(15, 4, 0), (5, 4, 0), (7, 8, 0), (7, 0, 0), (1, 4, 0), (0, 4, 1)]
K = 3
T = O.Tour(P, K)
print('niveaux de FULL_3 (rayons carres) :', [str(a) for a in T.levels])
for a in T.levels:
    print('  a =', a, ' composantes (amas discrets) :', sorted(sorted(c) for c in T.amas_discrets(a)))
print('premieres couvertures alpha_3^2 :')
for x in range(len(P)):
    a, Fs = T.cover_entries(x)
    print('  site', x, P[x], ' alpha^2 =', a, ' K-parties a egalite :', [sorted(F) for F in Fs], ' composantes distinctes :', len(T.cover_tie_components(x)[1]))
tmp = tempfile.mkdtemp(prefix='ci_', dir='/tmp/v11-audit/l03_math_points/work')
src = os.path.join(tmp, 'in.u32le'); np.ascontiguousarray(np.array(P, dtype='<u4')).tofile(src)
import subprocess
for mcs in (2, 6):
    for z in (1.0, 2.0):
        lab = os.path.join(tmp, 'lab'); tree = os.path.join(tmp, 'tree')
        r = subprocess.run(['/tmp/v11-audit/l03_math_points/build_v10/mhgp10_cluster', src, lab, '--k=3', '--mcs=%d' % mcs, '--z=%r' % z, '--entry=cover',
                            '--tree=' + tree, '--threads=1'], capture_output=True, text=True)
        if r.returncode != 0:
            print('refus', r.stdout[:200], r.stderr[:200]); continue
        lv, nd, pt = IC.read_tree(tree)
        if mcs == 2 and z == 1.0:
            print('arbre exporte : niveaux', lv)
            print('  noeuds (rang, parent) :', nd)
            print('  points (pid, noeud, rang d entree, poids) :', pt)
        l1, s1, _, m1 = IC.condense(lv, nd, pt, mcs, z, 'v10')
        l2, s2, dec, m2 = IC.condense(lv, nd, pt, mcs, z, 'cohortes')
        print('mcs =', mcs, 'z =', z, ': stabilites tete publiee', [round(v, 6) for v in s1], ' cohortes', [round(v, 6) for v in s2], ' morts par cohorte :', dec,
              ' etiquettes', l1.tolist(), l2.tolist(), ' binaire', np.fromfile(lab, dtype='<i4').tolist())
for f in os.listdir(tmp): os.remove(os.path.join(tmp, f))
os.rmdir(tmp)
print('attendu (recu de l auditeur) : mcs = 6, z = 1 : 88/65 =', 88 / 65, 'contre 6/5 ; z = 2 : 1294/4225 =', 1294 / 4225, 'contre 6/25 =', 6 / 25)
