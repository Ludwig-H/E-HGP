#!/usr/bin/env python3
"""Tiny exact grid/unit examples; no native or reference HGP imports."""
from fractions import Fraction as F
from itertools import combinations
import json


def need(ok, message):
    if not ok:
        raise RuntimeError(message)


def show(q):
    q = F(q)
    return str(q.numerator) if q.denominator == 1 else f"{q.numerator}/{q.denominator}"


def nearest_integer(q):
    q = F(q)
    low = q.numerator // q.denominator
    rem = q.numerator - low * q.denominator
    twice = 2 * rem
    return low if twice < q.denominator or (twice == q.denominator and low % 2 == 0) else low + 1


def rho(values, part):
    return (max(values[i] for i in part) - min(values[i] for i in part)) / 2


def gamma(values, k, radius):
    vertices = [p for p in combinations(range(len(values)), k) if rho(values, p) <= radius]
    parent = {p: p for p in vertices}

    def find(p):
        while parent[p] != p:
            p = parent[p]
        return p

    for part in combinations(range(len(values)), k + 1):
        if rho(values, part) > radius:
            continue
        faces = list(combinations(part, k))
        need(all(f in parent for f in faces), "All faces activated before export")
        for f in faces[1:]:
            a, b = find(faces[0]), find(f)
            parent[max(a, b)] = min(a, b)
    return {p: find(p) for p in vertices}


def check_transport(values, step):
    grid = [nearest_integer(x / step) for x in values]
    rounded = [step * q for q in grid]
    epsilon = step / 2  # Stronger bound for these collinear fixtures.
    need(all(abs(x - y) <= epsilon for x, y in zip(values, rounded)), "Rounding displacement")
    levels = {F(0)}
    parts = 0
    for k in range(1, len(values) + 1):
        for p in combinations(range(len(values)), k):
            a, b = rho(values, p), rho(rounded, p)
            need(abs(a - b) <= epsilon, "MEB radius1-Lipschitz")
            need(b * b == step * step * rho([F(q) for q in grid], p) ** 2, "Grid squared-level unit")
            levels.update((a, b))
            parts += 1
    cut_checks = component_checks = composition_checks = vertical_checks = 0
    for old, new in ((values, rounded), (rounded, values)):
        for k in range(1, len(values) + 1):
            for radius in sorted(levels):
                a = gamma(old, k, radius)
                b = gamma(new, k, radius + epsilon)
                c = gamma(old, k, radius + 2 * epsilon)
                need(set(a) <= set(b), "Identity inclusion on labeled Gamma vertices")
                groups = {}
                for p, root in a.items():
                    groups.setdefault(root, []).append(p)
                for group in groups.values():
                    images = {b[p] for p in group}
                    need(len(images) == 1, "A component has a unique image")
                    representative = group[0]
                    need(c[b[representative]] == c[representative], "Composite equals2epsilon inclusion")
                    component_checks += 1
                    composition_checks += 1
                    if k > 1:
                        low_a = gamma(old, k - 1, radius)
                        low_b = gamma(new, k - 1, radius + epsilon)
                        face = representative[:-1]
                        need(low_b[low_a[face]] == low_b[b[representative][:-1]], "Vertical naturality")
                        vertical_checks += 1
                # Coverage is an existence of a labeled active K-part containing i.
                for i in range(len(values)):
                    first_a = min(rho(old, p) for p in combinations(range(len(values)), k) if i in p)
                    first_b = min(rho(new, p) for p in combinations(range(len(values)), k) if i in p)
                    need(abs(first_a - first_b) <= epsilon, "First set-valued coverage date")
                cut_checks += 1
    return {"input": [show(x) for x in values], "h": show(step), "grid": grid,
            "rounded": [show(x) for x in rounded], "line_epsilon": show(epsilon),
            "labeled_parts": parts, "cut_checks": cut_checks,
            "component_checks": component_checks, "composition_checks": composition_checks,
            "vertical_checks": vertical_checks,
            "collision": len(set(grid)) < len(grid)}


