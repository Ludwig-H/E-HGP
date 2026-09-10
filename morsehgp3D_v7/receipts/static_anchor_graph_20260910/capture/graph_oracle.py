#!/usr/bin/env python3
"""Finite static-anchor-graph falsifier; no product helper or Gamma production.

The pinned rational helper enumerates all subsets ONLY to provide a bounded
geometry oracle. Static graph construction consumes shell representatives and
temporary descent chains. Gamma is used after construction, for judgment only.
"""
from __future__ import annotations

from collections import Counter, defaultdict
from fractions import Fraction as Q
from itertools import combinations
import hashlib
import json
from pathlib import Path
import sys

BASE = Path(__file__).resolve().parent
sys.path.insert(0, str(BASE / 'geometry'))
from plateau_model import Model, groups
from ball_anchor_model import produce, resolve

MUTANT = ''


def need(value, reason):
    if not value:
        raise ValueError(reason)


def admitted(ball, k):
    return len(ball['interior']) + min(map(len, ball['bases'])) - 1 <= k <= len(ball['closed'])


def target(model, k, catalogue, facet, policy, stats):
    """Geometry only: neither component state, time, nor prior_count exists here."""
    seen = set()
    while facet not in seen:
        seen.add(facet)
        stats['geometry_table_reads'] += 1
        key = model.key[facet]
        ball = model.balls[key]
        if key in catalogue and (admitted(ball, k) or MUTANT == 'admit_global_key_without_K'):
            return key
        if key in catalogue:
            stats['present_but_order_inadmissible'] += 1
        intruders = sorted(ball['interior'] - set(facet))
        need(intruders, 'complete_catalogue_has_weak_terminal')
        outsider = intruders[0 if policy == 'first' else -1]
        removed = min(model.support[facet])
        following = tuple(sorted((set(facet) - {removed}) | {outsider}))
        next_key = model.key[following]
        next_ball = model.balls[next_key]
        need(len(following) == k, 'exchange_cardinality')
        need(next_ball['radius'] <= ball['radius'], 'exchange_radius_nonincreasing')
        if next_ball['radius'] == ball['radius']:
            need(next_key == key, 'equal_radius_key_identity')
            need(len(set(following) & ball['shell']) + 1 == len(set(facet) & ball['shell']),
                 'equal_radius_selected_shell_decreases')
            stats['equal_radius_steps'] += 1
        else:
            stats['strict_radius_steps'] += 1
        coface = tuple(sorted(set(facet) | set(following)))
        need(model.key[coface] == key, 'exchange_coface_has_original_ball')
        facet = following
    raise ValueError('descent_cycle')


def build_graphs(model, kmax, policy, reverse=False):
    stats = Counter()
    catalogue = {key for key, ball in model.balls.items()
                 if len(ball['interior']) + min(map(len, ball['bases'])) <= min(kmax + 1, len(model.ids))}
    graphs = []
    for k in range(1, kmax + 1):
        vertices = {}
        edges = []
        for key in sorted(catalogue, reverse=reverse):
            ball = model.balls[key]
            if not admitted(ball, k):
                continue
            interior = ball['interior']
            if k <= len(interior):
                representatives = [tuple(sorted(interior)[:k])]
                local_cover = ball['closed']
            else:
                local = model.shell_quotient(ball, k)
                representatives = [tuple(sorted(interior | set(min(c)))) for c in local]
                local_cover = frozenset(x for c in local for f in c for x in (interior | set(f)))
            contribution = ball['closed'] - local_cover
            targets = []
            for facet in representatives:
                visible = set(vertices) if MUTANT == 'consult_partial_calendar' else catalogue
                terminal = target(model, k, visible, facet, policy, stats)
                need(model.balls[terminal]['radius'] <= model.level[facet] < ball['radius'],
                     'each_emitted_arc_strictly_descends_from_consumer')
                targets.append(terminal)
                edges.append((key, terminal))
                stats['representatives'] += 1
            if MUTANT == 'drop_unary_contribution' and len(representatives) == 1:
                contribution = frozenset()
            vertices[key] = dict(radius=ball['radius'], contribution=contribution,
                                 reps=representatives, targets=targets)
        need(all(a in vertices and b in vertices for a, b in edges), 'every_arc_endpoint_is_admitted')
        graphs.append(dict(vertices=vertices, edges=edges, catalogue=catalogue))
        stats['vertices'] += len(vertices)
        stats['arcs'] += len(edges)
    return graphs, stats


