#!/usr/bin/env python3
"""Temoin algebrique de perte des branches ouvertes dans un plateau, sans natif."""
import json


def need(condition, message):
    if not condition:
        raise RuntimeError(message)


def plateau(edges):
    """Modele du noyau par taille, puis contraction, pour trois naissances au rang 0."""
    n, rank = 3, 1
    parent = list(range(n))
    size, smallest, last = [1] * n, list(range(n)), [None] * n
    attach_parent, attach_rank = [None] * n, [0] * n
    events, event_cell, cell_top = [], [], []

    def root(x):
        while parent[x] != x:
            parent[x] = parent[parent[x]]
            x = parent[x]
        return x

    def top(x):
        return ['birth', x] if last[x] is None else ['event', last[x]]

    for cell, edge in enumerate(edges):
        x = root(edge[0])
        for leaf in edge[1:]:
            y = root(leaf)
            if x == y:
                continue
            a, b = top(x), top(y)
            survivor, loser = (y, x) if size[x] < size[y] else (x, y)
            ml = min(smallest[x], smallest[y])
            events.append(dict(rank=rank, a=a, b=b, minleaf=ml, surv=survivor))
            event_cell.append(cell)
            parent[loser] = survivor
            size[survivor] += size[loser]
            smallest[survivor] = ml
            last[survivor] = len(events) - 1
            attach_parent[loser], attach_rank[loser] = survivor, rank
            x = survivor
        cell_top.append(top(x))
    need(len(events) == n - 1 and len({root(i) for i in range(n)}) == 1, 'one component')

    # Les deux evenements de rang 1 sont lies ; une seule classe, donc une fusion ternaire.
    need(events[1]['a'] == ['event', 0], 'chosen same-rank link')
    children = sorted(op[1] for ev in events for op in [ev['a'], ev['b']] if op[0] == 'birth')
    need(children == list(range(n)), 'three external operands')
    survivor_values, survivor_offsets = [], [0]
    for leaf in range(n):
        survivor_values.extend(e for e, ev in enumerate(events) if ev['surv'] == leaf)
        survivor_offsets.append(len(survivor_values))
    registry = dict(k=1, births=n, root=n, birth_key=list(range(n)), birth_node=list(range(n)),
                    rank=[0, 0, 0, rank], parent=[n, n, n, None], minleaf=[0, 1, 2, 0],
                    children=dict(off=[0, 0, 0, 0, 3], val=children), lower=[],
                    cell_node=[n if t[0] == 'event' else t[1] for t in cell_top],
                    event_cell=event_cell, attach_parent=attach_parent, attach_rank=attach_rank,
                    survivor_events=dict(off=survivor_offsets, val=survivor_values),
                    event_rank=[ev['rank'] for ev in events], event_node=[n] * len(events))
    return dict(events=events, registry=registry)


def main():
    graphs = [[[0, 1], [0, 2]], [[0, 1], [1, 2]]]
    a, b = [plateau(g) for g in graphs]
    need(a == b, 'all stored mathematical fields coincide')
    # Avant le rang 1, les trois composantes sont encore les trois naissances.
    open_branches = [[sorted(set(edge)) for edge in g] for g in graphs]
    need(open_branches[0][1] != open_branches[1][1], 'retained second edge has different ant')
    need(a['registry']['event_cell'] == [0, 1], 'both edges are retained')
    print(json.dumps(dict(scope='algebraic_single_plateau_no_native_execution', graphs=graphs,
                          ant_at_open_cut=open_branches, same_binary_events=True, same_registry=True,
                          common=a), indent=2, sort_keys=True))


if __name__ == '__main__':
    main()
