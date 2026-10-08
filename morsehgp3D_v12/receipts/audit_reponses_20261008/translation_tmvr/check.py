#!/usr/bin/env python3
"""Oracle Fraction de quatre sites et protocole de transport ; aucun moteur appele."""
import argparse
import hashlib
import itertools as it
import json
import subprocess
from fractions import Fraction as F
from pathlib import Path


def need(ok, message):
    if not ok:
        raise SystemExit(message)


def sha(b):
    return hashlib.sha256(b).hexdigest()


def morton(p):
    return sum(((p[a] >> i) & 1) << (3 * i + a) for i in range(21) for a in range(3))


def meb(points):
    # Tous les sites du temoin sont dans z=0. En dimension affine 2, un support minimal a au plus trois sites.
    candidates = [(tuple(map(F, p[:2])), F(0)) for p in points]
    for a, b in it.combinations(points, 2):
        c = tuple(F(a[j] + b[j], 2) for j in range(2))
        candidates.append((c, sum((c[j] - a[j]) ** 2 for j in range(2))))
    for a, b, c in it.combinations(points, 3):
        u, v = [b[j] - a[j] for j in range(2)], [c[j] - a[j] for j in range(2)]
        det = u[0] * v[1] - u[1] * v[0]
        if not det:
            continue
        p, q = sum(x * x for x in u), sum(x * x for x in v)
        z = (F(p * v[1] - q * u[1], 2 * det), F(u[0] * q - v[0] * p, 2 * det))
        candidates.append(((F(a[0]) + z[0], F(a[1]) + z[1]), sum(x * x for x in z)))
    good = [(r, c) for c, r in candidates if all(sum((c[j] - p[j]) ** 2 for j in range(2)) <= r for p in points)]
    r, c = min(good)
    return c, r


def cuts(cloud, k, balls, levels):
    # Nerf des intersections de k boules de meme rayon : sommets = k-parties vivantes ;
    # une arete existe exactement quand la MEB de leur union tient dans le rayon.
    parts = list(it.combinations(sorted(cloud), k))
    result = []
    for level in levels:
        alive = [p for p in parts if balls[p][1] <= level]
        unseen, groups = set(alive), []
        while unseen:
            todo = [min(unseen)]
            unseen.remove(todo[0])
            component = set(todo)
            while todo:
                a = todo.pop()
                neighbours = [b for b in unseen if balls[tuple(sorted(set(a) | set(b)))][1] <= level]
                for b in neighbours:
                    unseen.remove(b)
                    component.add(b)
                    todo.append(b)
            groups.append(component)
        result.append(groups)
    return result


def at(tree, birth, rank):
    if rank < tree['rank'][birth]:
        return None
    node = birth
    while tree['parent'][node] is not None and tree['rank'][tree['parent'][node]] <= rank:
        node = tree['parent'][node]
    return node


