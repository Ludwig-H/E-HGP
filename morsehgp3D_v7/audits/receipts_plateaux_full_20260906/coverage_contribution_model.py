"""Dated coverage contributions, with no global coverage in the producer.

Topology has births and genuine multifusions only. Continuations keep their
token and may receive a dated, possibly redundant local contribution. Both
whole-ball and local-gap journals are read at exact open/closed cuts. Only
the reader and the finished-production Gamma judge construct global point
sets. Geometry and the resolver reuse pinned independent audit models.
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

from ball_anchor_model import resolve
from plateau_model import Facet, Model, groups

HERE = Path(__file__).resolve().parent
PINS = {
    'plateau_model.py': '8afb5663c1fa0384d5ae392294cc03853676a89fcb18f858cd7d764ed9b1b93a',
    'ball_anchor_model.py': '8f13f02d6b2c36c79708f6bdb8482c3d01ec81c4164b2bb60422993861e0ded4',
    'normal.json': 'f7a7379ff0fa1d0a7fc5c760b289e07daefe4122a2e4337a41083a73f31aba56',
    '../meb_rational_oracle_20260905.py': 'ad6c0d6c041ff788180a400f6ba2ad2b1546f8607e8f2c91fefca9133a8e7f2b',
}
BallKey = tuple[int, ...]


def need(ok: bool, reason: str) -> None:
    if not ok:
        raise ValueError(reason)


def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


@dataclass(frozen=True)
class Node:
    radius: Q
    parents: tuple[int, ...]
    kind: str


@dataclass(frozen=True)
class Contribution:
    radius: Q
    token: int
    ball: BallKey
    shell_mask: int
    include_interior: bool


@dataclass
class State:
    # No field stores the points/facets of a global component.
    nodes: list[Node] = field(default_factory=list)
    successors: dict[int, int] = field(default_factory=dict)
    roots: set[int] = field(default_factory=set)
    anchors: dict[BallKey, tuple[Q, int]] = field(default_factory=dict)
    whole: list[Contribution] = field(default_factory=list)
    gap: list[Contribution] = field(default_factory=list)

    def root(self, token: int) -> int:
        for _ in range(len(self.nodes) + 1):
            if token not in self.successors:
                need(token in self.roots, 'resolver_root_is_live')
                return token
            token = self.successors[token]
        raise ValueError('successor_cycle')

    def signature(self) -> tuple[Any, ...]:
        return (tuple(self.nodes), tuple(sorted(self.successors.items())),
                tuple(sorted(self.roots)), tuple(sorted(self.anchors.items())),
                tuple(self.whole), tuple(self.gap))

    def frozen(self) -> State:
        return State(self.nodes.copy(), self.successors.copy(), self.roots.copy(),
                     self.anchors.copy(), self.whole.copy(), self.gap.copy())


def close_batch(model: Model, state: State, k: int, radius: Q,
                blocks: list[tuple[BallKey, dict[str, Any]]], stats: Counter[str]) -> None:
    before = state.signature()
    edges: list[tuple[Any, Any]] = []
    local_payloads: list[tuple[int, bool]] = []
    hits: list[tuple[Facet, BallKey]] = []
    for index, (_, ball) in enumerate(blocks):
        interior, shell = ball['interior'], sorted(ball['shell'])
        positions = {point: bit for bit, point in enumerate(shell)}
        if k <= len(interior):
            representatives = [tuple(sorted(interior)[:k])]
            covered_shell = (1 << len(shell)) - 1
        else:
            local = model.shell_quotient(ball, k)
            representatives = [tuple(sorted(interior | set(min(component))))
                               for component in local]
            covered_shell = 0
            for component in local:
                for facet in component:
                    for point in facet:
                        covered_shell |= 1 << positions[point]
        # The only coverage computed in production is a local shell bitmask.
        # If a strict class exists, all interior points belong to its coverage.
        missing_shell = ((1 << len(shell)) - 1) & ~covered_shell
        local_payloads.append((missing_shell, not representatives))
        parents = {resolve(model, state, facet, radius, False, stats, hits)
                   for facet in representatives}
        edges.extend(((1, index), (0, parent)) for parent in sorted(parents))
        stats['local_classes'] += len(representatives)
    need(state.signature() == before, 'strict_parent_resolution_does_not_mutate_state')
    vertices = [(0, token) for token in sorted(state.roots)]
    vertices += [(1, index) for index in range(len(blocks))]
    clusters = sorted(groups(vertices, edges), key=lambda group: tuple(sorted(group)))
    live: set[int] = set()
    pending: dict[BallKey, tuple[Q, int]] = {}
    for cluster in clusters:
        parents = tuple(sorted(index for kind, index in cluster if kind == 0))
        indices = sorted(index for kind, index in cluster if kind == 1)
        if len(parents) == 1:
            token = parents[0]
            stats['continuation_blocks'] += len(indices)
        else:
            need(bool(indices), 'new_topology_node_requires_a_closed_block')
            token = len(state.nodes)
            kind = 'birth' if not parents else 'merge'
            state.nodes.append(Node(radius, parents, kind))
            for parent in parents:
                need(state.nodes[parent].radius < radius, 'atomic_strict_topology_parents')
                state.successors[parent] = token
            stats[kind + '_nodes'] += 1
        live.add(token)
        for index in indices:
            key, ball = blocks[index]
            full_mask = (1 << len(ball['shell'])) - 1
            state.whole.append(Contribution(radius, token, key, full_mask, True))
            missing_shell, include_interior = local_payloads[index]
            if missing_shell or (include_interior and ball['interior']):
                state.gap.append(Contribution(radius, token, key,
                                              missing_shell, include_interior))
                if len(parents) == 1:
                    stats['continuation_gap_contributions'] += 1
            need(key not in state.anchors and key not in pending, 'one_anchor_per_ball_order')
            pending[key] = (radius, token)
    state.roots = live
    state.anchors.update(pending)
    stats['scheduled_blocks'] += len(blocks)
    stats['anchors'] += len(pending)


def produce(model: Model, kmax: int) -> tuple[list[State], dict[tuple[Q, bool], list[State]], Counter[str]]:
    schedule: dict[tuple[Q, int], list[tuple[BallKey, dict[str, Any]]]] = defaultdict(list)
    smax = min(kmax + 1, len(model.ids))
    stats: Counter[str] = Counter()
    for key, ball in sorted(model.balls.items()):
        minimum = len(ball['interior']) + min(map(len, ball['bases']))
        if minimum > smax:
            stats['balls_outside_rank_window'] += 1
            continue
        for k in range(max(1, minimum - 1), min(kmax, len(ball['closed'])) + 1):
            schedule[ball['radius'], k].append((key, ball))
    states = [State() for _ in range(kmax)]
    snapshots = {}
    for radius in sorted(set(model.level.values())):
        snapshots[radius, False] = [state.frozen() for state in states]
        for k, state in enumerate(states, 1):
            close_batch(model, state, k, radius, schedule[radius, k], stats)
        snapshots[radius, True] = [state.frozen() for state in states]
    need(sum(len(state.whole) for state in states) == stats['scheduled_blocks'] == stats['anchors'],
         'whole_journal_exactly_one_reference_per_scheduled_block')
    need(sum(len(state.gap) for state in states) <= stats['scheduled_blocks'],
         'gap_journal_at_most_one_reference_per_scheduled_block')
    return states, snapshots, stats


def read_cut(model: Model, nodes: list[Node], rows: list[Contribution], cut: Q, closed: bool,
             ignore_contribution_time: bool = False) -> dict[int, frozenset[int]]:
    active = lambda radius: radius <= cut if closed else radius < cut
    successors: dict[int, int] = {}
    for token, node in enumerate(nodes):
        need(node.kind == ('merge' if node.parents else 'birth') and len(node.parents) != 1,
             'journal_contains_no_growth_topology_node')
        need(len(set(node.parents)) == len(node.parents), 'distinct_topology_parents')
        for parent in node.parents:
            need(0 <= parent < token and nodes[parent].radius < node.radius and
                 parent not in successors, 'valid_historical_topology')
            successors[parent] = token

    def root_at(token: int) -> int:
        need(0 <= token < len(nodes) and active(nodes[token].radius), 'active_target_at_query_cut')
        while token in successors and active(nodes[successors[token]].radius):
            token = successors[token]
        return token

    live = {root_at(token) for token, node in enumerate(nodes) if active(node.radius)}
    coverage: dict[int, set[int]] = {token: set() for token in live}
    for row in rows:
        need(0 <= row.token < len(nodes) and nodes[row.token].radius <= row.radius,
             'contribution_target_born_by_activation')
        need(row.token not in successors or nodes[successors[row.token]].radius > row.radius,
             'contribution_targets_closed_component_segment')
        ball = model.balls[row.ball]
        need(ball['radius'] == row.radius and 0 <= row.shell_mask < (1 << len(ball['shell'])),
             'contribution_ball_level_and_mask')
        if not active(row.radius) and not ignore_contribution_time:
            continue
        token = root_at(row.token)
        # Global point sets first appear here, in the reader, never in production.
        if row.include_interior:
            coverage[token].update(ball['interior'])
        coverage[token].update(point for bit, point in enumerate(sorted(ball['shell']))
                               if row.shell_mask & (1 << bit))
    return {token: frozenset(points) for token, points in coverage.items()}


def serialize(state: State) -> dict[str, Any]:
    def row(item: Contribution) -> dict[str, Any]:
        return dict(radius=str(item.radius), token=item.token, ball=list(item.ball),
                    shell_mask=item.shell_mask, include_interior=item.include_interior)
    return dict(nodes=[dict(radius=str(node.radius), parents=list(node.parents), kind=node.kind)
                       for node in state.nodes],
                whole=[row(item) for item in state.whole], gap=[row(item) for item in state.gap])


def deserialize(data: dict[str, Any]) -> tuple[list[Node], list[Contribution], list[Contribution]]:
    nodes = [Node(Q(node['radius']), tuple(node['parents']), node['kind']) for node in data['nodes']]
    streams = [[Contribution(Q(row['radius']), row['token'], tuple(row['ball']),
                             row['shell_mask'], row['include_interior']) for row in data[name]]
               for name in ('whole', 'gap')]
    return nodes, streams[0], streams[1]


def verify(model: Model, final: list[State], snapshots: dict[tuple[Q, bool], list[State]]) -> dict[str, Any]:
    # The reader receives a serialized/deserialized final journal, including
    # future records when answering an old cut. It receives neither live
    # producer anchors nor producer successor tables.
    wire = json.loads(json.dumps([serialize(state) for state in final], sort_keys=True))
    archives = [deserialize(data) for data in wire]
    stats: Counter[str] = Counter()
    hits: list[tuple[Facet, BallKey]] = []
    digest = hashlib.sha256()
    for (radius, closed), states in snapshots.items():
        for k, state in enumerate(states, 1):
            signature = state.signature()
            actual: dict[int, set[Facet]] = defaultdict(set)
            for facet in combinations(model.ids, k):
                if model.level[facet] <= radius if closed else model.level[facet] < radius:
                    token = resolve(model, state, facet, radius, closed, stats, hits)
                    actual[token].add(facet)
                    stats['active_facets'] += 1
            gamma = model.gamma(k, radius, closed)
            need({frozenset(component) for component in actual.values()} == set(gamma),
                 'topological_identities_equal_independent_Gamma')
            expected = {token: frozenset(point for facet in component for point in facet)
                        for token, component in actual.items()}
            nodes, whole, gap = archives[k - 1]
            full_cover = read_cut(model, nodes, whole, radius, closed)
            gap_cover = read_cut(model, nodes, gap, radius, closed)
            need(full_cover == gap_cover == expected and set(expected) == state.roots,
                 'both_contribution_journals_exact_at_open_or_closed_cut')
            need(read_cut(model, nodes, gap + gap, radius, closed) == expected,
                 'duplicate_contributions_are_idempotent_within_each_root')
            need(state.signature() == signature, 'finished_production_snapshot_unchanged')
            stats['order_cuts'] += 1
            stats['live_component_coverages'] += len(expected)
            if closed:
                digest.update(json.dumps([k, str(radius),
                    sorted(sorted(component) for component in actual.values())],
                    separators=(',', ':')).encode())
    return dict(counters=dict(stats), partition_digest=digest.hexdigest(), archive=wire)


def main() -> None:
    for name, pin in PINS.items():
        need(sha(HERE / name) == pin, 'source_pin_' + name)
    source = json.loads((HERE / 'normal.json').read_text())
    clouds = {name: [tuple(point) for point in points] for name, points in source['clouds'].items()}
    clouds['outside_removes_growth'] = [(1, 8, 0), (5, 10, 0), (9, 8, 0),
                                      (5, 0, 0), (10, 6, 0), (9, 1, 0)]
    clouds['arity_mix'] = [(10, 5, 0), (0, 5, 0), (2, 1, 0), (2, 9, 0)]
    clouds['arity_window_tetra_Kmax2'] = [(2, 2, 2), (2, 0, 0), (0, 2, 0),
                                        (0, 0, 2), (0, 0, 0)]
    models = {name: Model(points) for name, points in clouds.items()}
    models['window_shell7_Kmax5'] = models['window_shell7']
    runs, finals = {}, {}
    for name, model in models.items():
        kmax = 2 if name == 'arity_window_tetra_Kmax2' else 5 if name == 'window_shell7_Kmax5' else len(model.ids)
        final, snapshots, production = produce(model, kmax)
        judged = verify(model, final, snapshots)
        finals[name] = final
        runs[name] = dict(n=len(model.ids), kmax=kmax, production=dict(production), verification=judged,
                          whole_records=sum(len(state.whole) for state in final),
                          gap_records=sum(len(state.gap) for state in final),
                          topology_nodes=sum(len(state.nodes) for state in final))

    base = models['coverage_growth']
    state = finals['coverage_growth'][2]
    key = base.key[(0, 1, 2, 3)]
    named = [row for row in state.gap if row.ball == key and row.radius == Q(25)]
    need(len(named) == 1 and named[0].shell_mask == 1 << 3 and
         not named[0].include_interior, 'real_continuation_has_local_shell_Z_contribution')
    row = named[0]
    need(state.nodes[row.token].radius == Q(16) and len(state.nodes) == 1,
         'growth_keeps_birth_token_and_creates_no_topology_node')
    before = read_cut(base, state.nodes, state.gap, Q(25), False)
    after = read_cut(base, state.nodes, state.gap, Q(25), True)
    need(before[row.token] == frozenset({0, 1, 2}) and
         after[row.token] == frozenset({0, 1, 2, 3}), 'closed_activation_of_growth_contribution')
    dropped = [item for item in state.gap if item is not row]
    need(read_cut(base, state.nodes, dropped, Q(25), True) != after,
         'suppressed_continuation_contribution_mutant_refuted')
    early = read_cut(base, state.nodes, state.gap, Q(16), True)
    leaked = read_cut(base, state.nodes, state.gap, Q(16), True, ignore_contribution_time=True)
    need(early[row.token] == frozenset({0, 1, 2}) and leaked[row.token] == frozenset({0, 1, 2, 3}),
         'future_contribution_leaks_through_unchanged_token_mutant_refuted')

    extended = models['outside_removes_growth']
    external_state = finals['outside_removes_growth'][2]
    external_key = extended.key[(0, 1, 2, 3)]
    extra = [item for item in external_state.gap if item.ball == external_key]
    need(len(extra) == 1 and extra[0].shell_mask == 1 << 3, 'same_potential_gap_is_still_logged')
    external_before = read_cut(extended, external_state.nodes, external_state.gap, Q(25), False)
    external_after = read_cut(extended, external_state.nodes, external_state.gap, Q(25), True)
    need(external_before == external_after and len(external_before) == 1 and
         next(iter(external_before.values())) == frozenset(range(6)),
         'redundant_potential_gap_does_not_invent_a_global_growth')
    need(sum(run['production'].get('merge_nodes', 0) for run in runs.values()) > 0 and
         sum(run['production'].get('continuation_gap_contributions', 0) for run in runs.values()) > 0,
         'birth_merge_and_continuation_contribution_nonvacuum')
    print(json.dumps(dict(schema='mhgp7-independent-dated-coverage-contributions-v1', status='passed',
        public_status='not_claimed', source_sha256=PINS, script_sha256=sha(Path(__file__).resolve()),
        producer_stores_global_coverages=False, producer_stores_global_facet_memberships=False,
        producer_uses_Gamma_for_parent_resolution=False, reader_uses_serialized_final_journal=True,
        geometry_is_precomputed_bounded_rational_oracle=True,
        contributions_are_potential_coverage_not_exact_disjoint_deltas=True,
        topology_contains_no_growth_nodes=True, GCP_used=False, engine_executed=False,
        clouds={name: list(model.points) for name, model in models.items()}, runs=runs,
        targeted_mutants_refuted=['drop_continuation_contribution', 'ignore_contribution_activation_time'],
        same_potential_gap_with_and_without_actual_growth=True,
        contribution_reference_authority='same_pinned_input_exact_ball_census_and_canonical_shell_order',
        limitations=['No C++ producer or industrial timing qualification',
                     'Archive references the exact ball census in the input model; standalone geometry export is not implemented',
                     'Volume is bounded by scheduled blocks, not by public topology node count']),
        sort_keys=True, separators=(',', ':')))


if __name__ == '__main__':
    main()
