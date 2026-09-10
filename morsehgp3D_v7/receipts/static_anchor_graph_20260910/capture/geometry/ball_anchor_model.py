"""Bounded plateau producer with ball anchors, never global facet membership.

Geometry and local shell quotients share the pinned rational audit model.
Gamma is consulted only after every production snapshot has been completed.
No product helper, engine, subprocess, benchmark or assert-based gate.
"""

from __future__ import annotations

from collections import Counter, defaultdict
from dataclasses import dataclass, field
from fractions import Fraction as Q
import hashlib
from itertools import combinations
import json
from pathlib import Path
from typing import Any

from plateau_model import Facet, Model, groups

HERE = Path(__file__).resolve().parent
MODEL_SHA = "8afb5663c1fa0384d5ae392294cc03853676a89fcb18f858cd7d764ed9b1b93a"
SOURCE_SHA = "f7a7379ff0fa1d0a7fc5c760b289e07daefe4122a2e4337a41083a73f31aba56"
BallKey = tuple[int, ...]
Hit = tuple[Facet, BallKey]


def need(ok: bool, reason: str) -> None:
    if not ok:
        raise ValueError(reason)


def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


@dataclass(frozen=True)
class Node:
    # A growth node versions one component; it is not a new public H0 birth.
    radius: Q
    parents: tuple[int, ...]
    coverage: frozenset[int]
    kind: str


@dataclass
class State:
    # These are the entire mutable production state: no facet memberships.
    nodes: list[Node] = field(default_factory=list)
    successors: dict[int, int] = field(default_factory=dict)
    roots: set[int] = field(default_factory=set)
    anchors: dict[BallKey, tuple[Q, int]] = field(default_factory=dict)

    def frozen(self) -> State:
        return State(self.nodes.copy(), self.successors.copy(),
                     self.roots.copy(), self.anchors.copy())

    def root(self, token: int) -> int:
        # No path compression: resolving a saved snapshot must not mutate it.
        for _ in range(len(self.nodes) + 1):
            if token not in self.successors:
                need(token in self.roots, "normalized_token_is_a_live_root")
                return token
            token = self.successors[token]
        raise ValueError("successor_cycle")

    def signature(self) -> tuple[Any, ...]:
        return (tuple(self.nodes), tuple(sorted(self.successors.items())),
                tuple(sorted(self.roots)), tuple(sorted(self.anchors.items())))


def exchange(model: Model, facet: Facet, stats: Counter[str]) -> Facet:
    key = model.key[facet]
    ball = model.balls[key]
    intruders = sorted(ball['interior'] - set(facet))
    stats['census_after_anchor_miss'] += 1
    need(bool(intruders), "weak_terminal_requires_closed_ball_anchor")
    removed = min(model.support[facet])
    following = tuple(sorted((set(facet) - {removed}) | {intruders[0]}))
    following_key = model.key[following]
    next_ball = model.balls[following_key]
    need(len(following) == len(facet), "same_cardinality_exchange")
    need(next_ball['radius'] <= ball['radius'], "nonincreasing_radius")
    if next_ball['radius'] == ball['radius']:
        need(following_key == key and
             len(set(following) & ball['shell']) == len(set(facet) & ball['shell']) - 1,
             "equal_radius_same_ball_decreasing_selected_shell")
        stats['equal_radius_exchanges'] += 1
    else:
        stats['strict_radius_exchanges'] += 1
    coface = tuple(sorted(set(facet) | set(following)))
    need(len(coface) == len(facet) + 1 and model.key[coface] == key,
         "elementary_connection_at_original_MEB_level")
    stats['descent_steps'] += 1
    return following


def resolve(model: Model, state: State, facet: Facet, cut: Q, closed: bool,
            stats: Counter[str], hits: list[Hit]) -> int:
    stats['requests'] += 1
    seen: set[Facet] = set()
    while facet not in seen:
        seen.add(facet)
        stats['exact_MEB_requests'] += 1
        key = model.key[facet]
        radius = model.level[facet]
        need(radius <= cut if closed else radius < cut, "resolver_active_facet")
        stats['anchor_lookups'] += 1
        anchor = state.anchors.get(key)
        if anchor is not None:
            need(anchor[0] == radius and (radius <= cut if closed else radius < cut),
                 "closed_anchor_precedes_requested_cut")
            stats['anchor_hits'] += 1
            hits.append((facet, key))  # Audit observer, never a membership table.
            return state.root(anchor[1])
        stats['anchor_misses'] += 1
        facet = exchange(model, facet, stats)
    raise ValueError("facet_descent_cycle")


