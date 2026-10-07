#!/usr/bin/env python3
"""Piece THESE-5 : le theoreme 5 du manuscrit (K-arbre couvrant minimal du K-graphe de Gabriel) confronte a
l'oracle de definition de la v11 (hgp11_ref.definition, fractions exactes).

Usage : PYTHONDONTWRITEBYTECODE=1 python3 -B these_th5.py
Code de sortie : 0 si le contre-exemple est confirme (desaccord permanent), 1 sinon.

Enonce de la these (Def. 26, 28, 29, 30 ; Prop. 6 ; Th. 5), ordre K, rayon r :
  - position generale (Def. 26) : pour toute partie sigma d'au moins deux points, aucun point hors de sigma
    n'est sur la frontiere de la plus petite boule englobante de sigma ;
  - un K-simplexe (K + 1 points) est de Gabriel si l'interieur de sa plus petite boule englobante ne contient
    aucun point hors de lui ;
  - K-graphe de Gabriel : sommets = K-parties facettes d'au moins un K-simplexe de Gabriel ; chaque K-simplexe
    de Gabriel relie ses facettes (clique), poids = son rayon ;
  - Prop. 6 / Th. 5 : pour tout r, les ensembles de points des composantes non reduites a un sommet du graphe de
    Gabriel elague a r sont les K-polyedres de Cech non reduits a une K-partie isolee.
Ce script ne partage avec l'oracle que la boule minimale exacte (Definition.meb) et la lecture des coupes de
Gamma_K (Definition.order(K).cuts) ; le graphe de Gabriel est construit ici.
"""
import sys
from fractions import Fraction
from itertools import combinations

sys.path.insert(0, '/workspaces/E-HGP/build/v11-worktree/morsehgp3D_v11/reference')
from hgp11_ref import Definition  # noqa: E402
from hgp11_ref.model import members  # noqa: E402


def d2(c, p):
    return sum((Fraction(a) - b) ** 2 for a, b in zip(p, c))


def general_position_violations(oracle):
    n = oracle.n
    bad = []
    for size in range(2, n + 1):
        for sigma in combinations(range(n), size):
            level, center, _closed = oracle.meb(sigma)
            for x in range(n):
                if x not in sigma and d2(center, oracle.points[x]) == level:
                    bad.append((sigma, x))
    return bad


def gabriel_components(oracle, k, level):
    """Composantes du k-graphe de Gabriel elague au niveau (rayon carre) : liste de (nombre de sommets, points)."""
    n = oracle.n
    gabriel = []
    for sigma in combinations(range(n), k + 1):
        lv, center, _closed = oracle.meb(sigma)
        if all(d2(center, oracle.points[x]) >= lv for x in range(n) if x not in sigma):
            gabriel.append((lv, sigma))
    vertices = set()
    for _lv, sigma in gabriel:  # Def. 29 : facettes d'au moins un simplexe de Gabriel (sans condition de rayon)
        for i in range(k + 1):
            vertices.add(sigma[:i] + sigma[i + 1:])
    parent = dict((v, v) for v in vertices)

    def find(v):
        while parent[v] != v:
            parent[v] = parent[parent[v]]
            v = parent[v]
        return v
    for lv, sigma in gabriel:
        if lv <= level:
            faces = [sigma[:i] + sigma[i + 1:] for i in range(k + 1)]
            for f in faces[1:]:
                parent[find(f)] = find(faces[0])
    comps = {}
    for v in vertices:
        comps.setdefault(find(v), []).append(v)
    return sorted((len(vs), tuple(sorted(set(x for v in vs for x in v)))) for vs in comps.values()), gabriel


def cech_nontrivial(oracle, k, level):
    """K-polyedres de Cech non reduits a une k-partie isolee, a la coupe fermee : couverture d'au moins k + 1
    points (une composante d'au moins deux sommets couvre au moins k + 1 points, et reciproquement)."""
    res = oracle.order(k)
    shut = ()
    for cut in res.cuts:
        if cut.level > level:
            break
        shut = cut.closed
    return sorted(tuple(members(cov)) for _node, cov, _core in shut if bin(cov).count('1') >= k + 1)


def report(name, points, labels, k):
    oracle = Definition(points)
    n = oracle.n
    print('== %s : %s' % (name, points))
    viol = general_position_violations(oracle)
    print('   position generale (Def. 26) : %d violation(s)' % len(viol))
    levels = sorted(set(oracle.meb(part)[0] for size in (k, k + 1) for part in combinations(range(n), size)))
    lab = lambda pts: ''.join(labels[i] for i in pts)  # noqa: E731
    last = None
    ndiff = 0
    for level in levels:
        gab, gabriel = gabriel_components(oracle, k, level)
        got = sorted(pts for size, pts in gab if size >= 2)
        want = cech_nontrivial(oracle, k, level)
        if got != want:
            ndiff += 1
            last = level
            print('   niveau %s (~%.3f) : Cech %s ; Gabriel %s' % (level, float(level), [lab(p) for p in want],
                                                                    [lab(p) for p in got]))
    gab, gabriel = gabriel_components(oracle, k, levels[-1])
    print('   simplexes de Gabriel (ordre %d) : %s' % (k, [(lab(s), str(lv)) for lv, s in sorted(gabriel)]))
    print('   niveaux critiques : %d ; en desaccord : %d ; dernier niveau en desaccord : %s ; dernier niveau : %s'
          % (len(levels), ndiff, last, levels[-1]))
    final_gab = [lab(pts) for size, pts in gab]
    print('   composantes finales du graphe de Gabriel (tous sommets) : %s' % final_gab)
    res = oracle.order(k)
    roots = [v for v in range(len(res.nodes)) if all(v not in nd.children for nd in res.nodes)]
    print('   arbre de fusion exact d\'ordre %d : %d noeuds, %d racine(s), niveau de la racine %s'
          % (k, len(res.nodes), len(roots), res.nodes[roots[0]].level))
    return len(viol), ndiff, last == levels[-1], len(final_gab)


def main():
    # contre-exemple plan (audit L02 de la v10, translate dans le domaine positif) : A, C, z, y, w
    plan = [(0, 100, 0), (200, 100, 0), (101, 10, 0), (130, 15, 0), (103, 400, 0)]
    viol, ndiff, permanent, ncomp = report('plan a cinq points', plan, 'ACzyw', 2)
    ok = viol == 0 and ndiff >= 1 and permanent and ncomp == 2
    # fixture E5 du registre racine (gabriel-point-set-counterexample-5-points-v1) : desaccord sur un intervalle
    e5 = [(0, 0, 7), (0, 9, 6), (1, 4, 0), (0, 0, 1), (4, 1, 2)]
    viol5, ndiff5, permanent5, _ncomp5 = report('E5', e5, 'ABCDE', 2)
    ok = ok and viol5 == 0 and ndiff5 >= 1 and not permanent5
    print('VERDICT : %s' % ('CONTRE-EXEMPLE CONFIRME' if ok else 'NON CONFIRME'))
    return 0 if ok else 1


if __name__ == '__main__':
    sys.exit(main())