def unit_case(step, label):
    factor = 1 / step ** 2
    masses = (2, 3)
    split_beta = F(4)
    split_lambda = 1 / split_beta
    parent = sum(masses) * split_lambda  # Root cluster has lambda_birth0, not lambda(radius0).
    cases = []
    for exit_beta in (F(9, 4), F(1, 4)):
        exit_lambda = 1 / exit_beta
        children = [m * (exit_lambda - split_lambda) for m in masses]
        physical_split_lambda = 1 / (step * step * split_beta)
        physical_exit_lambda = 1 / (step * step * exit_beta)
        physical_parent = sum(masses) * physical_split_lambda
        physical_children = [m * (physical_exit_lambda - physical_split_lambda) for m in masses]
        need(physical_split_lambda == factor * split_lambda and physical_exit_lambda == factor * exit_lambda,
             "lambda_phys=h^-z lambda_grid at z2")
        need(physical_parent == factor * parent, "Parent stability scaling")
        need(physical_children == [factor * s for s in children], "Child stability scaling")
        need((physical_parent >= sum(physical_children)) == (parent >= sum(children)), "EOM comparison invariant")
        cases.append({"exit_beta_grid": show(exit_beta), "exit_beta_phys": show(step * step * exit_beta),
                      "parent_stability_grid": show(parent), "children_sum_grid": show(sum(children)),
                      "eom_parent_wins": parent >= sum(children),
                      "common_stability_factor": show(factor)})
    max_lambda = F(4)
    grid_cap = sum(masses) * max_lambda < 2 ** 1000
    physical_cap = sum(masses) * factor * max_lambda < 2 ** 1000
    return {"h_label": label, "h": show(step), "z": 2, "cases": cases,
            "grid_mass_lambda_ceiling_pass": grid_cap,
            "physical_mass_lambda_ceiling_pass": physical_cap,
            "ceiling": "strict M*lambda_max<2^1000; this exact comparison alone is not the full binary64 domain"}


line = check_transport([F(1, 10), F(4, 5), F(23, 10), F(37, 10)], F(1, 2))
copies = check_transport([F(0), F(1, 4), F(3, 4)], F(1))
need(copies["collision"] and copies["grid"] == [0, 0, 1], "Collision with three labeled copies")
dedup = sorted(set(copies["grid"]))
need(len(gamma([F(q) for q in dedup], 3, F(1))) == 0, "Dropping copies destroys K3 universe")
three_d = (F(1, 2), F(-1, 2), F(1, 2))
rounded_3d = tuple(F(nearest_integer(x)) for x in three_d)
error2 = sum((x - y) ** 2 for x, y in zip(three_d, rounded_3d))
need(error2 == F(3, 4), "sqrt3*h/2 bound attained at h1")
units = [unit_case(F(1, 1000), "1/1000"), unit_case(F(1, 2), "1/2"), unit_case(F(1, 2 ** 498), "2^-498")]
need(all(u["grid_mass_lambda_ceiling_pass"] for u in units), "Grid units within ceiling")
need([u["physical_mass_lambda_ceiling_pass"] for u in units] == [True, True, False], "Ceiling depends on unit")
print(json.dumps({"status": "ok", "arithmetic": "fractions.Fraction", "line_gamma": line,
                  "copy_preserved_gamma": copies, "dedup_k3_vertex_count": 0,
                  "error3d_h1_squared": show(error2), "units": units,
                  "limits": ["Tiny collinear Gamma calculations; no native/reference HGP import or benchmark.",
                             "No stable matching of critical supports, shell incidences, plateaus or hard projection.",
                             "Unit scaling preserves comparisons only for an unchanged tree, masses, z and zero policy.",
                             "Mathematical copies are retained; the current native weighted tower remains unsupported."]},
                 sort_keys=True, separators=(",", ":")))