def strict_groups(parts, balls, radius):
    # Quotient des traces par la fermeture transitive de beta(A union B) < lambda.
    unseen, groups = set(parts), []
    while unseen:
        todo = [min(unseen)]
        unseen.remove(todo[0])
        group = set(todo)
        while todo:
            a = todo.pop()
            adjacent = [b for b in unseen if balls[tuple(sorted(set(a) | set(b)))][1] < radius]
            for b in adjacent:
                unseen.remove(b)
                group.add(b)
                todo.append(b)
        groups.append(frozenset(group))
    return set(groups)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--repo', type=Path, required=True)
    ap.add_argument('--tmv', type=Path, required=True)
    args = ap.parse_args()
    here = Path(__file__).resolve().parent
    cap = json.loads((here / 'capture.json').read_text())
    fixture = json.loads((here / 'fixture.json').read_text())

    def pins():
        for rel, h in cap['git_sources'].items():
            data = subprocess.check_output(['git', 'show', cap['main_pin'] + ':morsehgp3D_v12/' + rel], cwd=args.repo)
            need(sha(data) == h, 'Git source: ' + rel)
        for rel, h in cap['prototype_sources'].items():
            need(sha((args.tmv / rel).read_bytes()) == h, 'prototype: ' + rel)
        need(sha((here / 'fixture.json').read_bytes()) == cap['fixture_sha256'], 'fixture')

    pins()
    clouds = []
    for shift in ([0, 0, 0], fixture['translation']):
        clouds.append({p['id']: tuple(p['xyz'][a] + shift[a] for a in range(3)) for p in fixture['points']})
    orders = [sorted(c, key=lambda i: (morton(c[i]), i)) for c in clouds]
    need(orders == fixture['site_point_ids'], 'Morton witness')
    need(orders[0] != orders[1], 'translation ne permute pas Morton')
    need(all(0 <= x < 2 ** 21 for c in clouds for p in c.values() for x in p), 'u21')
    all_balls, all_cuts = [], []
    for cloud in clouds:
        balls = {s: meb([cloud[i] for i in s]) for k in range(1, 5) for s in it.combinations(sorted(cloud), k)}
        levels = sorted({r for _, r in balls.values()})
        need(levels == [0, 1, 2], 'niveaux')
        all_balls.append(balls)
        all_cuts.append([cuts(cloud, k, balls, levels) for k in range(1, 5)])
    need(all_cuts[0] == all_cuts[1], 'partitions transportees')
    for s, (c, r) in all_balls[0].items():
        want = tuple(c[a] + fixture['translation'][a] for a in range(2))
        need(all_balls[1][s] == (want, r), 'MEB non transportee')
    # Catalogue du carre : cinq boules non nulles ; supports minimaux departages par positions.
    critical = {(c, r) for c, r in all_balls[0].values() if r > 0}
    need(len(critical) == 5, 'catalogue')
    trace_count = quotient_count = 0
    for row in fixture['catalogue']:
        sphere = (tuple(map(F, row['center'][:2])), F(row['radius_squared']))
        need(sphere in critical, 'boule attendue')
        supports = [s for s, b in all_balls[0].items() if b == sphere]
        q = min(map(len, supports))
        by_position = lambda s: tuple(sorted(clouds[0][i] for i in s))
        chosen = min((s for s in supports if len(s) == q), key=by_position)
        need(set(chosen) == set(row['support_point_ids']) and q == 2, 'S* canonique')
        shell = sorted(i for i, p in clouds[0].items() if sum((F(p[j]) - sphere[0][j]) ** 2 for j in range(2)) == sphere[1])
        need(shell == row['shell_point_ids'], 'coquille')
        for k in range(1, len(shell)):
            strict = [part for part in it.combinations(shell, k) if all_balls[0][part][1] < sphere[1]]
            groups = [strict_groups(strict, balls, sphere[1]) for balls in all_balls]
            need(groups[0] == groups[1], 'quotient transporte')
            chosen_reps = [{min(g, key=lambda p: tuple(sorted(orders[c].index(i) for i in p)))
                            for g in groups[c]} for c in range(2)]
            need(chosen_reps[0] == chosen_reps[1], 'representants minimaux de ce temoin')
            quotient_count += len(groups[0])
            for part in strict:
                target = all_balls[0][part]
                population = [i for i, p in clouds[0].items() if sum((F(p[j]) - target[0][j]) ** 2 for j in range(2)) <= target[1]]
                need(len(population) == k, 'trace exigeant un saut de politique')
                trace_count += 1
    need(trace_count == 16, 'traces strictes')
    need(quotient_count == 13, 'classes du quotient des traces')
    trees = fixture['forests']
    queries = 0
    for k, tree in enumerate(trees):
        birth_parts = [tuple(sorted(p)) for p in tree['birth_parts']]
        birth_keys = [(all_balls[0][p][1], all_balls[0][p][0]) for p in birth_parts]
        need(birth_keys == sorted(set(birth_keys)), 'numerotation canonique')
        children = [[j for j, p in enumerate(tree['parent']) if p == i] for i in range(len(tree['rank']))]
        need(children == tree['children'], 'parents/enfants')
        need([i for i, p in enumerate(tree['parent']) if p is None] == [tree['root']], 'racine')
        for i, p in enumerate(tree['parent']):
            need(p is None or tree['rank'][i] < tree['rank'][p], 'date parent')
        for rank, groups in enumerate(all_cuts[0][k]):
            expected = {frozenset(j for j, p in enumerate(birth_parts) if p in g) for g in groups}
            need(all(expected), 'composante sans naissance')
            got = {}
            for j in range(len(birth_parts)):
                node = at(tree, j, rank)
                queries += 1
                if node is not None:
                    got.setdefault(node, set()).add(j)
            need(set(map(frozenset, got.values())) == expected, 'foret/coupe')
        if k == 0:
            continue
        for node, rank in enumerate(tree['rank']):
            representative = next(j for j in range(len(birth_parts)) if at(tree, j, rank) == node)
            component = next(g for g in all_cuts[0][k][rank] if birth_parts[representative] in g)
            faces = {s for p in component for s in it.combinations(p, k)}
            lower_groups = [g for g in all_cuts[0][k - 1][rank] if g & faces]
            need(len(lower_groups) == 1 and faces <= lower_groups[0], 'naturalite')
            lower = trees[k - 1]
            lower_birth = next(j for j, p in enumerate(lower['birth_parts']) if tuple(sorted(p)) in lower_groups[0])
            need(at(lower, lower_birth, rank) == tree['lower'][node], 'verticale')
    pins()
    print(json.dumps({'status': 'ok', 'points': 4, 'orders': 4, 'site_point_ids': orders,
        'phi_site': [orders[1].index(i) for i in orders[0]], 'exact_MEB_pairs': 15,
        'catalogue_balls': 5, 'strict_traces_needing_no_policy_jump': trace_count,
        'trace_quotient_classes': quotient_count, 'minimum_representatives_transport_for_fixture': True,
        'closed_cut_pairs': 12, 'component_at_queries': queries, 'verticals_checked': 7,
        'native_executed': False, 'native_translation_qualified': False}, sort_keys=True))


if __name__ == '__main__':
    main()
