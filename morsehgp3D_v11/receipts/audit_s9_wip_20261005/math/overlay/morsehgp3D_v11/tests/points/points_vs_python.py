#!/usr/bin/env python3
"""Differentiel exact de la hierarchie de points native (tranche S9) contre la chaine Python qualifiee (label long :
numpy, hors suite fast).

    points_vs_python.py --probe <mhgp11_points_probe> --export <mhgp11_points_export> --work <dossier>
                        [--clouds N] [--seed S] [--uniform=n,...] [--data=<nom>,...] [--k=1,2,...] [--workers W]
                        [--min-clouds N] [--min-sites N] [--min-delayed N]

Pour chaque nuage et chaque ordre K : l'exportateur MHGP11PH (bench/points_export.cpp, kmax = K, ordres = K) est lu par
bench/points_hierarchy.py, puis pendu par bench/points_radius.hang_margin_radius(order, m(K)) et mis en arbre par
bench/points_flat.tower_point_tree ; la sonde native (memes entrees, meme catalogue Cat_K, donc memes rangs et meme
numerotation des noeuds) ecrit sa pendaison et son arbre de points. Identite EXACTE exigee, site par site : sites
(ordre de Morton, PointId), noeuds (parent, rang), date (niveaux exacts de t, M, Q egaux aux Fraction de la valeur
Python), proprietaire, rang plancher, drapeau strict ; puis plateaux (niveaux exacts), blocs (plateau, parent) et
entree de chaque site (bloc, plateau). Nuages : aleatoires a ex aequo frequents (comme bench/points_gate.py), temoins
exacts de la porte Python, familles uniformes (--uniform) et trames du dossier MHGP11_DATA_DIR (--data) ; --m-all
ajoute m = 1 et m = K + 1 a chaque ordre K >= 2 (sonde --m).

Codes : 0 conforme ; 1 ecart ; 2 refus (usage, programme en echec) ; 3 plancher. Derniere ligne :
    points_vs_python_verdict <conforme|ecart|refus|plancher> nuages=<c> ordres=<o> sites=<s> retardes=<d> plateaux=<p>
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
sys.path.insert(0, os.path.join(V11, 'bench'))
sys.path.insert(0, os.path.join(V11, 'reference'))

import points_flat as pf  # noqa: E402
import points_hierarchy as ph  # noqa: E402
import points_radius as prad  # noqa: E402

EQUILATERAL = [tuple(c + 2 for c in p) for p in ((-1, -1, 0), (-1, 0, -1), (0, 0, 0), (1, 1, 0), (2, 2, 0), (2, 1, 1))]
FIVE = [(6, 2, 0), (0, 0, 0), (0, 4, 0), (12, 0, 0), (12, 4, 0)]
EIGHT = [(20, 20, 0), (30, 20, 0), (40, 20, 0), (50, 20, 0), (64, 20, 0), (74, 20, 0), (0, 30, 0), (0, 10, 0)]
PLATEAU = [(4, 4, 4), (6, 6, 4), (0, 0, 4), (8, 8, 4)]
NONE = (1 << 32) - 1


class Gap(Exception):
    pass


class Refusal(Exception):
    pass


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


def uniform(count, seed=20261002):
    rng = random.Random(seed)
    pts = set()
    while len(pts) < count:
        pts.add((rng.getrandbits(18), rng.getrandbits(18), rng.getrandbits(18)))
    return sorted(pts)


def write_inputs(folder, name, points, ids=None):
    xyz, idp = os.path.join(folder, name + '.xyz'), os.path.join(folder, name + '.ids')
    with open(xyz, 'wb') as out:
        out.write(b''.join(struct.pack('<III', *p) for p in points))
    with open(idp, 'wb') as out:
        out.write(b''.join(struct.pack('<I', i) for i in (ids if ids is not None else range(len(points)))))
    return xyz, idp


def call(argv, timeout=3600):
    done = subprocess.run(argv, stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=timeout, check=False)
    if done.returncode != 0:
        raise Refusal('%s : code %d %s %s' % (os.path.basename(argv[0]), done.returncode,
                                               done.stdout.decode()[-300:], done.stderr.decode()[-300:]))
    return done.stdout.decode()


def fraction(pair):
    return Fraction(int(pair[0], 16), int(pair[1], 16))


def need(condition, what):
    if not condition:
        raise Gap(what)


def compare(native, order, hang, tree, ids):
    """Identite exacte native / Python pour un ordre ; rend (sites, retardes, plateaux)."""
    levels = {int(r): fraction(v) for r, v in native['levels'].items()}
    n = order.n
    need(native['n'] == n and native['ids'] == [int(i) for i in ids], 'sites')
    need(native['parent'] == [NONE if p < 0 else int(p) for p in order.parent.tolist()] and
         native['rank'] == [int(r) for r in order.rank.tolist()], 'noeuds')
    for s in range(n):
        v = hang.values[s]
        got = (levels[native['t'][s]], levels[native['M'][s]], levels[native['Q'][s]])
        need(got == (v.t, v.m, v.q), 'date du site %d : %r contre %r' % (s, got, (v.t, v.m, v.q)))
        need(native['owner'][s] == int(hang.owner[s]), 'proprietaire du site %d' % s)
        need(native['floor'][s] == int(hang.floor[s]) and native['strict'][s] == int(bool(hang.strict[s])),
             'plancher du site %d' % s)
        for r in (native['t'][s], native['M'][s], native['Q'][s], native['floor'][s]):
            need(Fraction(*order.levels.exact(r)) == levels[r], 'niveau du rang %d' % r)
    plateaus = [(levels[t], levels[m], levels[q]) for t, m, q in
                zip(native['plateau_t'], native['plateau_m'], native['plateau_q'])]
    want = [(lv.t, lv.m, lv.q) for lv in tree.levels]
    need([(t, m if m != q else 0, q if m != q else 0) for t, m, q in plateaus] == want, 'plateaux')
    need(native['block_plateau'] == list(tree.block_plateau), 'blocs : plateaux')
    need(native['block_parent'] == [NONE if p < 0 else p for p in tree.block_parent], 'blocs : parents')
    need(native['site_block'] == tree.site_block.tolist() and native['site_plateau'] == tree.site_plateau.tolist(),
         'entrees des sites')
    return n, sum(1 for s in range(n) if native['M'][s] != 0), len(plateaus)


def one(args, name, xyz, idp, k, m, stats):
    ph_path = os.path.join(args.work, name + '.ph')
    if os.path.exists(ph_path):
        os.remove(ph_path)
    call([args.export, xyz, idp, ph_path, str(k), str(k), str(args.workers), str(1 << 40)])
    data = ph.read_export(ph_path)
    os.remove(ph_path)
    order = data['orders'][k]
    hang = prad.hang_margin_radius(order, m)
    tree = pf.tower_point_tree(hang)
    dump = os.path.join(args.work, name + '.json')
    call([args.probe, '--input=%s,%s' % (xyz, idp), '--k=%d' % k, '--m=%d' % m, '--workers=%d' % args.workers,
          '--dump=%s' % dump])
    with open(dump) as handle:
        native = json.load(handle)
    os.remove(dump)
    sites, delayed, plateaus = compare(native, order, hang, tree, data['ids'].tolist())
    stats['orders'] += 1
    stats['sites'] += sites
    stats['delayed'] += delayed
    stats['plateaus'] += plateaus


def clouds(args):
    rng = random.Random(args.seed)
    out = [('triangles', EQUILATERAL), ('cinq', FIVE), ('huit', EIGHT), ('plateau', PLATEAU)]
    out += [('c%04d' % i, random_cloud(rng)) for i in range(args.clouds)]
    out += [('u%d' % n, uniform(n)) for n in args.uniform]
    return out


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--probe', required=True)
    parser.add_argument('--export', required=True)
    parser.add_argument('--work', required=True)
    parser.add_argument('--clouds', type=int, default=200)
    parser.add_argument('--seed', type=int, default=20261005)
    parser.add_argument('--uniform', type=lambda t: [int(x) for x in t.split(',') if x], default=[])
    parser.add_argument('--data', type=lambda t: [x for x in t.split(',') if x], default=[])
    parser.add_argument('--k', type=lambda t: [int(x) for x in t.split(',')], default=[1, 2, 3, 4])
    parser.add_argument('--m-all', action='store_true')
    parser.add_argument('--workers', type=int, default=2)
    parser.add_argument('--min-clouds', type=int, default=0)
    parser.add_argument('--min-sites', type=int, default=0)
    parser.add_argument('--min-delayed', type=int, default=0)
    args = parser.parse_args()
    os.makedirs(args.work, exist_ok=True)
    stats = dict(clouds=0, orders=0, sites=0, delayed=0, plateaus=0)
    verdict, code = 'conforme', 0
    try:
        items = [(name, write_inputs(args.work, name, pts), len(pts)) for name, pts in clouds(args)]
        root = os.environ.get('MHGP11_DATA_DIR', '')
        for frame in args.data:
            items.append((frame, (os.path.join(root, frame + '.u32le'), os.path.join(root, frame + '.ids.u32le')),
                          None))
        for name, (xyz, idp), n in items:
            for k in args.k:
                if n is not None and (k > n or (k >= 2 and k >= n)):
                    continue
                ms = sorted({prad.qualification(k)} | ({1, k + 1} if args.m_all and k >= 2 else set()))
                for m in ms:
                    one(args, '%s_k%d_m%d' % (name, k, m), xyz, idp, k, m, stats)
            stats['clouds'] += 1
    except Gap as error:
        print('ecart : %s (%s)' % (error, name), file=sys.stderr)
        verdict, code = 'ecart', 1
    except (Refusal, OSError, ValueError) as error:
        print('refus : %s' % error, file=sys.stderr)
        verdict, code = 'refus', 2
    if code == 0 and (stats['clouds'] < args.min_clouds or stats['sites'] < args.min_sites or
                      stats['delayed'] < args.min_delayed):
        verdict, code = 'plancher', 3
    print('points_vs_python_verdict %s nuages=%d ordres=%d sites=%d retardes=%d plateaux=%d' % (
        verdict, stats['clouds'], stats['orders'], stats['sites'], stats['delayed'], stats['plateaus']))
    return code


if __name__ == '__main__':
    raise SystemExit(main())
