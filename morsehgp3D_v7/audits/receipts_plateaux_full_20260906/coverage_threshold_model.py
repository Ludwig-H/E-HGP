"""Exact local coverage thresholds, independently judged by rational MEBs.

Let B have positive radius, strict interior I and complete shell U, p=|I|.
Call a shell mask A strict when the center of B is not in conv(A), and set
h=max |A| over strict masks and h_x=max |A| over strict masks containing x.
Every singleton shell point is strict. For 1 <= K <= |I union U|,

    D_B(K) = {x in U : K > p+h_x} union (I if K > p+h else empty).

Proof: the largest strict subset of I union U containing a shell point x
has p+h_x points, since every interior point may be inserted into any
strict shell mask without bringing the MEB radius up to that of B.
Every smaller positive cardinality occurs by taking subsets containing x.
For an interior point, the corresponding maximum is p+h. Hence the stated
inequalities characterize exactly the points absent from ALL strict
K-facets. At K>|I union U| there is no closed block, so it is not a birth
and its contribution is empty: do not use the complement of an empty
strict-facet family without this domain guard. Radius zero is excluded.

The sets D_B(K) increase with K; x first appears at p+h_x+1. Full local
coverage holds exactly through K=p+min_x h_x, and no strict facet remains
after K=p+h. Since every shell mask of size q_min-1 is strict, h_x is at
least q_min-1, including at the first anchor order p+q_min-1. Absence of a
coverage gap alone proves neither local connectedness nor global inertia.

For computation, mark the positive support masks (of size at most four in
3D), close this boolean table upward, then scan every strict mask and its
set bits. This obtains h and every h_x in O(u*2**u), shared by all K; no
maxima transform on all subsets is needed when only singletons are queried.
The model below computes this table, but judges coverage and each first
missing order from ALL small facets and their exact rational MEB radii.
It does not judge the formula merely by the quotient or its DSU.

For q_min=2 and u>=3, the first anchor order K=p+1 is locally inert: the
normalized singleton graph is the complete graph on U with only antipodal
pairs removed. Those pairs form a matching, so the graph is connected
and covers U; the interior-normalization lemma also includes all of I.
This special case does not generalize to q_min=3. Remaining middle ranks
still require topology: the hexagon and octahedron below have identical
coverage thresholds at every K and respectively six and eight strict
components at K=3. Their components are counted independently because all
strict triples are isolated, with no strict four-facet.

Only ten named clouds of at most six u16 sites are used. This is a bounded
mathematical fixture, not a product implementation, performance result,
catalog-completeness proof, cloud replay, or replacement for the DSU.
There is no C++, subprocess, cloud access, or assert-based gate.
"""

from __future__ import annotations

from fractions import Fraction as Q
import hashlib
from itertools import combinations
import json
from pathlib import Path
from typing import Any

from plateau_model import Model, cover

HERE = Path(__file__).resolve().parent
MODEL_SHA = "8afb5663c1fa0384d5ae392294cc03853676a89fcb18f858cd7d764ed9b1b93a"
ORACLE_SHA = "ad6c0d6c041ff788180a400f6ba2ad2b1546f8607e8f2c91fefca9133a8e7f2b"


def need(ok: bool, reason: str) -> None:
    if not ok:
        raise ValueError(reason)


def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def thresholds(ball: dict[str, Any]) -> dict[str, Any]:
    """Produce only shared local mask thresholds, without components."""
    shell = sorted(ball['shell'])
    u = len(shell)
    need(ball['radius'] > 0 and u >= 2, 'positive_radius_threshold_domain')
    positions = {point: bit for bit, point in enumerate(shell)}
    contains = [False] * (1 << u)
    for basis in ball['bases']:
        need(basis <= ball['shell'] and 2 <= len(basis) <= 4,
             'positive_shell_supports_in_three_dimensions')
        contains[sum(1 << positions[point] for point in basis)] = True
    closure_updates = 0
    for mask in range(1 << u):
        if contains[mask]:
            for bit in range(u):
                contains[mask | (1 << bit)] = True
                closure_updates += 1
    h, h_x = 0, [-1] * u
    witnesses = [0] * u
    singleton_updates = 0
    for mask, nonstrict in enumerate(contains):
        if nonstrict:
            continue
        size = mask.bit_count()
        h = max(h, size)
        remaining = mask
        while remaining:
            bit_mask = remaining & -remaining
            bit = bit_mask.bit_length() - 1
            if size > h_x[bit]:
                h_x[bit] = size
                witnesses[bit] = mask
            singleton_updates += 1
            remaining ^= bit_mask
    need(all(value >= 1 for value in h_x) and h < u,
         'strict_singletons_but_nonstrict_whole_shell')
    need(closure_updates <= u * (1 << u) and
         singleton_updates <= u * (1 << u), 'bounded_shared_mask_passes')
    return dict(shell=shell, h=h, h_x=h_x, witnesses=witnesses,
                contains=contains, masks=1 << u,
                closure_updates=closure_updates,
                singleton_updates=singleton_updates)


