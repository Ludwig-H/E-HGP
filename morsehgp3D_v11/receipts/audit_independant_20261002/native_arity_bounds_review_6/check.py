"""Bornes scalaires et geometrie Fraction autonome ; aucun import ou appel natif."""
from fractions import Fraction as F
from hashlib import sha256
from itertools import product
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parent
COUNTS = {}


def check(ok, group, message):
    COUNTS[group] = COUNTS.get(group, 0) + 1
    if not ok:
        raise ValueError(group + ': ' + message)


def dot(a, b):
    return sum(x * y for x, y in zip(a, b))


def sub(a, b):
    return tuple(x - y for x, y in zip(a, b))


def cross(a, b):
    return (a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0])


def solve(matrix, rhs):
    rows = [list(map(F, row)) + [F(value)] for row, value in zip(matrix, rhs)]
    size = len(rows)
    for j in range(size):
        pivot = next((i for i in range(j, size) if rows[i][j]), None)
        if pivot is None:
            return None
        rows[j], rows[pivot] = rows[pivot], rows[j]
        scale = rows[j][j]
        rows[j] = [v / scale for v in rows[j]]
        for i in range(size):
            if i != j:
                scale = rows[i][j]
                rows[i] = [a - scale * b for a, b in zip(rows[i], rows[j])]
    return tuple(row[-1] for row in rows)


def geometric_sphere(points):
    anchor = points[0]
    edges = tuple(sub(p, anchor) for p in points[1:])
    weights = solve([[dot(a, b) for b in edges] for a in edges], [F(dot(a, a), 2) for a in edges])
    if weights is None:
        return None
    c = tuple(F(anchor[j]) + sum(w * v[j] for w, v in zip(weights, edges)) for j in range(3))
    return c, dot(sub(c, anchor), sub(c, anchor)), (1 - sum(weights),) + weights


def square_corners(bits):
    m = 1 << bits
    length = m - 1
    vertices = tuple(product((0, length), repeat=2))
    areas = set()
    for a, b, c in product(vertices, repeat=3):
        # Expanded signed triangle area, independent of vector cross storage.
        area = a[0] * (b[1] - c[1]) + b[0] * (c[1] - a[1]) + c[0] * (a[1] - b[1])
        check(abs(area) <= length * length < m * m, 'shared_square', 'common-square determinant bound fails')
        areas.add(area)
    check(areas == {-length * length, 0, length * length}, 'shared_square', 'extrema not attained')
    arbitrary = cross((length, length, 0), (length, -length, 0))[2]
    check(abs(arbitrary) == 2 * length * length > length * length,
          'domain_guard', 'arbitrary Vec would inherit the Point refinement')


def q4_case(bits):
    m = 1 << bits
    length = m - 1
    a = length // 2
    # Positive coordinates in the same cube; determinant exactly one, center far outside.
    points = ((0, 0, 0), (a, a - 1, 1), (1, a, 1), (a, 2 * a - 1, 2))
    anchor = points[0]
    u, v, s = (sub(p, anchor) for p in points[1:])
    products = (cross(v, s), cross(s, u), cross(u, v))
    for values in products:
        check(all(abs(value) < m * m for value in values), 'q4_construction', 'cross exceeds common-square bound')
    determinant_terms = [u[j] * products[0][j] for j in range(3)]
    check(sum(determinant_terms) == 1, 'noncritical_fixture', 'unimodular fixture changed')
    determinant = 0
    for value in determinant_terms:
        determinant += value
        check(abs(value) < m ** 3 and abs(determinant) < 3 * m ** 3,
              'q4_construction', 'determinant intermediate exceeds proof')
    denominator = 2 * determinant
    numerator = []
    norms = [dot(edge, edge) for edge in (u, v, s)]
    for norm in norms:
        check(norm < 3 * m * m < (1 << 63), 'q4_construction', 'dot is not i64-safe')
    for j in range(3):
        total = 0
        for norm, values in zip(norms, products):
            term = norm * values[j]
            total += term
            check(abs(term) < 3 * m ** 4 and abs(total) < 9 * m ** 4,
                  'q4_construction', 'Cramer product or partial sum exceeds proof')
        numerator.append(total)
    geometry = geometric_sphere(points)
    check(geometry is not None, 'noncritical_fixture', 'sphere missing')
    check(tuple(F(value, denominator) for value in numerator) == geometry[0],
          'independent_geometry', 'Cramer center differs from Gram center')
    check(any(w < 0 for w in geometry[2]) and any(c < 0 or c > length for c in geometry[0]),
          'noncritical_fixture', 'center is no longer outside convex hull/cube')
    queries = sorted(set(product((0, length), repeat=3)) | set(points))
    maximum_term, maximum_partial, maximum_absolute_sum = 0, 0, 0
    for point in queries:
        offset = sub(point, anchor)
        norm = dot(offset, offset)
        terms = [denominator * norm] + [-2 * numerator[j] * offset[j] for j in range(3)]
        total = 0
        for term in terms:
            total += term
            check(abs(term) < 18 * m ** 5 and abs(total) < 72 * m ** 5 < (1 << 127),
                  'q4_power_intermediates', 'product or partial power is not i128-safe')
            maximum_term = max(maximum_term, abs(term))
            maximum_partial = max(maximum_partial, abs(total))
        check(sum(map(abs, terms)) < 72 * m ** 5, 'q4_absolute_sum', 'cancellation-independent bound fails')
        maximum_absolute_sum = max(maximum_absolute_sum, sum(map(abs, terms)))
        residual = dot(sub(point, geometry[0]), sub(point, geometry[0])) - geometry[1]
        check(total == denominator * residual, 'independent_geometry', 'power polynomial differs from geometric definition')
        if point in points:
            check(total == 0, 'exact_shell', 'support not on exact shell')
    return {'bits': bits, 'a': a, 'denominator': denominator, 'numerator': numerator,
            'critical': False, 'max_term_bits': maximum_term.bit_length(),
            'max_partial_bits': maximum_partial.bit_length(),
            'max_absolute_sum_bits': maximum_absolute_sum.bit_length()}


