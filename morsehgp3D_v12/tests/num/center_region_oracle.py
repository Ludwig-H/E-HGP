"""Oracle rationnel des lieux de centres : Gauss 2x3 puis intervalle, aucun test SAT produit."""
import hashlib
import itertools
import json
import random
import subprocess
import sys
from dataclasses import dataclass
from fractions import Fraction as F
from functools import lru_cache


def need(value, message):
    if not value:
        raise ValueError(message)


def distance(a, b):
    return sum((x-y)**2 for x, y in zip(a, b))


@dataclass(frozen=True)
class Case:
    name: str
    q: int
    points: tuple
    lo: tuple
    hi: tuple

    def encode(self):
        return ' '.join(map(str, (self.q, *(v for p in self.points+(self.lo, self.hi) for v in p))))+'\n'


def affine_line(points):
    """Resout les deux egalites de distances par Gauss ; un axe libre parametre la droite."""
    a = points[0]
    rows = [[F(2*(x-y)) for x, y in zip(p, a)] + [F(sum(x*x-y*y for x, y in zip(p, a)))]
            for p in points[1:]]
    pivots = []
    for col in range(3):
        at = len(pivots)
        pivot = next((i for i in range(at, 2) if rows[i][col]), None)
        if pivot is None:
            continue
        rows[at], rows[pivot] = rows[pivot], rows[at]
        scale = rows[at][col]
        rows[at] = [v/scale for v in rows[at]]
        for i in range(2):
            if i != at:
                scale = rows[i][col]
                rows[i] = [v-scale*w for v, w in zip(rows[i], rows[at])]
        pivots.append(col)
        if len(pivots) == 2:
            break
    if len(pivots) != 2:
        return None
    free = next(j for j in range(3) if j not in pivots)
    origin, direction = [F(0)]*3, [F(0)]*3
    direction[free] = F(1)
    for row, col in zip(rows, pivots):
        origin[col], direction[col] = row[3], -row[free]
    return tuple(origin), tuple(direction)


@lru_cache(maxsize=None)
def geometry(case, bits):
    maximum = 1 << bits
    if any(not 0 <= v < maximum for p in case.points for v in p):
        return dict(verdict='refused coordinate_out_of_domain', contact=False)
    if any(not 0 <= lo < hi <= maximum for lo, hi in zip(case.lo, case.hi)) or case.q not in (2, 3):
        return dict(verdict='refused parameter_out_of_range', contact=False)
    if case.q == 2:
        # Une difference de distances carrees est affine ; ses extrema sont atteints aux coins.
        values = [distance(z, case.points[0])-distance(z, case.points[1])
                  for z in itertools.product(*zip(case.lo, case.hi))]
        low, high = min(values), max(values)
        return dict(verdict='intersects' if low <= 0 <= high else 'disjoint', contact=low == 0 or high == 0)
    line = affine_line(case.points)
    if line is None:
        return dict(verdict='degenerate', contact=False)
    origin, direction = line
    lower, upper, boundary = None, None, False
    for a, d, lo, hi in zip(origin, direction, case.lo, case.hi):
        if d == 0:
            if not lo <= a <= hi:
                return dict(verdict='disjoint', contact=False)
            boundary |= a in (lo, hi)
            continue
        near, far = sorted((F(lo-a, d), F(hi-a, d)))
        lower = near if lower is None else max(lower, near)
        upper = far if upper is None else min(upper, far)
    need(lower is not None and upper is not None, 'droite sans parametre libre')
    meets = lower <= upper
    return dict(verdict='intersects' if meets else 'disjoint', contact=meets and (boundary or lower == upper),
                origin=origin, direction=direction, lower=lower, upper=upper)


