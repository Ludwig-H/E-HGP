"""Three exact global parent decisions for the pinned 50k extra-shell balls.

At ball 174406, K=5, an exterior point z=45617 gives the path
F1 -> H=I union {V,z} -> F2. Both elementary cofaces have MEB squared
radius strictly below the original ball. Thus the two local components
already have the same global parent, whose coverage includes the whole
local closed census. This block causes no public fusion or coverage gain.

At 254569 (K=2) and 996863 (K=6), one representative T is isolated in the
GLOBAL strict graph. For every point z outside T, a strict coface T+z
requires every distance d(z,x)^2, x in T, to be strictly less than 4r^2.
A complete scan excludes all but zero and four candidates respectively.
The remaining four exact rational MEBs are all larger than r^2. Therefore
there is no strict coface incident to T. This also proves isolation in
the complete Gamma graph: any first edge T -> G contains T+z for some
z in G minus T, and MEB radii are monotone under inclusion. The other
representative is already strict and distinct. Their common ball joins
them at its closed level, so at least two global parents merge there.
This does not determine the complete atomic plateau node or its arity.

At 254569, z=4912 attains distance equality 4r^2 and its coface has MEB
exactly r^2: the isolation certificate applies only to the open cut.
At 996863, passing the distance filter is insufficient for a strict
connection; all four survivors require exact geometric rejection.

Only the independently regenerated, digest-bound input is scanned. Exact
MEB enumeration is bounded to subsets of at most seven sites, using the
pinned earlier rational audit oracle. No global component enumeration,
product helper, engine, compilation, cloud access or assert-based gate.
"""

from __future__ import annotations

import copy
from fractions import Fraction as Q
import hashlib
from itertools import combinations
import json
from pathlib import Path
import struct
from typing import Any, Callable

from real_shell_census import CAPTURE, INPUT_SHA, RAW_SHA, labelled_digest, regenerate, scan
from meb_rational_oracle_20260905 import circumball, expected_meb, power

HERE = Path(__file__).resolve().parent
PINS = {
    'real_shell_census.py': 'd935ec655563c75c29ad2bd0de67802c59000d0d359c09c1837fb4729b9089b0',
    'shell_diagnostic.py': '1032cc5ef86da67e0064a56f68b35f0395181c16192ce92d863a1339254f426a',
    '../meb_rational_oracle_20260905.py': 'ad6c0d6c041ff788180a400f6ba2ad2b1546f8607e8f2c91fefca9133a8e7f2b',
}
Point = tuple[int, int, int]


def need(ok: bool, reason: str) -> None:
    if not ok:
        raise ValueError(reason)


def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def rejected(name: str, reason: str, action: Callable[[], Any]) -> dict[str, str]:
    try:
        action()
    except ValueError as error:
        need(str(error) == reason, 'mutant_wrong_reason:' + name)
        return dict(mutant=name, rejection=reason)
    raise ValueError('mutant_survived:' + name)


def distance_squared(points: list[Point], first: int, second: int) -> int:
    return sum((x - y) ** 2 for x, y in zip(points[first], points[second]))


def bind_sites(record: dict[str, Any], points: list[Point]) -> None:
    need(record['n'] == len(points) and record['input_digest'] == INPUT_SHA,
         'input_identity')
    for population in ('interior', 'shell'):
        for row in record[population]:
            point_id = row['point_id']
            need(type(point_id) is int and 0 <= point_id < len(points), 'point_id_domain')
            need(tuple(row['xyz']) == points[point_id], 'point_coordinate_binding')


def distance_scan(points: list[Point], population: tuple[int, ...], radius: Q
                  ) -> dict[str, Any]:
    """Necessary filter only; retain equality as nonstrict, with exact ints."""
    threshold = 4 * radius
    need(threshold.denominator == 1 and threshold > 0, 'integer_diameter_threshold')
    bound = threshold.numerator
    excluded = set(population)
    survivors, minimum_ids = [], []
    minimum: int | None = None
    digest = hashlib.sha256(b'audit-exact-max-distance-full-scan-v1\0')
    for point_id in range(len(points)):
        if point_id in excluded:
            continue
        distances = [(distance_squared(points, point_id, other), other)
                     for other in population]
        maximum, witness = max(distances)
        digest.update(struct.pack('<QQQ', point_id, maximum, witness))
        if minimum is None or maximum < minimum:
            minimum, minimum_ids = maximum, [point_id]
        elif maximum == minimum:
            minimum_ids.append(point_id)
        if maximum < bound:
            survivors.append(point_id)
    return dict(population=list(population), threshold_four_radius_squared=bound,
                points_scanned=len(points) - len(excluded), survivors=survivors,
                pruned_count=len(points) - len(excluded) - len(survivors),
                minimum_maximum_squared_distance=minimum,
                minimum_point_ids=minimum_ids, scan_sha256=digest.hexdigest())


