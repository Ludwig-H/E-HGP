#!/usr/bin/env python3
"""Independent finite rational counterexamples; no product/native imports."""
from fractions import Fraction as F
from itertools import combinations
import json

checks = 0


def check(condition, name):
    global checks
    checks += 1
    if not condition:
        raise ValueError(name)


def encode(value):
    if isinstance(value, F):
        return str(value)
    if isinstance(value, dict):
        return {k: encode(v) for k, v in value.items()}
    if isinstance(value, (list, tuple)):
        return [encode(v) for v in value]
    return value


def omega_axis(points, k, radius):
    """Intersection with the axis, preserving multiplicities of indexed sites."""
    if k < 1 or radius < 0:
        raise ValueError("domain")
    pieces = []
    for q in combinations(points, k):
        lo, hi = max(q) - radius, min(q) + radius
        if lo <= hi:
            pieces.append((lo, hi))
    union = []
    for lo, hi in sorted(pieces):
        if union and lo <= union[-1][1]:
            union[-1] = (union[-1][0], max(hi, union[-1][1]))
        else:
            union.append((lo, hi))
    return union


def contained(left, right):
    return all(any(c <= a and b <= d for c, d in right) for a, b in left)


def d_k(points, y, k):
    return sorted(abs(y - p) for p in points)[k - 1]


def support_hausdorff(left, right):
    return max(max(min(abs(a - b) for b in right) for a in left),
               max(min(abs(a - b) for a in left) for b in right))


def norm2(p):
    return sum(x * x for x in p)


def sub(a, b):
    return tuple(x - y for x, y in zip(a, b))


def dot(a, b):
    return sum(x * y for x, y in zip(a, b))


def circle(points):
    a, b, c = points
    u, v = sub(b, a), sub(c, a)
    rhs_u, rhs_v = (norm2(b) - norm2(a)) / 2, (norm2(c) - norm2(a)) / 2
    det = u[0] * v[1] - u[1] * v[0]
    if det == 0:
        raise ValueError("collinear")
    centre = ((rhs_u * v[1] - u[1] * rhs_v) / det,
              (u[0] * rhs_v - rhs_u * v[0]) / det)
    return centre, norm2(sub(a, centre))


def triangle_alpha(points):
    """For three noncollinear sites: constrained edge minima, and face date."""
    _, face = circle(points)
    edges, powers = {}, {}
    for i, j in combinations(range(3), 2):
        other = next(t for t in range(3) if t not in (i, j))
        centre = tuple((a + b) / 2 for a, b in zip(points[i], points[j]))
        meb = norm2(sub(points[i], centre))
        power = norm2(sub(points[other], centre)) - meb
        powers[(i, j)] = power
        # If the third point is inside the diametral ball, the constrained
        # minimum is its endpoint on the circumcircle, not this pair MEB.
        edges[(i, j)] = face if power < 0 else meb
    return edges, face, powers


def segment_distance2(point, a, b):
    v = sub(b, a)
    t = dot(sub(point, a), v) / norm2(v)
    check(0 < t < 1, "perpendicular foot lies inside segment")
    foot = tuple(x + t * y for x, y in zip(a, v))
    return norm2(sub(point, foot)), t


radius = F(2)
birth_p = tuple(map(F, (0, 6, 12)))
birth_new = birth_p + (F(1),)
birth_before, birth_after = omega_axis(birth_p, 2, radius), omega_axis(birth_new, 2, radius)
check(len(birth_before) == 0 and len(birth_after) == 1, "one added site creates order-two component")
check(birth_after == [(F(-1), F(2))], "new component axis interval")

bridge_p = tuple(map(F, (0, 2, 5)))
bridge_new = bridge_p + (F(3),)
bridge_before, bridge_after = omega_axis(bridge_p, 2, radius), omega_axis(bridge_new, 2, radius)
check(bridge_before == [(F(0), F(2)), (F(3), F(4))], "two separated original lenses")
check(bridge_after == [(F(0), F(5))], "one added site bridges components")
check(omega_axis((F(2), F(3)), 2, radius) == [(F(1), F(4))], "bridge pair crosses original gap")
separated = bridge_p + (F(100),)
check(omega_axis(separated, 2, radius) == bridge_before, "strictly separated added site has no effect here")

