#!/usr/bin/env python3
"""Porte de l'oracle borne des supports d'ordre K (tranche S1 de la sortie parametree de mhgp12).

    python3 test_supports.py                  faits graves (fixtures de la spec, paragraphe 2.9, et des audits
                                              1bf4be68f, de4ab58a8 et aef7182b3 : temoins D2, E5, cube a K1), puis la
                                              suite : fixtures et familles a positions distinctes, K <= min(5, n - 1) ;
                                              zero ecart, invariance par permutation effective, reetiquetage et
                                              translation entiere, compteurs exacts, planchers, empreintes des sorties
    python3 test_supports.py --inject=NOM     mutant de ref_mutants.SUPPORT_MUTANTS : 4 s'il est tue par sa cause
    python3 test_supports.py --list-mutants
    python3 test_supports.py --dump=NOM       sortie canonique (JSON) d'une fixture, a tous ses ordres
    python3 test_supports.py --suite=primitives
                                              suite longue des primitives sur sphere5 (24 sites de x^2 + y^2 + z^2 = 5,
                                              translates de (2, 2, 2) ; apport des auditeurs du 5 octobre 2026) : Q_b
                                              par Gram (12, 24 et 792 supports), N_j pour j <= 4 par combinaisons,
                                              comptes de K1 a K3 par les formules du lemme G, refus explicite de la
                                              force brute du lemme F (budget) ; ne qualifie pas tout S1 a 24 sites

Codes de sortie (ARCHITECTURE.md de la v11, paragraphe 5) : 0 conforme ; 1 ecart (fait grave, lemme viole, empreinte
d'une fixture, invariance ; une exception levee dans un fait ou dans la contre-epreuve d'invariance est un ecart,
jamais une trace Python) ; 2 refus d'usage ; 3 plancher, compteur exact ou empreinte de la suite ; 4 mutant tue
(--inject seulement ; un mutant qui survit rend 0 et ecrit mutant_survives).

L'oracle (hgp12_ref/supports.py) et le generateur des familles sont charges SANS le paquet hgp12_ref (ni __init__, ni
constructive, ni judge, ni dumps) par ref_mutants.load_private : l'oracle ne s'appuie que sur l'etage A. Python 3.10
nu ; aucun assert : la porte rend le meme code et la meme ligne sous python3 -O.
"""
import hashlib
import json
import os
import sys
from fractions import Fraction
from math import comb

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import ref_mutants  # noqa: E402

OK, DISAGREEMENT, REFUSAL, FLOOR, MUTANT_KILLED = 0, 1, 2, 3, 4
PACKAGE = os.path.join(HERE, 'hgp12_ref')
STAGE = ref_mutants.load_private(PACKAGE, 'hgp12_supports_stage_a')
FAMILIES = STAGE['families']

# ---------------------------------------------------------------- fixtures


def _plane(pts):
    return [(x, y, 0) for x, y in pts]


def _circle(n, perturbed):
    """Cercle de l'audit carrier (1bf4be68f), immersion entiere : t = 1/n, echelle d = n^2 + 1, centre (d, d).
    A, B, C, D aux points cardinaux ; B remplace par B_t = ((n + 1)^2, 2 n^2) s'il est perturbe (meme cercle)."""
    d = n * n + 1
    b = ((n + 1) ** 2, 2 * n * n, 0) if perturbed else (d, 2 * d, 0)
    return [(2 * d, d, 0), b, (0, d, 0), (d, 0, 0)]


SPHERE5 = [(5, 0, 0), (-5, 0, 0), (0, 5, 0), (0, -5, 0), (0, 0, 5), (0, 0, -5),
           (3, 4, 0), (-3, -4, 0), (0, 3, 4), (0, -3, -4), (4, 0, 3), (-4, 0, -3)]
CIRCLE5 = [(5, 0), (-5, 0), (0, 5), (0, -5), (3, 4), (-3, 4), (3, -4), (-3, -4), (4, 3), (-4, 3), (4, -3), (-4, -3)]
SPHERE50 = [(x + 10, y + 10, z + 10) for x in range(-7, 8) for y in range(-7, 8) for z in range(-7, 8)
            if x * x + y * y + z * z == 50]

