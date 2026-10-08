"""Oracle rationnel borne, sans moteur, fichier de points ni sous-processus."""
from fractions import Fraction as Q
from itertools import combinations, permutations, product
import json
import random


def require(ok, message):
    if not ok:
        raise RuntimeError(message)


def dot(a, b):
    return sum(x * y for x, y in zip(a, b))


def sub(a, b):
    return tuple(x - y for x, y in zip(a, b))


def solve(a, b):
    n = len(b)
    m = [[Q(x) for x in row] + [Q(rhs)] for row, rhs in zip(a, b)]
    for j in range(n):
        pivot = next((i for i in range(j, n) if m[i][j]), None)
        if pivot is None:
            return None
        m[j], m[pivot] = m[pivot], m[j]
        d = m[j][j]
        m[j] = [x / d for x in m[j]]
        for i in range(n):
            if i != j:
                d = m[i][j]
                m[i] = [x - d * y for x, y in zip(m[i], m[j])]
    return [row[-1] for row in m]


def sphere(s):
    if len(s) == 1:
        return s[0], Q(0)
    u = [sub(p, s[0]) for p in s[1:]]
    w = solve([[dot(a, b) for b in u] for a in u], [Q(dot(a, a), 2) for a in u])
    if w is None or min(w) <= 0 or sum(w) >= 1:
        return None
    c = tuple(s[0][j] + sum(w[i] * u[i][j] for i in range(len(u))) for j in range(3))
    return c, dot(sub(c, s[0]), sub(c, s[0]))


def oracle(points):
    candidates = []
    for q in range(1, min(4, len(points)) + 1):
        for s in combinations(sorted(points), q):
            ball = sphere(s)
            if ball is None:
                continue
            c, r = ball
            if all(dot(sub(p, c), sub(p, c)) <= r for p in points):
                candidates.append((r, q, s, c))
    require(bool(candidates), 'no independent enclosing ball')
    return min(candidates)


def proposal(points):
    pairs = list(combinations(points, 2))
    a, b = min(pairs, key=lambda s: (-dot(sub(*s), sub(*s)), tuple(sorted(s))))
    if all(dot(sub(x, a), sub(x, b)) <= 0 for x in points):
        return tuple(sorted((a, b)))
    return tuple(sorted(points)) if len(points) == 3 else None


def main():
    grid = list(product(range(3), range(3), (0,)))
    cases = list(combinations(grid, 2)) + list(combinations(grid, 3))
    rng = random.Random(20261008)
    space = list(product(range(4), repeat=3))
    for n in range(4, 13):
        cases += [tuple(rng.sample(space, n)) for _ in range(3)]
    rectangle = ((0, 0, 0), (4, 0, 0), (4, 2, 0), (0, 2, 0))
    cases += list(permutations(rectangle))
    bounds = {}
    for bits in (21, 24, 30, 31, 32):
        top = (1 << bits) - 1
        bound = 3 * top * top
        signed_bits = 64 if bits <= 30 else 128
        require(bound < 1 << (signed_bits - 1), 'insufficient integer width')
        bounds[str(bits)] = {'absolute_bound': bound, 'signed_bits': signed_bits}
        cases += [((0, 0, 0), (top, top, top)),
                  ((0, 0, 0), (top, 0, 0), (0, top, 0)),
                  ((0, 0, 0), (top, top, 0), (top, 0, top)),
                  ((0, 0, 0), (top, top, 0), (top, 0, top), (0, top, top))]
    totals = {'cases': len(cases), 'pair': 0, 'triangle': 0, 'deferred': 0}
    for points in cases:
        r, q, support, center = oracle(points)
        s = proposal(points)
        if s is None:
            require(q >= 3 and len(points) >= 4, 'diametral ball wrongly deferred')
            totals['deferred'] += 1
        else:
            require(s == support and sphere(s) == (center, r), 'proposal disagrees with independent oracle')
            totals['pair' if len(s) == 2 else 'triangle'] += 1
    whole = ((0, 0, 0), (0, 2, 0), (2, 0, 0), (2, 2, 0))
    part = (whole[1], whole[2])
    full, local = oracle(whole), oracle(part)
    require(full[0] == local[0] and full[3] == local[3] and full[2] != local[2], 'canonical counterexample missing')
    return {'totals': totals, 'integer_bounds': bounds, 'global_canonical_counterexample':
            {'whole': whole, 'part': part, 'global_support': full[2], 'part_support': local[2],
             'radius_squared': str(full[0]), 'center': [str(x) for x in full[3]]}}


if __name__ == '__main__':
    print(json.dumps(main(), indent=2, sort_keys=True))
