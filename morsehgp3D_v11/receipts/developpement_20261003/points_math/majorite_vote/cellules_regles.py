#!/usr/bin/env python3
"""Juge des cellules de l'utilisateur (v10, juge_final/verdict/cellules.py, LU et IMPORTE sans modification) applique
aux regles ecrites ici : H_1, H_{k+1}, H_{max(k+1, mcs)} (choix retenu par le developpeur v11), et le vote de la
these rendu hierarchique (VOTE[p, faces, kappa]). ER0h et ER0 sont rejugees par le code v10 comme temoins.

Le juge v10 construit FULL_K par Gamma_K exhaustif (vfull) ; nos regles lisent cet arbre via arbre.tree_from_vfull
(memes identifiants de noeuds) ; les faces du vote sont lues par l'oracle v11 (memes indices de sites) et rattachees
aux noeuds v10 par info['sommet_noeud'].

Usage : python3 -B cellules_regles.py REGLE[,REGLE...] > recus/cellules_REGLE.json
  REGLE parmi H1, Hk1, Hmcs, ER0h, ER0, VOTEg2, VOTEa2, VOTEa0, VOTEg2nc
"""
from fractions import Fraction
import json
import os
import sys
import time

sys.dont_write_bytecode = True
HERE = os.path.dirname(os.path.abspath(__file__))
os.environ['MHGP10_FIXTURES_SCRATCH'] = os.path.join(HERE, 'scratch_v10')
VERDICT = '/workspaces/E-HGP/build/v10-verrou-points/juge_final/verdict'
sys.path.insert(0, VERDICT)
import arbre  # noqa: E402
import regles as RG  # noqa: E402
import cellules as CEL  # noqa: E402  (v10, lecture seule)
import ver  # noqa: E402
from vrad import R  # noqa: E402

_ORIG = CEL.construire


def to_R(s):
    return R(dict(s.t))


_DEF = {}


def definition_for(T):
    key = id(T)
    return _DEF.get(key)


def our_rule(regle, T, info, prm, mcs):
    tree = arbre.tree_from_vfull(T)
    K = info['K']
    if regle.startswith('ER0hr'):
        kap = Fraction(int(regle[5:]))
        hang = RG.rule_er0hr(tree, 1, kap)
    elif regle in ('H1', 'Hk1', 'Hmcs'):
        m = {'H1': 1, 'Hk1': K + 1, 'Hmcs': max(K + 1, mcs)}[regle]
        hang = RG.rule_hm(tree, m)
    elif regle.startswith('VOTE'):
        faces = 'gabriel' if regle[4] == 'g' else 'all'
        p = int(regle[5])
        kappa = None if regle.endswith('nc') else Fraction(12)
        pts = CEL._CURRENT_POINTS
        d = arbre.definition(pts)
        data = RG.face_data(d, K, p, faces)
        parts, beta, S, inF, _vnode11 = data
        vnode = info['sommet_noeud']
        data10 = (parts, beta, S, inF, vnode)
        hang = []
        for x in range(T.n):
            cr, _repli = RG.vote_credits(d, K, x, data10)
            A = min(o for o, _v, _w in cr)
            date, owner, det = RG.majority_and_cone(tree, cr, Fraction(1), A, kappa)
            hang.append((date, owner, det))
    else:
        raise RuntimeError('regle inconnue %s' % regle)
    h = ver.Hier(T, [to_R(t) for t, _o, _d in hang], [o for _t, o, _d in hang])
    return h, hang


def construire(regle, T, info, prm, mcs):
    if regle in ('ER0', 'ER0h'):
        return _ORIG(regle, T, info, prm, mcs)
    if regle.startswith('ER0hv'):
        # variante ER-hv du verificateur v10 (ver_var, mode 'hv'), theta = 1/20 (ER0hv20), 1/5 (ER0hv5), 2/5 (ER0hv2.5)
        import ver_var
        theta = {'ER0hv20': Fraction(1, 20), 'ER0hv5': Fraction(1, 5), 'ER0hv25': Fraction(2, 5)}[regle]
        return ver_var.regle_var(T, CEL.LAMBDA_INFINI, prm['eta'], prm['kappa'], 1, 'hv', theta)
    return our_rule(regle, T, info, prm, mcs)


_full_orig = CEL.full


def full_tracking(points, K):
    CEL._CURRENT_POINTS = [tuple(p) for p in points]
    return _full_orig(points, K)


CEL.construire = construire
CEL.full = full_tracking
for r, dep in (('H1', False), ('Hk1', False), ('Hmcs', True), ('VOTEg2', False), ('VOTEa2', False),
               ('VOTEa0', False), ('VOTEg2nc', False), ('ER0hv20', False), ('ER0hv5', False), ('ER0hv25', False),
               ('ER0hr4', False), ('ER0hr6', False), ('ER0hr10', False), ('ER0hr16', False), ('ER0hr24', False)):
    CEL.DEPEND_MCS[r] = dep


def main():
    regles = sys.argv[1].split(',')
    with open(CEL.V2, 'rb') as f:
        v2 = json.loads(f.read())
    prm = {'lam': Fraction(9, 8), 'eta': Fraction(1), 'kappa': Fraction(12)}
    out = {}
    for regle in regles:
        t0 = time.time()
        bilan = {}
        cells = []
        for cel in CEL.cellules(v2):
            r = CEL.juger_cellule(cel, regle, prm, {})
            st = cel['statut']
            b = bilan.setdefault(st, [0, 0, 0, 0])
            if r['jugements']:
                b[0] += 1
                b[1] += 1 if r['passe'] else 0
                b[2] += r['jugements']
                b[3] += r['passes']
            cells.append({'nom': cel['nom'], 'statut': st, 'passes': r['passes'], 'jugements': r['jugements'],
                          'premier_viol': r['premier_viol'],
                          'rapport_base': r['rapport'].get('base') if r['rapport'] else None})
        anc = [c for c in cells if c['statut'] in ('ancree', 'ancree_1oct')]
        out[regle] = {'bilan': bilan, 'ancrees': [sum(c['passes'] for c in anc), sum(c['jugements'] for c in anc)],
                      'cellules': cells, 'secondes': round(time.time() - t0, 1)}
        sys.stderr.write('%s ancrees %d/%d (%.1fs)\n' % (regle, out[regle]['ancrees'][0], out[regle]['ancrees'][1],
                                                         time.time() - t0))
    print(json.dumps(out, indent=1, default=str, ensure_ascii=False))


if __name__ == '__main__':
    main()
