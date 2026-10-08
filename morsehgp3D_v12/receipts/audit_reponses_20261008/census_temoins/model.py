#!/usr/bin/env python3
"""Oracle rationnel borné des témoins de frontière. Aucun moteur ni donnée réelle."""
import argparse
from fractions import Fraction as F
import hashlib
import json
from pathlib import Path
import subprocess
import tempfile


def need(ok, message):
    if not ok:
        raise ValueError(message)


def dot(a, b):
    return sum(x*y for x, y in zip(a, b))


def ball(support):
    """Centre par système de Gram exact ; certificat barycentrique positif."""
    origin = support[0]
    vectors = [tuple(b-a for a, b in zip(origin, point)) for point in support[1:]]
    n = len(vectors)
    matrix = [[F(dot(v, w)) for w in vectors] + [F(dot(v, v), 2)] for v in vectors]
    for j in range(n):
        pivot = next(i for i in range(j, n) if matrix[i][j])
        matrix[j], matrix[pivot] = matrix[pivot], matrix[j]
        scale = matrix[j][j]
        matrix[j] = [x/scale for x in matrix[j]]
        for i in range(n):
            if i != j:
                scale = matrix[i][j]
                matrix[i] = [x-scale*y for x, y in zip(matrix[i], matrix[j])]
    weights = [row[-1] for row in matrix]
    need(all(x > 0 for x in [1-sum(weights)] + weights), 'support non certifié')
    center = tuple(F(origin[j]) + sum(w*v[j] for w, v in zip(weights, vectors)) for j in range(3))
    radius = sum((center[j]-origin[j])**2 for j in range(3))
    need(all(sum((center[j]-p[j])**2 for j in range(3)) == radius for p in support), 'coquille')
    return center, radius


def power(point, sphere):
    center, radius = sphere
    return sum((point[j]-center[j])**2 for j in range(3)) - radius


def morton(point):
    return sum(((point[j] >> bit) & 1) << (3*bit+j) for bit in range(32) for j in range(3))


def index(points, leaf):
    nodes = []
    def build(begin, end):
        at = len(nodes)
        box = tuple((min(p[j] for p in points[begin:end]), max(p[j] for p in points[begin:end])) for j in range(3))
        nodes.append([begin, end, None, box])
        if end-begin > leaf:
            middle = (begin+end)//2
            build(begin, middle)
            build(middle, end)
        nodes[at][2] = len(nodes)
    build(0, len(points))
    return nodes


def bounds(box, sphere):
    """Extrema exacts sur tous les sites entiers de la boîte, pas les seuls points."""
    center, radius = sphere
    low, high = -radius, -radius
    for (lo, hi), c in zip(box, center):
        floor = c.numerator//c.denominator
        nearest = min((max(lo, min(hi, q)) for q in (floor, floor+1)), key=lambda q: (F(q)-c)**2)
        low += (nearest-c)**2
        high += max((lo-c)**2, (hi-c)**2)
    return low, high


def oracle(points, sphere, threshold):
    inner = [i for i, p in enumerate(points) if power(p, sphere) < 0]
    if len(inner) >= threshold:
        return 'saturated', tuple(inner[:threshold]), ()
    return 'complete', tuple(inner), tuple(i for i, p in enumerate(points) if power(p, sphere) == 0)


def walk(points, nodes, sphere, threshold, leaf, witnesses=(), mutant=None):
    inner, shell, visited, tested, inside, outside = [], [], [], [], [], []
    cursor, avoided = 0, 0
    while cursor < len(nodes) and len(inner) < threshold:
        begin, end, escape, box = nodes[cursor]
        visited.append(cursor)
        hit = any(begin <= w < end for w in witnesses)
        if hit:
            avoided += 1
            lower, upper = (1, 1) if mutant == 'outside' else (-1, -1) if mutant == 'inside' else (0, 0)
        else:
            lower, upper = bounds(box, sphere)
        if lower > 0:
            outside.append(cursor)
            cursor = escape
        elif upper < 0:
            inside.append(cursor)
            inner.extend(range(begin, min(end, begin+threshold-len(inner))))
            cursor = escape
        elif end-begin <= leaf:
            for i in range(begin, end):
                if len(inner) == threshold:
                    break
                tested.append(i)
                side = power(points[i], sphere)
                if side < 0:
                    inner.append(i)
                elif side == 0:
                    shell.append(i)
            cursor = escape
        else:
            cursor += 1
        need(len(inner)+len(shell) <= len(points), 'capacité du workspace')
    saturated = len(inner) == threshold
    result = ('saturated' if saturated else 'complete', tuple(inner), () if saturated else tuple(shell))
    trace = (tuple(visited), tuple(tested), tuple(inside), tuple(outside))
    return result, trace, avoided


