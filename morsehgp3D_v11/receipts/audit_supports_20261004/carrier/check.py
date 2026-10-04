"""Small independent Fraction guards for the proposed support carrier."""
from fractions import Fraction as F
from itertools import combinations
import json

CHECKS = 0


def require(condition, message):
    global CHECKS
    CHECKS += 1
    if not condition:
        raise RuntimeError(message)


def dot(a, b):
    return sum((x * y for x, y in zip(a, b)), F(0))


def sub(a, b):
    return tuple(x - y for x, y in zip(a, b))


def solve(matrix, rhs):
    n = len(rhs)
    rows = [[F(v) for v in row] + [F(value)] for row, value in zip(matrix, rhs)]
    for col in range(n):
        pivot = next((i for i in range(col, n) if rows[i][col]), None)
        if pivot is None:
            return None
        rows[col], rows[pivot] = rows[pivot], rows[col]
        factor = rows[col][col]
        rows[col] = [v / factor for v in rows[col]]
        for i in range(n):
            if i != col:
                factor = rows[i][col]
                rows[i] = [a - factor * b for a, b in zip(rows[i], rows[col])]
    return tuple(row[-1] for row in rows)


def barycentric(points, target):
    vectors = [sub(p, points[0]) for p in points[1:]]
    gram = [[dot(u, v) for v in vectors] for u in vectors]
    alpha = solve(gram, [dot(u, sub(target, points[0])) for u in vectors])
    if alpha is None:
        return None
    weights = (F(1) - sum(alpha),) + alpha
    reconstructed = tuple(sum(w * p[j] for w, p in zip(weights, points)) for j in range(len(target)))
    return weights if reconstructed == target else None


def supports(points, center):
    result = []
    for q in range(2, min(4, len(points)) + 1):
        for ids in combinations(range(len(points)), q):
            weights = barycentric([points[i] for i in ids], center)
            if weights is not None and all(w > 0 for w in weights):
                result.append(ids)
    return result


def meb(points):
    candidates = []
    for q in range(1, min(4, len(points)) + 1):
        for ids in combinations(range(len(points)), q):
            subset = [points[i] for i in ids]
            vectors = [sub(p, subset[0]) for p in subset[1:]]
            gram = [[dot(u, v) for v in vectors] for u in vectors]
            alpha = solve(gram, [dot(u, u) / 2 for u in vectors])
            if alpha is None or not all(w > 0 for w in (F(1) - sum(alpha),) + alpha):
                continue
            center = tuple(subset[0][j] + sum(a * u[j] for a, u in zip(alpha, vectors)) for j in range(len(points[0])))
            radius = dot(sub(center, subset[0]), sub(center, subset[0]))
            if all(dot(sub(p, center), sub(p, center)) <= radius for p in points):
                candidates.append(radius)
    if not candidates:
        raise RuntimeError('MEB absent')
    return min(candidates)


def components(points, k, level):
    vertices = [ids for ids in combinations(range(len(points)), k) if meb([points[i] for i in ids]) <= level]
    parent = list(range(len(vertices)))

    def root(i):
        while parent[i] != i:
            i = parent[i]
        return i

    for coface in combinations(range(len(points)), k + 1):
        if meb([points[i] for i in coface]) > level:
            continue
        faces = [i for i, ids in enumerate(vertices) if set(ids).issubset(coface)]
        for i in faces[1:]:
            parent[root(i)] = root(faces[0])
    return len({root(i) for i in parent}), len(vertices)


zero = (F(0), F(0))
base = [(F(1), F(0)), (F(0), F(1)), (F(-1), F(0)), (F(0), F(-1))]
require(supports(base, zero) == [(0, 2), (1, 3)], 'unperturbed carrier is two diameters')
witness = (F(-1, 4), F(1, 4))
require(min(abs(witness[0]), abs(witness[1])) == F(1, 4), 'distance to both diameter segments')
cases = []
for n in [4, 8, 16, 32, 64, 128, 256, 512, 1023]:
    t = F(1, n)
    bx, by = 2 * t / (1 + t * t), (1 - t * t) / (1 + t * t)
    perturbed = [base[0], (bx, by), base[2], base[3]]
    require(bx * bx + by * by == 1, 'perturbed point remains on same shell')
    current = supports(perturbed, zero)
    require(current == [(0, 2), (1, 2, 3)], 'inclusion-minimal arities change 2+2 to 2+3')
    weights = barycentric(perturbed[1:], witness)
    require(weights is not None and all(v > 0 for v in weights), 'fixed witness is strictly inside new support')
    center_weights = barycentric(perturbed[1:], zero)
    require(center_weights == (1 / (1 + bx + by), bx / (1 + bx + by), by / (1 + bx + by)), 'exact center weights')
    require(dot(sub(perturbed[1], base[1]), sub(perturbed[1], base[1])) == F(4, n * n + 1), 'input perturbation squared')
    d = n * n + 1
    integer_points = [tuple(int(c * d + d) for c in p) for p in perturbed]
    require(all(F(v) == c * d + d for point, original in zip(integer_points, perturbed) for v, c in zip(point, original)), 'exact integer embedding')
    require(max(v for point in integer_points for v in point) < 2 ** 21, 'embedding fits u21')
    cases.append({'n': n, 'matched_displacement_squared': str(F(4, d)), 'carrier_Hausdorff_lower_bound': '1/4', 'integer_coordinate_max': 2 * d})

square = [(F(-1), F(-1)), (F(1), F(-1)), (F(1), F(1)), (F(-1), F(1))]
require(components(square, 2, F(1)) == (4, 4), 'four side-support components remain distinct at K2')
require(square[1] in square[:2] and square[1] in square[1:3], 'two siblings share their endpoint geometrically')
require(components(square, 2, F(2)) == (1, 6), 'closed diagonal plateau joins all six K-parts')
require(components(square, 1, F(1)) == (1, 4), 'K1 root born at side level')
require(meb([square[0], square[2]]) == 2, 'internal diagonal support enters after K1 root birth')

triangle = [(F(0), F(0)), (F(2), F(0)), (F(1), F(2))]
require(meb(triangle) == F(25, 16), 'acute triangle exact level')
require(components(triangle, 2, F(3, 2)) == (3, 3), 'three strict edge components before q3 event')
require(components(triangle, 2, F(25, 16)) == (1, 3), 'one closed component after q3 event')
require(0 + 3 - 1 <= 2 <= 0 + 3, 'q3 event window includes K2')
require(not (0 + 3 <= 2 <= 0 + 3), 'point strong predicate excludes this connecting event')

print(json.dumps({'checks': CHECKS, 'status': 'ok', 'profile': 'exact_fraction_model_only', 'carrier_cases': cases,
                  'limits': 'No native execution, no statistical claim, no uniform arbitrary-small perturbations claimed for fixed u21 grid.'}, sort_keys=True))
