#!/usr/bin/env python3
"""Read-only Fraction replay of two examples, not an alpha-complex builder."""
import hashlib
import itertools
import json
from pathlib import Path
import subprocess
from fractions import Fraction as F

HERE = Path(__file__).resolve().parent
PIN = 'ee2df036282cad79dbb233934d5c3880243004cf'
REPLY = 'morsehgp3D_v11/audits/REPONSE_CLAUDE_POLYEDRE_ORDRE_K_20261006.md'
REPLY_SHA = '221b629413e5d05a57d1358659a4c3842e8e842fcf14a7e29b0a8208ae5ec673'


def check(condition, message):
    if not condition:
        raise RuntimeError(message)


def sq(p, q):
    return sum((x - y) ** 2 for x, y in zip(p, q))


def radius2(pair):
    return F((max(pair) - min(pair)) ** 2, 4)


def circumcenter(p, q, r):
    a, b = 2 * (q[0] - p[0]), 2 * (q[1] - p[1])
    c, d = 2 * (r[0] - p[0]), 2 * (r[1] - p[1])
    e = sq(q, (0, 0)) - sq(p, (0, 0))
    f = sq(r, (0, 0)) - sq(p, (0, 0))
    det = a * d - b * c
    check(det != 0, 'nondegenerate circumcircle required')
    return F(e * d - b * f, det), F(a * f - e * c, det)


def cell_faces(cell):
    return [frozenset(face) for size in range(1, len(cell))
            for face in itertools.combinations(sorted(cell), size)]


def encode(value):
    if isinstance(value, F):
        return str(value)
    if isinstance(value, dict):
        return {k: encode(v) for k, v in value.items()}
    if isinstance(value, (list, tuple)):
        return [encode(v) for v in value]
    return value


