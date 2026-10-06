#!/usr/bin/env python3
"""Independent Fraction witnesses; default compares frozen JSON, never writes it."""
from fractions import Fraction as F
from itertools import combinations
from pathlib import Path
import argparse
import json


def need(condition, message):
    if not condition:
        raise ValueError(message)


def sub(a, b):
    return tuple(F(x) - F(y) for x, y in zip(a, b))


def dot(a, b):
    return sum(x * y for x, y in zip(a, b))


def d2(a, b):
    v = sub(a, b)
    return dot(v, v)


def positive_balls(points):
    """Complete positive MEB supports for these <=3 planar-site witnesses."""
    need(2 <= len(points) <= 3 and all(p[2] == 0 for p in points), 'witness domain')
    balls = []
    for a, b in combinations(points, 2):
        z = tuple((F(x) + F(y)) / 2 for x, y in zip(a, b))
        balls.append((z, d2(z, a), 2))
    if len(points) == 3:
        a, b, c = points
        acute = all(dot(sub(q, p), sub(t, p)) > 0 for p, q, t in
                    ((a, b, c), (b, a, c), (c, a, b)))
        if acute:
            u, v = sub(b, a), sub(c, a)
            det = u[0] * v[1] - u[1] * v[0]
            need(det != 0, 'acute support is nondegenerate')
            z = (F(a[0]) + (dot(u, u) * v[1] - dot(v, v) * u[1]) / (2 * det),
                 F(a[1]) + (u[0] * dot(v, v) - v[0] * dot(u, u)) / (2 * det), F(0))
            balls.append((z, d2(z, a), 3))
    return balls


def window(points, k, radius2):
    out = []
    for z, weight, q in positive_balls(points):
        p = sum(d2(z, x) < weight for x in points)
        m = sum(d2(z, x) == weight for x in points)
        if p + q <= k + 1 and p + m >= k and weight <= radius2:
            out.append({'center': z, 'weight': weight, 'qmin': q, 'p': p, 'm': m})
    return out


def covered(point, points, k, radius2):
    return sum(d2(point, x) <= radius2 for x in points) >= k


def json_value(value):
    if isinstance(value, F):
        return str(value)
    if isinstance(value, dict):
        return {k: json_value(v) for k, v in value.items()}
    if isinstance(value, (tuple, list)):
        return [json_value(v) for v in value]
    return value


def proof():
    points = [(0, 0, 0), (2, 0, 0)]
    r, z, q, y = F(2), (1, 0, 0), (2, 0, 0), (4, 0, 0)
    birth = window(points, 2, r * r)
    need(len(birth) == 1 and birth[0]['center'] == z and birth[0]['weight'] == 1, 'two-site birth')
    need(covered(q, points, 2, r * r) and d2(q, y) == r * r, 'offset membership')
    need(d2(y, z) > r * r > birth[0]['weight'], 'fixed and common-radius exclusions')
    minimal = {'X': points, 'k': 2, 'r': r, 'window_balls': birth, 'offset_witness': y,
               'witness_center_in_Omega': q, 'distance2_to_critical_center': d2(y, z)}

    a = [(0, 1, 0), (1, 1, 0), (2, 1, 0)]
    b = [(0, 1, 0), (1, 2, 0), (2, 1, 0)]
    wa, wb = window(a, 3, 4), window(b, 3, 4)
    need(len(wa) == len(wb) == 1, 'unique order-3 balls')
    need(wa[0]['center'] == wb[0]['center'] and wa[0]['weight'] == wb[0]['weight'] == 1,
         'identical weighted critical birth')
    q, y, c = (1, F(-1, 2), 0), (1, F(-5, 2), 0), b[1]
    need(covered(q, a, 3, 4) and d2(q, y) == 4, 'offset A membership')
    # Omega_B subset B(c,2), hence its offset by B(0,2) subset B(c,4).
    need(d2(y, c) > 16, 'offset B exclusion by enclosing ball')
    same_birth = {'X_A': a, 'X_B': b, 'k': 3, 'r': r, 'window_A': wa, 'window_B': wb,
                  'scope': 'same eligible birth ball of order 3, not same whole FULL tower',
                  'q_in_Omega_A': q, 'q_distances2_A': [d2(q, x) for x in a],
                  'y_in_offset_A_not_B': y, 'distance2_y_to_site_B': d2(y, c),
                  'offset_B_enclosing_radius2': 16}

    line = [(0, 0, 0), (2, 0, 0), (4, 0, 0)]
    wl = window(line, 2, 1)
    need(len(wl) == 2 and [e['center'] for e in wl] == [(1, 0, 0), (3, 0, 0)], 'two births')
    contact = (2, 0, 0)
    need(all(d2(contact, e['center']) == 1 for e in wl), 'dilation contact')
    # Adjacent original balls are tangent, nonadjacent balls are disjoint.
    need([d2(x, y) for x, y in combinations(line, 2)] == [4, 16, 4], 'Omega consists of two points')
    dilation = {'X': line, 'k': 2, 'r': 1, 'window_balls': wl, 'Omega_components': 2,
                'Omega': [e['center'] for e in wl], 'dilated_union_components': 1,
                'contact': contact, 'scope': 'keep original component labels before dilation'}

    triangle = [(0, 0, 0), (2, 0, 0), (1, 2, 0)]
    r2 = F(5, 4)
    wt = window(triangle, 1, r2)
    all_balls = positive_balls(triangle)
    enclosing = [e for e in all_balls if all(d2(e[0], x) <= e[1] for x in triangle)]
    meb = min(enclosing, key=lambda e: e[1])
    need(meb[0] == (1, F(3, 4), 0) and meb[1] == F(25, 16), 'triangle MEB')
    pair_distances = [d2(x, y) for x, y in combinations(triangle, 2)]
    need(all(d <= 4 * r2 for d in pair_distances) and meb[1] > r2, 'nerve cycle')
    need(len(wt) == 3 and all(d2(meb[0], e['center']) <= e['weight'] for e in wt), 'common point')
    h1 = {'X': triangle, 'k': 1, 'r2': r2, 'original_pair_distances2': pair_distances,
          'MEB_center': meb[0], 'MEB_radius2': meb[1], 'original_union_nerve': '3 vertices, 3 edges, no triangle',
          'Omega_betti1': 1, 'window_balls': wt,
          'common_point_distances2': [d2(meb[0], e['center']) for e in wt],
          'critical_union': 'star-shaped and contractible; any two retained balls also star-shaped',
          'scope': 'comparison to Omega, not to Omega offset; dilation changes topology itself'}
    return json_value({'schema': 'ehgp.audit.critical_ball_geometry.v1', 'source_pin':
                       'fd85f3bb582616324e9d4034b84ca96d742a439a', 'native_runs': 0, 'cloud_runs': 0,
                       'cases': {'minimal_growth': minimal, 'same_birth': same_birth,
                                 'dilation_merges_nodes': dilation, 'H1_not_in_H0_union': h1}})


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--emit', action='store_true')
    args = parser.parse_args()
    result = proof()
    if args.emit:
        print(json.dumps(result, indent=2, sort_keys=True))
    else:
        frozen = json.loads(Path(__file__).with_name('proof.json').read_text())
        need(result == frozen, 'frozen proof differs')
        print('critical_ball_geometry_verdict conforme cases4 Fraction native0 cloud0')


if __name__ == '__main__':
    main()
