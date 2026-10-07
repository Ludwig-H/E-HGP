#!/usr/bin/env python3
"""Oracle exact borne a quatre sites ; aucun natif, aucun assert, Python nu.

Geometrie independante des fenetres : plus petit disque par supports 1..3,
contre-verifiee par l'appartenance du centre a une enveloppe convexe.
--repo permet seulement un contre-rejeu du modele historique epingle.
"""
import argparse
from fractions import Fraction as F
import hashlib
import importlib.util
from itertools import combinations
import json
from pathlib import Path
import sys

sys.dont_write_bytecode = True
HERE = Path(__file__).resolve().parent


def need(condition, message):
    if not condition:
        raise ValueError(message)


def cross(a, b):
    return a[0] * b[1] - a[1] * b[0]


def delta(a, b):
    return tuple(x - y for x, y in zip(a, b))


def norm(a):
    return sum(x * x for x in a)


def barycentric(a, b, c, x):
    ab, ac, ax = delta(b, a), delta(c, a), delta(x, a)
    d = cross(ab, ac)
    if d == 0:
        return None
    v, w = F(cross(ax, ac), d), F(cross(ab, ax), d)
    return 1 - v - w, v, w


def in_hull(points, center):
    """Caratheodory dans le plan : point, segment ou triangle exact."""
    if center in points:
        return True
    for a, b in combinations(points, 2):
        u, v = delta(a, center), delta(b, center)
        if cross(u, v) == 0 and sum(x * y for x, y in zip(u, v)) <= 0:
            return True
    for a, b, c in combinations(points, 3):
        weights = barycentric(a, b, c, center)
        if weights is not None and min(weights) >= 0:
            return True
    return False


def meb(points):
    """Enumere les cercles de supports <=3 puis minimise parmi les contenants."""
    centers = [tuple(map(F, point)) for point in points]
    centers += [tuple(F(x + y, 2) for x, y in zip(a, b))
                for a, b in combinations(points, 2)]
    for a, b, c in combinations(points, 3):
        u, v = delta(b, a), delta(c, a)
        d = 2 * cross(u, v)
        if d:
            centers.append((a[0] + F(norm(u) * v[1] - u[1] * norm(v), d),
                            a[1] + F(u[0] * norm(v) - norm(u) * v[0], d)))
    candidates = [(max(norm(delta(p, c)) for p in points), c) for c in centers]
    return min(candidates)


