#!/usr/bin/env python3
"""Finite combinatorial model, NOT the product or a GPU implementation.

Reference: chronological batches. Candidate: one MSF, weighted ancestors,
component-event predecessors, then doubling continuation pointers. The MSF
is built by ordinary Kruskal here; parallel Boruvka remains a proposal.
"""
from bisect import bisect_left
from collections import defaultdict
from itertools import combinations
import random


def need(ok, why):
    if not ok:
        raise ValueError(why)


def validate(blocks):
    for index, block in enumerate(blocks):
        need(index == 0 or blocks[index - 1][0] <= block[0], "program chronology")
        need(all(0 <= target < index and blocks[target][0] < block[0] for target in block[1]),
             "terminal target must be strictly earlier")
        need(block[1] or block[2], "isolated birth needs contribution")


class Dsu:
    def __init__(self, n):
        self.parent = list(range(n))

    def find(self, x):
        while x != self.parent[x]:
            self.parent[x] = self.parent[self.parent[x]]
            x = self.parent[x]
        return x

    def union(self, a, b):
        a, b = self.find(a), self.find(b)
        if a != b:
            self.parent[max(a, b)] = min(a, b)
        return a != b


def reference(blocks):
    validate(blocks)
    anchors, nodes, successors, actions = [-1] * len(blocks), [], [], []
    at = 0
    def root(node):
        while successors[node] != -1:
            node = successors[node]
        return node
    while at < len(blocks):
        end = at + 1
        level = blocks[at][0]
        while end < len(blocks) and blocks[end][0] == level:
            end += 1
        roots = [sorted({root(anchors[t]) for t in blocks[b][1]}) for b in range(at, end)]
        dsu, owner = Dsu(end - at), {}
        for local, parents in enumerate(roots):
            for parent in parents:
                if parent in owner:
                    dsu.union(local, owner[parent])
                else:
                    owner[parent] = local
        grouped = defaultdict(list)
        for local in range(end - at):
            grouped[dsu.find(local)].append(at + local)
        for members in sorted(grouped.values(), key=lambda group: group[0]):
            parents = sorted({p for b in members for p in roots[b - at]})
            contributions = [b for b in members if blocks[b][2]]
            if len(parents) == 1:
                target = parents[0]
            else:
                need(parents or len(members) == len(contributions) == 1, "distinct births")
                target = len(nodes)
                nodes.append((level, parents))
                successors.append(-1)
                for parent in parents:
                    successors[parent] = target
            if len(parents) != 1 or contributions:
                actions.append((level, parents, contributions, target))
            for b in members:
                anchors[b] = target
        at = end
    return dict(nodes=nodes, successors=successors, actions=actions, anchors=anchors)


def candidate(blocks, mutant="", root_high=True):
    validate(blocks)
    n = len(blocks)
    edges = [(block[0], source, target, ordinal)
             for source, block in enumerate(blocks) for ordinal, target in enumerate(block[1])]
    ordered = sorted(edges, reverse=mutant == "maximum_forest")
    dsu, tree = Dsu(n), [[] for _ in blocks]
    for weight, source, target, _ in ordered:
        if dsu.union(source, target):
            tree[source].append((target, weight))
            tree[target].append((source, weight))
    up, maximum, visited = list(range(n)), [-1] * n, [False] * n
    for root in (range(n - 1, -1, -1) if root_high else range(n)):
        if visited[root]:
            continue
        visited[root] = True
        stack = [root]
        while stack:
            source = stack.pop()
            for target, weight in tree[source]:
                if not visited[target]:
                    visited[target] = True
                    up[target], maximum[target] = source, weight
                    stack.append(target)
    ancestors, maxima = [up], [maximum]
    for _ in range(1, max(1, n.bit_length())):
        p, w = ancestors[-1], maxima[-1]
        ancestors.append([p[p[v]] for v in range(n)])
        maxima.append([max(w[v], w[p[v]]) for v in range(n)])
    query_steps = 0
    def label(vertex, cut, closed):
        nonlocal query_steps
        for power in range(len(ancestors) - 1, -1, -1):
            query_steps += 1
            weight = maxima[power][vertex]
            if weight < cut or (closed and weight == cut):
                vertex = ancestors[power][vertex]
        return vertex
    grouped = defaultdict(list)
    for source, block in enumerate(blocks):
        grouped[(block[0], label(source, block[0], True))].append(source)
    groups = sorted(grouped.items(), key=lambda item: (item[0][0], item[1][0]))
    if mutant == "reverse_plateau_order":
        groups.sort(key=lambda item: (item[0][0], -item[1][0]))
    events = defaultdict(list)
    for index, ((level, component), members) in enumerate(groups):
        if mutant == "forget_silent" and not any(blocks[b][2] for b in members):
            continue
        events[component].append((level, index))
    parents, node_ids, redirects = [], [], []
    count = 0
    for index, ((level, _), members) in enumerate(groups):
        labels = sorted({label(target, level, mutant == "closed_parent_cut")
                         for source in members for target in blocks[source][1]})
        previous = []
        for component in labels:
            history = events[component]
            pos = bisect_left(history, (level, -1)) - 1
            need(pos >= 0, "component-event predecessor missing")
            previous.append(history[pos][1])
        need(all(prior < index for prior in previous), "event predecessor chronology")
        parents.append(previous)
        if len(previous) == 1:
            redirects.append(previous[0])
            node_ids.append(-1)
        else:
            need(previous or len(members) == 1 and blocks[members[0]][2], "distinct births")
            redirects.append(index)
            node_ids.append(count)
            count += 1
    rounds, jump_tests = 0, 0
    while True:
        jumped = [redirects[target] for target in redirects]
        rounds += 1
        jump_tests += len(groups)
        if jumped == redirects:
            break
        redirects = jumped
        need(rounds <= len(groups).bit_length() + 1, "pointer doubling failed to converge")
    nodes, successors, actions, anchors = [None] * count, [-1] * count, [], [-1] * n
    for index, ((level, _), members) in enumerate(groups):
        owner = node_ids[redirects[index]]
        old = sorted({node_ids[redirects[p]] for p in parents[index]})
        need(owner >= 0 and len(old) == len(parents[index]), "component/node identity mismatch")
        contribution = [b for b in members if blocks[b][2]]
        if len(old) != 1:
            nodes[owner] = (level, old)
            for parent in old:
                need(successors[parent] == -1, "parent written by two plateau groups")
                successors[parent] = owner
        if len(old) != 1 or contribution:
            actions.append((level, old, contribution, owner))
        for source in members:
            anchors[source] = owner
    result = dict(nodes=nodes, successors=successors, actions=actions, anchors=anchors)
    work = dict(vertices=n, edges=len(edges), forest_edges=sum(map(len, tree)) // 2,
                groups=len(groups), ancestor_table_entries=2 * n * len(ancestors),
                ancestor_query_steps=query_steps, continuation_rounds=rounds,
                continuation_tests=jump_tests)
    return result, work