def close_batch(model: Model, state: State, k: int, radius: Q,
                blocks: list[tuple[BallKey, dict[str, Any]]],
                stats: Counter[str], hits: list[Hit]) -> list[dict[str, Any]]:
    before = state.signature()
    edges = []
    for index, (_, ball) in enumerate(blocks):
        interior = ball['interior']
        if k <= len(interior):
            representatives = [tuple(sorted(interior)[:k])]
            stats['interior_connected_blocks'] += 1
        else:
            local = model.shell_quotient(ball, k)
            representatives = [tuple(sorted(interior | set(min(c)))) for c in local]
            stats['shell_quotient_blocks'] += 1
        stats['local_classes'] += len(representatives)
        parents = {resolve(model, state, facet, radius, False, stats, hits)
                   for facet in representatives}
        edges.extend(((1, index), (0, parent)) for parent in sorted(parents))
    need(state.signature() == before, "all_parents_use_immutable_strict_snapshot")
    vertices = [(0, root) for root in sorted(state.roots)]
    vertices += [(1, index) for index in range(len(blocks))]
    clusters = sorted(groups(vertices, edges), key=lambda c: tuple(sorted(c)))
    live: set[int] = set()
    pending_anchors: dict[BallKey, tuple[Q, int]] = {}
    events = []
    for cluster in clusters:
        parents = tuple(sorted(index for kind, index in cluster if kind == 0))
        indices = sorted(index for kind, index in cluster if kind == 1)
        old_cover = frozenset(x for parent in parents for x in state.nodes[parent].coverage)
        new_cover = old_cover | frozenset(x for index in indices for x in blocks[index][1]['closed'])
        if not indices or (len(parents) == 1 and new_cover == old_cover):
            need(len(parents) == 1, "unchanged_component_has_one_parent")
            token = parents[0]
        else:
            kind = 'birth' if not parents else 'merge' if len(parents) > 1 else 'growth'
            token = len(state.nodes)
            state.nodes.append(Node(radius, parents, new_cover, kind))
            for parent in parents:
                need(state.nodes[parent].radius < radius, "atomic_plateau_no_same_level_parent")
                state.successors[parent] = token
            events.append(dict(k=k, radius=str(radius), token=token, kind=kind,
                               parents=list(parents), before=sorted(old_cover),
                               after=sorted(new_cover), added=sorted(new_cover - old_cover),
                               blocks=len(indices)))
            stats[kind + '_nodes'] += 1
            if radius == 0:
                need(k == 1 and len(new_cover) == 1, "radius_zero_point_birth")
                stats['radius_zero_point_nodes'] += 1
        live.add(token)
        for index in indices:
            key = blocks[index][0]
            need(key not in state.anchors, "one_closed_anchor_per_order_and_ball")
            pending_anchors[key] = (radius, token)
    state.roots = live
    state.anchors.update(pending_anchors)
    stats['closed_ball_anchors'] += len(pending_anchors)
    return events


def produce(model: Model, kmax: int) -> tuple[dict[tuple[Q, bool], list[State]],
                                            Counter[str], list[Hit], list[dict[str, Any]]]:
    smax = min(kmax + 1, len(model.ids))
    stats: Counter[str] = Counter()
    hits: list[Hit] = []
    schedule: dict[tuple[Q, int], list[tuple[BallKey, dict[str, Any]]]] = defaultdict(list)
    for key, ball in sorted(model.balls.items()):
        a = len(ball['interior']) + min(map(len, ball['bases']))
        if a > smax:
            stats['balls_outside_rank_window'] += 1
            continue
        stats['shared_catalogue_balls'] += 1
        for k in range(1, kmax + 1):
            if k > len(ball['closed']):
                stats['empty_order_ball_skips'] += 1
            elif k < a - 1:
                stats['inert_order_ball_skips'] += 1
            else:
                schedule[ball['radius'], k].append((key, ball))
                stats['scheduled_order_ball_blocks'] += 1
    states = [State() for _ in range(kmax)]
    snapshots: dict[tuple[Q, bool], list[State]] = {}
    events = []
    for radius in sorted(set(model.level.values())):
        snapshots[radius, False] = [state.frozen() for state in states]
        for k, state in enumerate(states, 1):
            events.extend(close_batch(model, state, k, radius,
                                      schedule[radius, k], stats, hits))
        snapshots[radius, True] = [state.frozen() for state in states]
    need(stats['closed_ball_anchors'] == stats['scheduled_order_ball_blocks'],
         "every_scheduled_ball_has_one_closed_anchor")
    return snapshots, stats, hits, events


