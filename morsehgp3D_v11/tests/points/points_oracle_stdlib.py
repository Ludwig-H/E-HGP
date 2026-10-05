#!/usr/bin/env python3
"""Porte mhgp11_points_oracle (tranche S9) : la hierarchie de points native contre l'oracle de la DEFINITION, en
bibliotheque standard (Python 3.10 nu, aucun assert).

    points_oracle_stdlib.py --probe <mhgp11_points_probe> --work <dossier> [--clouds N] [--seed S]
                            [--min-clouds N] [--min-comparisons N] [--min-delayed N]

Oracle extrait de bench/points_reference.py (reference_radius_rules, Tree, two_roots_sign, reference_owner_signature),
sans numpy ni route banc : regle H^r_m par force brute sur les coupes fermees de Gamma_K de l'etage A
(reference/hgp11_ref, Definition), toutes decisions (rival maximal, proprietaire vivant a la date, egalites comprises)
par two_roots_sign en rationnels exacts. Pour chaque nuage (temoins exacts de bench/points_gate.py puis nuages
aleatoires a ex aequo frequents, au plus neuf sites), chaque ordre K <= min(4, n - 1) (K = 1 compris) et chaque
qualification m(K), plus m = 1 a K >= 2 (sonde --m) : la sonde ecrit pendaison, arbre de points et couverture a la
naissance de chaque noeud ; on compare EXACTEMENT, site par site, la date (somme de racines, egalite certifiee par
classes de carres) et le proprietaire (niveau de naissance et PointId couverts a la naissance), puis les blocs de
l'arbre de points a chaque plateau contre ceux de l'ultrametrique exacte de l'oracle (u(i, j) = rencontre des
remontees de (o_i, e_i) et (o_j, e_j)). Une comparaison = un site d'un ordre.

Codes : 0 conforme ; 1 desaccord ; 2 refus (sonde en echec, usage) ; 3 plancher. Derniere ligne :
    points_oracle_verdict <conforme|desaccord|refus|plancher> nuages=<c> ordres=<o> comparaisons=<s> retardes=<d>
        plateaux=<p>
"""
import argparse
from fractions import Fraction
import json
import os
import random
import struct
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
V11 = os.path.normpath(os.path.join(HERE, '..', '..'))
sys.path.insert(0, os.path.join(V11, 'reference'))
sys.path.insert(0, os.path.join(V11, 'bench'))
from hgp11_ref import Definition  # noqa: E402
import mhgp11_formats as formats  # noqa: E402

EQUILATERAL = [tuple(c + 2 for c in p) for p in ((-1, -1, 0), (-1, 0, -1), (0, 0, 0), (1, 1, 0), (2, 2, 0), (2, 1, 1))]
FIVE = [(6, 2, 0), (0, 0, 0), (0, 4, 0), (12, 0, 0), (12, 4, 0)]
EIGHT = [(20, 20, 0), (30, 20, 0), (40, 20, 0), (50, 20, 0), (64, 20, 0), (74, 20, 0), (0, 30, 0), (0, 10, 0)]
PLATEAU = [(4, 4, 4), (6, 6, 4), (0, 0, 4), (8, 8, 4)]
NONE = (1 << 32) - 1
ZERO = Fraction(0)


class Disagreement(Exception):
    pass


class ProbeError(Exception):
    pass


# ---------------------------------------------------------------- oracle (extrait de bench/points_reference.py)

def parents_of(nodes):
    parent = [-1] * len(nodes)
    for v, node in enumerate(nodes):
        for c in node.children:
            parent[c] = v
    return parent


def popcount(x):
    return bin(x).count('1')


class Tree(object):
    """Arbre de l'etage A relu sur les enfants : ancetre vivant (coupe fermee) et LCA par remontee."""

    def __init__(self, nodes):
        self.nodes, self.parent = nodes, parents_of(nodes)

    def chain(self, v):
        out = [v]
        while self.parent[out[-1]] >= 0:
            out.append(self.parent[out[-1]])
        return out

    def lca(self, a, b):
        seen = set(self.chain(a))
        for w in self.chain(b):
            if w in seen:
                return w
        raise Disagreement('foret non connexe')

    def meet(self, a, la, b, lb):
        w = self.lca(a, b)
        if w in (a, b):
            return max(la, lb)
        return max(la, lb, self.nodes[w].level)