def quotient(vertices, linked, expand):
    remaining = set(vertices)
    components = []
    while remaining:
        seen, todo = set(), [min(remaining)]
        while todo:
            x = todo.pop()
            if x in seen:
                continue
            seen.add(x)
            todo.extend(y for y in remaining if y not in seen and linked(x, y))
        remaining -= seen
        components.append(sorted(set(a for x in seen for a in expand(x))))
    return sorted(components)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo', type=Path)
    args = parser.parse_args()
    data = json.loads((HERE / 'fixture.json').read_text())
    labels = data['labels']
    points = [tuple(p[:2]) for p in data['points']]
    center = (0, 0)
    n = len(points)
    all_masks = range(1, 1 << n)
    members = lambda mask: [i for i in range(n) if mask >> i & 1]
    name = lambda mask: ''.join(labels[i] for i in members(mask))
    names = lambda masks: sorted(name(mask) for mask in masks)
    submasks = lambda mask, t: [sum(1 << i for i in subset)
                               for subset in combinations(members(mask), t)]
    need(n == 4 and all(norm(p) == 25 for p in points), 'sphere')
    need(len(set(points)) == n, 'distinct sites')
    shifted = [tuple(p[j] + data['translation'][j] for j in range(2)) for p in points]
    need([list(p) + [0] for p in shifted] == data['native_points'], 'translation')
    need(all(0 <= x < 2 ** 21 for p in data['native_points'] for x in p), 'u21')

    separable = {}
    beta = {}
    for mask in all_masks:
        part = [points[i] for i in members(mask)]
        radius, _ = meb(part)
        separable[mask] = radius < 25
        beta[name(mask)] = str(radius)
        need(separable[mask] == (not in_hull(part, center)), 'MEB/hull disagreement')
        moved = [shifted[i] for i in members(mask)]
        need(meb(moved)[0] == radius, 'MEB translation')
        need(in_hull(moved, tuple(data['translation'][:2])) == (not separable[mask]),
             'hull translation')
    need(meb(points) == (F(25), (F(0), F(0))), 'circle MEB')
    q_min = min(mask.bit_count() for mask in all_masks if not separable[mask])
    need(q_min == data['q_min'] == 3, 'q_min')
    need(data['window_orders'] == list(range(data['p'] + q_min - 1, data['p'] + n + 1)),
         'window orders')

    # Restriction au cercle du pseudocode historique : deux orientations, chaque ancre.
    windows = set()
    for sign in (1, -1):
        for i, a in enumerate(points):
            mask = 1 << i
            for j, b in enumerate(points):
                if sign * cross(a, b) > 0:
                    mask |= 1 << j
            windows.add(mask)
    maximal = {mask for mask in all_masks if separable[mask] and not any(
        mask != other and (mask & other) == mask and separable[other] for other in all_masks)}
    need(names(windows) == data['windows_both_orientations'], 'windows')
    need(names(maximal) == data['maximal_separable'], 'three maximal sets')
    need(maximal <= windows and all(separable[w] for w in windows), 'family completeness')
    gh, egh = 0b0110, 0b0111
    need(gh in windows and egh in maximal and (gh & egh) == gh and gh != egh,
         'strictly nonmaximal window')
    need([sum(p) for p in points[:3]] == [5, 7, 1], 'integer separator (1,1)')
    need(barycentric(points[0], points[2], points[3], center) ==
         (F(3, 8), F(5, 16), F(5, 16)), 'EHJ support')
    need(barycentric(points[1], points[2], points[3], center) ==
         (F(3, 7), F(1, 8), F(25, 56)), 'GHJ support')

    partitions = {}
    complements = {}
    for t in range(1, n + 1):
        vertices = [m for m in all_masks if m.bit_count() == t and separable[m]]
        raw = quotient(vertices, lambda a, b: separable[a | b], lambda a: [a])
        expected = data['partitions'][str(t)]
        translated_raw = sorted(sorted(name(m) for m in comp) for comp in raw)
        need(translated_raw == expected, 'geometric partition t=' + str(t))
        for family in (windows, maximal):
            alive = [m for m in family if m.bit_count() >= t]
            got = quotient(alive, lambda a, b: (a & b).bit_count() >= t,
                           lambda m: submasks(m, t))
            need(got == raw, 'quotient partition t=' + str(t))
        partitions[str(t)] = translated_raw
        covered = 0
        for m in windows:
            if m.bit_count() >= t:
                covered |= m
        complements[str(t)] = name(((1 << n) - 1) ^ covered)

    historical = None
    if args.repo:
        rel = 'morsehgp3D_v11/receipts/conception_v11_20261002/conception/preuves_tour/quotient_check.py'
        path = args.repo / rel
        capture = json.loads((HERE / 'capture.json').read_text())
        need(hashlib.sha256(path.read_bytes()).hexdigest() == capture['sources'][rel],
             'historical source hash')
        spec = importlib.util.spec_from_file_location('historical_quotient', path)
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        vectors = [tuple(p) for p in data['points']]
        old = module.maximal_separable(vectors)
        need(old == windows, 'historical full family')
        for t in range(1, n + 1):
            expected = data['partitions'][str(t)]
            for comps in (module.design_pieces(vectors, t, old), module.brute_pieces(vectors, t)):
                got = [] if comps is None else sorted(sorted(''.join(labels[i] for i in part)
                    for part in comp) for comp in comps)
                need(got == expected, 'historical quotient t=' + str(t))
        historical = 'bounded four-site model agrees; full suite not invoked'
    print(json.dumps({'status': 'ok', 'geometry_subsets': 15, 'translations': 2,
        'radius_squared': 25, 'q_min': q_min, 'window_orders': data['window_orders'],
        'windows': names(windows), 'maximal_separable': names(maximal),
        'all_windows_maximal': False, 'strict_inclusion': ['GH', 'EGH'],
        'partitions': partitions, 'piece_counts': [len(partitions[str(t)]) for t in range(1, 5)],
        'uncovered_sites': complements, 'beta': beta, 'historical_crosscheck': historical},
        indent=2, sort_keys=True))


if __name__ == '__main__':
    try:
        main()
    except (ValueError, OSError, KeyError, TypeError) as error:
        print(str(error), file=sys.stderr)
        sys.exit(1)