# (nom, points, ordres de la spec, ce que la fixture grave) ; spec = SPECIFICATION_FINALE.md, paragraphe 2.9
SPEC = (
    ('carre', _plane([(0, 0), (2, 0), (2, 2), (0, 2)]), (1, 2, 3, 4),
     'spec n. 1 : quatre cotes puis la diagonale ; naissance etendue a K3 ; cofaces 0 a K4'),
    ('triangle_droit', _plane([(0, 0), (4, 0), (0, 3)]), (1, 2),
     'spec n. 2 : hypotenuse 25/4, m = 3, Q_b reduit a l\'hypotenuse, origine orpheline'),
    ('growth_abcz', _plane([(1, 8), (5, 10), (9, 8), (5, 0)]), (3,),
     'spec n. 3 : naissance ABC a 16, boule interne a 25 de supports BZ et ACZ'),
    ('passagere', _plane([(0, 0), (2, 2), (4, 0), (8, 0)]), (1,),
     'spec n. 4 : cellule passagere, role fusion avec components 1'),
    ('triangle_aigu', _plane([(0, 0), (2, 0), (1, 2)]), (2,),
     'spec n. 5 : 25/16, fusion a trois enfants (audit incidences)'),
    ('ligne3', _plane([(0, 0), (1, 0), (2, 0)]), (2,), 'spec n. 6 : I = {1}, kparties 3, traces 2 (audit incidences)'),
    ('triangle_equilateral', [(0, 0, 0), (2, 2, 0), (2, 0, 2)], (2,),
     'spec n. 7, audit qb : evenement faible p + q - 1 = K, fusion a trois enfants a 8/3'),
    ('tetraedre_k5', [(20, 20, 20), (20, 0, 0), (0, 20, 0), (0, 0, 20), (10, 10, 10), (11, 10, 10), (10, 11, 10)],
     (5,), 'spec n. 8, audit plateau (translate de +10) : six naissances a 200, quatre faces a 800/3'),
    ('cube', [(x, y, z) for x in (0, 2) for y in (0, 2) for z in (0, 2)], (2,),
     'spec n. 9, audit qb : quatre diametres et deux tetraedres, aucun triangle'),
    ('octaedre', [(0, 1, 1), (2, 1, 1), (1, 0, 1), (1, 2, 1), (1, 1, 0), (1, 1, 2)], (2,),
     'spec n. 10 : trois paires, ni triangle ni tetraedre (tiroirs)'),
    ('cercle_n3', _circle(3, False), (2,), 'spec n. 11, MATHEMATIQUES 10.11 : n = 3, Q_b = {AC, BD}'),
    ('cercle_bt_n3', _circle(3, True), (2,), 'spec n. 11, MATHEMATIQUES 10.11 : n = 3, Q_b = {AC, B_tCD}'),
    ('cercle_n4', _circle(4, False), (2,), 'spec n. 11, audit carrier : Q_b = {AC, BD}'),
    ('cercle_bt_n4', _circle(4, True), (2,), 'spec n. 11, audit carrier : Q_b = {AC, B_tCD}, non-stabilite'),
    ('cercle_n5', _circle(5, False), (2,), 'spec n. 11, MATHEMATIQUES 10.11 : n = 5'),
    ('cercle_bt_n5', _circle(5, True), (2,), 'spec n. 11, MATHEMATIQUES 10.11 : n = 5'),
    ('cercle_n1023', _circle(1023, False), (2,), 'spec n. 11 en u21 (plus grande immersion de l\'audit)'),
    ('cercle_bt_n1023', _circle(1023, True), (2,), 'spec n. 11 en u21, perturbation de 2/sqrt(n^2 + 1)'),
    ('ligne_0_4_6_8_12', [(x, 0, 0) for x in (0, 4, 6, 8, 12)], (2,),
     'spec n. 12, FULL_BIRTH_RUNS : non-naissance au niveau 4, fusion a trois enfants'),
    ('losanges', _plane([(0, 12), (2, 10), (4, 12), (2, 14), (10, 2), (12, 0), (14, 2), (12, 4)]), (3,),
     'spec n. 12, FULL_BIRTH_RUNS : naissances etendues 4, 4, 34, fusion a trois enfants a 50'),
    ('ligne_0_2_4', [(x, 0, 0) for x in (0, 2, 4)], (2,), 'spec n. 12, T5 : deux graines menent au meme noeud'),
    ('d2_audit', _plane([(2, 10), (18, 10), (10, 20), (9, 3), (11, 3)]), (2,),
     'auditeur de4ab58a8 et aef7182b3, temoin D2 : trace stricte AB nee a 64, apres le niveau 41 de rang r_b - 1 '
     '(MATHEMATIQUES 10.2 et 10.5, registre) ; second temoin du lemme W.2'),
)
# nuages de tests/catalogue/euler_oracle.py : coquilles a plusieurs supports, triangles compris
EULER = (
    ('sphere12', [(x + 20, y + 20, z + 20) for x, y, z in SPHERE5], (), 'douze sites sur la sphere de rayon 5'),
    ('cercle12', [(x + 10, y + 10, 3) for x, y in CIRCLE5], (), 'douze sites sur un cercle de rayon 5'),
    ('sphere12_centre', [(x + 20, y + 20, z + 20) for x, y, z in SPHERE5[:10]] + [(20, 20, 20), (21, 20, 20)], (),
     'dix sites de la sphere, son centre et un voisin'),
)


def fixtures():
    """(nom, points, ordres) : fixtures de la spec et de l'audit, nuages d'Euler, puis fixtures historiques de
    families.py a positions distinctes (sans les ensembles deja presents). Ordres : ceux de la spec, et 1 a
    min(5, n - 1)."""
    out, seen = [], set()
    for name, pts, ks, _why in SPEC + EULER:
        out.append((name, pts, tuple(sorted(set(ks) | set(range(1, min(5, len(pts) - 1) + 1))))))
        seen.add(frozenset(pts))
    for cloud in FAMILIES.fixtures():
        if len(set(cloud.points)) == len(cloud.points) and frozenset(cloud.points) not in seen:
            out.append((cloud.name, cloud.points, tuple(range(1, min(5, len(cloud.points) - 1) + 1))))
            seen.add(frozenset(cloud.points))
    return out


def family_clouds():
    """16 nuages de 5 a 10 points par famille a positions distinctes (graine 31), K <= min(5, n - 1)."""
    out = []
    for name, _draw in FAMILIES.FAMILIES:
        if name == 'duplicates':
            continue
        for cloud in FAMILIES.family(name, 16, 5, 10, 5, 31):
            out.append((cloud.name, cloud.points, tuple(range(1, min(5, len(cloud.points) - 1) + 1))))
    return out


FIXTURES = dict((name, (pts, ks)) for name, pts, ks in fixtures())

# ---------------------------------------------------------------- faits graves (sur la sortie canonique)


def _tree(d):
    return [(n['level'], len(n['children'])) for n in d['nodes']]


def _balls(d, *fields):
    return [tuple(b[f] for f in fields) for b in d['balls']]


def _at(d, level, *fields):
    return sorted(tuple(b[f] for f in fields) for b in d['balls'] if b['level'] == level)


def _births(d):
    return [(n['level'], n['birth_center']) for n in d['nodes'] if n['kind'] == 1]


DIAMETERS = [[[0, 0, 0], [2, 2, 2]], [[0, 0, 2], [2, 2, 0]], [[0, 2, 0], [2, 0, 2]], [[0, 2, 2], [2, 0, 0]]]
TETRAHEDRA = [[[0, 0, 0], [0, 2, 2], [2, 0, 2], [2, 2, 0]], [[0, 0, 2], [0, 2, 0], [2, 0, 0], [2, 2, 2]]]
A1023, C1023, D1023 = [2093060, 1046530, 0], [0, 1046530, 0], [1046530, 0, 0]
B1023, BT1023 = [1046530, 2093060, 0], [1048576, 2093058, 0]