def q3_scope(bits):
    m = 1 << bits
    length = m - 1
    # Right triangle: its sphere has qmin2, but the q3 coefficients retain their scaling.
    numerator = (length ** 5, length ** 5, 0)
    denominator = 2 * length ** 4
    first = 6 * length ** 6
    total = first - 4 * length ** 6
    geometry = geometric_sphere(((0, 0, 0), (length, 0, 0), (0, length, 0)))
    check(geometry[0] == (F(length, 2), F(length, 2), F(0)) and geometry[1] == F(length * length, 2),
          'q3_scope', 'right triangle geometry changed')
    check(tuple(F(n, denominator) for n in numerator) == geometry[0], 'q3_scope', 'q3 center scaling incorrect')
    if bits == 18:
        check(216 * m ** 6 < (1 << 127) and first < (1 << 127), 'q3_scope', 'B18 general bound unsafe')
    elif bits == 21:
        check(first > (1 << 127) and total < (1 << 127),
              'q3_scope', 'B21 intermediate overflow witness missing')
    else:
        check(first > (1 << 127) and total > (1 << 127), 'q3_scope', 'B24 wide witness missing')
    return {'bits': bits, 'presentation_arity': 3, 'qmin': 2,
            'first_product_bits': first.bit_length(), 'total_bits': total.bit_length(),
            'native_route_expected': bits == 18}


def source_hashes():
    before = json.loads((ROOT / 'SOURCE_BEFORE.json').read_text())
    after = json.loads((ROOT / 'SOURCE_AFTER.json').read_text())
    for row in before['files']:
        data = (ROOT / 'source' / row['path']).read_bytes()
        check(len(data) == row['bytes'] and sha256(data).hexdigest() == row['sha256'],
              'source_hashes', 'captured source changed')
    check(after['captured_files'] == [{'path': r['path'], 'bytes': r['bytes'], 'sha256': r['sha256']}
                                     for r in before['files']], 'source_hashes', 'source closure mismatch')


def main():
    source_hashes()
    q4, q3 = [], []
    for bits in (18, 21, 24):
        m = 1 << bits
        square_corners(bits)
        check(3 * m ** 2 < (1 << 63) and 12 * m ** 2 < (1 << 127),
              'q1_q2_bounds', 'q1/q2 scalar capacity incorrect')
        check(72 * m ** 5 < (1 << 127), 'q4_general_bound', 'q4 absolute-sum bound unsafe')
        q4.append(q4_case(bits))
        q3.append(q3_scope(bits))
    manifest = ROOT / 'SHA256SUMS'
    if manifest.exists():
        for line in manifest.read_text().splitlines():
            wanted, name = line.split('  ', 1)
            path = Path(name)
            if path.is_absolute() or '..' in path.parts or sha256((ROOT / path).read_bytes()).hexdigest() != wanted:
                raise ValueError('manifest mismatch: ' + name)
    print(json.dumps({'checks': COUNTS, 'total_checks': sum(COUNTS.values()),
                      'q4_noncritical_examples': q4, 'q3_scope_examples': q3,
                      'native_executions': 0}, sort_keys=True))


if __name__ == '__main__':
    main()
