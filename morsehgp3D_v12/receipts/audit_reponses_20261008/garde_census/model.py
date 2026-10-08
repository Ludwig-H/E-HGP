#!/usr/bin/env python3
"""Bornes exactes de garde : modele Fraction, aucun code moteur execute."""
import argparse
import hashlib
import itertools as it
import json
import random
import subprocess
import tempfile
from fractions import Fraction as F
from pathlib import Path


def need(ok, message):
    if not ok:
        raise SystemExit(message)


def sign(x):
    return (x > 0) - (x < 0)


def solve(a, rhs):
    n = len(a)
    a = [[F(x) for x in row] + [F(rhs[i])] for i, row in enumerate(a)]
    for j in range(n):
        p = next((p for p in range(j, n) if a[p][j]), None)
        if p is None:
            return None
        a[j], a[p] = a[p], a[j]
        factor = a[j][j]
        a[j] = [x / factor for x in a[j]]
        for i in range(n):
            if i != j:
                factor = a[i][j]
                a[i] = [a[i][k] - factor * a[j][k] for k in range(n + 1)]
    return [a[i][-1] for i in range(n)]


def sphere(points, certified=True):
    base = points[0]
    vectors = [tuple(p[a] - base[a] for a in range(3)) for p in points[1:]]
    dot = lambda u, v: sum(a * b for a, b in zip(u, v))
    alpha = solve([[dot(u, v) for v in vectors] for u in vectors], [F(dot(u, u), 2) for u in vectors])
    if alpha is None:
        return None
    weights = [1 - sum(alpha)] + alpha
    if certified and min(weights) <= 0:
        return None
    center = tuple(F(base[a]) + sum(w * u[a] for w, u in zip(alpha, vectors)) for a in range(3))
    radius = sum((center[a] - base[a]) ** 2 for a in range(3))
    return center, radius


def power(ball, p):
    center, radius = ball
    return sum((F(p[a]) - center[a]) ** 2 for a in range(3)) - radius


def box_points(ball, box):
    center, _ = ball
    lo, hi = box
    near, far = [], []
    for a, c in enumerate(center):
        floor = c.numerator // c.denominator
        p = min((floor, floor + 1), key=lambda x: ((x - c) ** 2, x))
        near.append(max(lo[a], min(hi[a], p)))
        far.append(max((lo[a], hi[a]), key=lambda x: (x - c) ** 2))
    return near, far


def exact_box(ball, box):
    # Minimum entier separe par axe, maximum aux huit coins, sans garde.
    center, radius = ball
    lo, hi = box
    lower = -radius
    for a, c in enumerate(center):
        floor = c.numerator // c.denominator
        values = {lo[a], hi[a], max(lo[a], min(hi[a], floor)), max(lo[a], min(hi[a], floor + 1))}
        lower += min((F(x) - c) ** 2 for x in values)
    if lower > 0:
        return 1, 1
    upper = max(power(ball, p) for p in it.product(*zip(lo, hi)))
    return sign(lower), sign(upper)


