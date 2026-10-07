#!/usr/bin/env python3
"""Collecte ouverte avant/apres union ; modele exact d'entrees T, sans geometrie."""
import json
import random


def need(ok, message):
    if not ok:
        raise RuntimeError(message)


def run(n, cells):
    up, size, last = list(range(n)), [1] * n, [None] * n
    ap, ar, old_top = [None] * n, [0] * n, [None] * n
    events, witnesses, records = [], [], []
    reference = list(range(n))

    def find(parent, x):
        while parent[x] != x:
            x = parent[x]
        return x

    def top(x):
        return ('b', x) if last[x] is None else ('e', last[x])

    def open_root(x, rank):
        while ap[x] is not None and ar[x] < rank:
            x = ap[x]
        return x

    def open_sets(leaves, rank):
        roots = [open_root(i, rank) for i in range(n)]
        return {tuple(i for i in range(n) if roots[i] == roots[leaf]) for leaf in leaves}

    def token_members(token):
        if token[0] == 'b':
            return {token[1]}
        ev = events[token[1]]
        return token_members(ev['a']) | token_members(ev['b'])

    current_rank, frozen = None, None
    for ci, (rank, targets) in enumerate(cells):
        need(current_rank is None or current_rank <= rank, 'rank order')
        if rank != current_rank:
            current_rank = rank
            frozen = [find(reference, i) for i in range(n)]
        leaves = []
        for kind, index in targets:
            if kind == 'b':
                leaves.append(index)
            else:
                need(index < ci and cells[index][0] < rank, 'cell target strictly earlier')
                leaves.append(witnesses[index])
        expected = {tuple(i for i in range(n) if frozen[i] == frozen[leaf]) for leaf in leaves}
        need(open_sets(leaves, rank) == expected, 'before unions')
        before = len(events)
        x = find(up, leaves[0])
        for leaf in leaves[1:]:
            y = find(up, leaf)
            if x == y:
                continue
            a, b = top(x), top(y)
            survivor, loser = (y, x) if size[x] < size[y] else (x, y)
            # Sauvegarde locale facultative : premier remplacement du sommet de ce survivant au rang r.
            if last[survivor] is None or events[last[survivor]]['rank'] != rank:
                old_top[survivor] = top(survivor)
            events.append(dict(rank=rank, a=a, b=b, surv=survivor))
            up[loser] = survivor
            size[survivor] += size[loser]
            last[survivor] = len(events) - 1
            ap[loser], ar[loser] = survivor, rank
            x = survivor
        witnesses.append(x)  # Temoin immuable : ne pas le remplacer ensuite par sa racine courante.
        need(open_sets(leaves, rank) == expected, 'after same-rank unions')
        tokens = []
        for root in sorted({open_root(leaf, rank) for leaf in leaves}):
            token = old_top[root] if last[root] is not None and events[last[root]]['rank'] == rank else top(root)
            need(token is not None, 'snapshot exists')
            need(token[0] == 'b' or events[token[1]]['rank'] < rank, 'open token')
            tokens.append(token)
        need({tuple(sorted(token_members(t))) for t in tokens} == expected, 'online tokens')
        records.append(dict(rank=rank, leaves=leaves, expected=expected, count=len(tokens),
                            retained=len(events) > before, tokens=tokens))
        for leaf in leaves[1:]:
            reference[find(reference, leaf)] = find(reference, leaves[0])

    # M : classes d'evenements lies de meme rang, puis numerotation (rang, plus petite naissance).
    local = list(range(len(events)))
    for e, ev in enumerate(events):
        for kind, i in (ev['a'], ev['b']):
            if kind == 'e' and events[i]['rank'] == ev['rank']:
                local[find(local, i)] = find(local, e)
    groups = {}
    for e in range(len(events)):
        groups.setdefault(find(local, e), []).append(e)
    roots = sorted(groups, key=lambda root: (events[root]['rank'], min(token_members(('e', root)))))
    event_node = [None] * len(events)
    node_sets = {i: (i,) for i in range(n)}
    for node, root in enumerate(roots, n):
        node_sets[node] = tuple(sorted(token_members(('e', root))))
        for e in groups[root]:
            event_node[e] = node

    def component_at(leaf, cut):
        x = leaf
        while ap[x] is not None and ar[x] <= cut:
            x = ap[x]
        choices = [e for e, ev in enumerate(events) if ev['surv'] == x and ev['rank'] <= cut]
        return event_node[choices[-1]] if choices else x

    counts, rows, retained = [], [], []
    for ci, rec in enumerate(records):
        rank, leaves = rec['rank'], rec['leaves']
        need(open_sets(leaves, rank) == rec['expected'], 'final history still open')
        nodes = sorted({component_at(leaf, rank - 1) for leaf in leaves})
        need({node_sets[node] for node in nodes} == rec['expected'], 'final nodes at r-1')
        history_tokens = []
        for root in sorted({open_root(leaf, rank) for leaf in leaves}):
            earlier = [e for e, ev in enumerate(events) if ev['surv'] == root and ev['rank'] < rank]
            history_tokens.append(('e', earlier[-1]) if earlier else ('b', root))
        filled = sorted(t[1] if t[0] == 'b' else event_node[t[1]] for t in history_tokens)
        need(filled == nodes, 'history tokens filled before M, translated after M')
        tokens = sorted(t[1] if t[0] == 'b' else event_node[t[1]] for t in rec['tokens'])
        need(nodes == tokens, 'online snapshot and final component_at agree')
        if rec['retained']:
            counts.append(rec['count'])
            retained.append(ci)
            rows.append(nodes)
    offsets, values = [0], []
    for count, row in zip(counts, rows):
        need(count == len(row), 'count and fill agree')
        values.extend(row)
        offsets.append(len(values))
    need(len(events) <= n - 1, 'events bound')
    need(len(values) <= sum(len(cells[i][1]) for i in retained), 'representatives bound')
    return dict(births=n, cells=len(cells), events=len(events), retained=retained,
                ant_offsets=offsets, ant_values=values, ant_total=len(values),
                retained_representatives=sum(len(cells[i][1]) for i in retained))


