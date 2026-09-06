"""An identical local plateau gap can produce growth or no global growth.

Only the four-site base and its six-site extension are enumerated, at K=3
and squared radius 25. The pinned earlier rational model supplies exact
MEBs and the two Gamma cuts. No product helper, engine, subprocess, cloud
access, arbitrary cutoff, or assert-based gate is used.
"""

from __future__ import annotations

from fractions import Fraction as Q
import hashlib
from itertools import combinations
import json
from pathlib import Path
from typing import Any

from plateau_model import Component, Model, cover

HERE = Path(__file__).resolve().parent
MODEL_SHA = "8afb5663c1fa0384d5ae392294cc03853676a89fcb18f858cd7d764ed9b1b93a"
ORACLE_SHA = "ad6c0d6c041ff788180a400f6ba2ad2b1546f8607e8f2c91fefca9133a8e7f2b"
LABELS = "ABCZXY"
POINTS = [(1, 8, 0), (5, 10, 0), (9, 8, 0),
          (5, 0, 0), (10, 6, 0), (9, 1, 0)]
RADIUS = Q(25)
K = 3


def need(ok: bool, reason: str) -> None:
    if not ok:
        raise ValueError(reason)


def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def ids(labels: str) -> tuple[int, ...]:
    return tuple(sorted(LABELS.index(label) for label in labels))


def labels(points: frozenset[int]) -> str:
    return "".join(LABELS[i] for i in sorted(points))


def members(component: Component) -> list[str]:
    return [labels(frozenset(facet)) for facet in sorted(component)]


def diameter(model: Model, coface: str, pair: str, expected: Q) -> dict[str, Any]:
    a, b = (POINTS[LABELS.index(label)] for label in pair)
    center_twice = tuple(x + y for x, y in zip(a, b))
    diameter_squared = sum((x - y) ** 2 for x, y in zip(a, b))
    distances_four = {
        label: sum((2 * x - y) ** 2 for x, y in
                   zip(POINTS[LABELS.index(label)], center_twice))
        for label in coface
    }
    need(all(value <= diameter_squared for value in distances_four.values()),
         "coface_inside_stated_diameter_ball")
    need(Q(diameter_squared, 4) == expected < RADIUS,
         "stated_diameter_radius_is_strict")
    need(model.level[ids(coface)] == expected and
         model.key[ids(coface)] == model.key[ids(pair)],
         "independent_rational_MEB_matches_diameter_certificate")
    return dict(coface=coface, diameter=pair,
                center=[str(Q(x, 2)) for x in center_twice],
                radius_squared=str(expected),
                four_times_squared_distances=distances_four,
                four_times_powers={key: value - diameter_squared
                                  for key, value in distances_four.items()})


def inspect(model: Model, key: tuple[int, ...]) -> dict[str, Any]:
    ball = model.balls[key]
    need(ball['radius'] == RADIUS and ball['interior'] == frozenset() and
         ball['shell'] == ball['closed'] == frozenset(range(4)),
         "identical_complete_local_ball_in_both_clouds")
    local = model.strict_local(ball, K)
    need(local == [frozenset({ids('ABC')})], "same_single_local_strict_component")
    local_coverage = cover(local[0])
    local_gap = ball['closed'] - local_coverage
    need(local_gap == frozenset({3}), "same_nonempty_local_gap_Z")

    # Exactly two Gamma cuts for this cloud, never Model.verify()'s full tour.
    before = model.gamma(K, RADIUS, False)
    after = model.gamma(K, RADIUS, True)
    parents = [component for component in before if local[0] <= component]
    need(len(parents) == 1, "one_global_parent_of_local_component")
    need(len(before) == 1, "this_fixture_has_one_entire_global_strict_component")
    parent = parents[0]
    block = frozenset(combinations(sorted(ball['closed']), K))
    images = [component for component in after if block <= component]
    need(len(images) == 1 and parent <= images[0], "closed_block_in_its_unique_parent_image")
    image = images[0]
    actual_gap = ball['closed'] - cover(parent)

    reconstructed, events = model.batch(before, K, RADIUS)
    need(set(reconstructed) == set(after), "bounded_batch_equals_closed_Gamma_cut")
    need(cover(image) - cover(parent) == actual_gap,
         "closed_global_event_adds_exactly_the_actual_gap_in_this_fixture")
    return dict(points=len(model.points), ball_key=list(key),
                interior=labels(ball['interior']), shell=labels(ball['shell']),
                local_components=len(local), local_members=members(local[0]),
                local_coverage=labels(local_coverage), local_gap=labels(local_gap),
                global_parents=len(parents), global_parent_members=members(parent),
                global_parent_coverage=labels(cover(parent)),
                actual_added=labels(actual_gap), closed_members=members(image),
                closed_coverage=labels(cover(image)), batch_events=events)


