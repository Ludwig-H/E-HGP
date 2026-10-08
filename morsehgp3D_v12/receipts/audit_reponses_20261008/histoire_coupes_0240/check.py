#!/usr/bin/env python3
"""Contre-modele exhaustif borne du certificat d'historique, aucun moteur appele."""
import argparse
import hashlib
import itertools as it
import json
from bisect import bisect_right
from pathlib import Path


def need(ok, msg):
    if not ok:
        raise SystemExit(msg)


def depths(parent):
    result = []
    for x in range(len(parent)):
        seen = set()
        while parent[x] is not None:
            if x in seen:
                return None
            seen.add(x)
            x = parent[x]
        result.append(len(seen))
    return result


def query(tree, hist, x, rank):
    # Equation de vertical_images.cpp : attaches, puis dernier evenement du survivant.
    ap, ar, nodes, er, rows = hist
    if tree[1][x] > rank:
        return None
    while ap[x] is not None and ar[x] <= rank:
        x = ap[x]
    row = rows[x]
    at = bisect_right([er[e] for e in row], rank)
    return nodes[row[at - 1]] if at else x


def parent_cut(tree, x, rank):
    # Oracle independant : ne lit aucun champ d'historique.
    parent, dates, _, _ = tree
    if dates[x] > rank:
        return None
    while parent[x] is not None and dates[parent[x]] <= rank:
        x = parent[x]
    return x


def certificate(tree, hist):
    parent, dates, births, root = tree
    ap, ar, nodes, er, rows = hist
    events = births - 1
    if len(ap) != births or len(ar) != births or len(rows) != births or len(nodes) != events or len(er) != events:
        return False
    if any(a is not None and (not 0 <= a < births or a == x) for x, a in enumerate(ap)):
        return False
    if any(not births <= node < len(parent) or dates[node] != er[e] for e, node in enumerate(nodes)):
        return False
    # Dans le prototype : bitset/vecteur de marques O(E), pas un tri necessaire.
    seen = [False] * events
    for row in rows:
        for e in row:
            if not 0 <= e < events or seen[e]:
                return False
            seen[e] = True
    if not all(seen):
        return False
    depth = depths(ap)
    if depth is None or max(depth) > births.bit_length() - 1:
        return False
    for x, a in enumerate(ap):
        if a is not None and (ar[x] < dates[x] or ar[x] < dates[a]):
            return False
        current = x
        for e in rows[x]:
            node, rank = nodes[e], er[e]
            if a is not None and rank > ar[x]:
                return False
            if node == current:
                continue  # meme noeud de plateau, pas seulement meme rang
            if parent[current] != node:
                return False
            current = node
    # Toutes les lignes sont maintenant monotones et toutes les references sont protegees.
    for x, a in enumerate(ap):
        current = nodes[rows[x][-1]] if rows[x] else x
        if a is None:
            if current != root:
                return False
        else:
            rank = ar[x]
            if dates[current] == rank:
                want = current
            elif parent[current] is not None and dates[parent[current]] == rank:
                want = parent[current]
            else:
                return False
            if query(tree, hist, a, rank) != want:
                return False
    return True


def tree_cases():
    yield ((None,), (0,), 1, 0)
    yield ((None,), (2,), 1, 0)
    yield ((2, 2, None), (0, 0, 1), 2, 2)
    yield ((2, 2, None), (0, 1, 2), 2, 2)
    yield ((3, 3, 3, None), (0, 0, 0, 1), 3, 3)
    yield ((3, 3, 3, None), (0, 0, 1, 2), 3, 3)
    for pair in it.combinations(range(3), 2):
        p = tuple(3 if x in pair else 4 for x in range(3)) + (4, None)
        yield (p, (0, 0, 0, 1, 2), 3, 4)
        yield (p, (0, 0, 1, 2, 3), 3, 4)