def guarded_box(ball, box, low, high):
    lo, hi = box
    if any(hi[a] <= low[a] or lo[a] >= high[a] for a in range(3)):
        return (1, 1), 0
    near, far = box_points(ball, box)
    need(all(low[a] < near[a] < high[a] for a in range(3)), 'minorant hors garde')
    lower = sign(power(ball, near))
    if lower > 0:
        return (1, 1), 1
    if not all(lo[a] > low[a] and hi[a] < high[a] for a in range(3)):
        return (lower, 1), 1
    need(all(low[a] < far[a] < high[a] for a in range(3)), 'majorant hors garde')
    return (lower, sign(power(ball, far))), 2


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--repo', required=True, type=Path)
    args = parser.parse_args()
    here = Path(__file__).resolve().parent
    cap = json.loads((here / 'capture.json').read_text())

    def source(rel):
        return subprocess.check_output(['git', 'show', cap['pin'] + ':morsehgp3D_v12/' + rel], cwd=args.repo)

    def pins():
        for rel, sha in cap['source_sha256'].items():
            need(hashlib.sha256(source(rel)).hexdigest() == sha, 'source: ' + rel)
        need(hashlib.sha256((here / 'proposition.patch').read_bytes()).hexdigest() == cap['patch_sha256'], 'patch')
        for rel, sha in cap.get('artifact_sha256', {}).items():
            need(hashlib.sha256((here / rel).read_bytes()).hexdigest() == sha, 'artefact: ' + rel)

    pins()
    cube = list(it.product((32, 34), repeat=3))
    supports = [p for q in (2, 3, 4) for p in it.combinations(cube, q)]
    limit = 2 ** 32 - 1
    supports += [((0, 0, 0),), ((limit, limit, limit),)]
    supports += [((0, 0, 0), (limit, limit, limit)),
                 ((0, 0, 0), (limit, limit, 0), (limit, 0, limit)),
                 ((0, 0, 0), (limit, limit, 0), (limit, 0, limit), (0, limit, limit))]
    totals = dict(certified=0, rejected_candidates=0, points=0, boxes=0, point_powers_saved=0,
                  box_powers_saved=0, new_disjoint_boxes=0, upper_powers_saved=0)
    arities = set()
    for support in supports:
        ball = sphere(support)
        if ball is None:
            totals['rejected_candidates'] += 1
            continue
        totals['certified'] += 1
        arities.add(len(support))
        corner = tuple(min(p[a] for p in support) for a in range(3))
        width = tuple(max(p[a] for p in support) - corner[a] for a in range(3))
        span = max(width).bit_length()
        m = 1 << span
        need(ball[1] <= F(sum(x * x for x in width), 4) < m * m, 'rayon / demi-diagonale')
        old = (tuple(x - 2 * m for x in corner), tuple(x + 3 * m for x in corner))
        new = (tuple(x - m for x in corner), tuple(x + 2 * m for x in corner))
        need(all(-(1 << 63) < x < 1 << 63 for guard in (old, new) for face in guard for x in face), 'borne signee')
        center = tuple(c.numerator // c.denominator for c in ball[0])
        probes = set(support)
        for low, high in (old, new):
            for a in range(3):
                for value in (low[a] - 1, low[a], low[a] + 1, high[a] - 1, high[a], high[a] + 1):
                    point = list(center)
                    point[a] = value
                    probes.add(tuple(point))
        for point in probes:
            expected = sign(power(ball, point))
            calls = []
            for low, high in (old, new):
                inside = all(low[a] < point[a] < high[a] for a in range(3))
                need((expected if inside else 1) == expected, 'site perdu')
                calls.append(int(inside))
            need(calls[1] <= calls[0], 'appels site croissants')
            totals['point_powers_saved'] += calls[0] - calls[1]
            totals['points'] += 1
        # Contacts exacts des nouvelles faces, plus boites arbitraires ; uniquement domaine des boites du Cloud.
        corners = sorted({tuple(max(0, min(limit, x)) for x in p) for p in probes})
        boxes = {(p, p) for p in corners}
        rng = random.Random(20261008)
        for _ in range(24):
            a, b = rng.choice(corners), rng.choice(corners)
            boxes.add((tuple(map(min, a, b)), tuple(map(max, a, b))))
        for box in boxes:
            expected = exact_box(ball, box)
            before, n0 = guarded_box(ball, box, *old)
            after, n1 = guarded_box(ball, box, *new)
            need(before == after == expected, 'signes de boite')
            need(n1 <= n0, 'appels boite croissants')
            totals['box_powers_saved'] += n0 - n1
            totals['new_disjoint_boxes'] += int(n0 == 1 and n1 == 0)
            totals['upper_powers_saved'] += int(n0 == 2 and n1 == 1)
            totals['boxes'] += 1
    # Sans positivite du certificat, le support ne borne pas le rayon : ancien temoin exact.
    invalid = ((419, 0, 0), (435, 15, 0), (434, 14, 0))
    need(sphere(invalid) is None, 'candidate non certifiee admise')
    need(power(sphere(invalid, False), (0, 479, 0)) == 0, 'contact de candidate')
    need(not 419 - 32 < 0 < 419 + 64, 'temoin hors nouvelle garde')
    # Nouveau temoin de repli large, encore dans la petite garde, final representable en i128.
    h = 2 ** 21 - 1
    d, numerator = 6 * h ** 4, (4 * h ** 5, 2 * h ** 5, 2 * h ** 5)
    wb = sphere(((0, 0, 0), (h, h, 0), (h, 0, h)))
    need(wb == (tuple(F(n, d) for n in numerator), F(2 * h * h, 3)), 'sphere wide independante')
    first = d * (3 * h * h)
    final = first - 2 * h * sum(numerator)
    need(2 ** 127 <= first and 0 < final < 2 ** 127 and final == 2 * h ** 6, 'temoin intermediaire large')
    need(all(-2 ** 21 < h < 2 * 2 ** 21 for _ in range(3)), 'temoin wide hors garde')
    domain = max(t for t in range(62) if d < 2 ** (123 - 2 * t)
                 and all(abs(n) < 2 ** (124 - t) for n in numerator))
    need(domain == 17, 'certificat du nouveau temoin wide')
    # Lemme secondaire : ordre des sommes partielles de power_sign, sans changer le patch.
    partial_cases = 0
    for anchor, center, query in it.product(it.product((F(0), F(3, 4)), repeat=3),
                                           it.product((F(0), F(3, 4)), repeat=3),
                                           it.product((F(-3, 4), F(7, 4)), repeat=3)):
        v, w = [query[j] - anchor[j] for j in range(3)], [center[j] - anchor[j] for j in range(3)]
        total = sum(x * x for x in v)
        need(0 <= total < 12, 'produit initial normalise')
        for j in range(3):
            term = -2 * w[j] * v[j]
            need(abs(term) < 4, 'produit lineaire normalise')
            total += term
            need(-3 < total < 12, 'somme partielle normalisee')
        partial_cases += 1
    with tempfile.TemporaryDirectory(prefix='audit-garde-patch-') as td:
        root = Path(td)
        for rel in cap['patched_files']:
            path = root / 'morsehgp3D_v12' / rel
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(source(rel))
        subprocess.run(['git', 'apply', '--check', str(here / 'proposition.patch')], cwd=root, check=True)
    pins()
    result = dict(status='ok', **totals, arities=sorted(arities),
                  wide_first_bits=first.bit_length(), wide_final_bits=final.bit_length(),
                  wide_power_domain=domain, partial_sum_cases=partial_cases,
                  native_executed=False, performance_qualified=False)
    if 'expected_result' in cap:
        need(result == cap['expected_result'], 'resultat different de la capture')
    print(json.dumps(result, sort_keys=True))


if __name__ == '__main__':
    main()