def verify(model: Model, kmax: int, snapshots: dict[tuple[Q, bool], list[State]]) -> dict[str, Any]:
    stats: Counter[str] = Counter()
    hits: list[Hit] = []
    previous: list[dict[Facet, int]] = [{} for _ in range(kmax)]
    previous_vertical: dict[tuple[int, int], int] = {}
    digest_rows: dict[int, list[Any]] = defaultdict(list)
    for (radius, closed), states in snapshots.items():
        assignments = []
        for k, state in enumerate(states, 1):
            signature = state.signature()
            gamma = model.gamma(k, radius, closed)  # First oracle access: production is finished.
            actual: dict[int, set[Facet]] = defaultdict(set)
            current = {}
            for facet in combinations(model.ids, k):
                if model.level[facet] <= radius if closed else model.level[facet] < radius:
                    token = resolve(model, state, facet, radius, closed, stats, hits)
                    current[facet] = token
                    actual[token].add(facet)
                    stats['active_facets_compared'] += 1
            need({frozenset(c) for c in actual.values()} == set(gamma), "exact_Gamma_component_identities")
            need(set(actual) == state.roots, "no_missing_or_spurious_live_tokens")
            for token, members in actual.items():
                need(state.nodes[token].coverage == frozenset(x for f in members for x in f),
                     "exact_component_point_coverage")
            for facet, token in previous[k - 1].items():
                need(state.root(token) == current[facet], "horizontal_naturality")
                stats['horizontal_naturality_checks'] += 1
            need(state.signature() == signature, "verification_does_not_mutate_production_snapshot")
            previous[k - 1] = current
            assignments.append(current)
            stats['open_closed_order_cuts'] += 1
            if closed:
                digest_rows[k].append([k, str(radius), sorted(sorted(c) for c in actual.values())])
        vertical = {}
        for k in range(2, kmax + 1):
            for facet, upper in assignments[k - 1].items():
                images = {assignments[k - 2][f] for f in combinations(facet, k - 1)}
                need(len(images) == 1, "all_subfacets_have_one_vertical_image")
                image = next(iter(images))
                need(vertical.setdefault((k, upper), image) == image, "vertical_image_independent_of_upper_facet")
            stats['vertical_images'] += sum(order == k for order, _ in vertical)
            if closed:
                for key, (birth_radius, token) in states[k - 1].anchors.items():
                    node = states[k - 1].nodes[token]
                    if birth_radius != radius or node.radius != radius or node.kind != 'birth':
                        continue
                    anchor = states[k - 2].anchors.get(key)
                    need(anchor is not None and anchor[0] == radius,
                         "upper_birth_has_same_ball_closed_lower_anchor")
                    need(states[k - 2].root(anchor[1]) == vertical[k, states[k - 1].root(token)],
                         "same_ball_anchor_is_the_vertical_birth_image")
                    stats['vertical_birth_ball_anchor_checks'] += 1
        for (k, upper), lower in previous_vertical.items():
            need(vertical[k, states[k - 1].root(upper)] == states[k - 2].root(lower),
                 "vertical_naturality_square")
            stats['vertical_naturality_squares'] += 1
        previous_vertical = vertical
    digest = hashlib.sha256()
    for k in range(1, kmax + 1):
        for row in digest_rows[k]:
            digest.update(json.dumps(row, separators=(',', ':')).encode())
    stats['nonGabriel_anchor_hits_observed_afterward'] = sum(
        bool(model.balls[key]['interior'] - set(facet)) for facet, key in hits)
    return dict(counters=dict(stats), horizontal_digest=digest.hexdigest())


def run(model: Model, kmax: int) -> dict[str, Any]:
    snapshots, stats, hits, events = produce(model, kmax)
    stats['nonGabriel_anchor_hits_observed_afterward'] = sum(
        bool(model.balls[key]['interior'] - set(facet)) for facet, key in hits)
    judged = verify(model, kmax, snapshots)
    return dict(n=len(model.ids), kmax=kmax, smax=min(kmax + 1, len(model.ids)),
                production=dict(stats), verification=judged, events=events)


