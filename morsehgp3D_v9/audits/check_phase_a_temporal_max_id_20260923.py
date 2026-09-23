#!/usr/bin/env python3
"""Abstract phase-A graph lemma check; NOT a FULL engine or GPU qualification."""

from __future__ import annotations

import random


class DSU:
    def __init__(self, count: int) -> None:
        self.parent = list(range(count))

    def find(self, vertex: int) -> int:
        while self.parent[vertex] != vertex:
            vertex = self.parent[vertex]
        return vertex

    def join(self, a: int, b: int) -> None:
        a, b = self.find(a), self.find(b)
        if a != b:
            self.parent[max(a, b)] = min(a, b)


# A block is (exact-level rank, earlier target vertices, has contribution).
Block = tuple[int, tuple[int, ...], bool]


def components(count: int, edges: list[tuple[int, int, int]],
               rank: int, closed: bool) -> DSU:
    result = DSU(count)
    for weight, source, target in edges:
        if weight < rank or (closed and weight == rank):
            result.join(source, target)
    return result


def grouped_blocks(blocks: list[Block], sites: int, rank: int,
                   closed: DSU) -> list[tuple[int, ...]]:
    groups: dict[int, list[int]] = {}
    for index, block in enumerate(blocks):
        if block[0] == rank:
            vertex = sites + index
            groups.setdefault(closed.find(vertex), []).append(vertex)
    return [tuple(group) for group in sorted(groups.values(), key=lambda g: g[0])]


def check_one(sites: int, blocks: list[Block]) -> None:
    count = sites + len(blocks)
    levels = [0] * sites + [block[0] for block in blocks]
    ranks = sorted({block[0] for block in blocks})
    assert all(level > 0 for level in levels[sites:])
    assert all(blocks[i - 1][0] <= blocks[i][0] for i in range(1, len(blocks)))
    edges = [(level, sites + i, target)
             for i, (level, targets, _) in enumerate(blocks)
             for target in targets]
    assert all(target < source and levels[target] < level
               for level, source, target in edges)

    # Chronological reference: all roots of a plateau are read BEFORE any
    # union, then groups are closed in first-block order as in order_lot.
    anchors = list(range(sites)) + [-1] * len(blocks)
    node_next = list(range(sites))
    chronological = []

    def live(token: int) -> int:
        while node_next[token] != token:
            token = node_next[token]
        return token

    for rank in ranks:
        present = [sites + i for i, block in enumerate(blocks) if block[0] == rank]
        roots = {vertex: tuple(sorted({live(anchors[target])
                 for target in blocks[vertex - sites][1]})) for vertex in present}
        lot = DSU(count)
        first_owner: dict[int, int] = {}
        for vertex in present:
            for root in roots[vertex]:
                if root in first_owner:
                    lot.join(vertex, first_owner[root])
                else:
                    first_owner[root] = vertex
        for group in grouped_blocks(blocks, sites, rank, lot):
            parents = tuple(sorted({root for vertex in group for root in roots[vertex]}))
            contributions = tuple(vertex for vertex in group if blocks[vertex - sites][2])
            assert parents or (len(group) == 1 and len(contributions) == 1)
            created = len(parents) != 1
            if created:
                target = len(node_next)
                node_next.append(target)
                for parent in parents:
                    node_next[parent] = target
            else:
                target = parents[0]
            for vertex in group:
                anchors[vertex] = target
            chronological.append((rank, group, parents, contributions, target, created))

    # Offline structure: closed components group blocks; open components
    # identify parents. Neither operation knows historical node IDs yet.
    description = []
    marks = {site: site for site in range(sites)}
    next_id = sites
    for rank in ranks:
        opened = components(count, edges, rank, False)
        closed = components(count, edges, rank, True)
        for group in grouped_blocks(blocks, sites, rank, closed):
            parents = tuple(sorted({opened.find(target) for vertex in group
                                   for target in blocks[vertex - sites][1]}))
            contributions = tuple(vertex for vertex in group if blocks[vertex - sites][2])
            assert parents or (len(group) == 1 and len(contributions) == 1)
            created = len(parents) != 1
            new_id = None
            if created:
                new_id = next_id
                marks[group[0]] = new_id
                next_id += 1
            description.append((rank, group, parents, contributions, new_id, created))

    # The maximum creation ID marked in an OPEN component is precisely its
    # current canonical historical node. Future marks are isolated/excluded.
    reconstructed = []
    for rank, group, parent_components, contributions, new_id, created in description:
        opened = components(count, edges, rank, False)
        maximum: dict[int, int] = {}
        for vertex, node_id in marks.items():
            if levels[vertex] < rank:
                component = opened.find(vertex)
                maximum[component] = max(maximum.get(component, -1), node_id)
        parents = tuple(sorted(maximum[component] for component in parent_components))
        target = new_id if created else parents[0]
        reconstructed.append((rank, group, parents, contributions, target, created))
    assert chronological == reconstructed, (sites, blocks, chronological, reconstructed)

    # A deterministically tie-broken minimum spanning forest must preserve
    # every strict and closed prefix partition, including equal-rank edges.
    forest = DSU(count)
    kept = []
    for _, edge in sorted(enumerate(edges), key=lambda item: (item[1][0], item[0])):
        _, source, target = edge
        if forest.find(source) != forest.find(target):
            forest.join(source, target)
            kept.append(edge)
    for rank in ranks:
        for closed in (False, True):
            complete = components(count, edges, rank, closed)
            reduced = components(count, kept, rank, closed)
            for a in range(count):
                for b in range(a):
                    assert (complete.find(a) == complete.find(b)) == (
                        reduced.find(a) == reduced.find(b))


def main() -> None:
    # Three deliberately different plateaus: three-parent merge, an inert
    # shared-parent group with duplicate facets, and birth/continuation/merge.
    fixtures = [
        (3, [(1, (0, 1), False), (1, (1, 2), False)]),
        (1, [(1, (0, 0), False), (1, (0,), False)]),
        (2, [(1, (), True), (2, (2,), True), (3, (3, 0), False),
             (4, (4, 1), False)]),
        (0, [(1, (), True), (2, (0,), False)]),
    ]
    for sites, blocks in fixtures:
        check_one(sites, blocks)

    rng = random.Random(20260923)
    for _ in range(3000):
        sites = rng.randrange(0, 7)
        length = rng.randrange(1, 25)
        rank = 1
        blocks: list[Block] = []
        for index in range(length):
            if index:
                rank += rng.choice((0, 0, 0, 1, 1, 2))
            eligible = [target for target in range(sites + index)
                        if target < sites or blocks[target - sites][0] < rank]
            targets = tuple(rng.sample(eligible, rng.randrange(min(4, len(eligible)) + 1)))
            blocks.append((rank, targets, not targets or bool(rng.randrange(2))))
        check_one(sites, blocks)
    print("OK: 4 fixtures + 3000 abstract histories; groups, parents, canonical IDs,"
          " and MSF open/closed prefix partitions")


if __name__ == "__main__":
    main()