def calculate():
    points = (0, 2, 4)
    pairs = list(itertools.combinations(points, 2))
    eligible = [q for q in pairs if radius2(q) <= 1]
    check(eligible == [(0, 2), (2, 4)], 'two pair witnesses at r=1')
    rows = []
    for q in eligible:
        center = F(sum(q), 2)
        # Equality in the triangle inequality: two radius-one balls whose
        # centers are distance two meet only at this midpoint, also in R3.
        check(radius2(q) == 1, 'tangent pair')
        check(max((center - p) ** 2 for p in q) <
              min((center - p) ** 2 for p in points if p not in q),
              'strict nearest-neighbor inequalities at the component witness')
        offset_trace = [p for p in points if (p - center) ** 2 <= 1]
        shadow_trace = [p for p in points if min(q) <= p <= max(q)]
        check(offset_trace == shadow_trace == list(q), 'exact trace of each shadow')
        rows.append({'Q': q, 'omega_component': [center],
                     'shadow_interval': [min(q), max(q)],
                     'offset_trace_P': offset_trace, 'shadow_trace_P': shadow_trace})
    check(rows[0]['shadow_interval'][1] == rows[1]['shadow_interval'][0] == 2,
          'distinct components acquire a shadow contact')
    q4 = {'P': points, 'k': 2, 'r2': F(1), 'components': rows,
          'omega_component_count': 2, 'union_shadow_component_count': 1,
          'common_covered_site': 2,
          'j0_algebra_only': {'I': [2], 'U': [0, 4], 'j': 0,
                              'union_actual_Q': [2], 'I_union_U': [0, 2, 4]}}

    p = {'A': (0, 6), 'B': (4, 6), 'C': (2, 7), 'D': (2, 0)}
    circles = {}
    for triple, other, expected in [('ABC', 'D', F(25, 4)), ('ABD', 'C', F(100, 9))]:
        center = circumcenter(*(p[x] for x in triple))
        rs = [sq(p[x], center) for x in triple]
        check(rs[0] == rs[1] == rs[2] == expected, 'equal circle distances')
        outside = sq(p[other], center)
        check(outside > expected, 'empty Delaunay circle')
        circles[triple] = {'center': center, 'r2': expected,
                           'other_site': other, 'other_distance2': outside}
    dates = {frozenset(x): F(0) for x in p}
    for edge, expected in [('AC', F(5, 4)), ('BC', F(5, 4)),
                           ('AD', F(10)), ('BD', F(10))]:
        mid = tuple(F(p[edge[0]][i] + p[edge[1]][i], 2) for i in (0, 1))
        rs = sq(p[edge[0]], mid)
        check(rs == expected, 'Gabriel edge radius')
        check(all(sq(p[x], mid) > rs for x in p if x not in edge), 'empty edge ball')
        dates[frozenset(edge)] = rs
    # F_AB is x=2, 10/3 <= y <= 9/2 (times the z-axis in R3).
    # The closest point to midpoint(AB)=(2,6) is its upper endpoint.
    check(F(10, 3) < F(9, 2) < 6, 'AB dual interval and closest endpoint')
    dates[frozenset('AB')] = sq(p['A'], (F(2), F(9, 2)))
    dates[frozenset('ABC')] = circles['ABC']['r2']
    dates[frozenset('ABD')] = circles['ABD']['r2']
    check(all(f in dates and dates[f] <= birth
              for s, birth in dates.items() for f in cell_faces(s)),
          'fixed alpha filtration is face-closed and monotone')
    check(dates[frozenset('AB')] == dates[frozenset('ABC')], 'same birth local pair')
    early = {s for s, birth in dates.items() if birth <= F(25, 4)}
    early_cofaces = [s for s in early if frozenset('AB') < s]
    check(early_cofaces == [frozenset('ABC')], 'AB initially free')
    removed = {frozenset('AB'), frozenset('ABC')}
    proposed = set(dates) - removed
    missing = sorted((''.join(sorted(s)), ''.join(sorted(f)))
                     for s in proposed for f in cell_faces(s) if f not in proposed)
    check(missing == [('ABD', 'AB')], 'proposed static L is not face-closed')
    abd = frozenset('ABD')
    mates = [s for s in dates if abs(len(s) - len(abd)) == 1
             and (s < abd or abd < s) and dates[s] == dates[abd]]
    check(not mates, 'no same-birth face/coface mate of ABD in this fixed cell complex')
    q5_future = {'P': p, 'k': 1, 'circles': circles,
                 'cell_births_r2': {''.join(sorted(s)): a for s, a in sorted(
                     dates.items(), key=lambda item: (len(item[0]), sorted(item[0])))},
                 'AB_dual_xy': {'x': F(2), 'y_interval': [F(10, 3), F(9, 2)]},
                 'locally_removed_same_birth_pair': ['AB', 'ABC'],
                 'initial_AB_triangle_cofaces': ['ABC'],
                 'future_AB_triangle_cofaces': ['ABC', 'ABD'],
                 'missing_faces_in_proposed_static_L': missing,
                 'ABD_same_birth_mates_in_fixed_cells': []}

    # Finite-life node: {0,2} is born at r=1, a second node at r=4;
    # their first intersection is the triple MEB at r=5.
    check([radius2(q) for q in [(0, 2), (2, 10), (0, 10)]] == [1, 16, 25],
          'finite-life pair birth and merger radii')
    samples = []
    for r in map(F, ['1', '3/2', '2', '4', '9/2', '499/100']):
        later = (r + 5) / 2
        check(1 <= r < later < 5, 'later parameter remains before death')
        check(abs(later - 2) <= later, 'new point in later first pair lens')
        check(later > r and later < 10 - r, 'new point outside earlier Omega')
        samples.append({'r': r, 'later_r': later, 'new_axial_point': later})
    lifetimes = {'finite_node': {'P': [0, 2, 10], 'k': 2, 'birth_r': 1,
                                'death_r': 5, 'before_death_axial_interval': ['2-r', 'r'],
                                'A_node_left_limit': [[1, 0, 0]], 'growth_samples': samples},
                 'root': {'P': [0, 2], 'k': 2, 'birth_r': 1, 'death_r': 'infinity',
                          'A_root_all_r_ge_1': [[1, 0, 0]],
                          'Omega_root_axial_interval': ['2-r', 'r']}}
    return encode({'source_pin': PIN, 'reply_sha256': REPLY_SHA,
                   'scope': 'exact examples only; no general alpha construction or native execution',
                   'Q4_shadow': q4, 'Q5_fixed_future_cells': q5_future,
                   'Q5_level_examples': lifetimes})


def main():
    repo = subprocess.check_output(['git', 'rev-parse', '--show-toplevel'],
                                   cwd=HERE, text=True).strip()
    source = subprocess.check_output(['git', 'show', PIN + ':' + REPLY], cwd=repo)
    check(hashlib.sha256(source).hexdigest() == REPLY_SHA, 'pinned reply source')
    manifest = json.loads((HERE / 'source_manifest.json').read_text())
    check(manifest['pin'] == PIN, 'source manifest pin')
    for item in manifest['files']:
        data = subprocess.check_output(['git', 'show', PIN + ':' + item['path']], cwd=repo)
        check(len(data) == item['bytes'] and hashlib.sha256(data).hexdigest() == item['sha256'],
              'pinned source ' + item['path'])
    expected = json.loads((HERE / 'proof.json').read_text())
    check(calculate() == expected, 'read-only comparison against frozen proof.json')
    print('PASS: shadow traces/contact; exact alpha dates/future closure; level examples; source pin')


if __name__ == '__main__':
    main()