def births(*ids):
    return [('b', i) for i in ids]


def main():
    fixtures = [
        ('witness_left', 3, [(1, births(0, 1)), (1, births(0, 2))]),
        ('witness_right', 3, [(1, births(0, 1)), (1, births(1, 2))]),
        ('redundancy', 4, [(1, births(0, 1)), (1, births(0, 1, 2)),
                           (1, births(1, 2)), (1, births(0, 1, 2, 3))]),
        ('cell_targets', 5, [(1, births(0, 1)), (2, [('c', 0), ('b', 2), ('b', 2)]),
                             (2, births(1, 2, 3)), (3, [('c', 1), ('b', 4)]),
                             (4, births(0, 0))]),
        ('prefix_family', 7, [(1, births(*range(i))) for i in range(2, 8)]),
    ]
    outputs = {name: run(n, cells) for name, n, cells in fixtures}
    rng = random.Random(120710)
    random_cases = 64
    for _ in range(random_cases):
        n = rng.randrange(3, 11)
        cells = []
        for rank in range(1, 5):
            for _ in range(rng.randrange(1, 5)):
                earlier = [i for i, (r, _) in enumerate(cells) if r < rank]
                targets = births(*(rng.randrange(n) for _ in range(rng.randrange(1, n + 2))))
                if earlier and rng.randrange(2):
                    targets[0] = ('c', rng.choice(earlier))
                cells.append((rank, targets))
        run(n, cells)
    need(outputs['prefix_family']['events'] == 6 and outputs['prefix_family']['ant_total'] == 27,
         'sum ant is not bounded by births-1')
    print(json.dumps(dict(scope='abstract_T_inputs_not_geometry_or_native', fixtures=outputs,
                          random_cases=random_cases, verified=['before_vs_after_same_rank', 'final_history',
                          'online_tokens_vs_final_nodes', 'history_tokens_vs_final_nodes',
                          'count_vs_fill', 'cell_targets', 'strict_cut']),
                     sort_keys=True, indent=2))


if __name__ == '__main__':
    main()
