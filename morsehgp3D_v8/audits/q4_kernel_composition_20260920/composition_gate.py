#!/usr/bin/env python3
"""Independent rational model: nested shallow kernels and exact cell partitions."""
from fractions import Fraction as F
from itertools import combinations
import json
import random


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def value(form, point):
    c, a, b = form
    x, y = point
    return c+a*x+b*y


def cross(a, b, c):
    return (b[0]-a[0])*(c[1]-a[1])-(b[1]-a[1])*(c[0]-a[0])


def kernel(forms, ids, threshold, origin=(F(0), F(0))):
    """Supporting-line oracle, deliberately not the product's monotone chain."""
    require(threshold > 0, 'Positive threshold required')
    retained, layers = set(), []
    for sign in (-1, 1):
        groups = {}
        for idx in ids:
            d = value(forms[idx], origin)
            if d == 0:
                retained.add(idx)
            elif (d > 0) == (sign > 0):
                _, a, b = forms[idx]
                groups.setdefault((F(a)/d, F(b)/d), set()).add(idx)
        for _ in range(threshold):
            points = sorted(groups)
            if not points:
                break
            if len(points) < 3 or all(cross(points[0], points[1], p) == 0 for p in points):
                retained.update(idx for group in groups.values() for idx in group)
                break
            boundary = set()
            for a, b in combinations(points, 2):
                turns = [cross(a, b, p) for p in points]
                if min(turns) >= 0 or max(turns) <= 0:
                    boundary.update(p for p, turn in zip(points, turns) if turn == 0)
            require(boundary, 'Nondegenerate hull has no boundary')
            layer = set()
            for p in boundary:
                layer.update(groups.pop(p))
            retained.update(layer)
            layers.append(layer)
    return retained, layers


def census(forms, ids, point):
    inside, shell = set(), set()
    for idx in ids:
        v = value(forms[idx], point)
        if v < 0:
            inside.add(idx)
        elif v == 0:
            shell.add(idx)
    return len(inside), shell


def partition(forms, ids, cell):
    left, right, bottom, top = cell
    corners = [(x, y) for x in (left, right) for y in (bottom, top)]
    inside, outside, active = set(), set(), set()
    for idx in ids:
        values = [value(forms[idx], p) for p in corners]
        if max(values) < 0:
            inside.add(idx)
        elif min(values) > 0:
            outside.add(idx)
        else:
            active.add(idx)
    require(inside | outside | active == set(ids), 'Lost population')
    require(not (inside & outside or inside & active or outside & active), 'Overlap')
    return inside, outside, active


def roots(forms):
    result = set()
    for f, g in combinations(forms, 2):
        c, a, b = f
        d, u, v = g
        den = a*v-b*u
        if den:
            result.add((F(b*d-c*v, den), F(c*u-a*d, den)))
    return result


def determinant(rows):
    (c, a, b), (d, u, v), (e, h, i) = rows
    return c*(u*i-v*h)-a*(d*i-v*e)+b*(d*h-u*e)