# (fixture, K, ce qui est grave, extraction sur la sortie canonique, valeur attendue). Valeurs recalculees par l'oracle
# le 4 octobre 2026 et confrontees aux attendus de la spec (paragraphe 2.9) : voir le rapport l1_s1_oracle.md.
CLAIMS = (
    ('carre', 1, 'arbre : quatre sites puis une fusion a quatre enfants au niveau 1', _tree,
     [('0', 0)] * 4 + [('1', 4)]),
    ('carre', 1, 'cotes de role fusion (components 2), diagonale interne a 2 (kparties 4, cofaces 2, 1 par support)',
     lambda d: _balls(d, 'level', 'role', 'components', 'kparties_reliees', 'cofaces', 'cofaces_support'),
     [('1', 'fusion', 2, 2, 1, [1])] * 4 + [('2', 'interne', 1, 4, 2, [1, 1])]),
    ('carre', 2, 'quatre naissances a 1 ; diagonale de role fusion : components 4, kparties 6, traces 4, '
     'cofaces 4 (2, 2)',
     lambda d: _balls(d, 'level', 'role', 'components', 'kparties_reliees', 'strict_traces', 'cofaces',
                      'cofaces_support'),
     [('1', 'naissance', 0, 1, 0, 0, [0])] * 4 + [('2', 'fusion', 4, 6, 4, 4, [2, 2])]),
    ('carre', 3, 'naissance etendue : kparties 4, cofaces 1, une par diagonale (double compte)',
     lambda d: _balls(d, 'level', 'role', 'p', 'm', 'kparties_reliees', 'cofaces', 'cofaces_support'),
     [('2', 'naissance', 0, 4, 4, 1, [1, 1])]),
    ('carre', 4, 'naissance, cofaces 0', lambda d: _balls(d, 'level', 'role', 'kparties_reliees', 'cofaces'),
     [('2', 'naissance', 1, 0)]),
    ('triangle_droit', 1, 'fusions a 9/4 puis 4, hypotenuse interne, Q_b = {hypotenuse}',
     lambda d: (_tree(d), _at(d, '25/4', 'role', 'm', 'qmin', 'supports')),
     ([('0', 0)] * 3 + [('9/4', 2), ('4', 2)], [('interne', 3, 2, [[[0, 3, 0], [4, 0, 0]]])])),
    ('triangle_droit', 2, 'naissances a 9/4 et 4, fusion a 25/4, traces 2, cofaces 1',
     lambda d: (_tree(d), _at(d, '25/4', 'role', 'strict_traces', 'cofaces', 'supports')),
     ([('9/4', 0), ('4', 0), ('25/4', 2)], [('fusion', 2, 1, [[[0, 3, 0], [4, 0, 0]]])])),
    ('growth_abcz', 3, 'naissance ABC a 16 ; a 25 interne (m 4), Q_b = {BZ, ACZ}, cofaces 1 et 1, boule 1',
     lambda d: (_tree(d), _balls(d, 'level', 'role', 'p', 'm', 'kparties_reliees', 'cofaces', 'cofaces_support',
                                 'supports')),
     ([('16', 0)], [('16', 'naissance', 1, 2, 1, 0, [0], [[[1, 8, 0], [9, 8, 0]]]),
                    ('25', 'interne', 0, 4, 4, 1, [1, 1],
                     [[[5, 0, 0], [5, 10, 0]], [[1, 8, 0], [5, 0, 0], [9, 8, 0]]])])),
    ('passagere', 1, 'fusion de {a, b, c} a 2 ; a 4, la boule (c, d) components 2, la boule {a, b, c} fusion passagere',
     lambda d: (_tree(d), _balls(d, 'level', 'role', 'components', 'm', 'prior')),
     ([('0', 0)] * 4 + [('2', 3), ('4', 2)],
      [('2', 'fusion', 2, 2, [0, 1]), ('2', 'fusion', 2, 2, [1, 2]), ('4', 'fusion', 1, 3, [4]),
       ('4', 'fusion', 2, 2, [3, 4])])),
    ('triangle_aigu', 2, 'trois composantes avant 25/16, fusion a trois enfants, traces 3, cofaces 1',
     lambda d: (_tree(d), _at(d, '25/16', 'role', 'components', 'strict_traces', 'cofaces', 'qmin')),
     ([('1', 0), ('5/4', 0), ('5/4', 0), ('25/16', 3)], [('fusion', 3, 3, 1, 3)])),
    ('ligne3', 2, 'beta = 1, I = {1} : kparties 3, traces 2, components 2, cofaces 1',
     lambda d: _at(d, '1', 'role', 'p', 'm', 'kparties_reliees', 'compressed_parts', 'strict_traces', 'components',
                   'cofaces'),
     [('fusion', 1, 2, 3, 2, 2, 2, 1)]),
    ('triangle_equilateral', 2, 'evenement faible : trois paires a 2, fusion a trois enfants a 8/3',
     lambda d: (_tree(d), _at(d, '8/3', 'role', 'p', 'm', 'qmin', 'components')),
     ([('2', 0)] * 3 + [('8/3', 3)], [('fusion', 0, 3, 3, 3)])),
    ('tetraedre_k5', 5, 'six composantes a 200 ; quatre faces a 800/3 : components 3, kparties 6, traces 3, cofaces 1',
     lambda d: (_tree(d), _at(d, '800/3', 'role', 'p', 'm', 'components', 'kparties_reliees', 'compressed_parts',
                              'strict_traces', 'cofaces')),
     ([('200', 0)] * 6 + [('800/3', 6)], [('fusion', 3, 3, 3, 6, 3, 3, 1)] * 4)),
    ('cube', 2, 'boule du cube : quatre diametres et deux tetraedres, aucun triangle, premier support d\'arite q',
     lambda d: _at(d, '3', 'role', 'm', 'qmin', 'supports'), [('interne', 8, 2, DIAMETERS + TETRAHEDRA)]),
    ('octaedre', 2, 'boule de l\'octaedre : trois paires, ni triangle ni tetraedre',
     lambda d: _at(d, '1', 'm', 'qmin', 'supports'),
     [(6, 2, [[[0, 1, 1], [2, 1, 1]], [[1, 0, 1], [1, 2, 1]], [[1, 1, 0], [1, 1, 2]]])]),
    ('cercle_n4', 2, 'Q_b = {AC, BD}', lambda d: _at(d, '289', 'components', 'kparties_reliees', 'cofaces_support',
                                                        'supports'),
     [(4, 6, [2, 2], [[[0, 17, 0], [34, 17, 0]], [[17, 0, 0], [17, 34, 0]]])]),
    ('cercle_bt_n4', 2, 'Q_b = {AC, B_tCD} : le carrier change, la boule et kparties_reliees (6) non',
     lambda d: _at(d, '289', 'components', 'kparties_reliees', 'strict_traces', 'cofaces', 'cofaces_support',
                   'supports'),
     [(3, 6, 5, 3, [2, 1], [[[0, 17, 0], [34, 17, 0]], [[0, 17, 0], [17, 0, 0], [25, 32, 0]]])]),
    ('cercle_n1023', 2, 'Q_b = {AC, BD} en u21', lambda d: _at(d, '1095225040900', 'supports'),
     [([[C1023, A1023], [D1023, B1023]],)]),
    ('cercle_bt_n1023', 2, 'Q_b = {AC, B_tCD} en u21, coordonnees < 2^21',
     lambda d: (_at(d, '1095225040900', 'supports'), max(c for s in d['sites'] for c in s) < 1 << 21),
     ([([[C1023, A1023], [C1023, D1023, BT1023]],)], True)),
    ('ligne_0_4_6_8_12', 2, 'non-naissance au niveau 4 (p = 1), puis fusion a trois enfants a 9',
     lambda d: (_tree(d), _at(d, '4', 'role', 'p', 'components')),
     ([('1', 0), ('1', 0), ('4', 0), ('4', 0), ('4', 2), ('9', 3)],
      [('fusion', 1, 2), ('naissance', 0, 0), ('naissance', 0, 0)])),
    ('losanges', 3, 'naissances etendues 4, 4, 34 (centres (2,12), (12,2), (7,7)), fusion a trois enfants a 50',
     lambda d: (_births(d), _tree(d)[-1], _at(d, '50', 'role', 'components')),
     ([('4', ['2', '12', '0']), ('4', ['12', '2', '0']), ('34', ['7', '7', '0'])], ('50', 3),
      [('fusion', 2)] * 4)),
    ('ligne_0_2_4', 2, 'les deux graines {0, 2} et {2, 4} menent au meme noeud (fusion a 4)',
     lambda d: _at(d, '4', 'role', 'p', 'prior', 'components', 'node'), [('fusion', 1, [0, 1], 2, 2)]),
    ('d2_audit', 2, 'arbre ; boule faible ABC a 1681/25 de role fusion, components 3, branches [3, 4, 5]',
     lambda d: (_tree(d), _at(d, '1681/25', 'role', 'p', 'm', 'qmin', 'components', 'prior')),
     ([('1', 0), ('49/2', 0), ('49/2', 0), ('41', 0), ('41', 0), ('65/2', 3), ('1681/25', 3)],
      [('fusion', 0, 3, 3, 3, [3, 4, 5])])),
    ('cube', 1, 'K1 (audit qb) : boule interne de niveau 3, Q_b garde les deux tetraedres (cofaces 0) a cote des '
     'quatre diametres : aucun filtre par arite ni par cofaces',
     lambda d: _at(d, '3', 'role', 'supports', 'cofaces_support'),
     [('interne', DIAMETERS + TETRAHEDRA, [1, 1, 1, 1, 0, 0])]),
)

