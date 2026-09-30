"""Bounded Fraction checks: certified MEB versus a non-MEB circumcircle."""
from fractions import Fraction as F
from itertools import combinations
import json


def require(ok, reason):
    if not ok:
        raise ValueError(reason)


def dot(x, y):
    return sum(a*b for a, b in zip(x, y))


def sub(x, y):
    return tuple(a-b for a, b in zip(x, y))


def solve(matrix, rhs):
    a = [list(map(F, row))+[F(value)] for row, value in zip(matrix, rhs)]
    for j in range(len(a)):
        pivot = next((i for i in range(j, len(a)) if a[i][j]), None)
        if pivot is None:
            return None
        a[j], a[pivot] = a[pivot], a[j]
        scale = a[j][j]
        a[j] = [value/scale for value in a[j]]
        for i in range(len(a)):
            if i != j:
                scale = a[i][j]
                a[i] = [value-scale*base for value, base in zip(a[i], a[j])]
    return [row[-1] for row in a]


def circumball(points):
    if len(points) == 1:
        return points[0], F(0), [F(1)]
    a = points[0]
    u = [sub(p, a) for p in points[1:]]
    coeff = solve([[dot(x, y) for y in u] for x in u], [dot(x, x)/2 for x in u])
    if coeff is None:
        return None
    center = tuple(a[j]+sum(weight*x[j] for weight, x in zip(coeff, u)) for j in range(3))
    r2 = dot(sub(center, a), sub(center, a))
    weights = [1-sum(coeff)]+coeff
    require(sum(weights) == 1 and
            all(sum(w*p[j] for w, p in zip(weights, points)) == center[j] for j in range(3)),
            'barycentric reconstruction failed')
    require(all(dot(sub(center, p), sub(center, p)) == r2 for p in points),
            'support not cospherical')
    return center, r2, weights


def meb(points):
    candidates = []
    for q in range(1, min(4, len(points))+1):
        for ids in combinations(range(len(points)), q):
            ball = circumball([points[i] for i in ids])
            if ball is None:
                continue
            center, r2, weights = ball
            if any(w < 0 for w in weights):
                continue
            if not all(dot(sub(center, p), sub(center, p)) <= r2 for p in points):
                continue
            candidates.append((r2, ids, center, weights))
    require(candidates, 'no exact certified covering MEB found')
    # Nonnegative barycentric weights prove minimality by the variance identity.
    result = min(candidates, key=lambda row: (row[0], len(row[1]), row[1]))
    require(all(w > 0 for w in result[3]), 'these fixtures require positive minimal support')
    return result


def bbox_sum(points):
    return sum((max(p[j] for p in points)-min(p[j] for p in points))**2 for j in range(3))


def fixture(tag, integer_points, expected):
    points = [tuple(map(F, p)) for p in integer_points]
    r2, ids, center, weights = meb(points)
    global_sum = bbox_sum(points)
    support_sum = bbox_sum([points[i] for i in ids])
    require(r2 == expected, 'independent MEB does not match closed-form fixture')
    require(4*r2 <= support_sum <= global_sum, 'MEB box bound failed')
    # The integer shortcut is CLOSED: the first integer above S/4 is admitted.
    threshold = (global_sum.numerator+4*global_sum.denominator-1)//(4*global_sum.denominator)
    require(4*threshold >= global_sum and r2 <= threshold, 'closed global shortcut failed')
    return dict(family=tag, sites=len(points), level=str(r2), support_ids=list(ids),
                weights=[str(w) for w in weights], center=[str(x) for x in center],
                bbox_sum_support=str(support_sum), bbox_sum_global=str(global_sum),
                shortcut_threshold=str(threshold), certified_meb=True)


def run():
    fixtures = []
    for m in (31, 2**24-1, 2**32-1):
        fixtures.append(fixture('diameter_diagonal', [(0,0,0),(m,m,m)], F(3*m*m,4)))
        fixtures.append(fixture('regular_tetrahedron', [(0,0,0),(m,m,0),(m,0,m),(0,m,m)], F(3*m*m,4)))
        fixtures.append(fixture('sparse_axes_3D', [(0,0,0),(m,0,0),(0,m,0),(0,0,m),
                                                (m//2,m//3,m//5)], F(2*m*m,3)))
    m = 8
    obtuse = [tuple(map(F,p)) for p in ((0,0,0),(2*m,0,0),(m,1,0))]
    center, r2, weights = circumball(obtuse)
    s = bbox_sum(obtuse)
    require(any(w < 0 for w in weights) and 4*r2 > s, 'non-MEB condition counterexample absent')
    certified = meb(obtuse)
    require(certified[0] == m*m and 4*certified[0] <= s, 'actual obtuse-triangle MEB mismatch')
    maximum = 2**32-1
    knn = 3*maximum**2
    beta_bound = F(knn,4)
    require(beta_bound < 2**64 and knn.bit_length() == 66 and (4*knn).bit_length() == 68,
            'u32 radius/KNN/shortcut widths mismatch')
    # Under the separate D<2^200 emission bound, N=beta*D is <2^264.
    require(beta_bound*(2**200-1) < 2**264, 'conditional numerator bound failed')
    return dict(status='PASS', fixtures=fixtures, positive_meb_fixtures=len(fixtures),
                nonmeb_counterexample=dict(points=[[0,0,0],[16,0,0],[8,1,0]],
                    center=[str(x) for x in center], weights=[str(w) for w in weights],
                    circumlevel=str(r2), bbox_sum=str(s), violates_bound=True,
                    actual_meb_level=str(certified[0])),
                u32=dict(beta_global_bound=str(beta_bound), beta_lt_2_pow_64=True,
                         knn_max=str(knn), knn_bits=66, four_knn_bits=68,
                         level_numerator_lt_2_pow_264_only_if_den_lt_2_pow_200=True),
                native_constructor=False, native_sort=False, FULL=False, performance=False, GCP=False)


if __name__ == '__main__':
    print(json.dumps(run(), sort_keys=True))
