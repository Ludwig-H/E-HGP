#!/usr/bin/env python3
"""Bounded integer checks for a PROPOSED tangent-shell shortcut.

This is neither a census/collector implementation nor a benchmark. Exact
continuous maxima are checked against an independent half-grid dot-product
oracle; the half-grid contains every coordinatewise quadratic maximizer.
Only two midpoint splits and a first DFS block are checked for the fixtures.
No stream, queue, callback, memory residence or performance is qualified.
"""
from __future__ import annotations

import hashlib
import itertools
import json
from pathlib import Path


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def box_of(points):
    require(bool(points), 'empty fixture box')
    return (tuple(min(p[j] for p in points) for j in range(3)),
            tuple(max(p[j] for p in points) for j in range(3)))


def dot_power4(a, b, z_twice):
    # Direct 4H=(2z-2a).(2b-2z), independent of center/radius evaluation.
    return sum((z_twice[j] - 2 * a[j]) * (2 * b[j] - z_twice[j]) for j in range(3))


def maximum4(a, b, box):
    low, high = box
    center_twice = tuple(a[j] + b[j] for j in range(3))
    candidate_twice = tuple(min(2 * high[j], max(2 * low[j], center_twice[j]))
                            for j in range(3))
    diameter_squared = sum((b[j] - a[j]) ** 2 for j in range(3))
    value = diameter_squared - sum((candidate_twice[j] - center_twice[j]) ** 2
                                   for j in range(3))
    return value, candidate_twice


def check_box(a, b, box, counters):
    low, high = box
    require(a != b and all(0 <= low[j] <= high[j] <= 65535 for j in range(3)),
            'invalid bounded box fixture')
    require(all(0 <= x <= 65535 for p in (a, b) for x in p), 'non-u16 support')
    maximum, candidate = maximum4(a, b, box)
    best, maximizers = None, []
    for z_twice in itertools.product(*(range(2 * low[j], 2 * high[j] + 1) for j in range(3))):
        value = dot_power4(a, b, z_twice)
        counters['half_grid_evaluations'] += 1
        if best is None or value > best:
            best, maximizers = value, [z_twice]
        elif value == best:
            maximizers.append(z_twice)
    require(best == maximum and maximizers == [candidate], 'clamped continuous maximum disagrees')
    require(dot_power4(a, b, candidate) == maximum, 'center and dot-product conventions disagree')
    counters['boxes'] += 1
    counters['zero_max_boxes'] += int(maximum == 0)
    counters['half_integral_maxima'] += int(any(value % 2 for value in candidate))
    if maximum == 0:
        zeros = [z for z in itertools.product(*(range(low[j], high[j] + 1) for j in range(3)))
                 if dot_power4(a, b, tuple(2 * x for x in z)) == 0]
        require(len(zeros) <= 1, 'zero-maximum box has multiple lattice shell positions')


def split_once(points, ids):
    # Independent fixture topology only, not an implementation of the index.
    low, high = box_of([points[i] for i in ids])
    axis = max(range(3), key=lambda j: high[j] - low[j])
    midpoint = (low[axis] + high[axis]) // 2
    left = tuple(i for i in ids if points[i][axis] <= midpoint)
    right = tuple(i for i in ids if points[i][axis] > midpoint)
    require(left and right, 'fixture split is degenerate')
    return axis, midpoint, left, right


def tangent_fixture(present, counters):
    a, b = (0, 0, 0), (10, 0, 0)
    sites = ((8, 4, 0), (9, 4, 0)) if present else ((8, 5, 0), (9, 4, 0))
    points = (a, b) + sites
    axis, midpoint, left, right = split_once(points, tuple(range(4)))
    require((axis, midpoint, left, right) == (0, 5, (0,), (1, 2, 3)), 'first tangent split differs')
    axis2, midpoint2, left2, right2 = split_once(points, right)
    require((axis2, midpoint2, left2, right2) == (1, 2, (1,), (2, 3)),
            'tangent sites are not the promised global subtree')
    box = box_of(sites)
    check_box(a, b, box, counters)
    maximum, candidate = maximum4(a, b, box)
    powers4 = [dot_power4(a, b, tuple(2 * x for x in z)) for z in sites]
    shell_ids = [i + 2 for i, value in enumerate(powers4) if value == 0]
    require(maximum == 0 and candidate == (16, 8, 0), 'tangent maximum changed')
    require(shell_ids == ([2] if present else []), 'tangent site membership changed')
    require(powers4 == ([0, -28] if present else [-36, -28]), 'closed tangent dot products changed')
    return dict(points=points, witness_ids=(2, 3), box=box, maximum4=maximum,
                maximizer_twice=candidate, shell_ids=shell_ids, site_powers4=powers4,
                global_subtree_certified_by_two_splits=True)


