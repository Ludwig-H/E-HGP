"""Tiny exact independent checks for answers Q1-Q4; no native engine."""
from fractions import Fraction as Q
from itertools import combinations
import json

CHECKS = 0

def require(value):
    global CHECKS
    CHECKS += 1
    if not value:
        raise RuntimeError('check %d failed' % CHECKS)


def sqdist(a, b):
    return sum((Q(x)-Q(y))**2 for x, y in zip(a, b))


def sphere(support):
    if len(support) == 1:
        return tuple(map(Q, support[0])), Q(0)
    if len(support) == 2:
        c = tuple((Q(x)+Q(y))/2 for x, y in zip(*support))
        return c, sqdist(c, support[0])
    a, b, c = support
    ux, uy = (2*(b[i]-a[i]) for i in range(2))
    vx, vy = (2*(c[i]-a[i]) for i in range(2))
    det = ux*vy-uy*vx
    if not det:
        return None
    h = sum(x*x for x in b)-sum(x*x for x in a)
    j = sum(x*x for x in c)-sum(x*x for x in a)
    center = (Q(h*vy-uy*j, det), Q(ux*j-h*vx, det))
    return center, sqdist(center, a)


def meb(points):
    candidates = []
    for size in range(1, min(3, len(points))+1):
        for support in combinations(points, size):
            ball = sphere(support)
            if ball and all(sqdist(ball[0], p) <= ball[1] for p in points):
                candidates.append(ball)
    require(bool(candidates))
    return min(candidates, key=lambda b: b[1])


def gamma_partition(points, k, level, strict=False, omitted=frozenset()):
    vertices = [f for f in combinations(range(len(points)), k)
                if (meb([points[i] for i in f])[1] < level if strict
                    else meb([points[i] for i in f])[1] <= level)]
    root = list(range(len(vertices)))
    def find(i):
        while root[i] != i:
            i = root[i]
        return i
    for i, f in enumerate(vertices):
        for j, g in enumerate(vertices[:i]):
            ball = meb([points[x] for x in sorted(set(f+g))])
            if ball in omitted:
                continue
            if ball[1] < level if strict else ball[1] <= level:
                root[find(i)] = find(j)
    out = {}
    for i, f in enumerate(vertices):
        out.setdefault(find(i), set()).add(f)
    return list(out.values())


def main():
    line = [(0, 0), (2, 0), (4, 0)]
    require(meb([line[0], line[2]])[1] == 4)
    require(meb(line[:2])[1] == meb(line[1:])[1] == 1)
    require(len(gamma_partition(line, 2, Q(4), strict=True)) == 2)
    require(len(gamma_partition(line, 2, Q(4))) == 1)
    extended = line + [(2, 3)]
    require(meb([extended[i] for i in (0, 1, 3)])[1] == Q(13, 4))
    require(meb([extended[i] for i in (1, 2, 3)])[1] == Q(13, 4))
    local = gamma_partition(line, 2, Q(4), strict=True)
    glob = gamma_partition(extended, 2, Q(4), strict=True)
    require(len(local) == 2)
    require(any((0, 1) in group and (1, 2) in group for group in glob))

    # Degree three and 15-bit inputs alone do not imply the F2 bound.
    x = 32767
    a = 512*x**3
    require(2**53 < a < 2**54)
    require(Q(float(a)) == a)
    bad = ((float(a)+1.0)-float(a))-1.0
    require(bad == -1.0 and (a+1)-a-1 == 0)

    # Euler contributions cancel exactly, while the K1 partition changes.
    cloud = [(0, 5), (8, 9), (8, 1), (35, 5), (45, 5)]
    triangle = ((Q(5), Q(5)), Q(25))
    pair = ((Q(40), Q(5)), Q(25))
    require(meb(cloud[:3]) == triangle)
    require(meb(cloud[3:]) == pair)
    populations = []
    for size in range(1, len(cloud)+1):
        for part in combinations(cloud, size):
            populations.append((meb(part), (-1)**(size+1)))
    require(sum(w for b, w in populations if b == triangle) == 1)
    require(sum(w for b, w in populations if b == pair) == -1)
    levels = sorted({b[1] for b, _ in populations})
    cuts = levels + [(a+b)/2 for a, b in zip(levels, levels[1:])]
    omitted = frozenset((triangle, pair))
    for level in cuts:
        truth = sum(w for b, w in populations if b[1] <= level)
        amputated = sum(w for b, w in populations if b[1] <= level and b not in omitted)
        require(truth == amputated)
    require(len(gamma_partition(cloud, 1, Q(24))) == 3)
    require(len(gamma_partition(cloud, 1, Q(25))) == 2)
    require(len(gamma_partition(cloud, 1, Q(25), omitted=omitted)) == 3)

    # The cover tie at the middle site is swapped by a fixing reflection.
    require(len(gamma_partition(line, 2, Q(1))) == 2)
    c1 = meb(line[:2]); c2 = meb(line[1:])
    require(sqdist(c1[0], line[1]) == sqdist(c2[0], line[1]) == 1)
    require(4-c1[0][0] == c2[0][0] and 4-line[1][0] == line[1][0])
    print(json.dumps({'status': 'PASS', 'checks': CHECKS, 'native_engine_calls': 0,
                      'euler_cloud_u18_2d': cloud, 'euler_shared_level': '25',
                      'euler_cut_checks': len(cuts), 'F2_exact': 0, 'F2_binary64': bad,
                      'scope': 'Exact toy definitions, no production amputation or predicate executed'}, sort_keys=True))

if __name__ == '__main__':
    main()
