#!/usr/bin/env python3
"""Hypergraphes bornes, coupes ouvertes et classes binaires ; aucun moteur execute."""
import argparse
import hashlib
import itertools
import json
import subprocess
from pathlib import Path


def need(ok, what):
    if not ok:
        raise RuntimeError(what)


def oracle(dates, cells):
    """Graphe des composantes AVANT chaque plateau, independant des unions binaires."""
    n = len(dates)
    active = list(range(n))
    members = {i: frozenset([i]) for i in range(n)}
    ranks = dict(enumerate(dates))
    children = {i: [] for i in range(n)}
    cell_node, ant = [], []
    next_node, begin = n, 0
    while begin < len(cells):
        rank = cells[begin][0]
        end = begin + 1
        while end < len(cells) and cells[end][0] == rank:
            end += 1
        graph, leaves = {}, []
        for ci in range(begin, end):
            row = []
            for kind, index in cells[ci][1]:
                if kind == 'b':
                    need(0 <= index < n and dates[index] < rank, 'date naissance')
                    row.append(index)
                else:
                    need(kind == 'c' and index < begin and cells[index][0] < rank, 'date cible cellule')
                    row.append(min(members[cell_node[index]]))
            need(row, 'ligne vide')
            leaves.append(row)
            nodes = sorted({active[x] for x in row})
            ant.append(nodes)
            for x in nodes:
                graph.setdefault(x, set()).update(nodes)
        seen, groups = set(), []
        for node in sorted(graph):
            if node in seen:
                continue
            todo, group = [node], set()
            while todo:
                x = todo.pop()
                if x not in group:
                    group.add(x)
                    todo.extend(graph[x] - group)
            seen.update(group)
            if len(group) > 1:
                groups.append(group)
        for group in sorted(groups, key=lambda g: min(min(members[x]) for x in g)):
            children[next_node] = sorted(group)
            members[next_node] = frozenset().union(*(members[x] for x in group))
            ranks[next_node] = rank
            for leaf in members[next_node]:
                active[leaf] = next_node
            next_node += 1
        cell_node.extend(active[row[0]] for row in leaves)
        begin = end
    return members, ranks, children, cell_node, ant


def run(dates, cells):
    n = len(dates)
    need(all(cells[i][0] <= cells[i + 1][0] for i in range(len(cells) - 1)), 'rangs tries')
    members, ranks, children, oracle_cell, ant = oracle(dates, cells)
    up, size, last = list(range(n)), [1] * n, list(range(n))
    attach, attach_rank = [None] * n, [0] * n
    events, element, cell_top, counts = [], [], [], []

    def find(parents, x):
        while parents[x] != x:
            x = parents[x]
        return x

    for ci, (rank, targets) in enumerate(cells):
        leaves = [index if kind == 'b' else element[index] for kind, index in targets]
        before, x = len(events), find(up, leaves[0])
        for leaf in leaves[1:]:
            y = find(up, leaf)
            if x == y:
                continue
            survivor, loser = (y, x) if size[x] < size[y] else (x, y)
            events.append(dict(rank=rank, a=last[x], b=last[y], surv=survivor, cell=ci))
            up[loser] = survivor
            size[survivor] += size[loser]
            last[survivor] = n + len(events) - 1
            attach[loser], attach_rank[loser] = survivor, rank
            x = survivor
        element.append(x)
        cell_top.append(last[x])
        counts.append(len(events) - before)

    # Contraction par connexite des OPERANDES de meme rang, pas par nombre de cellules du plateau.
    local = list(range(len(events)))
    binary_members = [frozenset([i]) for i in range(n)]
    for e, ev in enumerate(events):
        binary_members.append(binary_members[ev['a']] | binary_members[ev['b']])
        for operand in (ev['a'], ev['b']):
            if operand >= n and events[operand - n]['rank'] == ev['rank']:
                local[find(local, operand - n)] = find(local, e)
    groups = {}
    for e in range(len(events)):
        groups.setdefault(find(local, e), []).append(e)
    canonical = {(ranks[v], members[v]): v for v in children if v >= n}
    event_node = [None] * len(events)
    for root, group in groups.items():
        node = canonical[(events[root]['rank'], binary_members[n + root])]
        for e in group:
            event_node[e] = node
    node_of = lambda token: token if token < n else event_node[token - n]
    made_cell = [node_of(t) for t in cell_top]
    # Une cellule inerte peut laisser une naissance, ensuite reunie ailleurs dans son plateau.
    # La cellule retenue laisse un evenement de la classe contractee, donc le noeud du plateau complet.
    need(all(made_cell[i] == oracle_cell[i] for i, d in enumerate(counts) if d), 'noeuds de cellule retenue')
    owners = {}
    for root, group in groups.items():
        node = event_node[root]
        outside = [node_of(ev[axis]) for e in group for ev in [events[e]] for axis in ('a', 'b')
                   if node_of(ev[axis]) != node]
        need(len(outside) == len(set(outside)), 'enfants externes distincts')
        need(sorted(outside) == children[node], 'contraction contre graphe')
        need(len(children[node]) == len(group) + 1, 'q = nombre evenements + 1')
        owners[node] = {events[e]['cell'] for e in group}

    def component_at(leaf, rank):
        need(dates[leaf] <= rank, 'domaine component_at')
        x = leaf
        while attach[x] is not None and attach_rank[x] <= rank:
            x = attach[x]
        choices = [e for e, ev in enumerate(events) if ev['surv'] == x and ev['rank'] <= rank]
        return x if not choices else event_node[choices[-1]]

    rows = saved = fallback = all_children_bad = duplicate_target_rows = 0
    values, copied = [], []
    for ci, (rank, targets) in enumerate(cells):
        if not counts[ci]:
            continue
        z, d = made_cell[ci], counts[ci]
        row_events = [e for e, ev in enumerate(events) if ev['cell'] == ci]
        need(all(event_node[e] == z for e in row_events), 'une cellule dans une classe')
        need(row_events == list(range(row_events[0], row_events[0] + d)), 'bloc contigu')
        witnesses = [index if kind == 'b' else min(members[made_cell[index]]) for kind, index in targets]
        expected = sorted({component_at(l, rank - 1) for l in witnesses})
        need(expected == ant[ci], 'historique contre coupe gelee')
        use_children = len(children[z]) == d + 1
        need(use_children == (owners[z] == {ci}), 'critere exact unicite')
        proposed = children[z] if use_children else expected
        need(proposed == expected, 'raccourci conserve ant')
        rows += 1
        saved += len(targets) if use_children else 0
        fallback += not use_children
        duplicate_target_rows += len(targets) != len(set(targets))
        all_children_bad += children[z] != expected
        values.extend(expected)
        copied.extend(proposed)
    need(values == copied, 'ordre CSR et multiplicites interlignes')
    need(len(events) == n - 1, 'racine unique du modele')
    delta = rows - len(groups)
    need(0 <= fallback <= 2 * delta, 'borne lignes generales')
    return dict(rows=rows, shortcut_rows=rows - fallback, fallback_rows=fallback,
                queries_saved=saved, values=len(values), all_children_counterexamples=all_children_bad,
                duplicate_target_rows=duplicate_target_rows)


