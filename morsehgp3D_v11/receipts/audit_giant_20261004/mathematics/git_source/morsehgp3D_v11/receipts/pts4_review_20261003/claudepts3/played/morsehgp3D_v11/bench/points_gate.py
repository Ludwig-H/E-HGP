#!/usr/bin/env python3
"""Porte de la pendaison FULL -> points : export natif + consommateur contre l'oracle de la definition.

    python3 bench/points_gate.py --export BUILD/mhgp11_points_export --work DIR --out FICHIER.json [--seconds 240]

Pour chaque nuage (fixtures gravees puis nuages aleatoires a ex aequo frequents, au plus neuf sites, k <= 4),
l'export natif est lu par bench/points_hierarchy.py ; les cinq regles (core, cover, first, margin1, margin, avec
m = 1, k + 1, k + 2) rendent des ultrametriques exactes comparees a celles de bench/points_reference.py (force
brute sur les coupes de Gamma_k, etage A) ; la marge en rayon (bench/points_radius.py, decisions exactes) est
comparee a l'oracle decimal a 120 chiffres, a 1e-9 relatif. Les sites natifs sont reindexes par leurs PointId.
Codes : 0 conforme ; 1 desaccord ; 3 plancher non atteint ou fixture fausse ; 2 refus d'entree.
"""
import argparse
from fractions import Fraction
import json
import os
from pathlib import Path
import random
import subprocess
import sys
import time

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import points_hierarchy as ph  # noqa: E402
import points_reference as pr  # noqa: E402
import points_radius as prad  # noqa: E402
from hgp11_ref import Definition  # noqa: E402

EQUILATERAL = [tuple(c + 2 for c in p) for p in ((-1, -1, 0), (-1, 0, -1), (0, 0, 0), (1, 1, 0), (2, 2, 0), (2, 1, 1))]
FIVE = [(6, 2, 0), (0, 0, 0), (0, 4, 0), (12, 0, 0), (12, 4, 0)]
FLOORS = dict(clouds=150, comparisons=4000, delayed=500, fixtures=4)


def run_export(binary, points, work, name, kmax):
    xyz, ids = work / (name + '.xyz'), work / (name + '.ids')
    np.asarray(points, dtype='<u4').tofile(xyz)
    np.arange(len(points), dtype='<u4').tofile(ids)
    out = work / (name + '.ph')
    orders = ','.join(str(k) for k in range(1, kmax + 1))
    done = subprocess.run([str(binary), str(xyz), str(ids), str(out), str(kmax), orders, '2', str(1 << 30)],
                          capture_output=True, text=True, timeout=60)
    if done.returncode != 0:
        raise RuntimeError('export %s code %d : %s' % (name, done.returncode, done.stdout + done.stderr))
    data = ph.read_export(out)
    for path in (xyz, ids, out):
        path.unlink()
    return data


def native_ultrametrics(data, k, m):
    order = data['orders'][k]
    ids = data['ids'].tolist()
    out = {}
    for rule, hanging in pr.bench_hangings(order, m).items():
        u = ph.ultrametric(hanging)
        n = len(u)
        back = [[None] * n for _ in range(n)]
        for a in range(n):
            for b in range(n):
                back[ids[a]][ids[b]] = u[a][b]
        out[rule] = back
    return out


def compare_cloud(binary, points, work, name, stats):
    n = len(points)
    kmax = min(4, n)
    data = run_export(binary, points, work, name, kmax)
    definition = Definition(points)
    delayed = 0
    for k in range(1, kmax + 1):
        res = definition.order(k)
        for m in sorted(set([1, k + 1, k + 2])):
            if m > n:
                continue
            ref, tree = pr.reference_rules(res, n, m)
            native = native_ultrametrics(data, k, m)
            for rule in pr.RULES:
                stats['comparisons'] += 1
                if pr.reference_ultrametric(ref[rule], tree) != native[rule]:
                    stats['disagreements'].append(dict(cloud=name, points=points, k=k, m=m, rule=rule))
            order = data['orders'][k]
            delayed += ph.hang_margin(order, m).extra['delayed']
            # Marge en rayon : regle exacte du banc contre l'oracle decimal (120 chiffres) de la definition.
            refr, treer = pr.reference_radius_rules(res, n, m)
            ur = pr.reference_radius_ultrametric(refr['margin_r'], treer)
            ub = prad.ultrametric_radius(prad.hang_margin_radius(order, m))
            ids = data['ids'].tolist()
            stats['comparisons'] += 1
            if any(abs(float(ur[ids[a]][ids[b]]) - ub[a][b]) > 1e-9 * max(1.0, abs(ub[a][b]))
                   for a in range(n) for b in range(n)):
                stats['disagreements'].append(dict(cloud=name, points=points, k=k, m=m, rule='margin_r'))
    stats['clouds'] += 1
    stats['delayed'] += delayed