def contribution(ball: dict[str, Any], table: dict[str, Any], k: int
                 ) -> tuple[str, frozenset[int]]:
    need(k >= 1, 'positive_order')
    if k > len(ball['closed']):
        return 'absent_block', frozenset()
    p = len(ball['interior'])
    gap = frozenset(point for point, limit in zip(table['shell'], table['h_x'])
                    if k > p + limit)
    if k > p + table['h']:
        gap |= ball['interior']
    return 'closed_block', gap


def inspect(points: list[tuple[int, int, int]], expected_p: int,
            expected_q: int, expected_radius: Q) -> tuple[Model, dict[str, Any]]:
    need(2 <= len(points) <= 6 and len(set(points)) == len(points),
         'bounded_distinct_fixture')
    need(all(0 <= coordinate <= 65535 for point in points for coordinate in point),
         'u16_fixture')
    model = Model(points)
    ball = model.balls[model.key[model.ids]]
    closed, shell = sorted(ball['closed']), sorted(ball['shell'])
    p, u, q_min = len(ball['interior']), len(shell), min(map(len, ball['bases']))
    need(closed == list(model.ids) and p == expected_p and q_min == expected_q and
         ball['radius'] == expected_radius, 'named_complete_census_and_radius')
    table = thresholds(ball)

    # This judge uses rational MEB values for every shell subset, not the
    # upward support table that produced h and h_x.
    shell_strict = [facet for facet, radius in model.level.items()
                    if set(facet) <= ball['shell'] and radius < ball['radius']]
    exact_h = max(map(len, shell_strict))
    exact_h_x = [max(len(facet) for facet in shell_strict if point in facet)
                 for point in shell]
    need(table['h'] == exact_h and table['h_x'] == exact_h_x,
         'shared_mask_maxima_equal_independent_shell_MEB_maxima')
    need(all(value >= q_min - 1 for value in table['h_x']),
         'first_anchor_order_always_has_full_local_coverage')
    mask_checks = 0
    for mask, contains in enumerate(table['contains']):
        if not mask:
            need(not contains, 'empty_shell_mask_is_strict')
            continue
        facet = tuple(point for bit, point in enumerate(shell) if mask & (1 << bit))
        need(contains == (model.level[facet] == ball['radius']),
             'every_shell_mask_checked_against_exact_MEB')
        mask_checks += 1

    rows, all_strict, facet_checks = [], [], 0
    for k in range(1, len(closed) + 1):
        facets = list(combinations(closed, k))
        strict = [facet for facet in facets if model.level[facet] < ball['radius']]
        all_strict.extend(strict)
        exact_coverage = frozenset(point for facet in strict for point in facet)
        exact_gap = ball['closed'] - exact_coverage
        state, predicted = contribution(ball, table, k)
        need(state == 'closed_block' and predicted == exact_gap,
             'threshold_gap_equals_all_strict_full_facets')
        need((not strict) == (k > p + table['h']), 'exact_no_strict_facet_threshold')
        need((not predicted) == (k <= p + min(table['h_x'])),
             'exact_full_coverage_prefix')
        if rows:
            need(set(rows[-1]['missing']) <= predicted, 'nested_missing_sets')
        rows.append(dict(k=k, state=state, all_facets=len(facets),
                         strict_facets=len(strict), missing=sorted(predicted),
                         strict_coverage=sorted(exact_coverage)))
        facet_checks += len(facets)

    # The maxima over FULL facets independently verify the first missing
    # order of each point, including all interior points.
    first_missing = []
    for point in closed:
        maximum = max(len(facet) for facet in all_strict if point in facet)
        limit = p + (table['h_x'][shell.index(point)]
                     if point in ball['shell'] else table['h'])
        need(maximum == limit and 1 <= limit < len(closed),
             'maximum_full_strict_facet_containing_point')
        need(point not in contribution(ball, table, limit)[1] and
             point in contribution(ball, table, limit + 1)[1],
             'each_point_enters_at_its_exact_first_missing_order')
        first_missing.append(dict(point=point, first_missing_order=limit + 1))

    state, missing = contribution(ball, table, len(closed) + 1)
    need(state == 'absent_block' and not missing and
         not list(combinations(closed, len(closed) + 1)),
         'empty_rank_is_not_birth_and_has_no_contribution')
    rows.append(dict(k=len(closed) + 1, state=state, all_facets=0,
                     strict_facets=0, missing=[]))

    q2_first_order = None
    if q_min == 2 and u >= 3:
        k = p + 1
        # Whole input is exactly I union U. The older Gamma oracle judges
        # the actual full strict graph, independently of the normalization.
        strict_components = model.gamma(k, ball['radius'], False)
        need(len(strict_components) == 1 and
             cover(strict_components[0]) == ball['closed'],
             'q2_extra_shell_first_anchor_order_is_locally_inert')
        nonstrict_pairs = [pair for pair in combinations(shell, 2)
                           if model.level[pair] == ball['radius']]
        need(all(sum(point in pair for pair in nonstrict_pairs) <= 1
                 for point in shell), 'antipodal_pairs_form_a_matching')
        q2_first_order = dict(k=k, components=1, coverage=closed,
                              nonstrict_pairs=[list(pair) for pair in nonstrict_pairs])

    witnesses = []
    for point, mask in zip(shell, table['witnesses']):
        facet = tuple(value for bit, value in enumerate(shell) if mask & (1 << bit))
        need(point in facet and len(facet) == table['h_x'][shell.index(point)] and
             model.level[facet] < ball['radius'], 'maximal_strict_shell_witness')
        witnesses.append(dict(point=point, shell_facet=list(facet),
                              radius_squared=str(model.level[facet])))
    return model, dict(points=points, radius_squared=str(ball['radius']),
                       ball_key=list(model.key[model.ids]), interior=sorted(ball['interior']),
                       shell=shell, p=p, u=u, q_min=q_min,
                       h=table['h'], h_x=table['h_x'],
                       first_missing_by_point=first_missing, witnesses=witnesses,
                       ranks=rows, mask_checks=mask_checks, facet_checks=facet_checks,
                       shared_passes={key: table[key] for key in
                                      ['masks', 'closure_updates', 'singleton_updates']},
                       q2_first_anchor_order=q2_first_order)