def fixtures():
    q1 = [(3, 3, 3)]
    q2 = [(1, 3, 3), (5, 3, 3)]
    q3 = [(0, 0, 0), (4, 4, 0), (4, 0, 4)]
    q4 = [(0, 0, 0), (4, 4, 0), (4, 0, 4), (0, 4, 4)]
    extra = [(2, 2, 2), (3, 1, 1), (1, 1, 1), (4, 4, 4), (6, 6, 6)]
    cases = [('singleton', q1, q1), ('singleton_mixte', q1, q1+extra),
             ('diametre_coquille6', q2, q2+[(3,1,3),(3,5,3),(3,3,1),(3,3,5),(3,3,3),(0,0,0),(6,6,6)]),
             ('saturation_avant_temoin', [(4,0,0),(4,4,0)], [(4,0,0),(4,4,0),(0,0,0),(3,2,0),(5,2,0),(6,2,0)]),
             ('triangle_aigu', q3, q3+extra), ('tetraedre_coquille5', q4, q4+extra)]
    scale = ((1 << 32)-8)//8
    translate = lambda p: tuple(scale*x+v for x, v in zip(p, (3,1,5)))
    cases.append(('u32', list(map(translate, q4)), list(map(translate, q4+extra))))
    return cases


def verify_sources(repo, here, cap):
    for name, digest in cap['receipt_artifacts_sha256'].items():
        need(hashlib.sha256((here/name).read_bytes()).hexdigest() == digest, 'reçu changé')
    with tempfile.TemporaryDirectory(prefix='audit-witness-model-') as tmp:
        root = Path(tmp)
        for path, pin in cap['sources'].items():
            body = subprocess.check_output(['git', '-C', str(repo), 'show', cap['base_commit']+':'+path])
            need(hashlib.sha256(body).hexdigest() == pin['base_sha256'], 'source de base')
            dst = root/path
            dst.parent.mkdir(parents=True, exist_ok=True)
            dst.write_bytes(body)
        subprocess.run(['git', 'apply', str(here/'prototype_index.diff')], cwd=root, check=True,
                       stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        for path, pin in cap['sources'].items():
            need(hashlib.sha256((root/path).read_bytes()).hexdigest() == pin['prototype_sha256'], 'delta prototype')
    for path, digest in cap['supporting_sources_sha256'].items():
        body = subprocess.check_output(['git', '-C', str(repo), 'show', cap['base_commit']+':'+path])
        need(hashlib.sha256(body).hexdigest() == digest, 'source des préconditions')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo', type=Path, required=True)
    args = parser.parse_args()
    here = Path(__file__).resolve().parent
    cap = json.loads((here/'capture.json').read_bytes())
    verify_sources(args.repo, here, cap)
    trials, bases, false_trials, false_work_changes, avoided, premature = 0, 0, 0, 0, 0, 0
    killed = set()
    for name, support, raw in fixtures():
        points = sorted(set(raw), key=morton)
        sphere = ball(support)
        ws = [points.index(p) for p in support]
        choices = [tuple(ws[j] for j in range(len(ws)) if mask & (1 << j)) for mask in range(1 << len(ws))]
        choices += [tuple(reversed(ws)), tuple(ws+ws)]
        for leaf in (1, 2, 4, 8):
            nodes = index(points, leaf)
            for threshold in sorted({1, 2, 3, len(points), len(points)+1}):
                expected = oracle(points, sphere, threshold)
                baseline = walk(points, nodes, sphere, threshold, leaf)
                need(baseline[0] == expected, 'référence contre oracle ponctuel')
                bases += 1
                if expected[0] == 'saturated' and not set(ws).intersection(baseline[1][1]):
                    premature += 1
                for markers in choices:
                    actual = walk(points, nodes, sphere, threshold, leaf, markers)
                    need(actual[:2] == baseline[:2], 'témoin valide : identité résultat/parcours')
                    trials += 1
                    avoided += actual[2]
                for marker in range(len(points)+1):
                    actual = walk(points, nodes, sphere, threshold, leaf, (marker,))
                    need(actual[0] == expected, 'raffinement conservateur même sans certificat de témoin')
                    false_trials += 1
                    false_work_changes += actual[1] != baseline[1]
                for mutant in ('outside', 'inside'):
                    if walk(points, nodes, sphere, threshold, leaf, ws, mutant)[0] != expected:
                        killed.add(mutant)
    need(killed == {'outside', 'inside'} and false_work_changes > 0 and premature > 0, 'témoins non discriminants')
    print(json.dumps(dict(fixtures=len(fixtures()), baseline_queries=bases, valid_witness_queries=trials,
                         arbitrary_marker_queries=false_trials, arbitrary_marker_work_changes=false_work_changes,
                         saturated_before_support_test=premature, avoided_box_evaluations_in_model=avoided,
                         wrong_classifications_rejected=sorted(killed), native_executed=False), sort_keys=True))


if __name__ == '__main__':
    main()