def cut_graph(graph, radius, closed):
    active = {key for key, vertex in graph['vertices'].items()
              if (vertex['radius'] <= radius if closed else vertex['radius'] < radius)}
    if MUTANT == 'drop_vertex_activation':
        active = set(graph['vertices'])
    edges = [(a, b) for a, b in graph['edges'] if a in active and b in active]
    components = groups(sorted(active), edges)
    index = {key: component for component in components for key in component}
    covers = {component: frozenset(x for key in component
              for x in graph['vertices'][key]['contribution']) for component in components}
    return components, index, covers


def event_signature(kind, parents, before, after, blocks):
    return kind, parents, tuple(sorted(before)), tuple(sorted(after)), blocks


def verify(model, kmax, graphs, snapshots, events, policy):
    stats = Counter()
    resolver_stats = Counter()
    terminal_stats = Counter()
    terminals = []
    for k, graph in enumerate(graphs, 1):
        terminals.append({facet: target(model, k, graph['catalogue'], facet, policy, terminal_stats)
                          for facet in combinations(model.ids, k)})
    rows = []
    for radius in sorted(set(model.level.values())):
        before = [cut_graph(g, radius, False) for g in graphs]
        for closed in (False, True):
            assignments = []
            current = [cut_graph(g, radius, closed) for g in graphs]
            for k, (graph, cut, state) in enumerate(zip(graphs, current, snapshots[radius, closed]), 1):
                components, index, covers = cut
                members = defaultdict(set)
                assigned = {}
                for facet, key in terminals[k - 1].items():
                    if not (model.level[facet] <= radius if closed else model.level[facet] < radius):
                        continue
                    need(key in index, 'active_facet_has_active_static_terminal')
                    component = index[key]
                    assigned[facet] = component
                    members[component].add(facet)
                    stats['facet_cut_checks'] += 1
                need({frozenset(c) for c in members.values()} == set(model.gamma(k, radius, closed)),
                     'graph_reconstructs_all_Gamma_components')
                need(set(members) == set(components), 'no_spurious_or_missing_graph_component')
                dynamic_roots = set()
                for component, facets in members.items():
                    need(covers[component] == frozenset(x for facet in facets for x in facet),
                         'dated_D_reconstructs_exact_point_cover')
                    roots = {state.root(state.anchors[key][1]) for key in component}
                    need(len(roots) == 1, 'all_static_anchors_normalize_to_one_nominal_root')
                    root = next(iter(roots))
                    dynamic_roots.add(root)
                    need(state.nodes[root].coverage == covers[component], 'nominal_cover_identity')
                need(dynamic_roots == state.roots, 'nominal_component_bijection')
                assignments.append(assigned)
                stats['order_cuts'] += 1
                rows.append([k, str(radius), closed, sorted(sorted(c) for c in members.values())])
                if not closed:
                    for vertex in graph['vertices'].values():
                        if vertex['radius'] != radius:
                            continue
                        for facet, key in zip(vertex['reps'], vertex['targets']):
                            nominal = resolve(model, state, facet, radius, False, resolver_stats, [])
                            need(nominal == state.root(state.anchors[key][1]),
                                 'static_target_normalized_equals_nominal_prelot_resolve')
                            stats['prelot_target_checks'] += 1
            for k in range(2, kmax + 1):
                vertical = {}
                for facet, upper in assignments[k - 1].items():
                    images = {assignments[k - 2][lower] for lower in combinations(facet, k - 1)}
                    need(len(images) == 1, 'subfacets_have_one_closed_or_open_vertical_image')
                    image = next(iter(images))
                    need(vertical.setdefault(upper, image) == image, 'vertical_independent_of_upper_facet')
                    stats['vertical_facet_checks'] += 1
                if closed:
                    for key, vertex in graphs[k - 1]['vertices'].items():
                        if vertex['radius'] != radius or vertex['reps']:
                            continue
                        lower_index = before[k - 2][1] if MUTANT == 'vertical_open' else current[k - 2][1]
                        need(key in lower_index, 'upper_birth_samekey_lower_anchor_is_active_closed')
                        need(vertical[current[k - 1][1][key]] == current[k - 2][1][key],
                             'upper_birth_image_uses_closed_same_ball_anchor')
                        stats['vertical_birth_anchor_checks'] += 1
        for k, graph in enumerate(graphs, 1):
            _, old_index, old_covers = before[k - 1]
            actual = []
            for component, after in current[k - 1][2].items():
                new = {key for key in component if graph['vertices'][key]['radius'] == radius}
                if not new:
                    continue
                parents = {old_index[key] for key in component if key in old_index}
                parent_count = len(parents)
                if MUTANT == 'count_keys_not_roots':
                    parent_count = len({target for key in new for target in graph['vertices'][key]['targets']})
                old_cover = frozenset(x for parent in parents for x in old_covers[parent])
                if parent_count == 1 and old_cover == after:
                    stats['inert_events'] += 1
                    continue
                kind = 'birth' if not parent_count else 'merge' if parent_count > 1 else 'growth'
                if not parents:
                    need(len(new) == 1, 'distinct_parentless_samelevel_blocks_never_coalesce')
                if MUTANT == 'binary_same_level' and parent_count > 2:
                    parent_count = 2
                actual.append(event_signature(kind, parent_count, old_cover, after, len(new)))
                stats[kind + '_events'] += 1
            expected = [event_signature(e['kind'], len(e['parents']), e['before'], e['after'], e['blocks'])
                        for e in events if e['k'] == k and e['radius'] == str(radius)]
            need(sorted(actual) == sorted(expected), 'simultaneous_graph_event_matches_dynamic_anchor_model')
    return dict(stats), hashlib.sha256(json.dumps(rows, separators=(',', ':')).encode()).hexdigest()