def isolated_components(model: Model, k: int) -> list[list[int]]:
    radius = model.level[model.ids]
    vertices = [list(facet) for facet in combinations(model.ids, k)
                if model.level[facet] < radius]
    need(bool(vertices) and
         all(model.level[coface] == radius for coface in combinations(model.ids, k + 1)),
         'strict_vertices_are_nonempty_and_isolated_without_strict_cofaces')
    return vertices


def main() -> None:
    need(sha(HERE / 'plateau_model.py') == MODEL_SHA, 'pinned_previous_model')
    need(sha(HERE.parent / 'meb_rational_oracle_20260905.py') == ORACLE_SHA,
         'pinned_previous_rational_oracle')
    growth = [(1, 8, 0), (5, 10, 0), (9, 8, 0), (5, 0, 0)]
    tetra = [(2, 2, 2), (2, 0, 0), (0, 2, 0), (0, 0, 2)]
    cases = {
        'regular_diameter': ([(0, 0, 0), (2, 0, 0)], 0, 2, Q(1)),
        'growth_ABCZ': (growth, 0, 2, Q(25)),
        'growth_with_interior': (growth + [(5, 5, 0)], 1, 2, Q(25)),
        'square': ([(0, 0, 0), (2, 0, 0), (2, 2, 0), (0, 2, 0)], 0, 2, Q(2)),
        'positive_triangle': ([(0, 0, 0), (4, 0, 0), (2, 3, 0)], 0, 3, Q(169, 36)),
        'positive_tetrahedron': (tetra, 0, 4, Q(3)),
        'tetrahedron_with_interior': (tetra + [(1, 1, 1)], 1, 4, Q(3)),
        'unequal_point_thresholds': ([(5, 5, 0), (5, 5, 10), (8, 5, 9),
                                       (5, 8, 9), (2, 5, 9), (5, 2, 9)], 0, 2, Q(25)),
        'antipodal_hexagon': ([(10, 5, 5), (8, 9, 5), (2, 9, 5),
                                (0, 5, 5), (2, 1, 5), (8, 1, 5)], 0, 2, Q(25)),
        'octahedron': ([(10, 5, 5), (0, 5, 5), (5, 10, 5),
                          (5, 0, 5), (5, 5, 10), (5, 5, 0)], 0, 2, Q(25)),
    }
    models, results = {}, {}
    for name, parameters in cases.items():
        models[name], results[name] = inspect(*parameters)

    gap = lambda name, k: results[name]['ranks'][k - 1]['missing']
    regular_first = models['regular_diameter'].gamma(1, Q(1), False)
    need(len(regular_first) == 2 and gap('regular_diameter', 1) == [],
         'q2_first_anchor_shortcut_requires_at_least_three_shell_sites')
    need(gap('growth_ABCZ', 3) == [3] and
         gap('growth_with_interior', 4) == [3] and
         gap('growth_with_interior', 5) == list(range(5)),
         'ABCZ_growth_and_interior_shift')
    need(gap('square', 3) == list(range(4)) and
         results['square']['ranks'][2]['strict_facets'] == 0,
         'square_birth_has_more_than_K_points')
    square_components = isolated_components(models['square'], 2)
    need(len(square_components) == 4 and gap('square', 2) == [],
         'empty_gap_does_not_imply_one_local_component')

    unequal = results['unequal_point_thresholds']
    need(unequal['h'] == 5 and unequal['h_x'] == [3, 5, 5, 5, 5, 5] and
         gap('unequal_point_thresholds', 4) == [0] and
         gap('unequal_point_thresholds', 5) == [0] and
         gap('unequal_point_thresholds', 6) == list(range(6)),
         'asymmetric_shell_has_genuinely_distinct_point_thresholds')
    mutant_h = [point for point in unequal['shell'] if 4 > unequal['h']]
    mutant_h_minus_one = [point for point in unequal['shell'] if 5 > unequal['h'] - 1]
    need(mutant_h != gap('unequal_point_thresholds', 4) and
         mutant_h_minus_one != gap('unequal_point_thresholds', 5),
         'replacing_per_point_thresholds_by_one_global_threshold_is_refuted')
    mutant_boundary = [point for point, limit in zip(unequal['shell'], unequal['h_x'])
                       if 3 >= limit]
    need(mutant_boundary != gap('unequal_point_thresholds', 3),
         'non_strict_threshold_comparison_is_refuted')

    first = results['antipodal_hexagon']
    second = results['octahedron']
    signature = lambda row: [row[key] for key in ['p', 'u', 'q_min', 'h', 'h_x']]
    need(signature(first) == signature(second) and
         [row['missing'] for row in first['ranks']] ==
         [row['missing'] for row in second['ranks']], 'identical_coverage_signatures')
    hexagon_components = isolated_components(models['antipodal_hexagon'], 3)
    octahedron_components = isolated_components(models['octahedron'], 3)
    need(len(hexagon_components) == 6 and len(octahedron_components) == 8,
         'coverage_thresholds_do_not_determine_local_components')
    need(all(gap(name, len(points)) == list(range(len(points)))
             for name, (points, _, _, _) in cases.items()), 'every_top_rank_birth_nonvacuum')
    q2_checks = sum(row['q2_first_anchor_order'] is not None for row in results.values())
    need(q2_checks == 6 and len(results) == 10 and
         sum(row['p'] > 0 for row in results.values()) == 2,
         'named_cases_interiors_and_q2_special_case_nonvacuum')
    output = dict(schema='mhgp7-independent-local-coverage-thresholds-v1', status='passed',
                  public_status='not_claimed',
                  scope='ten_bounded_exact_clouds_not_product_or_50k_records',
                  source_sha256={'model': MODEL_SHA, 'rational_oracle': ORACLE_SHA,
                                 'this_script': sha(Path(__file__).resolve())},
                  runs=results, clouds=len(results), maximum_points=6,
                  nonempty_rank_checks=sum(len(row['ranks']) - 1 for row in results.values()),
                  empty_rank_checks=len(results),
                  point_first_missing_order_checks=sum(row['p'] + row['u'] for row in results.values()),
                  exact_full_facet_checks=sum(row['facet_checks'] for row in results.values()),
                  exact_shell_mask_checks=sum(row['mask_checks'] for row in results.values()),
                  q2_first_anchor_order_checks=q2_checks,
                  q2_first_anchor_order_gamma_cuts=q2_checks,
                  regular_q2_first_anchor_components=len(regular_first),
                  isolated_component_certificates={
                      'square_K2': square_components,
                      'antipodal_hexagon_K3': hexagon_components,
                      'octahedron_K3': octahedron_components},
                  targeted_mutants_refuted=['replace_h_x_by_h', 'replace_h_x_by_h_minus_one',
                      'use_greater_equal_instead_of_greater', 'empty_gap_implies_local_inertia',
                      'coverage_thresholds_determine_local_components',
                      'q2_first_anchor_is_inert_with_two_shell_sites',
                      'empty_rank_is_a_birth_with_full_contribution'],
                  thresholds_producer_constructs_components=False,
                  component_DSU_eliminated_for_all_ranks=False,
                  support_table_is_judged_by_all_exact_MEB_facets=True,
                  shared_maxima_pass='scan_strict_masks_and_each_set_bit',
                  mathematical_shared_mask_bound='O(u*2**u)',
                  engine_executed=False, GCP_used=False)
    print(json.dumps(output, sort_keys=True, separators=(',', ':')))


if __name__ == '__main__':
    main()