def all_histories(tree):
    parent, dates, births, _ = tree
    events = births - 1
    rows_set = set()
    for owners in it.product(range(births), repeat=events):
        for perm in it.permutations(range(events)):
            rows_set.add(tuple(tuple(e for e in perm if owners[e] == x) for x in range(births)))
    attaches = []
    for ap in it.product(*[[None] + [a for a in range(births) if a != x] for x in range(births)]):
        depth = depths(ap)
        if depth is None or max(depth) > births.bit_length() - 1:
            continue
        linked = [x for x, a in enumerate(ap) if a is not None]
        for ranks in it.product(range(max(dates) + 2), repeat=len(linked)):
            ar = [0] * births
            for x, rank in zip(linked, ranks):
                ar[x] = rank
            attaches.append((ap, tuple(ar)))
    for nodes in it.product(range(births, len(parent)), repeat=events):
        er = tuple(dates[n] for n in nodes)
        for rows in sorted(rows_set):
            for ap, ar in attaches:
                yield ap, ar, nodes, er, rows


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--prototype', required=True, type=Path,
                        help='racine morsehgp3D_v12 de repo5 TMVR')
    args = parser.parse_args()
    here = Path(__file__).resolve().parent
    capture = json.loads((here / 'capture.json').read_text())

    def pins():
        for rel, expected in capture['source_sha256'].items():
            need(hashlib.sha256((args.prototype / rel).read_bytes()).hexdigest() == expected, 'source: ' + rel)

    pins()
    tested = accepted = queries = trees = 0
    for tree in tree_cases():
        trees += 1
        for hist in all_histories(tree):
            tested += 1
            if not certificate(tree, hist):
                continue
            accepted += 1
            for x in range(tree[2]):
                # Toutes les coupures entieres du modele, y compris avant naissance et apres la racine.
                for rank in range(max(tree[1]) + 3):
                    need(query(tree, hist, x, rank) == parent_cut(tree, x, rank), 'contre-exemple exhaustif')
                    queries += 1
    star = ((4, 4, 4, 4, None), (0, 0, 0, 0, 1), 4, 4)
    plateau = ((None, 0, 0, 0), (0, 1, 1, 1), (4, 4, 4), (1, 1, 1), ((0, 1, 2), (), (), ()))
    balanced = ((4, 4, 5, 5, 6, 6, None), (0, 0, 0, 0, 1, 1, 2), 4, 6)
    valid = ((None, 0, 0, 2), (0, 1, 2, 1), (4, 5, 6), (1, 1, 2), ((0, 2), (), (1,), ()))
    joined = ((5, 4, 4, 5, 5, None), (0, 0, 0, 0, 1, 2), 4, 5)
    nonchronological = ((1, 2, None, 2), (2, 1, 0, 2), (4, 5, 5), (1, 2, 2), ((), (), (0, 1, 2), ()))
    for tree, hist in ((star, plateau), (balanced, valid), (joined, nonchronological)):
        need(certificate(tree, hist), 'temoin sain de plateau')
        for x in range(4):
            for rank in range(4):
                need(query(tree, hist, x, rank) == parent_cut(tree, x, rank), 'coupe plateau')
    pair = ((2, 2, None), (0, 0, 1), 2, 2)
    dated_pair = ((2, 2, None), (0, 1, 2), 2, 2)
    healthy = ((None, 0), (0, 1), (2,), (1,), ((0,), ()))
    mutants = {
        'attache_trop_tard': (pair, (healthy[0], (0, 2), *healthy[2:])),
        'historique_vide': (pair, (healthy[0], healthy[1], (), (), ((), ()))),
        'mauvais_survivant': (pair, (*healthy[:4], ((), (0,)))),
        'noeud_naissance': (pair, (healthy[0], healthy[1], (0,), (0,), healthy[4])),
        'doublon_evenement': (star, (*plateau[:4], ((0, 0, 2), (), (), ()))),
        'autre_noeud_meme_rang': (balanced, (valid[0], valid[1], (5, 5, 6), valid[3], valid[4])),
        'rang_evenement_faux': (pair, (healthy[0], healthy[1], (2,), (0,), healthy[4])),
        'cycle_attaches': (pair, ((1, 0), (1, 1), *healthy[2:])),
        'attache_avant_naissance_perdant': (dated_pair, ((None, 0), (0, 0), (2,), (2,), ((0,), ()))),
        'attache_avant_naissance_survivant': (dated_pair, ((1, None), (0, 0), (2,), (2,), ((), (0,)))),
    }
    for name, (tree, hist) in mutants.items():
        need(not certificate(tree, hist), 'mutant admis: ' + name)
    pins()
    print(json.dumps(dict(status='ok', trees=trees, histories=tested, accepted=accepted, oracle_queries=queries,
                          counterexamples=0, additional_four_birth_cases=3, nonchronological_queries_correct=True,
                          mutations_refused=sorted(mutants),
                          native_executed=False), sort_keys=True))


if __name__ == '__main__':
    main()