def shell_prefix_fixture():
    sphere = tuple((x + 8, y + 8, z + 8)
                   for x, y, z in itertools.product(range(-5, 6), repeat=3)
                   if x*x + y*y + z*z == 25)
    require(len(sphere) == 30 and len(set(sphere)) == 30, 'shell30 fixture changed')
    a, b, interior = (3, 8, 8), (13, 8, 8), (12, 8, 8)
    require(a in sphere and b in sphere and interior not in sphere, 'support/extra point fixture changed')
    require(all(dot_power4(a, b, tuple(2*x for x in p)) == 0 for p in sphere),
            'sphere contains a non-shell site')
    points = sphere + (interior,)
    axis, midpoint, left, right = split_once(points, tuple(range(len(points))))
    require((axis, midpoint) == (0, 8) and len(left) == 21 and 30 in right,
            'late strict point is not after the shell-only first DFS block')
    require(all(dot_power4(a, b, tuple(2*x for x in points[i])) == 0 for i in left),
            'first DFS block is not entirely shell')
    powers4 = [dot_power4(a, b, tuple(2*x for x in p)) for p in points]
    require(sum(value > 0 for value in powers4) == 1 and powers4[-1] == 36,
            'late rejection depth changed')
    capacity, kmax = 8, 1
    require(len(left) > capacity and sum(value > 0 for value in powers4) >= kmax,
            'overflow-before-rejection counterexample is vacuous')
    return dict(sphere_sites=30, admitted_without_extra=True, extra_interior=interior,
                kmax=kmax, shell_capacity=capacity, first_dfs_block_shell_sites=len(left),
                first_dfs_block_strict_sites=0, complete_depth_with_extra=1,
                reject_with_extra=True, scalar_prefix_requires_fallback=True,
                scope='first DFS block proof; no collector or bounded-buffer machine executed')


def selftest():
    counters = dict(boxes=0, half_grid_evaluations=0, zero_max_boxes=0, half_integral_maxima=0)
    for a, b in itertools.permutations(range(5), 2):
        for low in range(5):
            for high in range(low, 5):
                check_box((a, 0, 0), (b, 0, 0), ((low, 0, 0), (high, 0, 0)), counters)
    state = 0xAB1749

    def random_value(bound):
        nonlocal state
        state = (1664525 * state + 1013904223) & 0xFFFFFFFF
        return (state >> 8) % bound

    for _ in range(256):
        a = tuple(random_value(8) for _ in range(3))
        b = tuple(random_value(8) for _ in range(3))
        if a == b:
            b = ((b[0] + 1) % 8, b[1], b[2])
        low = tuple(random_value(4) for _ in range(3))
        high = tuple(low[j] + random_value(3) for j in range(3))
        check_box(a, b, (low, high), counters)
    check_box((0, 0, 0), (65535, 65535, 65535), ((0, 0, 0), (2, 2, 2)), counters)
    check_box((65531, 65532, 65533), (65535, 65534, 65531),
              ((65531, 65531, 65531), (65535, 65535, 65535)), counters)
    present = tangent_fixture(True, counters)
    absent = tangent_fixture(False, counters)
    large = shell_prefix_fixture()
    # Deliberately false local deductions, not mutations of any product binary.
    require(absent['maximum4'] == 0 and not absent['shell_ids'], 'presence mutant not refuted')
    require(present['maximum4'] == 0 and present['shell_ids'], 'outside mutant not refuted')
    peak, location = maximum4((0, 0, 0), (1, 0, 0), ((0, 0, 0), (1, 0, 0)))
    require(peak == 1 and location == (1, 0, 0) and
            dot_power4((0, 0, 0), (1, 0, 0), (0, 0, 0)) != peak, 'rounded-center mutant not refuted')
    require(counters['boxes'] == 560 and counters['half_grid_evaluations'] > 3000 and
            counters['zero_max_boxes'] > 20 and counters['half_integral_maxima'] > 20,
            'maximum checks are vacuous')
    return dict(status='passed', schema='mhgp8_tangent_shell_math_v1',
                scope='bounded_integer_geometry_only_not_census_not_collection_not_benchmark',
                public_status='not_claimed', source_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                checks=counters, tangent_present=present, tangent_absent=absent,
                shell_prefix=large,
                countermodels=['zero_max_implies_site_exists', 'zero_max_means_no_shell',
                               'rounded_center_is_exact', 'capacity8_certifies_full_shell30',
                               'shell_prefix_certifies_final_admission'],
                limitation='capacity C bounds retained shell IDs, not searches for absent maximizers; '
                           'measure lookup work and copied IDs on accepted AND rejected supports')


if __name__ == '__main__':
    print(json.dumps(selftest(), sort_keys=True, allow_nan=False))
