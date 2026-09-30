"""Exact same-cloud q2 admissibility; no native catalogue or broad panel."""
from fractions import Fraction as F
import json


def require(ok, reason):
    if not ok:
        raise ValueError(reason)


def norm2(point):
    return sum(value*value for value in point)


def sphere(a, b, points):
    center = tuple((x+y)/2 for x, y in zip(a, b))
    r2 = norm2(tuple(x-y for x, y in zip(a, center)))
    powers = [norm2(tuple(x-y for x, y in zip(p, center)))-r2 for p in points]
    interior = [i for i, value in enumerate(powers) if value < 0]
    shell = [i for i, value in enumerate(powers) if value == 0]
    exterior = [i for i, value in enumerate(powers) if value > 0]
    # Distinct a,b are antipodal: midpoint has strictly positive weights 1/2.
    require(a != b and norm2(tuple(x-y for x, y in zip(b, center))) == r2,
            'two-point positive support not established')
    return r2, center, interior, shell, exterior


def run():
    fixtures = []
    for m in (2**23, 2**31, 2**32-2):
        points = [tuple(map(F, p)) for p in ((0,0,0), (m,0,0), (m,1,0), (m//2,0,0))]
        a, b, c, d = points
        first = sphere(a, b, points)
        second = sphere(a, c, points)
        e = norm2(d)
        require(first[0] == e < second[0] == e+F(1,4), 'q2/actual KNN threshold mismatch')
        require(first[2:] == ([3],[0,1],[2]) and second[2:] == ([3],[0,1,2],[]),
                'same-cloud exact incidence mismatch')
        k_ranges = [(len(row[2])+2, len(row[2])+len(row[3])) for row in (first,second)]
        require(all(low <= 3 <= high for low,high in k_ranges), 'both balls must be admissible at K3')
        h = F(1,10000)  # 0.1 mm in metres, exact.
        physical = [row[0]*h*h for row in (first,second)]
        fixtures.append(dict(m=m, coords_bits=m.bit_length(), first_level=str(first[0]),
                             second_level=str(second[0]), geometric_integer_threshold=str(e),
                             interior_ids=[3], first_shell_ids=[0,1], second_shell_ids=[0,1,2],
                             qmin=2, positive_support_weights=['1/2','1/2'], admissible_K_intervals=k_ranges,
                             double_grid_collision=float(first[0]) == float(second[0]),
                             h_metres=str(h), double_physical_collision=float(physical[0]) == float(physical[1])))
    return dict(status='PASS', fixtures=fixtures,
                native_wide_catalogue=False, FULL=False, statistical_or_performance_claim=False, GCP=False)


if __name__ == '__main__':
    print(json.dumps(run(), sort_keys=True))
