"""Juge independant : elimination de Gauss Fraction, aucune formule Cramer/double produit du produit."""
import hashlib
import itertools
import json
import random
import subprocess
import sys
from fractions import Fraction as F


def require(condition, message):
    if not condition:
        raise ValueError(message)


def sign(value):
    return (value > 0) - (value < 0)


def sub(a, b):
    return [x - y for x, y in zip(a, b)]


def dot(a, b):
    return sum(x * y for x, y in zip(a, b))


def solve(matrix, rhs):
    """Elimination rationnelle a pivot exact, retourne None si le systeme est singulier."""
    size = len(rhs)
    rows = [[F(v) for v in row] + [F(value)] for row, value in zip(matrix, rhs)]
    for col in range(size):
        pivot = next((i for i in range(col, size) if rows[i][col]), None)
        if pivot is None:
            return None
        rows[col], rows[pivot] = rows[pivot], rows[col]
        scale = rows[col][col]
        rows[col] = [v / scale for v in rows[col]]
        for i in range(size):
            if i != col:
                scale = rows[i][col]
                rows[i] = [v - scale * w for v, w in zip(rows[i], rows[col])]
    return [rows[i][-1] for i in range(size)]


def determinant(matrix):
    size = len(matrix)
    total = 0
    for permutation in itertools.permutations(range(size)):
        inversions = sum(permutation[i] > permutation[j] for i in range(size) for j in range(i + 1, size))
        term = -1 if inversions % 2 else 1
        for i, j in enumerate(permutation):
            term *= matrix[i][j]
        total += term
    return total


def orient(a, b, c, d):
    return determinant([sub(b, a), sub(c, a), sub(d, a)])


def center_of(points):
    anchor = points[0]
    edges = [sub(p, anchor) for p in points[1:]]
    if not edges:
        return list(map(F, anchor))
    gram = [[dot(u, v) for v in edges] for u in edges]
    weights = solve(gram, [F(dot(u, u), 2) for u in edges])
    if weights is None:
        return None
    return [anchor[j] + sum(w * edge[j] for w, edge in zip(weights, edges)) for j in range(3)]


def is_inside(center, points):
    matrix = [[F(p[j]) for p in points] for j in range(3)] + [[F(1)] * 4]
    weights = solve(matrix, list(center) + [F(1)])
    return weights is not None and all(w > 0 for w in weights)