def fixtures(binary, work, stats):
    """Faits exacts graves : deux triangles a k = 2, cinq points sans percolation, continuite {0,2,4}, k = 1."""
    facts = []
    data = run_export(binary, EQUILATERAL, work, 'equilateral', 2)
    for m, expected in ((3, [[0, 1, 2], [3, 4, 5]]), (1, [[0, 1], [4, 5]])):
        u = native_ultrametrics(data, 2, m)['margin' if m > 1 else 'margin1']
        got = [sorted(b) for b in pr.blocks_at(u, Fraction(1))]
        facts.append(dict(fixture='deux_triangles_k2_m%d' % m, blocks=got, ok=got == expected))
    data = run_export(binary, FIVE, work, 'five', 2)
    u = native_ultrametrics(data, 2, 3)['margin']
    got = [sorted(b) for b in pr.blocks_at(u, Fraction(35))]
    facts.append(dict(fixture='cinq_points_k2_m3', blocks=got, entry0=str(u[0][0]),
                      ok=got == [[1, 2], [3, 4]] and u[0][0] == 36))
    medians = []
    for d in (0, 1):
        data = run_export(binary, [(0, 0, 0), (2000, 0, 0), (4000 + d, 0, 0)], work, 'line%d' % d, 2)
        medians.append(native_ultrametrics(data, 2, 1)['margin1'][1][1])
    facts.append(dict(fixture='continuite_024', entries=[str(x) for x in medians],
                      ok=medians == [Fraction(4000000), Fraction(4001000)]))
    stats['fixtures'] = facts
    return all(f['ok'] for f in facts)


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
    parser.add_argument('--export', required=True, type=Path)
    parser.add_argument('--work', required=True, type=Path)
    parser.add_argument('--out', required=True, type=Path)
    parser.add_argument('--seconds', type=float, default=240.0)
    parser.add_argument('--seed', type=int, default=20261003)
    parser.add_argument('--mirror', type=Path, help='seconde copie du verdict (collectee avec la session)')
    args = parser.parse_args()
    if not args.export.is_file():
        print('refus : exportateur absent', file=sys.stderr)
        return 2
    args.work.mkdir(parents=True, exist_ok=True)
    stats = dict(clouds=0, comparisons=0, delayed=0, disagreements=[], fixtures=[])
    started = time.monotonic()
    fixtures_ok = fixtures(args.export, args.work, stats)
    rng = random.Random(args.seed)
    index = 0
    while time.monotonic() - started < args.seconds:
        compare_cloud(args.export, random_cloud(rng), args.work, 'c%05d' % index, stats)
        index += 1
    floors_ok = all(stats[key] >= value for key, value in FLOORS.items() if key != 'fixtures') and \
        len(stats['fixtures']) >= FLOORS['fixtures']
    verdict = 'desaccord' if stats['disagreements'] else ('conforme' if fixtures_ok and floors_ok else 'plancher')
    stats.update(verdict=verdict, floors=FLOORS, seconds=time.monotonic() - started, seed=args.seed,
                 disagreements=stats['disagreements'][:20])
    text = json.dumps(stats, indent=1, sort_keys=True, default=str) + '\n'
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(text)
    if args.mirror:
        args.mirror.parent.mkdir(parents=True, exist_ok=True)
        args.mirror.write_text(text)
    print('points_gate_verdict %s nuages%d comparaisons%d retardes%d fixtures%d/%d' % (
        verdict, stats['clouds'], stats['comparisons'], stats['delayed'],
        sum(f['ok'] for f in stats['fixtures']), len(stats['fixtures'])))
    return {'conforme': 0, 'desaccord': 1}.get(verdict, 3)


if __name__ == '__main__':
    raise SystemExit(main())