def cases(bits):
    maximum, zero = 1 << bits, (0, 0, 0)
    m = maximum-1
    out, pairs = [], []

    def add(name, q, points, lo, hi, permute=True):
        base = len(out)
        orders = list(itertools.permutations(range(q))) if permute and q in (2, 3) else [tuple(range(3))]
        for index, order in enumerate(orders):
            chosen = tuple(points[i] for i in order)
            if len(chosen) == 2:
                chosen += (points[2],)
            out.append(Case(name if index == 0 else name+'_p%d' % index, q, chosen, tuple(lo), tuple(hi)))
            if index:
                pairs.append((base, len(out)-1))

    pair_cases = [
        ('face_hi', zero, (4, 0, 0), zero, (2, 2, 2)),
        ('face_lo', zero, (4, 0, 0), (2, 0, 0), (3, 2, 2)),
        ('before_plane', zero, (4, 0, 0), zero, (1, 2, 2)),
        ('after_plane', zero, (4, 0, 0), (3, 0, 0), (4, 2, 2)),
        ('edge', zero, (2, 2, 0), zero, (1, 1, 1)),
        ('vertex', zero, (2, 2, 2), zero, (1, 1, 1)),
        ('vertex_lo', zero, (2, 2, 2), (1, 1, 1), (2, 2, 2)),
        ('oblique_miss', zero, (4, 4, 4), zero, (1, 1, 1)),
        ('extreme_domain', zero, (m, m, m), zero, (maximum,)*3),
        ('extreme_miss', zero, (m, m, m), zero, (1, 1, 1)),
        ('last_cell', zero, (m, m, m), (m,)*3, (maximum,)*3),
        ('coincident', zero, zero, (m,)*3, (maximum,)*3),
        ('last_cell_diagonal', (m, 0, 0), (0, m, 0), (m, m, 0), (maximum, maximum, 1)),
    ]
    for name, a, b, lo, hi in pair_cases:
        add('pair_'+name, 2, (a, b, zero), lo, hi)
    triangle = (zero, (4, 0, 0), (0, 4, 0))
    for name, lo, hi in [('edge_hi', zero, (2, 2, 1)), ('edge_lo', (2, 2, 0), (3, 3, 1)),
                         ('face', (0, 1, 0), (2, 3, 1)), ('before', zero, (1, 2, 1)),
                         ('after', (3, 2, 0), (4, 3, 1))]:
        add('line_'+name, 3, triangle, lo, hi)
    vertex = ((2, 2, 1), (1, 2, 2), (0, 0, 1))
    missed = ((7, 4, 2), (7, 0, 1), (7, 3, 4))
    add('line_vertex', 3, vertex, zero, (1, 1, 1))
    add('line_planes_meet_separately', 3, missed, zero, (2, 2, 2))
    add('line_axis2_separation', 3, (zero, (4, 0, 0), (0, 0, 4)), zero, (1, 1, 3))
    extreme = (zero, (m, 0, 0), (0, m, 0))
    add('line_extreme_miss', 3, extreme, zero, (1, 1, 1))
    add('line_extreme_domain', 3, extreme, zero, (maximum,)*3)
    add('line_thin', 3, (zero, (m, 1, 0), (m-1, 1, 0)), zero, (maximum,)*3)
    add('line_collinear', 3, (zero, (1, 1, 1), (m, m, m)), zero, (maximum,)*3)
    add('line_duplicate', 3, (zero, zero, (m, 1, 0)), zero, (maximum,)*3)
    add('line_all_equal', 3, (zero,)*3, zero, (maximum,)*3)
    upper = ((maximum-5, 0, 0), (maximum-4, 3, 0), (maximum-3, 4, 0))
    add('line_T0_upper_contact', 3, upper, (m, 0, 0), (maximum, 1, 1))
    tetra = ((1, 2, 6), (8, 4, 8), (2, 1, 3), (7, 8, 5))
    for omit in range(4):
        add('line_tetra_face%d' % omit, 3, tuple(p for i, p in enumerate(tetra) if i != omit), (4,)*3, (5,)*3)
    # Axes permutes : les signes et les pivots libres du juge changent, le verdict reste invariant.
    for label, points, lo, hi in [('vertex_axes', vertex, zero, (1,)*3),
                                  ('miss_axes', missed, zero, (2,)*3),
                                  ('T0_axes', upper, (m, 0, 0), (maximum, 1, 1))]:
        base = len(out)
        for perm in itertools.permutations(range(3)):
            rearrange = lambda p: tuple(p[i] for i in perm)
            out.append(Case(label+str(perm), 3, tuple(map(rearrange, points)), rearrange(lo), rearrange(hi)))
            if len(out)-1 != base:
                pairs.append((base, len(out)-1))
    rng = random.Random(110031)
    for q in (2, 3):
        for scale in (7, m):
            for i in range(40):
                points = tuple(tuple(rng.randrange(scale+1) for _ in range(3)) for _ in range(3))
                low = tuple(rng.randrange(scale+1) for _ in range(3))
                high = tuple(rng.randrange(x+1, scale+2) for x in low)
                add('random_q%d_s%d_%d' % (q, scale, i), q, points, low, high, False)
    for q in (2, 3):
        for axis in range(3):
            for kind in ('negative', 'equal', 'reversed', 'oversized'):
                low, high = [0]*3, [1]*3
                if kind == 'negative': low[axis] = -1
                if kind == 'equal': high[axis] = 0
                if kind == 'reversed': low[axis] = 2
                if kind == 'oversized': high[axis] = maximum+1
                add('bad_box_%d_%d_%s' % (q, axis, kind), q, triangle, low, high, False)
            for bad in (-1, maximum, -(1 << 63), (1 << 63)-1):
                points = list(triangle); changed = list(points[2]); changed[axis] = bad; points[2] = tuple(changed)
                add('bad_point_%d_%d_%d' % (q, axis, bad), q, tuple(points), zero, (maximum,)*3, False)
    for q in (-1, 0, 1, 4, (1 << 31)-1):
        add('bad_q%d' % q, q, triangle, zero, (maximum,)*3, False)
    return out, pairs