def fixture_checks(name, model, graphs):
    checks = {}
    if name == 'pair_terminal':
        graph = graphs[1]
        need(len(graph['vertices']) == 1 and not graph['edges'], 'terminal_isolated_vertex')
        need(not cut_graph(graph, Q(0), True)[0], 'terminal_not_born_at_zero')
        checks['omitted_vertex_activation_killed'] = True
    if name == 'triangle_reused_root':
        graph = graphs[0]
        candidates = []
        for vertex in graph['vertices'].values():
            _, old_index, _ = cut_graph(graph, vertex['radius'], False)
            keys = set(vertex['targets'])
            if len(keys) >= 2 and len({old_index[key] for key in keys}) == 1:
                candidates.append(str(vertex['radius']))
        need(candidates, 'distinct_target_keys_can_share_one_prelot_parent')
        checks['distinct_keys_are_not_parent_count'] = candidates
    if name == 'square':
        before = cut_graph(graphs[0], Q(1), False)
        after = cut_graph(graphs[0], Q(1), True)
        need(len(before[0]) == 4 and len(after[0]) == 1, 'one_four_way_atomic_merge')
        ballkey = model.key[(0, 1, 2, 3)]
        need(ballkey not in cut_graph(graphs[1], Q(2), False)[1] and
             ballkey in cut_graph(graphs[1], Q(2), True)[1], 'vertical_open_cut_has_no_same_ball_anchor')
        checks['simultaneous_arity'] = 4
        checks['vertical_open_cut_killed'] = True
    if name == 'ABCZ_growth':
        graph = graphs[2]
        key = model.key[(0, 1, 2, 3)]
        vertex = graph['vertices'][key]
        need(vertex['radius'] == 25 and vertex['contribution'] == {3}, 'ABCZ_unary_adds_Z')
        need(len(vertex['targets']) == 1, 'ABCZ_is_a_unary_topological_continuation')
        _, index, covers = cut_graph(graph, Q(25), True)
        component = index[key]
        stripped = frozenset(x for b in component if b != key for x in graph['vertices'][b]['contribution'])
        need(covers[component] == {0, 1, 2, 3} and stripped == {0, 1, 2}, 'delete_unary_payload_loses_Z')
        checks['delete_unary_payload_killed'] = True
    if name == 'equal_radius_admitted_window':
        stats = Counter()
        facet = (0, 1, 2, 3)
        key = model.key[facet]
        need(model.balls[key]['radius'] == 4225, 'equal_radius_fixture_level')
        need(key in graphs[3]['catalogue'] and not admitted(model.balls[key], 4), 'global_key_exists_but_K4_inadmissible')
        terminal = target(model, 4, graphs[3]['catalogue'], facet, 'first', stats)
        need(terminal != key and stats['equal_radius_steps'] > 0, 'real_samekey_equalradius_exchange_then_smaller_terminal')
        consumers = [str(v['radius']) for v in graphs[3]['vertices'].values() if facet in v['reps']]
        need(consumers, 'fixture_facet_is_a_produced_strict_representative')
        checks.update(equal_radius_steps=stats['equal_radius_steps'], consumer_levels=consumers,
                      present_but_inadmissible_killed=True)
    return checks