def regular_enumeration():
    checked, largest_queries, exponentials_avoided = 0, 0, 0
    for kmax in (5, 10):
        for q in (2, 3, 4):
            for p in range(kmax + 2 - q):
                interior, shell = tuple(range(p)), tuple(range(p, p + q))
                m = p + q
                expected = {}
                for k in range(1, kmax + 1):
                    if k == m - 1:
                        facets = [interior + subset for subset in combinations(shell, q - 1)]
                        need(len(facets) == q and all(len(f) == k for f in facets), "regular facets")
                        expected[k] = dict(facets=facets, contributes=False)
                    elif k == m:
                        expected[k] = dict(facets=[], contributes=True)
                need(len(expected) <= 2 and sum(len(v["facets"]) for v in expected.values()) == q,
                     "regular ball must not enumerate all subsets")
                largest_queries = max(largest_queries, sum(len(v["facets"]) for v in expected.values()))
                exponentials_avoided = max(exponentials_avoided, (1 << m) - 1)
                checked += 1
    return dict(cases=checked, maximum_representatives_per_regular_ball=largest_queries,
                largest_nonempty_power_set_not_materialized=exponentials_avoided,
                note="combinatorial count matching current regular-ball rules, not a new optimization")


def check():
    fixtures = [
        [(0, [], True), (0, [], True), (1, [0, 1], False), (2, [0, 1], False)],
        [(0, [], True), (1, [0], False), (2, [1], True)],
        [(0, [], True), (0, [], True), (0, [], True), (1, [0, 1], False), (1, [1, 2], True)],
        [(0, [], True), (0, [], True), (1, [0], True), (1, [1], True), (2, [2, 3], False)],
    ]
    rng = random.Random(260926)
    for _ in range(3000):
        blocks, level = [], 0
        for index in range(rng.randrange(2, 55)):
            level += rng.randrange(3)
            allowed = [j for j, block in enumerate(blocks) if block[0] < level]
            targets = rng.sample(allowed, rng.randrange(min(4, len(allowed)) + 1))
            if targets and rng.randrange(7) == 0:
                targets.append(targets[0])  # repeated representative, not a distinct parent
            blocks.append((level, targets, not targets or rng.randrange(3) == 0))
        fixtures.append(blocks)
    totals = defaultdict(int)
    for blocks in fixtures:
        witness = reference(blocks)
        for root_high in (False, True):
            result, work = candidate(blocks, root_high=root_high)
            need(result == witness, "event reconstruction differs from chronological witness")
            for name, value in work.items():
                totals[name] += value
    mutants = {}
    for name, index, root_high in (("maximum_forest", 0, False), ("closed_parent_cut", 0, False),
                                   ("forget_silent", 1, True), ("reverse_plateau_order", 3, True)):
        try:
            result, _ = candidate(fixtures[index], mutant=name, root_high=root_high)
            killed = result != reference(fixtures[index])
            outcome = "different_complete_actions_or_anchors"
        except ValueError as error:
            killed, outcome = True, "causal_invariant_refusal: " + str(error)
        need(killed, "surviving mutant: " + name)
        mutants[name] = outcome
    return dict(schema="mhgp9_phase_a_component_event_model_v1", fixtures=len(fixtures),
                tested_rootings=2, comparisons=len(fixtures) * 2, mutants=mutants,
                regular_enumeration=regular_enumeration(), work_totals=dict(totals),
                geometric_catalogue_tested=False, FULL_product_tested=False, GPU_used=False,
                global_subquadratic_claim=False, parallel_implementation_tested=False)


if __name__ == "__main__":
    import json
    print(json.dumps(check(), sort_keys=True, indent=2))
