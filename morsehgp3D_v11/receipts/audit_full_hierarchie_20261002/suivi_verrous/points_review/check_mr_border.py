#!/usr/bin/env python3
"""Trois sites exacts : MR2-bord ne contient pas tous les blocs cover.

Calcul definitionnel borne, sans moteur natif ni sklearn. La formule MR est
celle du temoin v10 capture dans sources/mreach.hpp, pas une reimplementation
d'HDBSCAN ou de sa selection. Les niveaux MR sont multiplies par alpha^2.
"""
from fractions import Fraction as F
from itertools import combinations
import json


def need(condition, message):
    if not condition:
        raise RuntimeError(message)


def components(vertices, edges):
    groups = {vertex: {vertex} for vertex in vertices}
    for a, b in edges:
        left, right = groups[a], groups[b]
        union = left | right
        for vertex in union:
            groups[vertex] = union
    return groups


def normalized(blocks):
    return tuple(sorted({tuple(sorted(block)) for block in blocks}))


def cover_curve(points):
    # Dans une droite, MEB(F) a pour rayon (max(F)-min(F))/2.
    vertices = list(combinations(range(len(points)), 2))
    birth = {v: F((points[v[1]] - points[v[0]]) ** 2, 4) for v in vertices}
    join = {}
    for a, b in combinations(vertices, 2):
        union = set(a) | set(b)
        span = max(points[x] for x in union) - min(points[x] for x in union)
        join[a, b] = F(span * span, 4)
    entries = []
    for point in range(len(points)):
        choices = sorted((value, pair) for pair, value in birth.items() if point in pair)
        minimum = choices[0][0]
        owners = [pair for value, pair in choices if value == minimum]
        need(len(owners) == 1, 'fixture has ambiguous cover ownership')
        entries.append((minimum, owners[0]))
    curve = []
    for level in sorted({F(0)} | set(birth.values()) | set(join.values())):
        active = [v for v in vertices if birth[v] <= level]
        groups = components(active, [(a, b) for (a, b), value in join.items() if value <= level])
        blocks = {}
        for point, (entry, owner) in enumerate(entries):
            key = ('component', tuple(sorted(groups[owner]))) if entry <= level else ('single', point)
            blocks.setdefault(key, []).append(point)
        curve.append((level, normalized(blocks.values())))
    return curve


def mr_curve(points, alpha, border):
    n = len(points)
    distance = [[(x - y) ** 2 for y in points] for x in points]
    core = [alpha * alpha * sorted(row)[1] for row in distance]  # K=2, self counts.
    edges = {(x, y): max(core[x], core[y], distance[x][y])
             for x, y in combinations(range(n), 2)}
    entries, carriers = list(core), list(range(n))
    if border:
        for x in range(n):
            for y in range(n):
                candidate = max(core[y], distance[x][y])
                if candidate < entries[x] or (candidate == entries[x] and carriers[x] != x and y < carriers[x]):
                    entries[x], carriers[x] = candidate, y
    curve = []
    for level in sorted({0} | set(core) | set(entries) | set(edges.values())):
        active = [x for x in range(n) if core[x] <= level]
        groups = components(active, [pair for pair, value in edges.items() if value <= level])
        blocks = {}
        for point in range(n):
            key = ('component', tuple(sorted(groups[carriers[point]]))) if entries[point] <= level else ('single', point)
            blocks.setdefault(key, []).append(point)
        curve.append((F(level, alpha * alpha), normalized(blocks.values())))
    return curve, core, entries, carriers


def block_family(curve):
    return {block for _, partition in curve for block in partition}


def best_iou(family, target):
    return max(F(len(set(block) & target), len(set(block) | target)) for block in family)


def render(curve):
    return [{'squared_radius': str(level), 'partition': partition} for level, partition in curve]


def main():
    points, target = [0, 2, 5], {0, 1}
    cover = cover_curve(points)
    need(cover == [(F(0), ((0,), (1,), (2,))),
                   (F(1), ((0, 1), (2,))),
                   (F(9, 4), ((0, 1), (2,))),
                   (F(25, 4), ((0, 1, 2),))], 'cover curve differs from analytic expected')
    out = {'cover': {'curve': render(cover), 'best_iou_AB': str(best_iou(block_family(cover), target))}}
    for alpha in (1, 2):
        for border in (False, True):
            curve, core, entries, carriers = mr_curve(points, alpha, border)
            score = best_iou(block_family(curve), target)
            expected = F(2, 3) if alpha == 2 and border else F(1)
            need(score == expected, 'control score differs')
            out[f'mr{alpha}_' + ('border' if border else 'core')] = {
                'curve': render(curve), 'core_scaled_squared': core,
                'entry_scaled_squared': entries, 'carriers': carriers,
                'best_iou_AB': str(score), 'contains_AB': (0, 1) in block_family(curve)}
    # Une translation positive et une homothetie entiere conservent les blocs.
    for transformed in ([7, 9, 12], [0, 6, 15]):
        need(block_family(cover_curve(transformed)) == block_family(cover), 'cover affine control')
        need(block_family(mr_curve(transformed, 2, True)[0]) ==
             block_family(mr_curve(points, 2, True)[0]), 'MR affine control')
    print(json.dumps({'status': 'PASS', 'points_xyz': [[x, 0, 0] for x in points],
                      'K': 2, 'target_ids': sorted(target), 'methods': out,
                      'scope': 'exact bounded definition-level fixture; no native or HDBSCAN execution'},
                     indent=2, sort_keys=True))


if __name__ == '__main__':
    main()
