#!/usr/bin/env python3
"""Juge v10 des cellules (juge_final/verdict/cellules.py, importe SANS modification) applique a MES regles
(mes_regles.py), ecrites independamment du rapport majorite_vote.

Usage : python3 -B juge_mes_regles.py REGLE[,REGLE...] > recus/juge_REGLES.json
  REGLE : ER0h, ER0, ER0hr<k> (k entier), H1, Hk1, Hmcs (H_m en niveau), Hr1, Hrk1, Hrmcs (H^r_m en rayon)
"""
from fractions import Fraction
import json
import os
import sys
import time

sys.dont_write_bytecode = True
HERE = os.path.dirname(os.path.abspath(__file__))
os.environ['MHGP10_FIXTURES_SCRATCH'] = os.path.join(HERE, 'scratch_v10')
sys.path.insert(0, '/workspaces/E-HGP/build/v10-verrou-points/juge_final/verdict')
import mes_regles as M  # noqa: E402
import cellules as CEL  # noqa: E402  (v10, lecture seule)
import ver  # noqa: E402

CODES_GROUPE = {'T0': 'T0', 'Q1': 'Q1+Q1bis', 'Q1bis': 'Q1+Q1bis', 'Q2': 'Q2', 'Q3': 'Q3', 'QPi2': 'Q-Pi2',
                'Q4': 'Q4'}


def mes_pendaisons(regle, T, info, mcs):
    K = info['K']
    out = []
    for x in range(T.n):
        if regle == 'ER0h':
            out.append(M.er0h(T, x, 1, 12))
        elif regle == 'ER0':
            out.append(M.er0h(T, x, 1, 12, sans_heritage=True))
        elif regle.startswith('ER0hr'):
            out.append(M.er0h(T, x, 1, int(regle[5:]), relatif=True))
        elif regle in ('H1', 'Hk1', 'Hmcs'):
            m = {'H1': 1, 'Hk1': K + 1, 'Hmcs': max(K + 1, mcs)}[regle]
            out.append(M.hm_niveau(T, x, m))
        elif regle in ('Hr1', 'Hrk1', 'Hrmcs'):
            m = {'Hr1': 1, 'Hrk1': K + 1, 'Hrmcs': max(K + 1, mcs)}[regle]
            out.append(M.hm_rayon(T, x, m))
        elif regle.startswith('VOTE'):
            break
        else:
            raise RuntimeError('regle inconnue ' + regle)
    if regle.startswith('VOTE'):
        import vote_these as V
        mode = 'gabriel' if regle[4] == 'g' else 'toutes'
        p = int(regle[5])
        kappa = None if regle.endswith('nc') else 12
        S, bt = V.faces(CEL._POINTS_COURANTS, K, p, mode)
        out = [V.vote_point(T, info, S, bt, x, kappa) for x in range(T.n)]
    return out


def construire(regle, T, info, prm, mcs):
    if regle.startswith('ERhv'):  # ER-hv du verificateur v10 (ver_var), temoin : code v10, pas le mien
        import ver_var
        theta = {'ERhv20': Fraction(1, 20), 'ERhv5': Fraction(1, 5), 'ERhv25': Fraction(2, 5)}[regle]
        return ver_var.regle_var(T, CEL.LAMBDA_INFINI, Fraction(1), Fraction(12), 1, 'hv', theta)
    hang = mes_pendaisons(regle, T, info, mcs)
    return ver.Hier(T, [d for d, _o in hang], [o for _d, o in hang]), hang


CEL.construire = construire
_full_v10 = CEL.full


def _full_suivi(points, K):
    CEL._POINTS_COURANTS = [tuple(p) for p in points]
    return _full_v10(points, K)


CEL.full = _full_suivi
for r in ('ERhv20', 'ERhv5', 'ERhv25', 'VOTEg2', 'VOTEg2nc', 'VOTEa2', 'VOTEa0', 'VOTEa2nc', 'VOTEg0'):
    CEL.DEPEND_MCS[r] = False
for r in ('ER0h', 'ER0', 'H1', 'Hk1', 'Hr1', 'Hrk1') + tuple('ER0hr%d' % k for k in (4, 6, 10, 16, 24, 2, 3, 5)):
    CEL.DEPEND_MCS[r] = False
CEL.DEPEND_MCS['Hmcs'] = True
CEL.DEPEND_MCS['Hrmcs'] = True


def main():
    regles = sys.argv[1].split(',')
    with open(CEL.V2, 'rb') as f:
        v2 = json.loads(f.read())
    out = {}
    for regle in regles:
        t0 = time.time()
        par_groupe = {}
        cells = []
        forcees = [0, 0]
        for cel in CEL.cellules(v2):
            r = CEL.juger_cellule(cel, regle, {}, {})
            st = cel['statut']
            if st in ('ancree', 'ancree_1oct'):
                g = CODES_GROUPE[cel['code']]
                b = par_groupe.setdefault(g, [0, 0])
                b[0] += r['passes']
                b[1] += r['jugements']
            if st == 'forcee':
                forcees[0] += r['passes']
                forcees[1] += r['jugements']
            cells.append({'nom': cel['nom'], 'statut': st, 'passes': r['passes'], 'jugements': r['jugements'],
                          'premier_viol': r['premier_viol']})
        tot = [sum(v[0] for v in par_groupe.values()), sum(v[1] for v in par_groupe.values())]
        out[regle] = {'ancrees': tot, 'par_groupe': par_groupe, 'forcees': forcees, 'cellules': cells,
                      'secondes': round(time.time() - t0, 1)}
        sys.stderr.write('%s ancrees %d/%d %s forcees %d/%d (%.1fs)\n' % (
            regle, tot[0], tot[1], json.dumps(par_groupe), forcees[0], forcees[1], time.time() - t0))
    print(json.dumps(out, indent=1, default=str, ensure_ascii=False))


if __name__ == '__main__':
    main()