def main() -> None:
    need(sha(HERE / 'plateau_model.py') == MODEL_SHA, "pinned_geometry_model")
    need(sha(HERE / 'normal.json') == SOURCE_SHA, "pinned_six_cloud_receipt")
    source = json.loads((HERE / 'normal.json').read_text())
    clouds = source['clouds']
    models = {name: Model([tuple(p) for p in points]) for name, points in clouds.items()}
    runs = {name: run(model, len(model.ids)) for name, model in models.items()}
    for name in models:
        need(runs[name]['verification']['horizontal_digest'] == source['digests'][name],
             "same_complete_horizontal_partitions_as_pinned_six_cloud_model")
    runs['window_shell7_Kmax5'] = run(models['window_shell7'], 5)
    mix = Model([(10, 5, 0), (0, 5, 0), (2, 1, 0), (2, 9, 0)])
    key = mix.key[(0, 2, 3)]
    need(len(mix.support[(0, 2, 3)]) == 3 and min(map(len, mix.balls[key]['bases'])) == 2,
         "local_positive_basis_arity_differs_from_global_qmin")
    runs['arity_mix'] = run(mix, 4)
    tetra = Model([(2, 2, 2), (2, 0, 0), (0, 2, 0), (0, 0, 2), (0, 0, 0)])
    tetra_key = tetra.key[(0, 1, 2, 3)]
    tetra_ball = tetra.balls[tetra_key]
    need(len(tetra.support[(0, 1, 2, 3)]) == 4 and
         min(map(len, tetra_ball['bases'])) == 2 and tetra_ball['radius'] == Q(3),
         "global_qmin_not_chosen_tetra_arity_controls_window")
    runs['arity_window_tetra_Kmax2'] = run(tetra, 2)
    tetra_snapshots, _, _, tetra_events = produce(tetra, 2)
    original_state = tetra_snapshots[Q(3), True][1]
    need(not any(e['k'] == 2 and e['radius'] == '3' for e in tetra_events),
         "tetra_ball_K2_is_H0_and_coverage_inert")
    mutant_stats: Counter[str] = Counter()
    mutant_hits: list[Hit] = []
    resolve(tetra, original_state, (0, 4), Q(3), True, mutant_stats, mutant_hits)
    mutated_state = original_state.frozen()
    need(mutated_state.anchors.pop(tetra_key, None) is not None, "mutant_removes_existing_inert_anchor")
    need(mutated_state.nodes == original_state.nodes and
         mutated_state.successors == original_state.successors and
         mutated_state.roots == original_state.roots, "anchor_mutant_preserves_public_component_state")
    try:
        resolve(tetra, mutated_state, (0, 4), Q(3), True, mutant_stats, mutant_hits)
    except ValueError as error:
        need(str(error) == 'weak_terminal_requires_closed_ball_anchor', "targeted_missing_anchor_rejection")
        mutant_reason = str(error)
    else:
        raise ValueError("missing_inert_anchor_mutant_survived")
    unit_stats: Counter[str] = Counter()
    original = (0, 1, 2, 3, 4)
    following = exchange(models['portal_equal'], original, unit_stats)
    need(unit_stats['equal_radius_exchanges'] == 1, "equal_radius_exchange_unit_nonvacuous")
    need(sum(r['production'].get('descent_steps', 0) for r in runs.values()) > 0,
         "production_resolver_descent_nonvacuous")
    need(sum(r['production'].get('inert_order_ball_skips', 0) for r in runs.values()) > 0,
         "inert_ball_omission_nonvacuous")
    growth = [e for e in runs['coverage_growth']['events'] if e['k'] == 3 and e['radius'] == '25']
    need(len(growth) == 1 and growth[0]['kind'] == 'growth' and growth[0]['added'] == [3],
         "growth_delta_is_retained")
    large = [e for e in runs['window_shell7_Kmax5']['events'] if e['k'] == 5 and e['radius'] == '25']
    need(len(large) == 1 and large[0]['kind'] == 'birth' and len(large[0]['after']) == 7,
         "closed_cardinality_above_smax_is_retained")
    print(json.dumps(dict(status='passed', public_status='not_claimed',
        scope='bounded_ball_anchor_H0_model_not_product_or_industrial_performance',
        script_sha256=sha(Path(__file__)), geometry_model_sha256=MODEL_SHA,
        input_receipt_sha256=SOURCE_SHA,
        rational_oracle_sha256=sha(HERE.parent / 'meb_rational_oracle_20260905.py'),
        producer_stores_global_facet_memberships=False, gamma_used_for_parent_resolution=False,
        growth_nodes_are_component_versions_not_new_H0_births=True,
        MEB_request_counters_are_resolver_reads_of_precomputed_exact_geometry=True,
        MEB_lookup_precedes_new_census=True, engine_executed=False, GCP_used=False,
        runs=runs, equal_radius_exchange_unit=dict(before=original, after=following, counters=dict(unit_stats)),
        arity_mix=dict(points=mix.points, local_arity=3, global_qmin=2),
        inert_anchor_mutant=dict(points=tetra.points, local_arity=4, global_qmin=2,
                                 kmax=2, smax=3, cut='3', facet=[0, 4],
                                 unchanged_public_component_state=True,
                                 rejection=mutant_reason, counters=dict(mutant_stats))),
        sort_keys=True, separators=(',', ':')))


if __name__ == '__main__':
    main()