def two_roots_sign(a1, a2, b1, b2):
    """Signe exact de (sqrt a1 + sqrt a2) - (sqrt b1 + sqrt b2), rationnels >= 0 (routine propre a l'oracle)."""
    c = a1 + a2 - b1 - b2
    p, q = a1 * a2, b1 * b2
    s = (p > q) - (p < q)
    if c == 0 and s == 0:
        return 0
    if c >= 0 and s >= 0:
        return 1
    if c <= 0 and s <= 0:
        return -1
    g = c * c - 4 * (p + q)
    if g >= 0:
        dominant = 0 if g == 0 and p * q == 0 else 1
    else:
        h = 64 * p * q - g * g
        dominant = (h > 0) - (h < 0)
    return dominant if c > 0 else -dominant


def reference_radius(res, n, m):
    """Regle H^r_m par force brute sur les coupes : par point d'entree, (proprietaire, (t, meet, h))."""
    tree = Tree(res.nodes)
    qualified = [[] for _ in range(n)]
    for cut in res.cuts:
        for v, coverage, _core in cut.closed:
            if popcount(coverage) >= m:
                for i in range(n):
                    if coverage >> i & 1:
                        qualified[i].append((cut.level, v))
    out = []
    for i in range(n):
        pts = qualified[i]
        if not pts:
            raise Disagreement('oracle : site %d jamais qualifie' % i)
        t = min(level for level, _ in pts)
        v1 = next(v for level, v in pts if level == t)
        arg = (ZERO, ZERO)
        for level, v in pts:
            meet = tree.meet(v1, t, v, level)
            if two_roots_sign(meet, arg[1], level, arg[0]) > 0:
                arg = (meet, level)
        owner = v1
        for w in tree.chain(v1):
            if two_roots_sign(t, arg[0], arg[1], res.nodes[w].level) >= 0:
                owner = w
            else:
                break
        out.append((owner, (t, arg[0], arg[1])))
    return out, tree


def owner_signature(res, n, owner):
    """(niveau de naissance, indices couverts a la naissance) d'un noeud de l'etage A."""
    level = res.nodes[owner].level
    for cut in res.cuts:
        if cut.level == level:
            for v, coverage, _core in cut.closed:
                if v == owner:
                    return (level, frozenset(i for i in range(n) if coverage >> i & 1))
    raise Disagreement('oracle : coupe de naissance absente')


# ---------------------------------------------------------------- comparaison

def date_sign(a, b):
    return formats.compare_dates(a, b)


def at_most(date, value):
    """date (t, M, Q) <= sqrt(value) (niveau carre) ?"""
    return formats.radical_sign([(1, date[0]), (1, date[1]), (-1, date[2]), (-1, value)]) <= 0


def native_blocks(dump, levels, p):
    """Blocs (frozenset de PointId) de l'arbre de points natif au plateau p (coupe fermee)."""
    groups = {}
    for s in range(dump['n']):
        if dump['site_plateau'][s] > p:
            continue
        b = dump['site_block'][s]
        while dump['block_parent'][b] != NONE and dump['block_plateau'][dump['block_parent'][b]] <= p:
            b = dump['block_parent'][b]
        groups.setdefault(b, set()).add(dump['ids'][s])
    return sorted(sorted(g) for g in groups.values())


def oracle_blocks(entries, tree, value):
    """Blocs de l'ultrametrique exacte de l'oracle a la date `value` (t, M, Q) : sites entres, reunis si leurs
    remontees se rencontrent au plus a value."""
    n = len(entries)
    entered = [i for i in range(n) if date_sign(entries[i][1], value) <= 0]
    parent = {i: i for i in entered}

    def find(x):
        while parent[x] != x:
            x = parent[x]
        return x
    for a in entered:
        for b in entered:
            if a < b and find(a) != find(b):
                w = tree.lca(entries[a][0], entries[b][0])
                if w in (entries[a][0], entries[b][0]) or date_sign((tree.nodes[w].level, ZERO, ZERO), value) <= 0:
                    parent[find(a)] = find(b)
    groups = {}
    for i in entered:
        groups.setdefault(find(i), set()).add(i)
    return sorted(sorted(g) for g in groups.values())