# Empreintes sha256 des sorties canoniques de chaque fixture (tous ses ordres) : graves au premier passage.
FIXTURE_DIGESTS = {
    'carre': 'ecb1e15b97e7252ddaafb77491ac80afd68c255c499e96492e5d93711785cef0',
    'triangle_droit': '844c36688fba2dab72ac8481953ce5f5a8699ed1a773b1dee8cf73ddc71413e0',
    'growth_abcz': 'c6dcc815819995e1aed81c757dd8043530ac44a90bd86b1d1ff09291469fdf9b',
    'passagere': '24fe16e4ef2bf4fd226601475b55df39f5b2f2f8c758df4884f93dbcf22c6aee',
    'triangle_aigu': '4a26c90a0b81e206b7cb6887572c09391101ffcb60f65a7b7fef912846cbc5d1',
    'ligne3': 'b6240ac09bc00dae23a368de5c046e6aa0b3f1258a12ff86a9f233ec08836e7c',
    'triangle_equilateral': '7c7028599a70a2d29848b958fb4222033c30c43e52813802ebcbbc614c058d9b',
    'tetraedre_k5': '7f6c48796c59f4c494dd5cedb338f1e3d430c442f3c3fa479a74b7c6a0ca9834',
    'cube': '99d3a757f5510912e333c75d7fcf0029e419ec3080e376144ad6af6ce94d622c',
    'octaedre': '947eec8c1b3dfa20bfc9a6cc48723e5ccad70dbaf0629299670ddeefd7c470a5',
    'cercle_n3': 'f3b41e50de9643d70177b65123592dd2043adc624eafbd97a2719183518f1752',
    'cercle_bt_n3': 'fd9afdb2c31791fbb14fb835403d8e7295c92b7e46080aedecdf2a3d5e117642',
    'cercle_n4': 'd465c90ac2fa71ebf93c0c620110ed5dc14886bc8f3dd13ef7afd43a50750a53',
    'cercle_bt_n4': 'fc51a69b0c4c1210d878fc45b465b6177a72c7cfd56d90094d36df2e06cd3368',
    'cercle_n5': 'ad0fcc9d8a393f85c944f8f9dae9d23e3bd9a896660fb52892fa0caf9aa734db',
    'cercle_bt_n5': '2d2d789488cec3a226fe9ca72dcf1512dfe1a300337289486749f559d3ef5dab',
    'cercle_n1023': '1bd0064a0d6d993b168c464c5f1eca0fb22061cd32c95278b757ebe5468ea604',
    'cercle_bt_n1023': '3b43a2f3a184e91792255df0c7540c7714f28efdb837db7760b721fb9d193596',
    'ligne_0_4_6_8_12': '49124ca6d5a6f518f96e0987d6241cba111382272dc489b786692bfed63b5773',
    'losanges': '95115eae240c6da4fbf9037b85f085b22f06ea774aa655a3057853e0facaabd1',
    'ligne_0_2_4': 'ba55cc67fe381049e8b35e1215147bb32f4040d118cb2138290e2c2472791570',
    'd2_audit': '671b0cb7742fbed10debbef0c35d411bfe70d73d3dd8c4367d4356928d166bd3',
    'sphere12': '7f861cccbbb291528af40ca06d277c99e6a9a448f4c8a482557064839f97fe2c',
    'cercle12': '02e160353d9c3be4b5330cc192bbfa51973bd46c767bee448b191217f081db4a',
    'sphere12_centre': '9f1df0ed95ef80b61d722f113d4ceb76b5acbcc29630dcc5e05633fcd16ae956',
    'e5': '3eb1822393a2027b3c576973b88dc3bf132ddb787bb1f34cd187f4a16b56f2c4',
    'audit3': 'febe71f56dc8427d7f23d85812a74e92324c4011583ec2cd99f73b91f07d8c9d',
    'pair': '64ebf163c10713fc4910012e66961edafdc071100e5c8e06f1360fd265ff0bf7',
    'line4': '5644e96bd2f9c1b228dd13252b31b101b3faa114d59adf1fd1318052b2f5ee00',
    'line5': 'c1c70edb1c787189d59ec218cbc57bcbfa738ed331c74bc2a5d94cae59056877',
    'line01269': '6c6907e1e4d19ba70727ddb489912286c7253acae4ee2762df7693df3cacb5d2',
    'line_f3': '342f949828cb759341d0ee02631121c98c07cf04d18aea2580dc1f23654c1d58',
    'triangle_far': '94c42c498d8a0a0110fd0ce0c14c5b2441f7f8da42488b16519fffaba73163f9',
    'square_plus': 'b15c788a80a43ba172d996d790c091fd5470d0d7c599d7fb8f5c9262826e9021',
    'octa': '32730d438c60e972def4d20de96de49a7cd0460d0cc19bd76058a002c89e46ee',
    'tetra_center': '4f0122642d3f730fd4f3bb8871048c107f432748165b43264adedaafb940445e',
    'right_triangle': 'fd47389adda251a0188348cd4927167b5f7ce4bdf2f3c6c77f4c76d978b8ddc4',
    'generic6': '1ab09645358c260af90f73ffb1ca153429b284d6b47ab3fd6f8032dd4f3facd9',
    'firstcov_k3_n6': 'b4bf43588a0ce026539b657f71815a014cf07294ab0d62b34dac998716e5d38c',
    'neighbour_q2': '504f6cd94ca2f1793c21de2798c24d4f1ed80424e7f8e0ddd82fcffc7c8fb33c',
    'cover_tie_n4': '41ef74e9a3d101970203e2cc735a6319c9909ff09d11f550f3b6203602cf8b56',
    'two_triangles': 'e6d9cfa78c2147e717863f8b9c3766b68e2ae5b14888806815b4f8ca025cc132',
    'two_triangles_1998': '6b4a3e1b5a036b4ea13eabdfd819e57e690015da2a541e42271676a564848071',
    'two_triangles_1700': 'f1ff6205a012cae8efaa7d8cf8e4287b536e3141d3412dbf657a4c341f7d960a',
    'pair_boundary': 'b955ebc06244d4ec9ae530ae0f24325a406f3e6dca2987c71138a72a374220b3',
    'square_top': '6117593dd6d4c2a7dcf1c238ad87e6f6cdf8b1857fc8608ef66a887c06105885',
    'cube_max': '39d33f8e48f0b757461b9f4cbd9b6309d4272706ba420a782954e69e794fec93',
    'circle25_pair': '5d7048c6615c3b72fe85b869d1f09a4ad456b6bf09858aa49317e6530dacb69b',
    'double_collision': '837918d9f7c3bfcbec415fca9830fa62d83e9a00d23f2d3f5fc100baa61bd0bc',
    'q3_tetra_form': '630d2842989339dd8a9148e50332c9b1f066a0d1ac6ee4fc4b0cd1d2bc328edd',
}
# Compteurs EXACTS de la suite (fixtures et familles) : toute derive est un plancher viole.
SUITE_EXACT = dict(clouds=210, orders=951, balls=15062, supports=16943, nodes=12441, cuts=48074, births=6558,
                   merge_balls=5768, internal=2736, passing=271, merges=4428, nary_merges=1890, arity2=10158,
                   arity3=5640, arity4=1145, extended=2551, multi_support=530, extended_higher_arity=226,
                   strong=7584, weak=7478, kparties=54659, cofaces=20463, shared_cofaces=277, gabriel=17058,
                   non_gabriel=40053, invariance=70, permuted=70)