def main() -> None:
    need(sha(HERE / 'plateau_model.py') == MODEL_SHA, "pinned_previous_model")
    oracle_path = HERE.parent / 'meb_rational_oracle_20260905.py'
    need(sha(oracle_path) == ORACLE_SHA, "pinned_previous_rational_oracle")
    need(all(0 <= coordinate <= 65535 for point in POINTS for coordinate in point),
         "u16_coordinates")
    base, extended = Model(POINTS[:4]), Model(POINTS)
    key = base.key[ids('ABCZ')]
    need(extended.key[ids('ABCZ')] == key, "same_exact_BallKey")
    squared_distances = {
        LABELS[i]: sum((x - y) ** 2 for x, y in zip(point, (5, 5, 0)))
        for i, point in enumerate(POINTS)
    }
    need(all(squared_distances[label] == 25 for label in 'ABCZ') and
         squared_distances['X'] == 26 and squared_distances['Y'] == 32,
         "both_added_points_strictly_exterior_to_original_ball")
    certificates = [diameter(extended, 'ABCX', 'AX', Q(85, 4)),
                    diameter(extended, 'BCXY', 'BY', Q(97, 4)),
                    diameter(extended, 'CXYZ', 'CZ', Q(20))]
    path = [ids(text) for text in ['ABC', 'BCX', 'CXY', 'XYZ']]
    for first, second in zip(path, path[1:]):
        coface = tuple(sorted(set(first) | set(second)))
        need(len(coface) == K + 1 and extended.level[coface] < RADIUS,
             "three_strict_elementary_connections_before_plateau")

    base_result, extended_result = inspect(base, key), inspect(extended, key)
    need(base_result['actual_added'] == 'Z' and
         len(base_result['batch_events']) == 1 and
         base_result['batch_events'][0]['kind'] == 'growth', "base_really_grows")
    need(extended_result['actual_added'] == '' and
         extended_result['global_parent_coverage'] == LABELS and
         extended_result['batch_events'] == [], "exterior_bridge_removes_global_growth")
    need(extended_result['local_gap'] != extended_result['actual_added'],
         "mutant_local_gap_is_necessarily_actual_growth_refuted")

    # With only one exterior point, the first strict edge introducing Z from
    # a three-facet without Z would contain Z and two of A/B/C. Each such
    # triple already has radius 25, so that edge cannot be strict. Thus two
    # added points are minimal for this fixed four-point base, in any 3D position.
    need(all(base.level[tuple(sorted(pair + (3,)))] == RADIUS
             for pair in combinations((0, 1, 2), 2)),
         "one_external_point_obstruction_three_contained_triples")
    output = dict(schema='mhgp7-independent-local-gap-context-v1', status='passed',
                  public_status='not_claimed', k=K, radius_squared=str(RADIUS),
                  scope='two_bounded_exact_clouds_not_product_or_50k_records',
                  source_sha256={'model': MODEL_SHA, 'rational_oracle': ORACLE_SHA,
                                 'this_script': sha(Path(__file__).resolve())},
                  points={name: list(point) for name, point in zip(LABELS, POINTS)},
                  squared_distances_to_original_center=squared_distances,
                  strict_path=['ABC', 'BCX', 'CXY', 'XYZ'],
                  diameter_certificates=certificates, base=base_result,
                  extended=extended_result, gamma_cuts=4,
                  same_local_gap_different_actual_growth=True,
                  targeted_mutant_refuted='local_gap_equals_forced_global_growth',
                  two_exterior_points_minimal_for_this_base=True,
                  engine_executed=False, GCP_used=False)
    print(json.dumps(output, sort_keys=True, separators=(',', ':')))


if __name__ == '__main__':
    main()
