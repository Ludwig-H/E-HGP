"""Lemme de cliques borne : definition exhaustive, puis geometrie Fraction aux coins/Gauss.

Aucun import du produit. Le premier juge enumere toutes les combinaisons et les prefixages
admissibles, tandis que le second intersecte des masques. Les boules critiques du modele Gram
restent la reference geometrique bornee ; ce fichier ne mesure aucune performance.
"""
import copy
import itertools
import json
import random
import sys
from pathlib import Path

import fraction_model as geometry
from fixtures import fixtures

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'num'))
from center_region_oracle import Case, geometry as center_geometry


def need(value, message):
    if not value:
        raise ValueError(message)


def empty():
    return dict(prefixes=0, pair_rejects=0, admitted=[], calls=[])


def definition(n, adjacent, survives, depth=4):
    """Toutes les combinaisons ; un prefixe est visite ssi son parent a passe les gardes."""
    reached = {()}
    visited, passed = [], []
    for q in range(1, min(n, depth)+1):
        for part in itertools.combinations(range(n), q):
            if part[:-1] not in reached:
                continue
            visited.append(part)
            if not all(adjacent[i][part[-1]] for i in part[:-1]):
                continue
            passed.append(part)
            if survives(part):
                reached.add(part)
    return dict(prefixes=len(visited), pair_rejects=len(visited)-len(passed),
                admitted=sorted(reached-{()}), calls=sorted(passed))


def masked(n, adjacent, survives, depth=4):
    if n > 32:
        return definition(n, adjacent, survives, depth)
    rows = [sum(1 << j for j in range(n) if adjacent[i][j] and j != i) for i in range(n)]
    out = empty()

    def walk(part, candidates):
        while candidates:
            one = candidates & -candidates
            candidates ^= one
            i = one.bit_length()-1
            need(i < n, 'bit hors feuille')
            current = part+(i,)
            out['prefixes'] += 1
            out['calls'].append(current)
            if survives(current):
                out['admitted'].append(current)
                if len(current) < depth:
                    walk(current, candidates & rows[i])

    walk((), (1 << n)-1)
    return out


def judge(before, after):
    need(type(after) is dict and set(after) == set(before), 'champs')
    need(type(after['prefixes']) is int and type(after['pair_rejects']) is int, 'compteur')
    need(0 <= after['pair_rejects'] <= before['pair_rejects'], 'rejets')
    need(after['prefixes'] == before['prefixes']-before['pair_rejects']+after['pair_rejects'], 'prefixes')
    need(after['calls'] == before['calls'], 'demandes/ordre')
    need(after['admitted'] == before['admitted'], 'admissions/ordre')


def abstract_cases():
    rng = random.Random(111032)
    rows = []
    for n in (0, 1, 2, 3, 4, 8, 16, 31, 32, 33, 64, 65):
        for kind in ('empty', 'complete', 'random'):
            edge = [[False]*n for _ in range(n)]
            for i, j in itertools.combinations(range(n), 2):
                edge[i][j] = edge[j][i] = kind == 'complete' or (kind == 'random' and rng.randrange(4) != 0)
            # Garde independante des aretes, modelisant un arret J2 triplet/G3 avant descendance.
            gate = lambda p: len(p) == 1 or sum((i+1)*(i+3) for i in p) % 7 != 2
            depth = 4 if n <= 32 else 3
            rows.append((n, edge, gate, depth))
    return rows


def geometric_case(points, box, bits):
    points = tuple(sorted(points, key=geometry.morton))
    n = len(points)
    corners = tuple(itertools.product(*zip(*box)))
    adjacent = [[False]*n for _ in range(n)]
    dominance = [set() for _ in range(n)]
    contacts = 0
    for i, j in itertools.combinations(range(n), 2):
        values = [geometry.distance2(c, points[i])-geometry.distance2(c, points[j]) for c in corners]
        low, high = min(values), max(values)
        adjacent[i][j] = adjacent[j][i] = low <= 0 <= high
        contacts += int(low == 0 or high == 0)
        if low > 0:
            dominance[i].add(j)
        elif high < 0:
            dominance[j].add(i)
    line = {}
    for ids in itertools.combinations(range(n), 3):
        case = Case('leaf', 3, tuple(points[i] for i in ids), box[0], box[1])
        line[ids] = center_geometry(case, bits)['verdict'] == 'intersects'

    def survives(part):
        if any(not line[face] for face in itertools.combinations(part, 3)):
            return False
        witnesses = set().union(*(dominance[i] for i in part))
        return len(witnesses) <= 6-len(part)

    before = definition(n, adjacent, survives)
    after = masked(n, adjacent, survives)
    judge(before, after)
    emitted = [ball for ball in geometry.catalogue(points, 5)
               if all(lo <= value < hi for value, lo, hi in zip(ball.center, *box))]
    for ball in emitted:
        need(ball.support in after['admitted'], 'support critique absent du graphe')
    return before, after, len(emitted), contacts


def corruptions(before, after):
    changes = []
    for field in ('prefixes', 'pair_rejects'):
        changed = copy.deepcopy(after)
        changed[field] += 1
        changes.append(changed)
    for field in ('calls', 'admitted'):
        need(len(after[field]) > 1, 'temoin mutation vide')
        for action in ('omit', 'duplicate', 'reverse', 'outside'):
            changed = copy.deepcopy(after)
            if action == 'omit': changed[field].pop()
            elif action == 'duplicate': changed[field].append(changed[field][-1])
            elif action == 'reverse': changed[field].reverse()
            else: changed[field][-1] = (64,)
            changes.append(changed)
    changed = copy.deepcopy(after)
    changed['prefixes'] = True
    changes.append(changed)
    refused = 0
    for changed in changes:
        try:
            judge(before, changed)
        except ValueError:
            refused += 1
    need(refused == len(changes), 'corruption acceptee')
    return refused


def main():
    abstract = geometric = emissions = contacts = rejected = 0
    reduction = fallback = 0
    for n, edge, gate, depth in abstract_cases():
        before = definition(n, edge, gate, depth)
        after = masked(n, edge, gate, depth)
        judge(before, after)
        abstract += 1
        reduction += before['prefixes']-after['prefixes']
        fallback += int(n > 32 and before == after)
    need(reduction > 0 and fallback == 9, 'non vacuite/fallback')
    for bits in (18, 21, 24):
        bound = 1 << bits
        for fixture in fixtures(bits):
            if len(fixture.points) > 8:
                continue
            for box in (((0, 0, 0), (bound,)*3), ((0, 0, 0), (2, 2, 2)), ((2, 0, 0), (3, 2, 2))):
                before, after, balls, touch = geometric_case(fixture.points, box, bits)
                geometric += 1
                emissions += balls
                contacts += touch
                if fixture.name == 'regular_tetra' and box[1] == (bound,)*3:
                    rejected += corruptions(before, after)
    need(emissions > 0 and contacts > 0 and rejected == 33, 'non vacuite geometrique')
    # Les tailles 32, 33 et 64 ne sont pas un modulo32 : aucun bit perdu au repli.
    for n in (1, 31, 32):
        mask = (1 << n)-1
        need(mask.bit_length() == n and mask >> n == 0, 'masque final')
    print('small_pair_graph_model conforme ' + json.dumps(dict(
        abstract=abstract, geometric=geometric, emissions=emissions, contacts=contacts,
        corruptions=rejected, fallback=fallback, native=0), sort_keys=True))


if __name__ == '__main__':
    main()
