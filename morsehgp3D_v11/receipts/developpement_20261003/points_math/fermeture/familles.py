#!/usr/bin/env python3
"""Familles exactes pour la critique « fusion parasite avant FULL » (chapitre 7 de la these) et cibles Q1-Q4.

    python3 familles.py > resultats/familles.txt

Pour chaque nuage, ordre k et seuil m : niveau de reunion des deux amas designes A et B (germes = sites de plus
petit D_k de chaque amas) selon FULL (semantique de couverture), la fermeture qualifiee, H_m, H_1, la fermeture
exclusive (EC), first et core ; fraction de A recuperee avant cette reunion (FULL : couverture de la composante du
germe ; regles laminaires : bloc du germe juste avant la reunion) ; evenements parasites de la fermeture classes
(frange / deux amas entiers, direct / par chaine). Niveaux : rayons carres exacts ; rayons affiches en flottant.
"""
from fractions import Fraction
import math
import sys

sys.dont_write_bytecode = True
import lib_fermeture as lf  # noqa: E402


def r(x):
    return '%.2f' % math.sqrt(x)


def seeds(defn, k, group):
    return min(group, key=lambda i: (defn.nearest(i)[k - 1][0], i))


def analyse(name, pts, A, B, k, ms, note=''):
    n = len(pts)
    defn = lf.Definition(pts)
    res = defn.order(k)
    sa, sb = seeds(defn, k, A), seeds(defn, k, B)
    mf, ff = lf.full_fraction_before_merge(res, n, sa, sb, A, k)
    print('\n## %s (n=%d, k=%d) %s' % (name, n, k, note))
    print('   A=%s B=%s germes %d,%d ; FULL : reunion des lignees des germes a r=%s (beta=%s), fraction de A'
          ' couverte avant : %s' % (A, B, sa, sb, r(mf), mf, ff))
    rows = []
    for m in ms:
        if m > n:
            continue
        c = lf.closure(res, n, m)
        ref, tree = lf.faithful_rules(res, n, m)
        rules = dict(fermeture=c['u'])
        for rule in ('margin', 'margin1', 'first', 'core'):
            rules[rule] = lf.ultrametric_of(ref[rule], tree)
        rules['ec'] = lf.ultrametric_of(lf.ec_hanging(res, n, m), tree)
        parts = []
        for rule in ('fermeture', 'margin', 'ec', 'first', 'margin1', 'core'):
            u = rules[rule]
            lev, frac = lf.fraction_before_merge_laminar(u, sa, sb, A)
            parts.append('%s r=%s frac=%s' % (rule, r(lev), frac))
        print('   m=%d : %s' % (m, ' | '.join(parts)))
        ev = lf.parasitic_events(res, n, m, k)
        for e in sorted(ev, key=lambda e: e['U']):
            kind = e['kind']
            how = 'direct' if e['direct'] else 'par chaine'
            print('      fermeture avant FULL : noeud %d (fusion r=%s) enfants %d|%d reunis a r=%s (rapport rayon %.3f)'
                  ' ; %s, %s ; a U : partages %d, exclusifs %s, tailles %s ; max min exclusifs avant fusion %d'
                  % (e['node'], r(e['h']), e['s'], e['t'], r(e['U']), e['ratio_r'], kind, how, e['shared_at_U'],
                     e['excl_at_U'], e['sizes_at_U'], e['maxmin_excl']))
        rows.append((m, ev))
    return rows


def valley(d, t, third=False):
    """Deux amas de 3 (ou 4) sites, apex face a face, un site de vallee au milieu (plan z = 0, ou 3D si third)."""
    A = [(-d - t, -t, 0), (-d - t, t, 0), (-d, 0, 0)]
    B = [(d, 0, 0), (d + t, -t, 0), (d + t, t, 0)]
    if third:
        A.append((-d - t, 0, t))
        B.append((d + t, 0, t))
    pts = A + [(0, 0, 0)] + B
    na = len(A)
    return pts, list(range(na)), list(range(na + 1, 2 * na + 1))


def valley_fringe(d, t, f):
    """Vallee + un site de frange a gauche de A (a distance f du bord de A) et a droite de B."""
    A = [(-d - t, -t, 0), (-d - t, t, 0), (-d, 0, 0), (-d - t - f, 0, 0)]
    B = [(d, 0, 0), (d + t, -t, 0), (d + t, t, 0), (d + t + f, 0, 0)]
    pts = A + [(0, 0, 0)] + B
    return pts, [0, 1, 2, 3], [5, 6, 7, 8]


def split_cluster(d, t, g):
    """A = paire A1 + triangle A2 relies par un seul site de vallee v (pont mince) ; B = paire au contact epais de
    A2 (distance g). FULL reunit A2 et B avant A1 et A2 si g/2 < d ; la fermeture reunit A1 et A2 d'abord."""
    A1 = [(-d - t, -t // 2, 0), (-d - t, t // 2, 0)]
    v = [(-d // 2 + 0, 0, 0)]
    A2 = [(0, 0, 0), (t, -t, 0), (t, t, 0)]
    Bp = [(t + g, -t // 2, 0), (t + g, t // 2, 0)]
    pts = A1 + v + A2 + Bp
    return pts, [0, 1, 2, 3, 4, 5], [6, 7]


def shared_vertex(L, wdt, z):
    """Deux objets (triangles) qui se touchent en un seul site s ; z ecarte B du plan de A."""
    s = (0, 0, 0)
    A = [(-L, -wdt, 0), (-L, wdt, 0)]
    B = [(L, -wdt, z), (L, wdt, z)]
    pts = [s] + A + B
    return pts, [1, 2], [3, 4]


def main():
    print('# Familles exactes : la fermeture reunit-elle avant FULL ?')
    print('# (rayons affiches ; niveaux exacts dans le code ; m = seuil de transmission de la fermeture)')
    for d, t in ((100, 10), (100, 30), (100, 50)):
        pts, A, B = valley(d, t)
        analyse('vallee d=%d t=%d' % (d, t), pts, A, B, 2, [1, 2, 3, 4])
    pts, A, B = valley(100, 20, third=True)
    analyse('vallee 3D d=100 t=20 (amas de 4)', pts, A, B, 2, [3, 4, 5])
    analyse('vallee 3D d=100 t=20 (amas de 4)', pts, A, B, 3, [3, 4, 5])
    pts, A, B = valley_fringe(100, 10, 150)
    analyse('vallee + frange d=100 t=10 f=150', pts, A, B, 2, [3])
    for g in (140, 180, 300):
        pts, A, B = split_cluster(200, 20, g)
        analyse('amas scinde (pont mince interne) + contact epais d=200 t=20 g=%d' % g, pts, A, B, 2, [3])
    for z in (0, 2, 5):
        pts, A, B = shared_vertex(12, 2, z)
        analyse('contact en un site L=12 w=2 z=%d' % z, pts, A, B, 2, [2, 3])
    five = [(6, 2, 0), (0, 0, 0), (0, 4, 0), (12, 0, 0), (12, 4, 0)]
    analyse('cinq points de l auditeur', five, [1, 2], [3, 4], 2, [2, 3])
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