SUITE_DIGEST = '9bddd8aa22694f013dd81e7e70c0f1f352fa2e7a893e0c0dd66bd440d95fc697'
# Planchers de la spec (paragraphe 8.2) contre le vert par vacuite.
FLOORS = dict(clouds=150, balls=5000, extended=200, multi_support=50, internal=100, passing=10, nary_merges=20,
              extended_higher_arity=1)
COUNTERS = ('clouds', 'orders', 'balls', 'supports', 'nodes', 'cuts', 'births', 'merge_balls', 'internal', 'passing',
            'merges', 'nary_merges', 'arity2', 'arity3', 'arity4', 'extended', 'multi_support',
            'extended_higher_arity', 'strong', 'weak', 'kparties', 'cofaces', 'shared_cofaces', 'gabriel',
            'non_gabriel', 'invariance', 'permuted')

# ---------------------------------------------------------------- calcul


def documents(supports_module, pts, ks, ids=None):
    """Sorties canoniques d'un nuage a ses ordres, et compteurs. Leve InvariantError (lemme viole) ou ValueError."""
    oracle = supports_module.Supports(pts)
    docs, cnt = [], {}
    for k in ks:
        res = oracle.order(k)
        for key, value in res.counters.items():
            cnt[key] = cnt.get(key, 0) + value
        docs.append(oracle.canonical(k, ids))
    return docs, cnt


def text(docs):
    return '\n'.join(json.dumps(d, sort_keys=True, separators=(',', ':')) for d in docs)


def digest(docs):
    return hashlib.sha256(text(docs).encode('ascii')).hexdigest()


def judge_fixtures(modules, names):
    """Faits graves et empreintes des fixtures nommees, avec les modules donnes (intacts ou mutes) : 'nom' juge tous
    les ordres de la fixture, ses faits et son empreinte ; 'nom@K' ne calcule que l'ordre K et n'en juge que les faits.
    Toute exception de l'oracle (lemme viole ou non) est un ecart. Rend (ecarts, nombre de faits juges)."""
    errors, judged = [], 0
    for spec in names:
        name, _at_sign, only = spec.partition('@')
        pts, ks = FIXTURES[name]
        if only:
            ks = (int(only),)
        try:
            docs = documents(modules['supports'], pts, ks)[0]
        except Exception as exc:  # noqa: BLE001 -- une exception d'une copie mutee la tue aussi
            errors.append('%s : %s : %s' % (spec, type(exc).__name__, exc))
            continue
        by_k = dict(zip(ks, docs))
        for fixture, k, what, extract, want in CLAIMS:
            if fixture != name or k not in by_k:
                continue
            judged += 1
            try:
                got = extract(by_k[k])
            except (LookupError, TypeError, ValueError) as exc:
                got = 'illisible (%s)' % exc
            if got != want:
                errors.append('%s@%d : %s : %r au lieu de %r' % (name, k, what, got, want))
        if not only:
            judged += 1
            if FIXTURE_DIGESTS.get(name) != digest(docs):
                errors.append('%s : empreinte %s au lieu de %s' % (name, digest(docs), FIXTURE_DIGESTS.get(name)))
    return errors, judged


def sphere50(modules):
    """Fixture n. 13 : les 84 points entiers de x^2 + y^2 + z^2 = 50, translates de +10. Seul m = 84 est controle ici
    (le refus support_shell_capacity, m > 24, est natif) : la boule de la paire (15,15,10), (5,5,10) a p = 0 et
    m = 84, la paire en est un support, donc q = 2 et la boule est dans W_2."""
    S = modules['supports']
    oracle = S.Supports(SPHERE50)
    pair = (SPHERE50.index((15, 15, 10)), SPHERE50.index((5, 5, 10)))
    ball = oracle.shell_ball(pair)
    got = (len(SPHERE50), ball.level, ball.p, ball.m, oracle.is_support(ball, pair), ball.m > S.SHELL_CAPACITY)
    want = (84, 50, 0, 84, True, True)
    return [] if got == want else ['sphere50 : %r au lieu de %r' % (got, want)]