def cases(bits):
    rng = random.Random(11002)
    maximum = (1 << bits) - 1
    corners = list(itertools.product((0, maximum), repeat=3))
    fixtures = [corners[:4], [corners[i] for i in (0, 3, 5, 6)], [(0, 0, 0)] * 4,
                [(0, 0, 0), (maximum, 1, 0), (maximum - 1, 1, 0), (maximum, 1, 1)],
                [(0, 0, 0), (2, 0, 0), (0, 2, 0), (0, 0, 2)],
                [(maximum, maximum, maximum), (0, 1, 2), (maximum - 1, 0, 3), (2, 3, 0)]]
    out = []
    for points in fixtures:
        for q in range(1, 5):
            for query in corners + points:
                out.append((q, points, query))
    for points in itertools.permutations(fixtures[1]):
        out.append((4, points, (maximum // 2,) * 3))
    for q in range(1, 5):
        for scale in (7, maximum):
            for _ in range(24):
                points = [tuple(rng.randrange(scale + 1) for _ in range(3)) for _ in range(4)]
                query = tuple(rng.randrange(scale + 1) for _ in range(3))
                out.append((q, points, query))
    return out


def check_geometry(case, line, previous):
    q, points, query = case
    center = center_of(points[:q])
    if center is None:
        require(line == 'degenerate', 'degenerescence non rendue : ' + line)
        return previous, 1, True
    words = line.split()
    require(len(words) == 14 and words[0] == 'ok', 'ligne geometrique mal formee : ' + line)
    n = [int(word, 16) for word in words[1:4]]
    denominator, numerator_level, denominator_level, power, orientation = [int(w, 16) for w in words[4:9]]
    require(denominator > 0 and denominator_level > 0, 'denominateur non positif')
    actual_center = [F(value, denominator) + points[0][j] for j, value in enumerate(n)]
    require(actual_center == center, 'centre different')
    level = dot(sub(center, points[0]), sub(center, points[0]))
    require(F(numerator_level, denominator_level) == level, 'niveau different')
    require(power == denominator * (dot(sub(query, center), sub(query, center)) - level), 'puissance differente')
    require(orientation == orient(*points), 'orientation des sites differente')
    require(int(words[9]) == sign(orient(*points[:3], center)), 'orientation du centre differente')
    acute = all(dot(sub(points[j], points[i]), sub(points[k], points[i])) > 0 for i, j, k in ((0, 1, 2), (1, 0, 2), (2, 0, 1)))
    require(int(words[10]) == acute, 'angle strict different')
    require(int(words[11]) == is_inside(center, points), 'convexite stricte differente')
    require(int(words[12]) == all(2 * center[j] == points[0][j] + points[1][j] for j in range(3)), 'milieu different')
    require(int(words[13]) == sign(level - previous), 'ordre des niveaux different')
    return level, 11, False


def integer_cases():
    rng = random.Random(11128)
    edge = (1 << 256) - 1
    values = [0, 1, -1, edge, -edge, 1 << 64, 1 << 127, 1 << 192]
    out = list(itertools.product(values, repeat=2))
    out += [(rng.getrandbits(256) * rng.choice((-1, 1)), rng.getrandbits(256) * rng.choice((-1, 1))) for _ in range(96)]
    return out


def check_integer(pair, line):
    a, b = pair
    words = line.split()
    require(len(words) == 7 and words[0] == 'wide', 'ligne entiere mal formee')
    for actual_ok, actual, value in ((words[1], words[2], a + b), (words[3], words[4], a - b)):
        valid = abs(value) < 1 << 256
        require(int(actual_ok) == valid, 'depassement entier different')
        require(int(actual, 16) == (value if valid else 7), 'operation entiere non exacte/transactionnelle')
    require(int(words[5], 16) == a * b, 'produit entier different')
    require(int(words[6]) == sign(a - b), 'ordre entier different')
    return 7


def run(probe):
    info = subprocess.run([probe], input='', capture_output=True, text=True, timeout=15)
    require(info.returncode == 0 and not info.stderr, 'sonde metadata en echec')
    words = info.stdout.split()
    require(len(words) == 2 and words[0] == 'bits' and int(words[1]) in (18, 21, 24), 'profil absent')
    bits = int(words[1])
    geometry, integers = cases(bits), integer_cases()
    inputs = [' '.join(map(str, [q] + [v for p in list(points) + [query] for v in p])) for q, points, query in geometry]
    inputs += ['0 %s %s' % (format(a, 'x'), format(b, 'x')) for a, b in integers]
    payload = '\n'.join(inputs) + '\n'
    completed = subprocess.run([probe], input=payload, capture_output=True, text=True, timeout=45)
    require(completed.returncode == 0 and not completed.stderr, 'sonde en echec : ' + completed.stderr)
    lines = completed.stdout.splitlines()
    require(lines[0] == 'bits %d' % bits and len(lines) == 1 + len(inputs), 'nombre de reponses different')
    previous, checks, degeneracies = F(0), 0, 0
    for index, case in enumerate(geometry):
        try:
            previous, count, degenerate = check_geometry(case, lines[index + 1], previous)
        except (ValueError, IndexError) as error:
            raise ValueError('cas %d %s : %s' % (index, case, error)) from error
        checks += count
        degeneracies += degenerate
    for pair, line in zip(integers, lines[1 + len(geometry):]):
        checks += check_integer(pair, line)
    require(len(geometry) == 504 and len(integers) == 160 and degeneracies >= 30 and checks >= 5000, 'plancher non atteint')
    print(json.dumps({'bits': bits, 'geometry': len(geometry), 'integers': len(integers), 'degeneracies': degeneracies,
                      'checks': checks, 'input_sha256': hashlib.sha256(payload.encode()).hexdigest()}, sort_keys=True))


if __name__ == '__main__':
    if len(sys.argv) != 2:
        print('usage : fraction_oracle.py sonde', file=sys.stderr)
        sys.exit(2)
    try:
        run(sys.argv[1])
    except (ValueError, OSError, subprocess.TimeoutExpired) as error:
        print('ECHEC num Fraction : ' + str(error), file=sys.stderr)
        sys.exit(1)