def run():
    fixtures = [
        ('pair_terminal', [(0, 0, 0), (2, 0, 0)], 2),
        ('triangle_reused_root', [(0, 0, 0), (4, 0, 0), (1, 3, 0)], 3),
        ('square', [(0, 0, 0), (2, 0, 0), (2, 2, 0), (0, 2, 0)], 4),
        ('ABCZ_growth', [(1, 8, 0), (5, 10, 0), (9, 8, 0), (5, 0, 0)], 4),
        ('ABCZ_doubled', [(1, 8, 0), (5, 10, 0), (9, 8, 0), (5, 0, 0),
                          (101, 8, 0), (105, 10, 0), (109, 8, 0), (105, 0, 0)], 3),
        ('equal_radius_admitted_window', [(35, 100, 0), (139, 48, 0), (152, 139, 0), (48, 139, 0),
                                         (100, 36, 0), (101, 37, 0), (99, 37, 0), (100, 200, 0)], 6),
        ('tetra_interior', [(0, 0, 0), (8, 0, 0), (0, 8, 0), (0, 0, 8), (1, 1, 1)], 5),
    ]
    output = []
    totals = Counter()
    production_gamma_queries = 0
    for name, points, kmax in fixtures:
        model = Model(points)
        original_gamma = model.gamma

        def forbidden_gamma(*args, **kwargs):
            nonlocal production_gamma_queries
            production_gamma_queries += 1
            raise ValueError('production_must_not_query_Gamma')

        model.gamma = forbidden_gamma
        first, first_stats = build_graphs(model, kmax, 'first')
        last, last_stats = build_graphs(model, kmax, 'last', reverse=True)
        reordered, _ = build_graphs(model, kmax, 'first', reverse=True)
        for a, b in zip(first, reordered):
            need(a['vertices'] == b['vertices'] and sorted(a['edges']) == sorted(b['edges']),
                 'same_geometry_policy_is_identical_under_reversed_block_schedule')
        model.gamma = original_gamma
        # Calendar-dependent authority is constructed only AFTER both graphs.
        snapshots, _, _, events = produce(model, kmax)
        checks, digest = verify(model, kmax, first, snapshots, events, 'first')
        last_checks, last_digest = verify(model, kmax, last, snapshots, events, 'last')
        need(digest == last_digest, 'intruder_policy_and_reverse_block_schedule_preserve_object')
        changed = sum(a['targets'] != last[k]['vertices'][key]['targets']
                      for k, graph in enumerate(first) for key, a in graph['vertices'].items())
        totals.update(checks)
        totals['changed_target_blocks'] += changed
        row = dict(name=name, n=len(points), kmax=kmax, digest=digest, first_work=dict(first_stats),
                   last_work=dict(last_stats), checks=checks, last_checks=last_checks,
                   changed_target_blocks=changed, named_checks=fixture_checks(name, model, first))
        output.append(row)
    need(totals['changed_target_blocks'] > 0, 'alternative_intruder_policy_is_nonvacuous')
    print(json.dumps(dict(schema='mhgp7-private-static-anchor-graph-oracle-v1',
                         status='passed_relative_bounded', public_status='not_claimed',
                         production_Gamma_queries=production_gamma_queries, fixtures=output, totals=dict(totals)),
                     indent=2, sort_keys=True))


if __name__ == '__main__':
    try:
        mutants = {'drop_vertex_activation', 'drop_unary_contribution', 'count_keys_not_roots',
                   'binary_same_level', 'vertical_open', 'admit_global_key_without_K', 'consult_partial_calendar'}
        if len(sys.argv) == 3 and sys.argv[1] == '--mutant':
            need(sys.argv[2] in mutants, 'known_mutant_required')
            MUTANT = sys.argv[2]
        else:
            need(len(sys.argv) == 1, 'no_unknown_arguments')
        run()
    except (ValueError, KeyError) as error:
        print(str(error), file=sys.stderr)
        sys.exit(1)
