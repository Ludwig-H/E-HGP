"""Tiny Fraction oracle: exact MEB classes and missing votes; no native/GCP."""
from fractions import Fraction as F
from itertools import combinations
from math import comb
import json


def require(ok, why):
    if not ok:
        raise ValueError(why)


def dot(a, b):
    return sum((x*y for x, y in zip(a, b)), F(0))


def sub(a, b):
    return tuple(x-y for x, y in zip(a, b))


def d2(a, b):
    return dot(sub(a, b), sub(a, b))


def solve(a, b):
    m = [list(row)+[x] for row, x in zip(a, b)]
    for j in range(len(b)):
        i = next((i for i in range(j, len(b)) if m[i][j]), None)
        if i is None:
            return None
        m[j], m[i] = m[i], m[j]
        q = m[j][j]
        m[j] = [x/q for x in m[j]]
        for i in range(len(b)):
            if i != j:
                q = m[i][j]
                m[i] = [x-q*y for x, y in zip(m[i], m[j])]
    return [row[-1] for row in m]


def critical_sphere(points):
    a = points[0]
    ds = [sub(x, a) for x in points[1:]]
    if not ds:
        return a, F(0)
    ts = solve([[dot(x, y) for y in ds] for x in ds], [dot(x, x)/2 for x in ds])
    if ts is None or min([1-sum(ts), *ts]) <= 0:
        return None
    c = tuple(a[j]+sum((t*x[j] for t, x in zip(ts, ds)), F(0)) for j in range(3))
    return c, d2(c, a)


def meb(points):
    balls = []
    for q in range(1, min(4, len(points))+1):
        for support in combinations(points, q):
            ball = critical_sphere(support)
            if ball is not None and all(d2(x, ball[0]) <= ball[1] for x in points):
                balls.append(ball)
    require(balls, 'no MEB')
    return min(balls, key=lambda b: b[1])


def in_convex_hull(center, points):
    # Caratheodory, direct affine reconstruction; NOT a sphere test.
    for q in range(1, min(4, len(points))+1):
        for support in combinations(points, q):
            a = support[0]
            ds = [sub(x, a) for x in support[1:]]
            target = sub(center, a)
            if not ds:
                if target == (0, 0, 0):
                    return True
                continue
            ts = solve([[dot(x, y) for y in ds] for x in ds], [dot(x, target) for x in ds])
            if ts is not None and min([1-sum(ts), *ts]) >= 0 and all(
                    sum((t*x[j] for t, x in zip(ts, ds)), F(0)) == target[j] for j in range(3)):
                return True
    return False


def choose(n, k):
    return comb(n, k) if 0 <= k <= n else 0


def count_fixture(name, I, U, beta):
    P = I+U
    center = (F(0), F(0), F(0))
    require(all(d2(x, center) < beta for x in I) and all(d2(x, center) == beta for x in U), 'I/U')
    h = {}
    hx = {x: {} for x in range(len(U))}
    for j in range(len(U)+1):
        valid = [S for S in combinations(range(len(U)), j) if in_convex_hull(center, [U[i] for i in S])]
        h[j] = len(valid)
        for x in range(len(U)):
            hx[x][j] = sum(x in S for S in valid)
    checks = []
    for K in range(1, len(P)+1):
        exact = [S for S in combinations(range(len(P)), K) if meb([P[i] for i in S]) == (center, beta)]
        for x in range(len(P)):
            if x < len(I):
                predicted = sum(h[j]*choose(len(I)-1, K-j-1) for j in h)
            else:
                predicted = sum(hx[x-len(I)][j]*choose(len(I), K-j) for j in h)
            observed = sum(x in S for S in exact)
            require(predicted == observed, 'class formula mismatch')
            checks.append(dict(K=K, x=x, formula=predicted, enumerated=observed))
    return dict(name=name, p=len(I), q=len(U), beta=str(beta), h=h, hx=hx, checks=checks)


def main():
    vec = lambda a: tuple(F(x) for x in a)
    fixtures = [
        ('segment_regular', [(0, 0, 0), (F(1, 2), 0, 0)], [(-1, 0, 0), (1, 0, 0)], F(1)),
        ('triangle_regular', [(0, 0, 0), (F(1, 4), 0, 0)], [(1, 0, 0), (-F(3, 5), F(4, 5), 0), (-F(3, 5), -F(4, 5), 0)], F(1)),
        ('square_contact', [(0, 0, 0), (F(1, 4), F(1, 4), 0)], [(1, 0, 0), (-1, 0, 0), (0, 1, 0), (0, -1, 0)], F(1)),
        ('tetrahedron_regular', [(0, 0, 0), (F(1, 4), 0, 0)], [(1, 1, 1), (1, -1, -1), (-1, 1, -1), (-1, -1, 1)], F(3)),
    ]
    reports = [count_fixture(name, list(map(vec, I)), list(map(vec, U)), beta) for name, I, U, beta in fixtures]
    for r in (reports[0], reports[1], reports[3]):
        require(r['h'][r['q']] == 1 and sum(r['h'].values()) == 1, 'regular reduction')
    require(reports[2]['h'] == {0: 0, 1: 0, 2: 2, 3: 4, 4: 1}, 'nonregular shell multiplicity')
    P = [vec((0, 0, 0)), vec((F(19, 10), 0, 0)), vec((F(39, 20), 0, 0)), vec((2, 0, 0))]
    K = Kmax = 2
    eta = F(1, 8)
    pairs = list(combinations(range(4), 2))
    alpha2 = min(meb([P[i] for i in S])[1] for S in pairs if 0 in S)
    threshold2 = (1+eta)**2*alpha2
    votes = [S for S in pairs if 0 in S and meb([P[i] for i in S])[1] <= threshold2]
    require(alpha2 == F(361, 400) and threshold2 == F(29241, 25600), 'band')
    selected = meb([P[0], P[3]])
    I = [i for i in range(4) if d2(P[i], selected[0]) < selected[1]]
    U = [i for i in range(4) if d2(P[i], selected[0]) == selected[1]]
    qmin = 2
    require(votes == [(0, 1), (0, 2), (0, 3)] and I == [1, 2] and U == [0, 3], 'omitted identity')
    require(selected == (vec((1, 0, 0)), F(1)) and len(I)+qmin > Kmax+1 and not len(I) < K <= len(I)+len(U), 'FULL bound')
    kept = [S for S in votes if sum(d2(P[i], meb([P[j] for j in S])[0]) < meb([P[j] for j in S])[1] for i in range(4))+2 <= Kmax+1]
    require(kept == [(0, 1), (0, 2)], 'retained vote identities')
    print(json.dumps(dict(status='MEB_CLASS_FORMULAS_AND_FULL_VOTE_GAP_PROVED', class_fixtures=reports,
        formula_checks=sum(len(r['checks']) for r in reports), missing_vote=dict(points=[[str(c) for c in p] for p in P],
        K=K, Kmax=Kmax, eta=str(eta), alpha2=str(alpha2), threshold2=str(threshold2), beta='1',
        I=I, U=U, p=2, qmin=2, selected_parts=votes, retained_parts=kept, missing_part=[0, 3],
        mass_full=3, mass_from_bounded_catalogue=2,
        scope='loss of reference vote identity and mass; no changed majority height claimed'),
        native_calls=0, GCP_used=False, scope='tiny Fraction combinatorial oracle; no industrial enumeration proposed'), sort_keys=True, indent=2))


if __name__ == '__main__':
    main()