class Geometry:
    """Finite exact MEB witnesses; never enumerate a global Gamma graph."""

    def __init__(self, points: list[Point]) -> None:
        self.points = points
        self.cache: dict[tuple[int, ...], dict[str, Any]] = {}
        self.support_tests = 0

    def meb(self, point_ids: tuple[int, ...]) -> dict[str, Any]:
        point_ids = tuple(sorted(point_ids))
        need(2 <= len(point_ids) <= 7 and len(set(point_ids)) == len(point_ids),
             'bounded_distinct_geometry')
        need(all(type(i) is int and 0 <= i < len(self.points) for i in point_ids),
             'geometry_point_id_domain')
        if point_ids in self.cache:
            return self.cache[point_ids]
        need(len(self.cache) < 32, 'bounded_rational_MEB_call_budget')
        ball = expected_meb([self.points[i] for i in point_ids])
        self.support_tests += ball['charged']
        support = [point_ids[i] for i in ball['support']]
        witness = circumball([self.points[i] for i in support])
        need(witness is not None and witness['positive'] and
             witness['key'] == ball['key'] and witness['radius'] == ball['radius'],
             'positive_support_reconstructs_exact_ball')
        powers = [power(witness, self.points[i]) for i in point_ids]
        need(all(value <= 0 for value in powers) and
             all(powers[point_ids.index(i)] == 0 for i in support),
             'positive_support_and_full_containment_certify_MEB')
        result = dict(point_ids=list(point_ids), support_point_ids=support,
                      positive_support=True, radius_squared=str(ball['radius']),
                      ball_key=[str(value) for value in ball['key']],
                      center=[str(value) for value in ball['center']],
                      powers=[str(value) for value in powers],
                      points=[list(self.points[i]) for i in point_ids])
        self.cache[point_ids] = result
        return result


def certify_isolation(scan_result: dict[str, Any], witnesses: dict[int, dict[str, Any]],
                      radius: Q) -> None:
    need(set(witnesses) == set(scan_result['survivors']),
         'survivor_certificate_complete')
    population = set(scan_result['population'])
    for point_id, certificate in witnesses.items():
        need(set(certificate['point_ids']) == population | {point_id},
             'coface_witness_identity')
        need(Q(certificate['radius_squared']) >= radius,
             'strict_coface_refutes_isolation')