# Addition/deletion and bounded motion: exact axis checks of the rank-shift
# inclusions. Their general count proof is in README, not inferred by testing.
mixed_new = (F(-1, 4), F(9, 4), F(3))
epsilon_mixed = F(1, 4)
for r in map(F, (0, F(1, 4), 1, 2, 3)):
    for k in (2, 3):
        check(contained(omega_axis(bridge_p, k, r), omega_axis(mixed_new, k - 1, r + epsilon_mixed)),
              "delete one, move matched sites: lower rank inclusion")
        check(contained(omega_axis(mixed_new, k, r), omega_axis(bridge_p, k - 1, r + epsilon_mixed)),
              "add one, move matched sites: reverse lower rank inclusion")

# Quantification to nearest integer: geometrically close support, lost counts.
quant_p = (F(1, 10), F(1, 5), F(10), F(20))
quant_labels = tuple(F((p + F(1, 2)).numerator // (p + F(1, 2)).denominator) for p in quant_p)
quant_unique = tuple(sorted(set(quant_labels)))
epsilon_quant = max(abs(p - q) for p, q in zip(quant_p, quant_labels))
check(quant_labels == (F(0), F(0), F(10), F(20)), "nearest-grid labels collide")
check(epsilon_quant == F(1, 5), "maximum paired quantification displacement")
check(support_hausdorff(quant_p, quant_unique) == epsilon_quant, "small support Hausdorff")
quant_r = F(1, 10)
check(omega_axis(quant_p, 2, quant_r) == [(F(1, 10), F(1, 5))], "component before unique-site merge")
check(not omega_axis(quant_unique, 2, quant_r + epsilon_quant), "fixed-k inclusion fails after deduplication")
check(d_k(quant_p, F(0), 2) == F(1, 5) and d_k(quant_unique, F(0), 2) == F(10), "second distance jumps after merge")
for r in map(F, (0, F(1, 10), F(1, 5), 1)):
    for k in range(1, 5):
        check(contained(omega_axis(quant_p, k, r), omega_axis(quant_labels, k, r + epsilon_quant)), "paired labelled quantification forward")
        check(contained(omega_axis(quant_labels, k, r), omega_axis(quant_p, k, r + epsilon_quant)), "paired labelled quantification reverse")

# An arbitrarily close unweighted net can likewise lose local multiplicity.
sample = (quant_p[0], quant_p[2], quant_p[3])
net_error = support_hausdorff(quant_p, sample)
check(net_error == F(1, 10), "subsample is close as a geometric net")
check(not omega_axis(sample, 2, quant_r + net_error), "net does not preserve fixed-k component")
for r in map(F, (0, 1, 5, 10)):
    for k in (1, 2, 3):
        check(contained(omega_axis(quant_p, k + 1, r), omega_axis(sample, k, r)), "one deletion gives upper-rank lower bound")
        check(contained(omega_axis(sample, k, r), omega_axis(quant_p, k, r)), "subsampling cannot enlarge fixed-k region")

# Paired small displacement: a component disappears at a fixed cut, while
# another remains far away. L is a freely enlargeable separation parameter.
length, epsilon = F(100), F(1, 10)
moving_p = (F(0), F(2), length, length + 1)
moving_new = (-epsilon, 2 + epsilon, length, length + 1)
old_region, new_region = omega_axis(moving_p, 2, F(1)), omega_axis(moving_new, 2, F(1))
check(old_region == [(F(1), F(1)), (length, length + 1)], "old component at birth")
check(new_region == [(length, length + 1)], "near component disappears at same radius")
check(max(abs(a - b) for a, b in zip(moving_p, moving_new)) == epsilon, "paired displacement bound")
check(d_k(moving_new, F(1), 2) - d_k(moving_p, F(1), 2) == epsilon, "distance stability still holds")
region_distance = min(abs(F(1) - lo) for lo, _ in new_region)
check(region_distance == length - 1, "finite arbitrary spatial separation after disappearance")

# More strongly: the SAME living alpha component is contractible before and
# after, yet its raw geometric realization jumps at an equal-date edge/face.
triangle = tuple(tuple(map(F, p)) for p in ((-4, 0), (4, 0), (1, 2)))
centre, cut = circle(triangle)
check(centre == (F(0), F(-11, 4)) and cut == F(377, 16), "rational constrained circumcircle")
edges, face, powers = triangle_alpha(triangle)
check(powers[(0, 1)] < 0 and powers[(0, 2)] > 0 and powers[(1, 2)] > 0, "obtuse base is not Gabriel, short edges are")
check(edges[(0, 1)] == face and edges[(0, 2)] == F(29, 4) and edges[(1, 2)] == F(13, 4), "exact alpha edge and face dates")
delta = F(1, 100)
old_triangle = tuple(tuple((1 - delta) * x for x in p) for p in triangle)
new_triangle = tuple(tuple((1 + delta) * x for x in p) for p in triangle)
old_edges, old_face, _ = triangle_alpha(old_triangle)
new_edges, new_face, _ = triangle_alpha(new_triangle)
active_old = [ij for ij, date in old_edges.items() if date <= cut]
active_new = [ij for ij, date in new_edges.items() if date <= cut]
check(len(active_old) == 3 and old_face < cut, "old alpha component is filled triangle")
check(active_new == [(0, 2), (1, 2)] and new_face > cut, "new alpha component is two-edge V")
check(len(active_old) - 3 + 1 - 1 == 0 and len(active_new) - 3 + 1 == 0, "both connected complexes have H1 zero")
check(old_edges[(0, 1)] == old_face and new_edges[(0, 1)] == new_face, "free base/face pair has same exact birth")
paired_displacement2 = max(norm2(sub(a, b)) for a, b in zip(old_triangle, new_triangle))
check(paired_displacement2 == (8 * delta) ** 2, "maximum paired displacement is eight delta")
origin = (F(0), F(0))
check(tuple((x + y) / 2 for x, y in zip(old_triangle[0], old_triangle[1])) == origin, "origin belongs to old filled triangle")
distances = [segment_distance2(origin, new_triangle[i], new_triangle[j]) for i, j in active_new]
representative_distance2 = min(d for d, _ in distances)
check(representative_distance2 == (1 + delta) ** 2 * F(64, 29), "nonvanishing Hausdorff lower bound to new V")
check(distances[0][1] == F(20, 29), "base-origin projection parameter")

result = {
    "status": "conforme", "checks": checks,
    "scope": "Independent rational finite cases; no native code, cloud, performance, or general mosaic implementation.",
    "outlier_birth": {"P": birth_p, "added": [F(1)], "k": 2, "radius": radius, "before": birth_before, "after": birth_after},
    "outlier_bridge": {"P": bridge_p, "added": [F(3)], "k": 2, "radius": radius, "before": bridge_before, "after": bridge_after},
    "quantification_collision": {"P": quant_p, "labelled_quantized": quant_labels, "unique_quantized": quant_unique, "epsilon": epsilon_quant,
                               "old_component": omega_axis(quant_p, 2, quant_r), "unique_region_at_r_plus_epsilon": omega_axis(quant_unique, 2, quant_r + epsilon_quant)},
    "subsampling": {"P": quant_p, "sample": sample, "support_hausdorff": net_error, "sample_region_at_r_plus_net_error": omega_axis(sample, 2, quant_r + net_error)},
    "same_cut_component_loss": {"epsilon": epsilon, "P": moving_p, "P_prime": moving_new, "before": old_region, "after": new_region,
                                "region_hausdorff_lower_bound": region_distance, "free_parameter_L": length},
    "same_component_alpha_jump": {"P": triangle, "level_squared": cut, "delta": delta, "old_scale": 1 - delta, "new_scale": 1 + delta,
                                  "old_active_edges": active_old, "new_active_edges": active_new, "old_face_date": old_face, "new_face_date": new_face,
                                  "h0_before_after": [1, 1], "h1_before_after": [0, 0], "paired_displacement_squared": paired_displacement2,
                                  "representative_hausdorff_lower_bound_squared": representative_distance2,
                                  "lower_bound_squared_over_displacement_squared": representative_distance2 / paired_displacement2,
                                  "general_formula": "lower bound = (1+delta)*8/sqrt(29), paired displacement = 8*delta; 0<delta<1/10 suffices"}
}
print(json.dumps(encode(result), ensure_ascii=False, sort_keys=True, indent=2))