def b(*ids):
    return [('b', i) for i in ids]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--repo', required=True, type=Path)
    args = parser.parse_args()
    here = Path(__file__).resolve().parent
    cap = json.loads((here / 'capture.json').read_text())

    def pins():
        for rel, sha in cap['source_sha256'].items():
            raw = subprocess.check_output(['git', 'show', cap['pin'] + ':morsehgp3D_v12/' + rel], cwd=args.repo)
            need(hashlib.sha256(raw).hexdigest() == sha, 'source ' + rel)
        for rel, sha in cap.get('artifact_sha256', {}).items():
            need(hashlib.sha256((here / rel).read_bytes()).hexdigest() == sha, 'artefact ' + rel)

    pins()
    fixtures = [
        ('singleton', [0], []),
        ('deux_classes_meme_plateau', [0] * 4, [(1, b(0, 1)), (1, b(2, 3))]),
        ('sous_aretes_partagees', [0] * 3, [(1, b(0, 1)), (1, b(0, 2))]),
        ('sous_aretes_inclusions', [0] * 3, [(1, b(0, 1)), (1, b(0, 1, 2))]),
        ('grande_cellule_et_inerte', [0] * 6, [(1, b(0, 1, 2, 3, 4, 5, 0, 2)), (1, b(0, 2))]),
        ('ancrage_cellule_inerte', [0] * 3, [(1, b(1)), (1, b(0, 1)), (2, [('c', 0), ('b', 2)])]),
        ('cibles_cellules_et_doublons', [0] * 5, [(1, b(2, 1)), (1, b(1, 0)),
          (2, [('c', 0), ('c', 1), ('b', 3), ('b', 3)]), (3, [('c', 2), ('b', 4)])]),
        ('naissances_datees', [0, 0, 2, 3], [(1, b(0, 1)), (3, [('c', 0), ('b', 2)]),
          (4, [('c', 1), ('b', 3)])]),
        ('branches_non_bornees_par_evenements', [0] * 7, [(1, b(*range(i))) for i in range(2, 8)])]
    def closed_case(dates, cells):
        final = max([*dates, *(r for r, _ in cells)]) + 1
        return run(dates, cells + [(final, b(*range(len(dates))))])
    results = {name: closed_case(dates, cells) for name, dates, cells in fixtures}
    subsets = [b(*ids) for q in (1, 2, 3) for ids in itertools.combinations(range(3), q)]
    exhaustive = [closed_case([0] * 3, [(r, left), (s, right)])
                  for left, right in itertools.product(subsets, repeat=2) for r, s in ((1, 1), (1, 2), (2, 2))]
    need(results['sous_aretes_partagees']['all_children_counterexamples'] == 2, 'contretemoin raccourci global')
    need(results['branches_non_bornees_par_evenements']['values'] == 27, 'sortie non bornee par B-1')
    need(results['grande_cellule_et_inerte']['queries_saved'] == 8, 'doublons et cellule inerte')
    bounds = []
    for sample in cap['historical_counts']:
        rows = sample['orders']
        need(all(x['R'] >= x['F'] and x['Q'] >= x['A'] >= x['B'] + x['F'] - 1 for x in rows),
             'invariants des comptes historiques')
        saved = sum(max(0, x['B'] + x['F'] - 1 - x['max_arity'] * (x['R'] - x['F'])) for x in rows)
        q = sum(x['Q'] for x in rows)
        bounds.append(dict(K=sample['K'], query_savings_lower_bound=saved, existing_queries=q,
                           fallback_rows_upper_bound=sum(2 * (x['R'] - x['F']) for x in rows)))
    result = dict(status='ok', domain='abstract_T_inputs_not_geometry', exhaustive_cases=len(exhaustive),
                  fixtures=results, historical_conditional_bounds=bounds, native_executed=False,
                  performance_qualified=False)
    pins()
    if 'expected_result' in cap:
        need(result == cap['expected_result'], 'resultat capture')
    print(json.dumps(result, sort_keys=True))


if __name__ == '__main__':
    main()