def main() -> None:
    for name, pin in PINS.items():
        need(sha(HERE / name) == pin, 'source_pin:' + name)
    raw = (CAPTURE / 'run_r3/n50000_k10.stderr').read_bytes()
    need(hashlib.sha256(raw).hexdigest() == RAW_SHA, 'raw_capture_pin')
    lines = raw.decode().splitlines()
    need(all(line.startswith('{') for line in lines[:4]) and
         not any(line.lstrip().startswith('{') for line in lines[4:]),
         'exact_four_record_prefix')
    raw_records = [json.loads(line) for line in lines[:4]]
    need([record['ball_index'] for record in raw_records] ==
         [174406, 254569, 996863, 1251653], 'fixed_raw_record_identities')
    points, attempts = regenerate()
    need(len(points) == 50000 and attempts == 50000 and labelled_digest(points) == INPUT_SHA,
         'independently_regenerated_whole_input_binding')
    geometry = Geometry(points)
    cases, internals = {}, {}
    expected = {
        174406: (3, 5, (36860, 46707), 42779, Q(14352441, 4)),
        254569: (0, 2, (4912, 34292), 32276, Q(2904043, 2)),
        996863: (4, 6, (25389, 40661), 43571, Q(6675549, 2)),
    }
    for record in raw_records[:3]:
        index = record['ball_index']
        bind_sites(record, points)
        census = scan(record, points)
        for population in ('interior', 'shell'):
            need(sorted(row['point_id'] for row in record[population]) ==
                 census[population + '_point_ids'], 'complete_' + population + '_census')
        interior = set(census['interior_point_ids'])
        shell = set(census['shell_point_ids'])
        closed = interior | shell
        p, k, diameter, v, radius = expected[index]
        need(len(interior) == p and len(shell) == 3 and set(diameter) | {v} == shell and
             distance_squared(points, *diameter) == 4 * radius,
             'named_positive_diameter_and_extra_shell')
        antipodal = [pair for pair in combinations(sorted(shell), 2)
                     if distance_squared(points, *pair) == 4 * radius]
        need(antipodal == [diameter], 'unique_positive_shell_support')
        facets = [tuple(sorted(interior | {endpoint, v})) for endpoint in diameter]
        certificates = [geometry.meb(facet) for facet in facets]
        closed_certificate = geometry.meb(tuple(sorted(closed)))
        need(all(len(facet) == k for facet in facets) and
             all(Q(certificate['radius_squared']) < radius for certificate in certificates),
             'both_representatives_are_strict_vertices')
        raw_key = record['ball_key']
        need(closed_certificate['radius_squared'] == str(radius) and
             closed_certificate['ball_key'] ==
             [raw_key['a'], *raw_key['b'], raw_key['c']], 'closed_union_has_original_exact_BallKey')
        cases[index] = dict(ball_index=index, k=k, original_radius_squared=str(radius),
                            interior_point_ids=sorted(interior), shell_point_ids=sorted(shell),
                            complete_census=census, facet_certificates=certificates,
                            closed_union_certificate=closed_certificate)
        internals[index] = (facets, radius, closed, interior, v)

    facets, radius, closed, interior, v = internals[174406]
    bridge_scan = distance_scan(points, tuple(sorted(closed)), radius)
    need(bridge_scan['survivors'] == [12140, 21745, 40641, 45617],
         'bounded_full_scan_bridge_candidates')
    bridge_trials = []
    for z in bridge_scan['survivors']:
        cofaces = [geometry.meb(tuple(sorted(set(facet) | {z}))) for facet in facets]
        bridge_trials.append(dict(point_id=z, cofaces=cofaces,
                                 both_strict=all(Q(row['radius_squared']) < radius for row in cofaces)))
    successes = [row for row in bridge_trials if row['both_strict']]
    need([row['point_id'] for row in successes] == [45617], 'unique_one_point_bridge_among_candidates')
    z = 45617
    need(points[z] == (57873, 31035, 50862) and z not in closed, 'named_exterior_bridge_point')
    h = tuple(sorted(interior | {v, z}))
    need(len(h) == 5 and all(len(set(h) | set(facet)) == 6 for facet in facets),
         'two_elementary_Gamma_edges')
    middle = geometry.meb(h)
    need(Q(middle['radius_squared']) < radius, 'bridge_middle_is_a_strict_vertex')
    cases[174406].update(verdict='same_global_parent', proof='two_strict_elementary_cofaces',
                        distance_scan=bridge_scan, bridge_trials=bridge_trials,
                        bridge_point_id=z, bridge_point_xyz=list(points[z]),
                        middle_facet_certificate=middle,
                        full_local_coverage_already_in_parent=True,
                        local_block_has_public_fusion_or_coverage_gain=False)

    isolation_scans, isolation_certificates = {}, {}
    for index, chosen in ((254569, 1), (996863, 0)):
        facets, radius, _, _, _ = internals[index]
        scan_result = distance_scan(points, facets[chosen], radius)
        expected_survivors = [] if index == 254569 else [19323, 21608, 34650, 38604]
        need(scan_result['survivors'] == expected_survivors, 'fixed_isolation_scan_survivors')
        witnesses = {z: geometry.meb(tuple(sorted(set(facets[chosen]) | {z})))
                     for z in scan_result['survivors']}
        certify_isolation(scan_result, witnesses, radius)
        if index == 996863:
            need(all(Q(row['radius_squared']) > radius for row in witnesses.values()),
                 'four_distance_survivors_rejected_by_strict_MEB_inequalities')
        cases[index].update(verdict='distinct_global_parents', proof='isolated_strict_facet',
                            isolated_facet_index=chosen, isolated_facet_point_ids=list(facets[chosen]),
                            distance_scan=scan_result, survivor_coface_certificates=witnesses,
                            strict_parent_is_singleton_facet=True,
                            open_cut_only=True, at_least_two_parents_merge_at_closed_level=True,
                            complete_atomic_node_arity_claimed=False)
        isolation_scans[index], isolation_certificates[index] = scan_result, witnesses

    equality_scan = isolation_scans[254569]
    equality_radius = internals[254569][1]
    need(equality_scan['minimum_point_ids'] == [4912] and
         equality_scan['minimum_maximum_squared_distance'] == 5808086 == 4 * equality_radius,
         'unique_diameter_equality_witness')
    equality_certificate = cases[254569]['closed_union_certificate']
    need(Q(equality_certificate['radius_squared']) == equality_radius,
         'open_isolation_does_not_persist_at_closed_cut')
    cases[254569]['equality_witness'] = dict(point_id=4912,
        maximum_squared_distance=5808086, open_connection=False, closed_connection=True)

    # These are certificate rejections with exact reasons, not assert gates.
    mutant_results = [rejected('claim_isolation_from_distance_filter_only',
        'survivor_certificate_complete', lambda: certify_isolation(
            isolation_scans[996863], {}, internals[996863][1]))]
    facets, radius, _, _, _ = internals[174406]
    false_scan = distance_scan(points, facets[1], radius)
    need(false_scan['survivors'] == [12140, 15110, 21745, 36581, 40641, 45617],
         'false_isolation_case_has_real_strict_survivor')
    false_witnesses = {z: geometry.meb(tuple(sorted(set(facets[1]) | {z})))
                       for z in false_scan['survivors']}
    mutant_results.append(rejected('claim_isolation_despite_a_strict_coface',
        'strict_coface_refutes_isolation', lambda: certify_isolation(false_scan, false_witnesses, radius)))
    wrong_id = copy.deepcopy(raw_records[0])
    wrong_id['shell'][0]['point_id'] = 0
    mutant_results.append(rejected('change_raw_point_id', 'point_coordinate_binding',
                                   lambda: bind_sites(wrong_id, points)))
    wrong_input = copy.deepcopy(raw_records[0])
    wrong_input['input_digest'] = '0' * 64
    mutant_results.append(rejected('change_input_digest', 'input_identity',
                                   lambda: bind_sites(wrong_input, points)))
    need(any(not trial['both_strict'] for trial in bridge_trials) and
         any(trial['both_strict'] for trial in bridge_trials),
         'bridge_search_success_and_failure_are_both_nonvacuous')
    need(len(cases) == 3 and not isolation_scans[254569]['survivors'] and
         len(isolation_scans[996863]['survivors']) == 4 and len(geometry.cache) <= 32,
         'three_distinct_certificate_cases_nonvacuum')
    all_scans = [bridge_scan, *isolation_scans.values(), false_scan]
    output = dict(schema='mhgp7-independent-real-global-parent-certificates-v1',
                  status='passed', public_status='not_claimed',
                  source_sha256=PINS, script_sha256=sha(Path(__file__)),
                  raw_capture_sha256=RAW_SHA, input_digest=INPUT_SHA,
                  n=len(points), generated_triples=attempts,
                  cases=cases, targeted_mutants=mutant_results,
                  counterexamples_verified=['distance_filter_survival_is_not_a_strict_edge',
                                            'open_isolation_cannot_be_transferred_to_closed_cut'],
                  false_isolation_scan=false_scan,
                  scanned_distance_populations=len(all_scans),
                  scanned_point_population_pairs=sum(row['points_scanned'] for row in all_scans),
                  complete_ball_census_scans=3, complete_ball_census_point_pairs=3 * len(points),
                  exact_MEB_calls=len(geometry.cache), rational_support_tests=geometry.support_tests,
                  maximum_geometric_subset_size=max(map(len, geometry.cache)),
                  exact_global_parent_relation_for_these_three_blocks=True,
                  global_components_enumerated=False, global_catalogue_completeness_verified=False,
                  entire_atomic_plateau_events_reconstructed=False, full_tower_verified=False,
                  engine_executed=False, GCP_used=False)
    print(json.dumps(output, sort_keys=True, separators=(',', ':')))


if __name__ == '__main__':
    main()