def check_case(case, line, bits):
    need(type(line) is str and line in ('intersects', 'disjoint', 'degenerate',
         'refused coordinate_out_of_domain', 'refused parameter_out_of_range'), 'forme du verdict')
    need(line == geometry(case, bits)['verdict'], 'verdict '+case.name)
    return 2


def judge(data, pairs, lines, bits):
    need(len(data) == len(lines) == 371 and len(pairs) == 123, 'inventaire des requetes')
    stats = dict(checks=0, intersects=0, disjoint=0, degenerate=0, refused=0, contacts=0, pairs=0, triangles=0)
    for case, line in zip(data, lines):
        stats['checks'] += check_case(case, line, bits)
        verdict = geometry(case, bits)
        stats['refused' if line.startswith('refused') else line] += 1
        stats['contacts'] += int(verdict['contact'])
        if not line.startswith('refused'):
            stats['pairs' if case.q == 2 else 'triangles'] += 1
    for a, b in pairs:
        need(lines[a] == lines[b], 'permutation change le verdict')
        stats['checks'] += 1
    need(stats['intersects'] >= 60 and stats['disjoint'] >= 50 and stats['degenerate'] >= 18 and
         stats['refused'] == 53 and stats['contacts'] >= 40, 'strates non vacantes')
    return stats


def run(probe):
    info = subprocess.run([probe], input='', text=True, capture_output=True, timeout=15)
    need(info.returncode == 0 and not info.stderr, 'metadata pilote')
    words = info.stdout.split()
    need(len(words) == 2 and words[0] == 'bits' and words[1] in ('21', '24', '32'), 'profil pilote')
    bits = int(words[1]); data, pairs = cases(bits)
    payload = ''.join(case.encode() for case in data)
    process = subprocess.run([probe], input=payload, text=True, capture_output=True, timeout=45)
    need(process.returncode == 0 and not process.stderr, 'echec du pilote')
    lines = process.stdout.splitlines()
    need(lines and lines[0] == 'bits %d' % bits, 'profil modifie')
    stats = judge(data, pairs, lines[1:], bits)
    print(json.dumps(dict(bits=bits, cases=len(data), permutations=len(pairs),
                         input_sha256=hashlib.sha256(payload.encode()).hexdigest(), **stats), sort_keys=True))


if __name__ == '__main__':
    try:
        need(len(sys.argv) == 2, 'usage: center_region_oracle.py probe|--selftest')
        if sys.argv[1] == '--selftest':
            from center_region_model_test import main
            main()
        else:
            run(sys.argv[1])
    except (ValueError, OSError, subprocess.TimeoutExpired, IndexError) as error:
        print('ECHEC CenterRegion Fraction : '+str(error), file=sys.stderr)
        sys.exit(1)
