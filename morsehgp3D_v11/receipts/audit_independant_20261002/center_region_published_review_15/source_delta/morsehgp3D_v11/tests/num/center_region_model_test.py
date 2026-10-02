"""Faits fixes et sensibilite du juge CenterRegion, Python rationnel seul, aucune sonde native."""
import json
from fractions import Fraction as F

from center_region_oracle import Case, affine_line, cases, check_case, distance, geometry, judge, need
from fraction_oracle import center_of, dot, is_inside, sub


def fixed_facts(data, bits):
    by_name = {case.name: case for case in data}
    states = {
        'pair_face_hi': 'intersects', 'pair_face_lo': 'intersects', 'pair_before_plane': 'disjoint',
        'pair_after_plane': 'disjoint', 'pair_edge': 'intersects', 'pair_vertex': 'intersects',
        'pair_oblique_miss': 'disjoint', 'pair_extreme_domain': 'intersects', 'pair_extreme_miss': 'disjoint',
        'pair_last_cell': 'disjoint', 'pair_coincident': 'intersects', 'pair_last_cell_diagonal': 'intersects',
        'line_edge_hi': 'intersects', 'line_edge_lo': 'intersects', 'line_face': 'intersects',
        'line_before': 'disjoint', 'line_after': 'disjoint', 'line_vertex': 'intersects',
        'line_planes_meet_separately': 'disjoint', 'line_axis2_separation': 'disjoint',
        'line_extreme_miss': 'disjoint', 'line_extreme_domain': 'intersects', 'line_thin': 'disjoint',
        'line_collinear': 'degenerate', 'line_duplicate': 'degenerate', 'line_all_equal': 'degenerate',
        'line_T0_upper_contact': 'intersects',
    }
    states.update({'line_tetra_face%d' % i: 'intersects' for i in range(4)})
    for name, expected in states.items():
        need(geometry(by_name[name], bits)['verdict'] == expected, 'fait fixe '+name)
    checks = len(states)
    vertex = geometry(by_name['line_vertex'], bits)
    need(vertex['lower'] == vertex['upper'] and tuple(a+vertex['lower']*d for a, d in
         zip(vertex['origin'], vertex['direction'])) == (1, 1, 1), 'unique contact au sommet')
    checks += 1
    maximum = 1 << bits
    upper = geometry(by_name['line_T0_upper_contact'], bits)
    need(upper['origin'] == (maximum, 0, 0) and upper['direction'] == (0, 0, 1) and
         (upper['lower'], upper['upper']) == (0, 1) and upper['contact'], 'contact T0 hi=M exact')
    checks += 1
    missed = by_name['line_planes_meet_separately']
    for i, j in ((0, 1), (0, 2), (1, 2)):
        pair = Case('bisectrice_seule', 2, (missed.points[i], missed.points[j], (0, 0, 0)), missed.lo, missed.hi)
        need(geometry(pair, bits)['verdict'] == 'intersects', 'chacune des trois mediatrices rencontre la boite')
        checks += 1
    tetra = ((1, 2, 6), (8, 4, 8), (2, 1, 3), (7, 8, 5))
    center = tuple(F(v, 82) for v in (397, 331, 391))
    need(tuple(center_of(tetra)) == center and is_inside(center, tetra), 'tetra strict centre connu')
    need(len({distance(p, center) for p in tetra}) == 1 and all(4 < v < 5 for v in center), 'centre dans boite')
    need(any(dot(sub(tetra[j], tetra[i]), sub(tetra[k], tetra[i])) < 0
             for i, j, k in ((0, 1, 2), (1, 0, 2), (2, 0, 1))), 'face012 obtuse')
    checks += 3
    # Ce contrôle des equations ne partage aucune projection/SAT avec le produit.
    for case in data:
        if case.q != 3 or geometry(case, bits)['verdict'].startswith('refused'):
            continue
        line = affine_line(case.points)
        if line is None:
            continue
        origin, direction = line
        for t in (F(0), F(1), F(-3, 2)):
            point = tuple(a+t*d for a, d in zip(origin, direction))
            need(len({distance(point, p) for p in case.points}) == 1, 'droite equidistante')
    return checks


def main():
    profiles = []
    for bits in (18, 21, 24):
        data, pairs = cases(bits)
        lines = [geometry(case, bits)['verdict'] for case in data]
        stats = judge(data, pairs, lines, bits)
        facts = fixed_facts(data, bits)
        by_name = {case.name: case for case in data}
        corrupted = 0

        def refused(action):
            nonlocal corrupted
            try:
                action()
            except ValueError:
                corrupted += 1
            else:
                raise ValueError('corruption acceptee')

        for name, wrong in [
            ('pair_face_hi', 'disjoint'), ('pair_vertex', 'disjoint'),
            ('line_vertex', 'disjoint'), ('line_T0_upper_contact', 'refused parameter_out_of_range'),
            ('line_tetra_face3', 'disjoint'), ('line_tetra_face3', 'degenerate'),
            ('line_planes_meet_separately', 'intersects'), ('line_axis2_separation', 'intersects'),
            ('line_collinear', 'disjoint'), ('line_duplicate', 'intersects'),
            ('line_extreme_miss', 'intersects'), ('line_extreme_domain', 'disjoint'),
            ('pair_coincident', 'degenerate'), ('bad_point_2_0_%d' % (1 << bits), 'intersects'),
            ('bad_box_2_0_equal', 'intersects'), ('bad_box_3_0_oversized', 'intersects')]:
            refused(lambda name=name, wrong=wrong: check_case(by_name[name], wrong, bits))
        for malformed in ('', 'ok intersects', 'intersects extra', ' intersects', 'intersects\n',
                          'refused none', 'degenerate true', 'true'):
            refused(lambda malformed=malformed: check_case(by_name['pair_face_hi'], malformed, bits))
        refused(lambda: judge(data, pairs, lines[:-1], bits))
        refused(lambda: judge(data, pairs, lines+['intersects'], bits))
        profiles.append(dict(bits=bits, cases=len(data), permutations=len(pairs), fixed_facts=facts,
                             corruptions=corrupted, **stats))
    need(all(p['corruptions'] == 26 for p in profiles), 'plancher corruptions')
    print(json.dumps(dict(native_calls=0, profiles=profiles), sort_keys=True))


if __name__ == '__main__':
    main()
