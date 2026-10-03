#!/usr/bin/env python3
"""Fixtures exactes (coordonnees entieres). Sources :
  - EQUILATERAL, FIVE : morsehgp3D_v11/bench/points_gate.py ;
  - T0 (quatre bases), Q1 (T1_1700), Q2 (S17), Q3 (filament), Q4 (T6, K = 3) : catalogue v10
    juge_final/cibles/fixtures_catalogue_v2.json (lu, jamais modifie) ; coordonnees recopiees de
    revision_cible/QUESTIONS_UTILISATEUR.md pour Q1-Q4 et controlees contre le catalogue (controle_catalogue()).
"""
import json

EQUILATERAL = [tuple(c + 2 for c in p) for p in ((-1, -1, 0), (-1, 0, -1), (0, 0, 0), (1, 1, 0), (2, 2, 0), (2, 1, 1))]
FIVE = [(6, 2, 0), (0, 0, 0), (0, 4, 0), (12, 0, 0), (12, 4, 0)]

Q1 = dict(K=2, names=['A', 'B', 'C', 'D', 'E', 'F'],
          points=[(268, 3000, 0), (268, 1000, 0), (2000, 2000, 0), (3700, 2000, 0), (5432, 3000, 0), (5432, 1000, 0)])
Q2 = dict(K=2, names=['x', 'a', 'b1', 'b2'],
          points=[(1000, 1000, 1000), (1100, 1000, 1000), (1010, 1120, 1000), (1010, 1119, 1016)])
O3 = (10000, 10000, 10000)


def _o(d):
    return tuple(O3[i] + d[i] for i in range(3))


Q3 = dict(K=2, names=['x', 'f1', 'f2', 'f3', 'f4', 'f5', 'c0', 'c1', 'c2', 'c3', 'c4', 'c5', 'c6', 'c7'],
          points=[_o((0, 0, 0)), _o((700, 3, 0)), _o((1401, -2, 0)), _o((2100, 4, 0)), _o((2802, 0, 0)),
                  _o((3500, -3, 0)), _o((-900, 0, 0)), _o((-880, 200, 0)), _o((-880, -200, 0)), _o((-880, 0, 200)),
                  _o((-880, 0, -200)), _o((-1100, 0, 0)), _o((-1080, 150, 100)), _o((-1080, -150, -100))])
Q4 = dict(K=3, names=['C', 'P', 'Q', 'R', 'm', 'D', 'P2', 'Q2', 'R2'],
          points=[(3000, 3000, 3000), (4000, 4000, 3000), (4000, 3000, 4000), (3000, 4000, 4000), (2600, 2600, 2600),
                  (2200, 2200, 2200), (1200, 1200, 2200), (1200, 2200, 1200), (2200, 1200, 1200)])

CATALOGUE = '/workspaces/E-HGP/build/v10-verrou-points/juge_final/cibles/fixtures_catalogue_v2.json'
_CAT = None


def catalogue():
    global _CAT
    if _CAT is None:
        with open(CATALOGUE) as f:
            _CAT = json.load(f)
    return _CAT


def from_catalogue(name):
    e = [f for f in catalogue()['fixtures'] if f['name'] == name][0]
    names = list(e['points'])
    return dict(K=e['K'], names=names, points=[tuple(e['points'][x]) for x in names], entry=e)


def t0_bases():
    out = {}
    for nm in ('T0_P1_aretes_courtes', 'T0_P2_pont_court', 'T0_S_plan_egalites', 'T0_S_3D_equilateral'):
        out[nm] = from_catalogue(nm + '__mcs2-3')
    return out


def controle_catalogue():
    """Les coordonnees recopiees de Q1-Q4 sont celles des bases du catalogue v2 (memes noms, memes points)."""
    ok = {}
    for nm, fx in (('Q1_T1_1700_K2__mcs3', Q1), ('Q2_S17_K2__mcs2', Q2), ('Q3_filament_K2__mcs2-6', Q3),
                   ('Q4_T6_chaine_K3__mcs2-3', Q4)):
        c = from_catalogue(nm)
        ok[nm] = (c['K'] == fx['K'] and dict(zip(c['names'], c['points'])) == dict(zip(fx['names'], fx['points'])))
    return ok


if __name__ == '__main__':
    print(controle_catalogue())
