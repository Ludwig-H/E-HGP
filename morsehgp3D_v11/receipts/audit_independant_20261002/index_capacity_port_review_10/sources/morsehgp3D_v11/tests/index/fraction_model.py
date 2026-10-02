"""Oracle du census GLOBAL : Gram/Fraction puis scan, sans arbre ni bornes centre-boite."""
from fractions import Fraction as F


def require(condition, message):
    if not condition:
        raise ValueError(message)


def dot(a, b):
    return sum(x * y for x, y in zip(a, b))


def sub(a, b):
    return tuple(x - y for x, y in zip(a, b))


def solve(matrix, rhs):
    rows = [[F(x) for x in row] + [F(value)] for row, value in zip(matrix, rhs)]
    size = len(rows)
    for col in range(size):
        pivot = next((i for i in range(col, size) if rows[i][col]), None)
        if pivot is None:
            return None
        rows[col], rows[pivot] = rows[pivot], rows[col]
        divisor = rows[col][col]
        rows[col] = [value / divisor for value in rows[col]]
        for i in range(size):
            if i != col:
                factor = rows[i][col]
                rows[i] = [a - factor * b for a, b in zip(rows[i], rows[col])]
    return tuple(row[-1] for row in rows)


def sphere(support):
    require(1 <= len(support) <= 4, 'arite 1..4')
    anchor = support[0]
    edges = [sub(point, anchor) for point in support[1:]]
    weights = solve([[dot(a, b) for b in edges] for a in edges], [F(dot(a, a), 2) for a in edges])
    require(weights is not None, 'support independant requis')
    center = tuple(F(anchor[j]) + sum(w * edge[j] for w, edge in zip(weights, edges)) for j in range(3))
    radius = dot(sub(center, anchor), sub(center, anchor))
    return center, radius


def morton(point):
    # Une cle Python sans troncature ; tous les bits presents participent a l'ordre canonique.
    return sum(((value >> bit) & 1) << (3 * bit + axis)
               for axis, value in enumerate(point) for bit in range(value.bit_length()))


def sites_of(records):
    grouped = {}
    for x, y, z, point_id in records:
        grouped.setdefault((x, y, z), []).append(point_id)
    sites = sorted(grouped, key=morton)
    return sites, [sorted(grouped[point]) for point in sites]


def population(records, support):
    sites, ids = sites_of(records)
    center, radius = sphere(support)
    powers = [dot(sub(point, center), sub(point, center)) - radius for point in sites]
    return dict(sites=[list(p) for p in sites], site_ids=ids, center=center, radius=radius,
                inner=[i for i, value in enumerate(powers) if value < 0],
                shell=[i for i, value in enumerate(powers) if value == 0])