def _segment2(x, a, b):
    """Distance carree exacte du point x au segment [a, b]."""
    u = [bj - aj for aj, bj in zip(a, b)]
    t = sum((xj - aj) * uj for xj, aj, uj in zip(x, a, u)) / Fraction(sum(uj * uj for uj in u))
    t = min(max(t, Fraction(0)), Fraction(1))
    return sum((xj - aj - t * uj) ** 2 for xj, aj, uj in zip(x, a, u))


def circle_witness(modules):
    """Cercle de l'audit carrier, n = 3, 4, 5, 1023 (MATHEMATIQUES 10.9 et 10.11), a l'echelle d = n^2 + 1 : le
    temoin w = (3d/4, 5d/4) est strictement dans le triangle B_tCD (poids barycentriques > 0) et a la distance carree
    d^2/16 des deux diametres AC et BD, alors que B se deplace de 2 sqrt(d) (carre 4d) : au cercle unite, saut de
    Hausdorff d'au moins 1/4 du carrier pour un deplacement 2/sqrt(n^2 + 1). kparties_reliees vaut 6 avant et apres ;
    Q_b passe de {AC, BD} a {AC, B_tCD}."""
    S = modules['supports']
    errors = []
    for n in (3, 4, 5, 1023):
        d = n * n + 1
        a, b, c, e = _circle(n, False)
        bt = _circle(n, True)[1]
        w = (Fraction(3 * d, 4), Fraction(5 * d, 4), Fraction(0))
        weights = S.barycentric([bt, c, e], w)
        before = S.Supports(_circle(n, False)).canonical(2)
        after = S.Supports(_circle(n, True)).canonical(2)
        circle = str(d * d)
        got = (weights is not None and all(x > 0 for x in weights), _segment2(w, a, c), _segment2(w, b, e),
               sum((x - y) ** 2 for x, y in zip(bt, b)), _at(before, circle, 'kparties_reliees', 'supports'),
               _at(after, circle, 'kparties_reliees', 'supports'))
        want = (True, Fraction(d * d, 16), Fraction(d * d, 16), 4 * d,
                [(6, [sorted([list(c), list(a)]), sorted([list(e), list(b)])])],
                [(6, [sorted([list(c), list(a)]), sorted([list(c), list(e), list(bt)])])])
        if got != want:
            errors.append('cercle n = %d : %r au lieu de %r' % (n, got, want))
    return errors


def e5_window(modules):
    """Temoin du lemme W, point 2 (MATHEMATIQUES 10.3), sur E5 a K = 2 : la boule de la paire AC (niveau 33/2, p = 2,
    q = m = 2) est hors de W_2 ; T_2 a trois fusions a trois enfants (162/25, 189/17, 83886/3563). Sans les liaisons
    de cette boule, sommet AC garde, la fusion de 83886/3563 garde trois enfants ({AB}, {AC}, {BC} au lieu de {AB},
    {AC, AD, AE, CD, CE, DE}, {BC}) et une fusion a deux enfants apparait au niveau 24 ; sans ses liaisons ni son
    sommet, elle n'a que deux enfants, et la fusion de 24 apparait aussi."""
    S = modules['supports']
    pts = FIXTURES['e5'][0]
    oracle = S.Supports(pts)
    ball = oracle.shell_ball((0, 2))
    q = len(oracle._supports(ball)[0])
    tree = [(str(node.level), len(node.children)) for node in oracle.order(2).tree.nodes if node.children]
    kept = [(str(level), kids) for level, kids in oracle.window_tree(2)]
    strict = [(str(level), kids) for level, kids in oracle.window_tree(2, all_vertices=False)]
    got = (str(ball.level), ball.p, ball.m, q, tree, kept, strict)
    common = [('162/25', 3), ('189/17', 3)]
    want = ('33/2', 2, 2, 2, common + [('83886/3563', 3)], common + [('83886/3563', 3), ('24', 2)],
            common + [('83886/3563', 2), ('24', 2)])
    return [] if got == want else ['e5 : %r au lieu de %r' % (got, want)]


def d2_witness(modules):
    """Temoin D2 de l'auditeur (de4ab58a8, aef7182b3 ; MATHEMATIQUES 10.2, 10.5 et 10.11 ; registre) a K = 2 : la
    boule faible ABC (niveau 1681/25) a une trace stricte AB nee au niveau 64 (boule de p = 2, q = 2, hors de Cat_2),
    apres le niveau precedent de Cat_2, 41 = l(r_b - 1). La descente de AB passe par I = {Z, W} et aboutit a la
    naissance ZW (niveau 1, noeud 0), dont le noeud a la coupe fermee 41 est celui de AB a la coupe ouverte de ABC :
    seuls les ensembles de noeuds coincident. Lemme W.2 : sans les liaisons de la boule de AB (hors fenetre), une
    fusion a deux enfants apparait a 145/2 et la fusion de 1681/25 change d'enfants, que l'on garde le sommet AB (trois
    enfants) ou non (deux)."""
    S = modules['supports']
    oracle = S.Supports(FIXTURES['d2_audit'][0])
    D = oracle.definition
    res = oracle.order(2)
    ab, zw, lam = (0, 1), (3, 4), Fraction(1681, 25)
    ball = oracle.shell_ball(ab)
    previous = max([b.level for b in res.balls if b.level < lam] + [Fraction(0)])
    opened = S._before([cut.level for cut in res.tree.cuts], lam)
    got = (str(D.beta(ab)), ball.p, ball.p + len(oracle._supports(ball)[0]), ball.inner, str(previous),
           str(D.beta(zw)), D.node_at(2, zw, D.beta(zw)), D.node_at(2, zw, previous) == D.node_at(2, ab, opened),
           [(str(level), kids) for level, kids in oracle.window_tree(2)],
           [(str(level), kids) for level, kids in oracle.window_tree(2, all_vertices=False)])
    want = ('64', 2, 4, (3, 4), '41', '1', 0, True, [('65/2', 3), ('1681/25', 3), ('145/2', 2)],
            [('65/2', 3), ('1681/25', 2), ('145/2', 2)])
    return [] if got == want else ['d2 : %r au lieu de %r' % (got, want)]


