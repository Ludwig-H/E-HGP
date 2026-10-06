#!/usr/bin/env python3
"""Bounded exact graph witnesses for O7; source verification, no native run."""
import argparse
import hashlib
import itertools
import json
from fractions import Fraction
from pathlib import Path
import subprocess

HERE = Path(__file__).resolve().parent
CHECKS = 0


def require(condition, message):
    global CHECKS
    CHECKS += 1
    if not condition:
        raise RuntimeError(message)


def rational(value):
    value = Fraction(value)
    return [value.numerator, value.denominator]


def make_births(n):
    return [{'rank': Fraction(0), 'children': (), 'leaves': (i,)} for i in range(n)]


def find(parent, a):
    while parent[a] != a:
        a = parent[a]
    return a


def atomic(n, edges):
    nodes = make_births(n)
    parent = list(range(n))
    tops = list(range(n))
    ledger = dict(unions=0, touched_components=0, continuations=0, plateaus=0)
    for level, group in itertools.groupby(sorted(edges), key=lambda edge: edge[0]):
        group = list(group)
        before = {find(parent, a) for _, a, b in group} | {find(parent, b) for _, a, b in group}
        old_top = {root: tops[root] for root in before}
        ledger['plateaus'] += 1
        ledger['touched_components'] += len(before)
        for _, a, b in group:
            a, b = find(parent, a), find(parent, b)
            if a != b:
                lo, hi = sorted((a, b))
                parent[hi] = lo
                ledger['unions'] += 1
        groups = {}
        for old, top in old_top.items():
            groups.setdefault(find(parent, old), []).append(top)
        for root in sorted(groups):
            children = tuple(sorted(groups[root]))
            if len(children) < 2:
                ledger['continuations'] += 1
                continue
            leaves = tuple(sorted(i for child in children for i in nodes[child]['leaves']))
            nodes.append(dict(rank=level, children=children, leaves=leaves))
            tops[root] = len(nodes) - 1
    return nodes, tops[find(parent, 0)], ledger


def kruskal(n, edges):
    nodes = make_births(n)
    parent = list(range(n))
    tops = list(range(n))
    selected = []
    for level, a, b in edges:
        left, right = find(parent, a), find(parent, b)
        if left == right:
            continue
        children = (tops[left], tops[right])
        leaves = tuple(sorted(nodes[children[0]]['leaves'] + nodes[children[1]]['leaves']))
        nodes.append(dict(rank=level, children=children, leaves=leaves))
        lo, hi = sorted((left, right))
        parent[hi] = lo
        tops[lo] = len(nodes) - 1
        selected.append((level, a, b))
    return nodes, tops[find(parent, 0)], selected


def shape(nodes, node, contract_equal=False):
    data = nodes[node]
    if not data['children']:
        return node
    children = []
    for child in data['children']:
        other = nodes[child]
        if contract_equal and other['children'] and other['rank'] == data['rank']:
            children.extend(flat_children(nodes, child, data['rank']))
        else:
            children.append(child)
    children.sort(key=lambda child: min(nodes[child]['leaves']))
    return [rational(data['rank']), [shape(nodes, child, contract_equal) for child in children]]


def flat_children(nodes, node, level):
    out = []
    for child in nodes[node]['children']:
        other = nodes[child]
        if other['children'] and other['rank'] == level:
            out.extend(flat_children(nodes, child, level))
        else:
            out.append(child)
    return out


def cut(edges, n, level):
    parent = list(range(n))
    for weight, a, b in edges:
        if weight <= level:
            a, b = find(parent, a), find(parent, b)
            if a != b:
                parent[max(a, b)] = min(a, b)
    groups = {}
    for a in range(n):
        groups.setdefault(find(parent, a), []).append(a)
    return sorted(groups.values())


