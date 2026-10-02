"""Three tiny exact planar fixtures; no product call or random campaign."""
from fractions import Fraction as Q
from functools import lru_cache
from itertools import combinations
import json


def need(ok, message):
    if not ok:
        raise RuntimeError(message)


def dot(a, b):
    return sum(x * y for x, y in zip(a, b))


def d2(a, b):
    return sum((x - y) ** 2 for x, y in zip(a, b))


def ball(points):
    """Enumerate enclosing circles with one, two or three boundary sites."""
    candidates = [(p, Q(0)) for p in points]
    for a, b in combinations(points, 2):
        c = tuple((x + y) / 2 for x, y in zip(a, b))
        candidates.append((c, d2(a, c)))
    for a, b, c in combinations(points, 3):
        u = tuple(2 * (y - x) for x, y in zip(a, b))
        v = tuple(2 * (y - x) for x, y in zip(a, c))
        r = dot(b, b) - dot(a, a)
        s = dot(c, c) - dot(a, a)
        det = u[0] * v[1] - u[1] * v[0]
        if det:
            center = ((r * v[1] - u[1] * s) / det,
                      (u[0] * s - r * v[0]) / det)
            candidates.append((center, d2(a, center)))
    valid = [(r, c) for c, r in candidates
             if all(d2(p, c) <= r for p in points)]
    need(bool(valid), 'no enclosing circle')
    return min(valid)[0]


def judge(points):
    points = tuple(tuple(Q(x) for x in p) for p in points)

    @lru_cache(None)
    def beta(ids):
        return ball(tuple(points[i] for i in ids))

    def components(k, level, strict=False, universe=None):
        ids = tuple(range(len(points))) if universe is None else tuple(universe)
        admitted = lambda b: b < level if strict else b <= level
        vertices = [v for v in combinations(ids, k) if admitted(beta(v))]
        parent = {v: v for v in vertices}

        def root(v):
            while parent[v] != v:
                v = parent[v]
            return v

        for simplex in combinations(ids, k + 1):
            if admitted(beta(simplex)):
                faces = list(combinations(simplex, k))
                for face in faces[1:]:
                    parent[root(face)] = root(faces[0])
        groups = {}
        for v in vertices:
            groups.setdefault(root(v), []).append(v)
        return sorted(sorted(group) for group in groups.values())

    return beta, components


def main():
    # Existing Q1 fixture: local pieces merge via sites outside P_b.
    beta, comp = judge([(0, 0), (2, 0), (4, 0), (2, 3)])
    need(beta((0, 1, 3)) == beta((1, 2, 3)) == Q(13, 4), 'external path')
    local = comp(2, Q(4), True, (0, 1, 2))
    global_strict = comp(2, Q(4), True)
    need(local == [[(0, 1)], [(1, 2)]], 'two local pieces')
    need(len(global_strict) == 1, 'global surjection')
    surjection = {'local_components': local, 'global_components': global_strict,
                  'external_triangle_level': '13/4', 'closed_cell_level': '4'}

    # Existing Q1 descent fixture: two valid terminals, only same after closing 4.
    beta, comp = judge([(0, 0), (2, 0), (4, 0)])
    need(beta((0, 2)) == Q(4), 'start level')
    need(beta((0, 1)) == beta((1, 2)) == Q(1), 'terminal levels')
    strict_four = comp(2, Q(4), True)
    closed_four = comp(2, Q(4))
    need(len(comp(2, Q(1))) == len(strict_four) == 2, 'pre-cell distinction')
    need(len(closed_four) == 1, 'closed-cell class')
    memo = {'terminal_levels': ['1', '1'], 'start_level': '4',
            'components_at_closed_1': 2, 'components_at_open_4': strict_four,
            'components_at_closed_4': closed_four}

    # Additional wording fixture: an admitted sphere need not affect pi_0.
    beta, comp = judge([(2, 1), (1, 2), (0, 1), (1, 0)])
    need(beta((0, 2)) == beta((1, 3)) == Q(1), 'diameter supports')
    need(all(beta(pair) == Q(1, 2) for pair in [(0, 1), (1, 2), (2, 3), (0, 3)]), 'square sides')
    need(len(comp(1, Q(1, 2))) == len(comp(1, Q(1), True)) == len(comp(1, Q(1))) == 1,
         'admitted inert cell')
    inert = {'p': 0, 'q': 2, 'm': 4, 'K': 1, 'admitted': True,
             'sphere_level': '1', 'already_connected_at': '1/2',
             'open_cell_components': 1, 'closed_cell_components': 1}
    print(json.dumps({'fixtures': 3, 'max_sites': 4, 'product_invocations': 0,
                      'surjection': surjection, 'dated_memo': memo,
                      'admitted_inert_sphere': inert}, indent=2, sort_keys=True))


if __name__ == '__main__':
    main()