def minimal_budget(modules):
    """Budget de la force brute du lemme F (_minimal_nonseparable, au plus 2^m - 1 candidats) : fixture d'egalite sur
    la boule de la diagonale du carre (m = 4) : refus BudgetRefusal au budget 14, parties minimales egales a Q_b au
    budget 15 ; refus explicite, avant tout calcul, sur la coquille de sphere50 (m = 84) au budget par defaut."""
    S = modules['supports']
    oracle = S.Supports(FIXTURES['carre'][0])
    ball = oracle.ball_of((0, 2))
    got = [ball.m]
    for budget in (14, 15):
        try:
            got.append(sorted(oracle._minimal_nonseparable(ball, budget)) == sorted(ball.supports))
        except S.BudgetRefusal:
            got.append('refus')
    wide = S.Supports(SPHERE50)
    big = wide.shell_ball((SPHERE50.index((15, 15, 10)), SPHERE50.index((5, 5, 10))))
    try:
        wide._minimal_nonseparable(big)
        got.append('calcule')
    except S.BudgetRefusal:
        got.append('refus')
    want = [4, 'refus', True, 'refus']
    return [] if got == want else ['budget du lemme F : %r au lieu de %r' % (got, want)]


EXTRA_FACTS = (sphere50, circle_witness, e5_window, d2_witness, minimal_budget)


SHIFT = (5, 11, 17)  # translation entiere de la contre-epreuve d'invariance (MATHEMATIQUES 10.10)


def _untranslate(doc):
    """Sortie canonique ramenee de la translation SHIFT : sites, centres et supports ; niveaux et numerotation
    inchanges."""
    def back(point):
        return [str(Fraction(c) - v) if isinstance(c, str) else c - v for c, v in zip(point, SHIFT)]
    doc['sites'] = [back(p) for p in doc['sites']]
    for node in doc['nodes']:
        if node['birth_center'] is not None:
            node['birth_center'] = back(node['birth_center'])
    for ball in doc['balls']:
        ball['center'] = back(ball['center'])
        ball['supports'] = [[back(p) for p in q] for q in ball['supports']]
    return doc


def invariance(supports_module, name, pts, ks, docs, rng):
    """Permutation NON TRIVIALE de l'entree, reetiquetage injectif (0xFFFFFFFF compris) et translation entiere SHIFT :
    memes sorties, a la colonne ids et a la translation pres (numerotation, niveaux, ordres, comptes). Rend (ecarts,
    permutation effective)."""
    order = list(range(len(pts)))
    rng.shuffle(order)
    if order == sorted(order) and len(order) > 1:
        order = order[1:] + order[:1]
    label = [(0xFFFFFFFF - 7919 * i) & 0xFFFFFFFF for i in range(len(pts))]
    moved = [tuple(c + v for c, v in zip(pts[i], SHIFT)) for i in order]
    other = documents(supports_module, moved, ks, [label[i] for i in order])[0]
    errors = []
    for k, base, doc in zip(ks, docs, other):
        ids = doc.pop('ids')
        doc = _untranslate(doc)
        want = [label[pts.index(tuple(s))] for s in doc['sites']]
        if doc != base or ids != want:
            errors.append('%s K=%d : sortie non invariante par permutation, reetiquetage ou translation' % (name, k))
    return errors, order != sorted(order)


def run_suite():
    """Fixtures puis familles : (ecarts, compteurs, empreinte de la suite, empreintes des fixtures)."""
    cnt = dict((name, 0) for name in COUNTERS)
    errors, prints = [], {}
    whole = hashlib.sha256()
    rng = FAMILIES.Rng(20261004)
    clouds = fixtures() + family_clouds()
    S = STAGE['supports']
    for index, (name, pts, ks) in enumerate(clouds):
        try:
            docs, found = documents(S, pts, ks)
        except (STAGE['model'].InvariantError, ArithmeticError, LookupError, TypeError, ValueError) as exc:
            errors.append('%s %r : %s : %s' % (name, pts, type(exc).__name__, exc))
            continue
        cnt['clouds'] += 1
        for key, value in found.items():
            if key in cnt:
                cnt[key] += value
        if name in FIXTURES:
            prints[name] = digest(docs)
        whole.update((name + '\n' + text(docs) + '\n').encode('ascii'))
        if name in FIXTURES or index % 8 == 0:
            cnt['invariance'] += 1
            try:
                found_errors, permuted = invariance(S, name, pts, ks, docs, rng)
            except Exception as exc:  # noqa: BLE001 -- un lemme viole dans le calcul permute est un ecart
                errors.append('%s (invariance) : %s : %s' % (name, type(exc).__name__, exc))
                continue
            errors.extend(found_errors)
            cnt['permuted'] += permuted
    return errors, cnt, whole.hexdigest(), prints


def floors(cnt, suite_digest):
    low = ['%s %d < %d' % (name, cnt[name], want) for name, want in sorted(FLOORS.items()) if cnt[name] < want]
    low += ['%s %d != %d' % (name, cnt[name], want) for name, want in sorted(SUITE_EXACT.items())
            if cnt[name] != want]
    if not SUITE_EXACT:
        low.append('compteurs exacts non graves')
    if cnt['permuted'] != cnt['invariance']:
        low.append('permutations effectives %d != contre-epreuves %d' % (cnt['permuted'], cnt['invariance']))
    if suite_digest != SUITE_DIGEST:
        low.append('empreinte de la suite %s au lieu de %s' % (suite_digest, SUITE_DIGEST))
    return low


def main_gate():
    facts, judged = judge_fixtures(STAGE, [name for name, _pts, _ks, _why in SPEC])
    for fact in EXTRA_FACTS:
        try:
            facts += fact(STAGE)
        except Exception as exc:  # noqa: BLE001 -- un lemme viole dans un fait est un ecart, pas une trace Python
            facts.append('%s : %s : %s' % (fact.__name__, type(exc).__name__, exc))
        judged += 1
    errors, cnt, suite_digest, prints = run_suite()
    for name, value in sorted(prints.items()):
        if name not in [s[0] for s in SPEC] and FIXTURE_DIGESTS.get(name) != value:
            facts.append('%s : empreinte %s au lieu de %s' % (name, value, FIXTURE_DIGESTS.get(name)))
    for line in (facts + errors)[:10]:
        print('ECART %s' % line[:400])
    print('reference_supports faits=%d ecarts=%d %s empreinte=%s'
          % (judged, len(facts) + len(errors), ' '.join('%s=%d' % (k, cnt[k]) for k in COUNTERS), suite_digest))
    if facts or errors:
        return DISAGREEMENT
    low = floors(cnt, suite_digest)
    if judged != len(CLAIMS) + len(SPEC) + len(EXTRA_FACTS):
        low.append('faits %d != %d' % (judged, len(CLAIMS) + len(SPEC) + len(EXTRA_FACTS)))
    if low:
        print('PLANCHER : %s' % ', '.join(low))
        return FLOOR
    print('reference_supports_ok nuages=%d ordres=%d boules=%d supports=%d noeuds=%d coupes=%d'
          % (cnt['clouds'], cnt['orders'], cnt['balls'], cnt['supports'], cnt['nodes'], cnt['cuts']))
    return OK