def ancestor_sweep(nodes, birth_count, levels):
    parent = list(range(len(nodes)))
    sizes = [1] * len(nodes)
    tops = list(range(len(nodes)))
    next_node = birth_count
    activations = unions = 0
    rows = []
    for level in levels:
        while next_node < len(nodes) and nodes[next_node]['rank'] <= level:
            for child in nodes[next_node]['children']:
                a, b = find(parent, next_node), find(parent, child)
                require(a != b, 'canonical children must be disjoint')
                if sizes[a] < sizes[b]:
                    a, b = b, a
                parent[b] = a
                sizes[a] += sizes[b]
                tops[a] = next_node
                unions += 1
            activations += 1
            next_node += 1
        active = [node for node in nodes[birth_count:] if node['rank'] <= level]
        require(activations == len(active), 'activation formula excludes births')
        require(unions == sum(len(node['children']) for node in active), 'union formula')
        images = [tops[find(parent, seed)] for seed in range(birth_count)]
        rows.append(dict(level=rational(level), activations=activations, unions=unions, birth_images=images))
    return rows


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--repo', default='/workspaces/E-HGP')
    args = parser.parse_args()
    manifest = json.loads((HERE / 'sources.json').read_text())
    pin = manifest['pin']
    for source in manifest['sources']:
        raw = subprocess.check_output(['git', '-C', args.repo, 'show', pin + ':' + source['path']])
        require(hashlib.sha256(raw).hexdigest() == source['sha256'], 'source hash: ' + source['path'])

    points = [(0, 0), (3, 0), (1, 2)]
    edges = []
    gabriel = []
    for a, b in itertools.combinations(range(3), 2):
        level = Fraction(sum((points[a][axis] - points[b][axis]) ** 2 for axis in range(2)), 4)
        mid = [Fraction(points[a][axis] + points[b][axis], 2) for axis in range(2)]
        for c in range(3):
            if c not in (a, b):
                power = sum((Fraction(points[c][axis]) - mid[axis]) ** 2 for axis in range(2)) - level
                require(power > 0, 'third point strictly outside diameter ball')
        edges.append((level, a, b))
        gabriel.append(dict(sites=[a, b], level=rational(level)))
    edges.sort()
    full_nodes, full_root, full_ledger = atomic(3, edges)
    mst_nodes, mst_root, selected = kruskal(3, edges)
    reduced_nodes, reduced_root, reduced_ledger = atomic(3, selected)
    require(shape(full_nodes, full_root) == shape(reduced_nodes, reduced_root), 'MST preserves hierarchy')
    require(full_ledger == dict(unions=2, touched_components=5, continuations=1, plateaus=3), 'raw continuation ledger')
    require(reduced_ledger == dict(unions=2, touched_components=4, continuations=0, plateaus=2), 'MST discards continuation')
    for level in [Fraction(0), Fraction(5, 4), Fraction(2), Fraction(9, 4)]:
        require(cut(edges, 3, level) == cut(selected, 3, level), 'MST closed cuts')

    equal = [(Fraction(1), 0, 1), (Fraction(1), 1, 2)]
    tie_nodes, tie_root, tie_ledger = atomic(3, equal)
    orders = [equal, list(reversed(equal))]
    for order in orders:
        binary, root, tree_edges = kruskal(3, order)
        require(shape(binary, root, True) == shape(tie_nodes, tie_root), 'equal-rank contraction')
        require(shape(binary, root, False) != shape(tie_nodes, tie_root), 'uncontracted tie is binary')
        require(len(tie_nodes[tie_root]['children']) == 3, 'plateau must be ternary')
    strict_levels = [(Fraction(1), 0, 1), (Fraction(2), 1, 2)]
    require(cut(strict_levels, 3, Fraction(1)) == [[0, 1], [2]], 'strict-level cut before second merge')
    wrong_contraction = [(Fraction(1), 0, 1), (Fraction(1), 1, 2)]
    require(cut(wrong_contraction, 3, Fraction(1)) == [[0, 1, 2]], 'cross-rank contraction changes filtration')
    for dataset in [edges, equal, strict_levels]:
        require(all(weight > 0 for weight, _, _ in dataset), 'edges strictly above incident births')

    ancestor = ancestor_sweep(full_nodes, 3, [Fraction(0), Fraction(5, 4), Fraction(2), Fraction(9, 4)])
    tie_ancestor = ancestor_sweep(tie_nodes, 3, [Fraction(0), Fraction(1)])
    require(tie_ancestor[-1]['birth_images'] == [tie_root] * 3, 'closed equal-rank ancestor')
    result = dict(pin=pin, native_runs=0, source_files_verified=len(manifest['sources']), checks=CHECKS,
                  gabriel_triangle=dict(points=points, edges=gabriel, full_ledger=full_ledger, mst_ledger=reduced_ledger,
                                        hierarchy=shape(full_nodes, full_root)),
                  atomic_plateau=dict(hierarchy=shape(tie_nodes, tie_root), tie_orders_verified=len(orders)),
                  cross_rank_contraction=dict(correct_cut=[[0, 1], [2]], wrong_cut=[[0, 1, 2]], level=[1, 1]),
                  ancestor_formula=ancestor, closed_plateau_ancestor=tie_ancestor)
    print(json.dumps(result, sort_keys=True, separators=(',', ':')))


if __name__ == '__main__':
    main()