def compare(dump, res, n, m, stats):
    stats['exact'] += dump['probe']['exact_decisions']
    levels = {int(r): Fraction(int(v[0], 16), int(v[1], 16)) for r, v in dump['levels'].items()}
    ref, tree = reference_radius(res, n, m)
    for s in range(n):
        orig = dump['ids'][s]
        owner, want = ref[orig]
        got = (levels[dump['t'][s]], levels[dump['M'][s]], levels[dump['Q'][s]])
        if date_sign(got, want) != 0:
            raise Disagreement('date du site %d : %r contre %r' % (orig, got, want))
        o = dump['owner'][s]
        native = (levels[dump['rank'][o]], frozenset(dump['cover'][o]))
        if native != owner_signature(res, n, owner):
            raise Disagreement('proprietaire du site %d' % orig)
        stats['comparisons'] += 1
        stats['delayed'] += dump['M'][s] != 0
    entries = [ref[i] for i in range(n)]
    for p in range(len(dump['plateau_t'])):
        value = (levels[dump['plateau_t'][p]], levels[dump['plateau_m'][p]], levels[dump['plateau_q'][p]])
        if native_blocks(dump, levels, p) != oracle_blocks(entries, tree, value):
            raise Disagreement('blocs au plateau %d' % p)
        stats['plateaus'] += 1


def probe(args, points, name, k, m):
    xyz, idp = os.path.join(args.work, name + '.xyz'), os.path.join(args.work, name + '.ids')
    with open(xyz, 'wb') as out:
        out.write(b''.join(struct.pack('<III', *p) for p in points))
    with open(idp, 'wb') as out:
        out.write(b''.join(struct.pack('<I', i) for i in range(len(points))))
    dump = os.path.join(args.work, name + '.json')
    argv = [args.probe, '--input=%s,%s' % (xyz, idp), '--k=%d' % k, '--workers=2', '--dump=' + dump, '--cover']
    if m is not None:  # sans --m : qualification m(K) du produit
        argv.append('--m=%d' % m)
    done = subprocess.run(argv, stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=120, check=False)
    if done.returncode != 0:
        raise ProbeError('%s K%d m%s : code %d %s' % (name, k, m, done.returncode, done.stdout.decode()[-200:]))
    with open(dump) as handle:
        data = json.load(handle)
    data['probe'] = json.loads(done.stdout.decode().splitlines()[-1])
    for path in (xyz, idp, dump):
        os.remove(path)
    return data


def random_cloud(rng):
    n = rng.randint(4, 9)
    side = rng.choice([3, 4, 6, 10, 40])
    scale = rng.choice([1, 7, 1000])
    pts = set()
    while len(pts) < n:
        pts.add((rng.randrange(side) * scale, rng.randrange(side) * scale, rng.choice([0, rng.randrange(side) * scale])))
    pts = sorted(pts)
    rng.shuffle(pts)
    return pts


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--probe', required=True)
    parser.add_argument('--work', required=True)
    parser.add_argument('--clouds', type=int, default=160)
    parser.add_argument('--seed', type=int, default=20261003)
    parser.add_argument('--min-clouds', type=int, default=0)
    parser.add_argument('--min-comparisons', type=int, default=0)
    parser.add_argument('--min-delayed', type=int, default=0)
    parser.add_argument('--min-exact', type=int, default=0, help='decisions natives tranchees par le repli exact')
    args = parser.parse_args()
    os.makedirs(args.work, exist_ok=True)
    rng = random.Random(args.seed)
    clouds = [('triangles', EQUILATERAL), ('cinq', FIVE), ('huit', EIGHT), ('plateau', PLATEAU)]
    clouds += [('c%04d' % i, random_cloud(rng)) for i in range(args.clouds)]
    stats = dict(clouds=0, orders=0, comparisons=0, delayed=0, plateaus=0, exact=0)
    verdict, code = 'conforme', 0
    try:
        for name, points in clouds:
            n = len(points)
            definition = Definition(points)
            for k in range(1, min(4, n - 1) + 1):
                res = definition.order(k)
                for m in sorted({1, formats.qualification(k)}):
                    compare(probe(args, points, '%s_k%d_m%d' % (name, k, m), k, m), res, n, m, stats)
                    stats['orders'] += 1
            stats['clouds'] += 1
    except Disagreement as error:
        print('desaccord : %s (%s K%d m%d)' % (error, name, k, m), file=sys.stderr)
        verdict, code = 'desaccord', 1
    except (ProbeError, OSError, ValueError, subprocess.TimeoutExpired) as error:
        print('refus : %s' % error, file=sys.stderr)
        verdict, code = 'refus', 2
    if code == 0 and (stats['clouds'] < args.min_clouds or stats['comparisons'] < args.min_comparisons or
                      stats['delayed'] < args.min_delayed or stats['exact'] < args.min_exact):
        verdict, code = 'plancher', 3
    print('points_oracle_verdict %s nuages=%d ordres=%d comparaisons=%d retardes=%d plateaux=%d replis=%d' % (
        verdict, stats['clouds'], stats['orders'], stats['comparisons'], stats['delayed'], stats['plateaus'],
        stats['exact']))
    return code


if __name__ == '__main__':
    raise SystemExit(main())