# ---------------------------------------------------------------- mutants

def run_mutant(name):
    """Fixtures du mutant sur les sources intactes (temoin), puis sur la copie mutee. Rend le code."""
    mutant = ref_mutants.SUPPORT_MUTANTS[name]
    found, _judged = judge_fixtures(STAGE, mutant['fixtures'])
    if found:
        print('temoin non conforme : %s' % found[0])
        return DISAGREEMENT
    try:
        mutated = ref_mutants.load_supports(name, PACKAGE)
    except ref_mutants.StalePatch as exc:
        print('PLANCHER : mutant %s inapplicable : %s' % (name, exc))
        return FLOOR
    killers, _judged = judge_fixtures(mutated, mutant['fixtures'])
    if not killers:
        print('mutant_survives %s' % name)
        return OK
    print('mutant %s tue par %d ecart(s) ; premier : %s' % (name, len(killers), killers[0][:300]))
    if not any(mutant['cause'] in k for k in killers):
        print('PLANCHER : mutant %s tue sans sa cause (%s)' % (name, mutant['cause']))
        return FLOOR
    print('mutant_killed %s' % name)
    return MUTANT_KILLED


# ---------------------------------------------------------------- lancement

def usage(message):
    print('refus : %s' % message)
    print(__doc__)
    return REFUSAL


# Sphere5 (apport des auditeurs du 5 octobre 2026, receipts/audit_supports_contract_20261005/qb) : les 24 points
# entiers de x^2 + y^2 + z^2 = 5, translates de (2, 2, 2) ; une coquille mixte au plafond natif de 24 sites.
SPHERE_NORM5 = [(x + 2, y + 2, z + 2) for x in range(-2, 3) for y in range(-2, 3) for z in range(-2, 3)
                if x * x + y * y + z * z == 5]
# (K, kparties_reliees, strict_traces, cofaces) de la boule centrale (p = 0, m = 24), d'apres le recu des auditeurs.
SPHERE_NORM5_COUNTS = ((1, 24, 24, 12), (2, 276, 264, 288), (3, 2024, 1736, 3906))


def sphere5_primitives(S):
    """Primitives de l'oracle sur la boule centrale de sphere5 : rend (ecarts, ligne de couverture)."""
    errors = []
    oracle = S.Supports(SPHERE_NORM5)
    ball = oracle.ball_of((SPHERE_NORM5.index((0, 1, 2)), SPHERE_NORM5.index((4, 3, 2))))
    arities = [sum(1 for q in ball.supports if len(q) == a) for a in (2, 3, 4)]
    shape = (len(SPHERE_NORM5), str(ball.level), [str(c) for c in ball.center], ball.p, ball.m, ball.q, arities)
    if shape != (24, '5', ['2', '2', '2'], 0, 24, 2, [12, 24, 792]):
        errors.append('sphere5 : boule centrale %r' % (shape,))
    closure = oracle.closure_upto(ball, 4)
    if closure != [0, 0, 12, 288, 3906]:
        errors.append('sphere5 : N_0..N_4 = %r au lieu de [0, 0, 12, 288, 3906]' % (closure,))
    for k, kparts, strict, cofaces in SPHERE_NORM5_COUNTS:
        got = (comb(24, k), comb(24, k) - closure[k], closure[k + 1])
        incidences = sum(comb(24 - len(q), k + 1 - len(q)) for q in ball.supports if k + 1 >= len(q))
        q4 = sum(comb(20, k - 3) for q in ball.supports if len(q) == 4 and k + 1 >= 4)
        want = {1: 12, 2: 288, 3: 4068}[k]
        if got != (kparts, strict, cofaces) or incidences != want or (k == 1 and q4 != 0):
            errors.append('sphere5 K%d : %r au lieu de %r, incidences %d au lieu de %d, q4 %d'
                          % (k, got, (kparts, strict, cofaces), incidences, want, q4))
    refusals = 0
    for name, call in (('_minimal_nonseparable', lambda: oracle._minimal_nonseparable(ball)),
                       ('_lemma_f', lambda: oracle._lemma_f(ball))):
        try:
            call()
            errors.append('sphere5 : %s calcule au lieu d\'un refus explicite' % name)
        except S.BudgetRefusal:
            refusals += 1
    line = ('reference_supports_primitives_ok sites=24 supports=%d q2=%d q3=%d q4=%d N2=%d N3=%d N4=%d refus=%d'
            % (len(ball.supports), arities[0], arities[1], arities[2], closure[2], closure[3], closure[4], refusals))
    return errors, line


def main_primitives():
    """Suite longue des primitives sur sphere5 : Q_b (Gram), N_j pour j <= 4 (combinaisons), comptes de K1 a K3 par
    les formules du lemme G (p = 0 : cofaces = N_{K+1}, traces strictes C(24, K) - N_K), incidences par support
    (4 068 a K3 ; les 792 tetraedres sans coface a K1), et refus explicite de la force brute du lemme F au budget par
    defaut. Une exception levee est un ecart, jamais une trace Python. Rend le code de la porte."""
    try:
        errors, line = sphere5_primitives(STAGE['supports'])
    except Exception as exc:  # noqa: BLE001 -- un lemme viole ou un refus inattendu est un ecart
        errors, line = ['sphere5 : %s : %s' % (type(exc).__name__, exc)], ''
    for error in errors[:10]:
        print('ECART %s' % error[:400])
    if errors:
        return DISAGREEMENT
    print(line)
    return OK


def main(argv):
    if not argv:
        return main_gate()
    if argv == ['--suite=primitives']:
        return main_primitives()
    if argv == ['--list-mutants']:
        for name in sorted(ref_mutants.SUPPORT_MUTANTS):
            print('%s %s' % (name, 'equivalent' if ref_mutants.SUPPORT_MUTANTS[name]['equivalent'] else 'reel'))
        return OK
    if len(argv) == 1 and argv[0].startswith('--inject='):
        name = argv[0][len('--inject='):]
        if name not in ref_mutants.SUPPORT_MUTANTS:
            return usage('mutant inconnu %r' % name)
        return run_mutant(name)
    if len(argv) == 1 and argv[0].startswith('--dump='):
        name = argv[0][len('--dump='):]
        if name not in FIXTURES:
            return usage('fixture inconnue %r' % name)
        pts, ks = FIXTURES[name]
        print(text(documents(STAGE['supports'], pts, ks)[0]))
        return OK
    return usage('arguments %r' % (argv,))


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