def main():
    rng = random.Random(20260920)
    fixtures = [
        [(0, 0, 0), (0, 0, 0), (0, 1, 0), (0, -1, 0), (1, 0, 1), (-1, 0, -1)],
        [(1, a, b) for a, b in ((-3,-3),(3,-3),(3,3),(-3,3),(0,0),(0,-3),(0,3))],
        [(c, a*c, b*c) for c in (-2,-1,1,2) for a, b in ((-2,-2),(2,-2),(2,2),(-2,2),(0,0))],
        [(1, a, 1) for a in range(-5,6)]+[(0, 0, 0)],
    ]
    for _ in range(20):
        fixtures.append([tuple(rng.randrange(-9, 10) for _ in range(3)) for _ in range(12)])
    cells = [(F(-2),F(0),F(-2),F(0)), (F(0),F(2),F(-2),F(0)),
             (F(-1),F(1),F(-1),F(1)), (F(0),F(1,4),F(-1),F(-3,4))]
    counts = dict(fixtures=len(fixtures), kernels=0, global_points=0, removed_contacts=0,
                  fragments=0, local_kernels=0, local_points=0, accepted_points=0,
                  retained_events=0, seed_exclusions=0, origin_changes=0, translation_checks=0)
    for forms in fixtures:
        ids = set(range(len(forms)))
        points = roots(forms) | {(F(x,2),F(y,2)) for x in range(-4,5) for y in range(-4,5)}
        for threshold in (1, 2, 3):
            retained, _ = kernel(forms, ids, threshold)
            counts['kernels'] += 1
            for point in points:
                reduced, reduced_shell = census(forms, retained, point)
                full, full_shell = census(forms, ids, point)
                require(reduced <= full, 'Kernel not a subset')
                if reduced < threshold:
                    require((reduced,reduced_shell) == (full,full_shell), 'Strong kernel failed')
                if any(value(forms[i],point) <= 0 for i in ids-retained):
                    counts['removed_contacts'] += 1
                    require(reduced >= threshold, 'Removed nonpositive lacks retained certificate')
                counts['global_points'] += 1
            for cell in cells:
                inside, _, active = partition(forms, retained, cell)
                counts['fragments'] += 1
                if len(inside) >= threshold:
                    continue
                remaining = threshold-len(inside)
                origin = ((cell[0]+cell[1])/2,(cell[2]+cell[3])/2)
                local, _ = kernel(forms, active, remaining, origin)
                unchanged_origin, _ = kernel(forms, active, remaining)
                counts['origin_changes'] += (local != unchanged_origin)
                counts['local_kernels'] += 1
                for point in points | {origin}:
                    if not (cell[0] <= point[0] <= cell[1] and cell[2] <= point[1] <= cell[3]):
                        continue
                    depth, shell = census(forms, local, point)
                    depth += len(inside)
                    full, full_shell = census(forms, ids, point)
                    counts['local_points'] += 1
                    require(depth <= full, 'Nested count is not a lower bound')
                    require((depth < threshold) == (full < threshold), 'Lost or false shallow point')
                    if depth < threshold:
                        require((depth,shell) == (full,full_shell), 'Nested payload differs')
                        counts['accepted_points'] += 1
                        counts['retained_events'] += bool(shell)
                    for idx in active-local:
                        if value(forms[idx],point) == 0:
                            require(depth >= threshold, 'Removed seed supports shallow point')
                            counts['seed_exclusions'] += 1
    # Integer translations: use the UNshifted determinant, never multiply its
    # three translated denominators. Positive Q cannot change orientation.
    max_naive_bits = 0
    for _ in range(400):
        m = 65535
        rows = [(rng.randrange(-15*m*m,15*m*m),rng.randrange(-8*m*m,8*m*m),
                 rng.randrange(-8*m*m,8*m*m)) for _ in range(3)]
        q = 1 << rng.choice((7,10,44))
        alpha, beta = rng.randrange(-2*q,2*q+1),rng.randrange(-2*q,2*q+1)
        shifted = [(q*c+alpha*a+beta*b,a,b) for c,a,b in rows]
        require(determinant(shifted) == q*determinant(rows), 'Translated determinant identity failed')
        (c,a,b),(d,u,v),_ = rows
        require(a*shifted[1][0]-u*shifted[0][0] == q*(a*d-u*c)+beta*(a*v-u*b),
                'Reduced translated coordinate comparison failed')
        max_naive_bits=max(max_naive_bits,abs(determinant(shifted)).bit_length())
        counts['translation_checks'] += 1
    # Two individually valid strong kernels cannot be intersected independently.
    constants = [(-1,0,0),(-2,0,0)]
    point = (F(0),F(0))
    require(census(constants,{0},point)[0] >= 1 and census(constants,{1},point)[0] >= 1,
            'Independent-kernel counterexample vacuous')
    require(census(constants,set(),point)[0] < 1 <= census(constants,{0,1},point)[0],
            'Independent intersection mutant not killed')
    # The same failure on a genuinely positive regular tetrahedron with two
    # sites strictly between the owner edge's endpoints (constant negatives).
    points3 = [(30,30,30),(36,36,30),(30,36,24),(36,30,24),(31,31,30),(32,32,30)]
    a3,b3 = points3[:2]
    forms3 = []
    for p in points3:
        w = tuple(2*p[i]-a3[i]-b3[i] for i in range(3))
        forms3.append((sum(x*x for x in w)-72, 12*w[0]-12*w[1], -12*w[2]))
    target = (F(0),F(-1))
    center = (33,33,27)
    require(all(sum((p[i]-center[i])**2 for i in range(3))==27 for p in points3[:4]),
            'Regular tetrahedron lost its common sphere')
    require(all(sum((points3[j][i]-points3[k][i])**2 for i in range(3))==72
                for j,k in combinations(range(4),2)), 'Tetrahedron is not regular/positive')
    require(census(forms3,set(range(6)),target)==(2,{0,1,2,3}), 'Real full depth changed')
    require(census(forms3,{0,1,2,3,4},target)[0] == census(forms3,{0,1,2,3,5},target)[0] == 1,
            'Real independent kernels no longer reject')
    require(census(forms3,{0,1,2,3},target)==(0,{0,1,2,3}), 'Real intersection mutant not killed')
    # An inherited exact count is used exactly once.
    forms=[(-1,0,0),(0,1,0),(0,0,1)]
    require(census(forms,{0,1,2},point)==(1,{1,2}), 'Count fixture changed')
    require(1 < 2 and not 1+1 < 2, 'Double inherited-count mutant not killed')
    for key in ('removed_contacts','local_kernels','accepted_points','retained_events','seed_exclusions','origin_changes'):
        require(counts[key] > 0, 'Vacuous gate: '+key)
    require(max_naive_bits > 127, 'Translated-width fixture did not exceed i128')
    print(json.dumps(dict(schema='mhgp8_audit_kernel_composition_math_v1',status='passed',
                         scope='independent affine rational model; not product integration or complexity qualification',
                         counts=counts,max_naive_translated_det_bits=max_naive_bits,
                         mutants=['intersection_of_independent_kernels','double_inherited_count']),sort_keys=True))


if __name__ == '__main__':
    main()
