#!/usr/bin/env python3
"""Independent tiny-cloud Fraction oracle; never a production coface enumerator.

Capture once with --binary/--build-receipt/--output. --readback rechecks the
recorded commands, live pins and exact geometry without executing the binary.
No imports from the product geometry or weighted_model implement this oracle.
"""
from __future__ import annotations

import argparse
from collections import defaultdict
from fractions import Fraction as F
from functools import lru_cache
import hashlib
from itertools import combinations
import json
import math
from pathlib import Path
import random
import struct
import subprocess
import sys
import time

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
SCHEMA = 'mhgp9_weighted_geometry_qualification_v1'


def need(ok, why):
    if not ok:
        raise ValueError(why)


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def save(path, value):
    Path(path).write_text(json.dumps(value, sort_keys=True, indent=2, allow_nan=False) + '\n')


def integer(value, why='integer', minimum=0):
    need(type(value) is int and value >= minimum, why)
    return value


def rational(value, positive=False):
    need(type(value) is dict and set(value) == {'num', 'den'}, 'rational shape')
    parts = []
    for name in ('num', 'den'):
        part = value[name]
        if isinstance(part, str):
            need(part.isascii() and part.isdecimal(), 'rational unsigned decimal')
            part = int(part)
        parts.append(integer(part, 'rational coefficient'))
    need(parts[1] > 0 and parts[0] >= int(positive), 'rational positive domain')
    return F(*parts)


def dist2(a, b):
    return sum((x-y)**2 for x, y in zip(a, b))


def solve(matrix, vector):
    """Gauss-Jordan in Q. Singular affine supports are deliberately skipped."""
    n = len(vector)
    rows = [[F(x) for x in row] + [F(y)] for row, y in zip(matrix, vector)]
    for col in range(n):
        pivot = next((j for j in range(col, n) if rows[j][col]), None)
        if pivot is None:
            return None
        rows[col], rows[pivot] = rows[pivot], rows[col]
        scale = rows[col][col]
        rows[col] = [x/scale for x in rows[col]]
        for j in range(n):
            if j != col:
                scale = rows[j][col]
                rows[j] = [x-scale*y for x, y in zip(rows[j], rows[col])]
    return [row[-1] for row in rows]


def support_ball(points):
    """Circumcenter in affine hull with nonnegative barycentric weights."""
    origin = points[0]
    if len(points) == 1:
        return tuple(map(F, origin)), F(0), (F(1),)
    vectors = [tuple(F(x-y) for x, y in zip(p, origin)) for p in points[1:]]
    gram = [[sum(x*y for x, y in zip(a, b)) for b in vectors] for a in vectors]
    alpha = solve(gram, [gram[i][i]/2 for i in range(len(vectors))])
    if alpha is None:
        return None
    bary = (1-sum(alpha), *alpha)
    if min(bary) < 0:
        return None
    center = tuple(F(origin[j])+sum(a*v[j] for a, v in zip(alpha, vectors)) for j in range(3))
    beta = dist2(center, origin)
    need(all(dist2(center, p) == beta for p in points), 'support equidistance')
    return center, beta, bary


def mask_of(ids):
    return sum(1 << i for i in ids)


class Oracle:
    def __init__(self, points):
        need(1 <= len(points) <= 12, 'tiny oracle domain: 1..12 sites only')
        need(all(len(p) == 3 and all(type(x) is int for x in p) for p in points), 'integer XYZ')
        need(len(set(map(tuple, points))) == len(points), 'distinct sites')
        self.points = tuple(map(tuple, points))
        self.n = len(points)
        self.candidates = []
        self.balls = {}
        self.cache = {}
        for size in range(1, min(4, self.n)+1):
            for ids in combinations(range(self.n), size):
                ball = support_ball([points[i] for i in ids])
                if ball is None:
                    continue
                center, beta, bary = ball
                powers = [dist2(center, p)-beta for p in points]
                closed = mask_of(i for i, x in enumerate(powers) if x <= 0)
                self.candidates.append((mask_of(ids), closed, center, beta))
                if beta > 0 and min(bary) > 0:
                    item = self.balls.setdefault((center, beta), dict(
                        interior=tuple(i for i, x in enumerate(powers) if x < 0),
                        shell=tuple(i for i, x in enumerate(powers) if x == 0), supports=[]))
                    item['supports'].append(ids)

    def meb(self, ids):
        ids = tuple(ids)
        need(ids and len(set(ids)) == len(ids) and all(0 <= i < self.n for i in ids), 'MEB IDs')
        bits = mask_of(ids)
        if bits not in self.cache:
            for support, closed, center, beta in self.candidates:
                if support & bits == support and closed & bits == bits:
                    # Center in conv(support) + containment certifies the MEB;
                    # no product comparator or floating minimizer is involved.
                    self.cache[bits] = center, beta
                    break
            need(bits in self.cache, 'support enumeration failed to find MEB')
        return self.cache[bits]

    def cofaces(self, k):
        all_rows, gabriel = {}, {}
        for ids in combinations(range(self.n), k+1):
            center, beta = self.meb(ids)
            all_rows[ids] = beta
            if all(i in ids or dist2(center, p) >= beta for i, p in enumerate(self.points)):
                gabriel[ids] = beta
        return all_rows, gabriel


@lru_cache(maxsize=16)
def oracle_for(points):
    return Oracle(points)


def fixtures():
    result = []
    def add(name, points, orders):
        for k in orders:
            result.append(dict(name=f'{name}_k{k}', points=points, k=k))
    add('equilateral', [[0,0,0], [2,2,0], [2,0,2]], [1,2])
    add('obtuse', [[0,0,0], [4,0,0], [1,1,0]], [1,2])
    add('square', [[0,0,0], [2,0,0], [2,2,0], [0,2,0]], [1,2,3])
    octa = [[2,1,1], [0,1,1], [1,2,1], [1,0,1], [1,1,2], [1,1,0]]
    add('octahedron', octa, [1,2,3,5])
    interior = [[3,3,3], [6,3,3], [0,3,3], [3,6,3], [3,0,3], [3,3,6], [3,3,0]]
    add('mandatory_interior', interior, [1,2,3,5])
    for seed in (1,2,3):
        rng = random.Random(seed)
        points = []
        while len(points) < 8:
            p = [rng.randrange(33) for _ in range(3)]
            if p not in points:
                points.append(p)
        add(f'random8_seed{seed}', points, [1,2,3,5])
    add('eleven_k10', [[0,0,0], [4,0,0], [0,4,0], [0,0,4], [7,3,2], [9,8,5],
                      [2,9,11], [13,6,5], [5,17,3], [21,4,19], [8,12,23]], [10])
    return result


def point_ids(values, n, size=None):
    need(type(values) is list and values == sorted(set(values)), 'sorted unique IDs')
    need(all(type(x) is int and 0 <= x < n for x in values), 'ID domain')
    need(size is None or len(values) == size, 'ID cardinality')
    return tuple(values)


def native_coverage(native, cut, closed):
    admitted = lambda beta: beta <= cut if closed else beta < cut
    nodes, populations = native['nodes'], native['populations']
    result = []
    for root, node in enumerate(nodes):
        successor = node['successor']
        if not admitted(rational(node['level'])) or (
                successor is not None and admitted(rational(nodes[successor]['level']))):
            continue
        descendants, stack = set(), [root]
        while stack:
            child = stack.pop()
            need(child not in descendants, 'native descendant repeated')
            descendants.add(child)
            stack.extend(nodes[child]['children'])
        points = set()
        for row in native['contributions']:
            if row['segment'] not in descendants or not admitted(rational(row['level'])):
                continue
            pop = populations[row['population']]
            if row['include_interior']:
                points.update(pop['interior'])
            points.update(x for j, x in enumerate(pop['shell']) if row['shell_mask'] & (1 << j))
        if len(points) > native['k']:
            result.append(tuple(sorted(points)))
    return sorted(result)


def graph_coverage(k, cofaces, cut, closed, births=None):
    """Rebuild components directly, independently of Kruskal/FULL/model code."""
    admitted = lambda beta: beta <= cut if closed else beta < cut
    faces = set(births) if births is not None else {
        face for ids in cofaces for face in combinations(ids, k)}
    if births is not None:
        faces = {face for face in faces if admitted(births[face])}
    parents = {face: face for face in faces}
    def find(face):
        while parents[face] != face:
            face = parents[face]
        return face
    for ids, beta in cofaces.items():
        if not admitted(beta):
            continue
        boundary = list(combinations(ids, k))
        need(all(face in parents for face in boundary), 'coface before facet birth')
        first = find(boundary[0])
        for face in boundary[1:]:
            parents[find(face)] = first
    groups = defaultdict(list)
    for face in faces:
        groups[find(face)].append(face)
    return sorted(tuple(sorted(set().union(*map(set, group))))
                  for group in groups.values() if len(group) > 1)


def validate_forest(native, points, k):
    need(native['schema'] == 'mhgp9_fixed_k_export_v1' and native['status'] == 'completed', 'native schema')
    need(native['points'] == points and native['point_count'] == len(points) and native['k'] == k, 'native input binding')
    need(native['validation']['all_cut_coverage_replayed'] is True, 'native closed replay required')
    nodes = native['nodes']
    parents = {}
    for i, node in enumerate(nodes):
        need(node['id'] == i and len(node['children']) != 1, 'native node shape')
        level = rational(node['level'])
        for child in node['children']:
            need(type(child) is int and 0 <= child < i and child not in parents, 'native child topology')
            need(rational(nodes[child]['level']) < level, 'native strict levels')
            parents[child] = i
    for i, node in enumerate(nodes):
        need(node['successor'] == parents.get(i), 'native successor binding')
    need(native['roots'] == [i for i in range(len(nodes)) if i not in parents], 'native roots')
    for pop in native['populations']:
        interior = point_ids(pop['interior'], len(points))
        shell = point_ids(pop['shell'], len(points))
        need(not set(interior) & set(shell), 'native disjoint census')
    for row in native['contributions']:
        segment = integer(row['segment'])
        pop = integer(row['population'])
        need(segment < len(nodes) and pop < len(native['populations']), 'native contribution references')
        level = rational(row['level'])
        need(rational(nodes[segment]['level']) <= level, 'native contribution after birth')
        if segment in parents:
            need(level < rational(nodes[parents[segment]]['level']), 'native contribution before successor')
        need(type(row['include_interior']) is bool, 'native interior flag')
        bits = integer(row['shell_mask'])
        need(bits < 1 << len(native['populations'][pop]['shell']), 'native shell mask')


def check_export(data, fixture):
    points, k = fixture['points'], fixture['k']
    oracle = oracle_for(tuple(map(tuple, points)))
    need(data['schema'] == 'mhgp9_weighted_catalogue_export_v1' and data['status'] == 'completed', 'export schema')
    need(data['catalog_universe'] == 'gabriel_complete_boundary', 'catalogue universe')
    need(data['beta_unit'] == 'squared_radius_grid_units' and data['shell_cap'] == 12, 'catalogue units/domain')
    validate_forest(data['native'], points, k)
    expected_balls = {key: row for key, row in oracle.balls.items()
                      if len(row['interior']) + min(map(len, row['supports'])) <= k+1}
    observed, geometries, total_masks, rejected = {}, [], 0, 0
    for i, row in enumerate(data['catalogue']):
        need(row['id'] == i, 'catalogue sequential IDs')
        key = row['key']
        a, b, c = int(key['a']), tuple(map(int, key['b'])), int(key['c'])
        need(a > 0 and len(b) == 3 and math.gcd(a, *b, c) == 1, 'primitive ball key')
        center = tuple(F(-x, 2*a) for x in b)
        beta = sum(x*x for x in center) - F(c, a)
        need(beta == rational(row['beta'], True), 'key/beta consistency')
        geometry = center, beta
        need(geometry in expected_balls and geometry not in observed, 'unexpected/repeated catalogue ball')
        expected = expected_balls[geometry]
        interior, shell = point_ids(row['interior'], oracle.n), point_ids(row['shell'], oracle.n)
        need(interior == expected['interior'] and shell == expected['shell'], 'complete exact ball census')
        support_masks = sorted(mask_of(shell.index(x) for x in support) for support in expected['supports'])
        need(sorted(row['minimal_support_masks']) == support_masks, 'all minimal support masks')
        need(row['q_min'] == min(map(len, expected['supports'])), 'minimal support arity')
        wanted = k+1-len(interior)
        for ids in combinations(range(len(shell)), wanted) if 0 <= wanted <= len(shell) else ():
            total_masks += 1
            bits = mask_of(ids)
            rejected += not any(bits & support == support for support in support_masks)
        observed[geometry] = row
        geometries.append(geometry)
    need(set(observed) == set(expected_balls), 'complete catalogue of eligible support balls')
    all_cofaces, expected_cofaces = oracle.cofaces(k)
    actual = {}
    for row in data['cofaces']:
        ids = point_ids(row['vertices'], oracle.n, k+1)
        beta = rational(row['beta'], True)
        need(ids not in actual, 'repeated coface')
        ball_id = integer(row['ball'])
        need(ball_id < len(geometries), 'coface ball ID')
        need(oracle.meb(ids) == geometries[ball_id], 'coface MEB/ball binding')
        ball = data['catalogue'][ball_id]
        bits = integer(row['shell_mask'])
        need(bits < 1 << len(ball['shell']), 'coface shell mask range')
        selected = tuple(sorted(ball['interior'] + [x for j,x in enumerate(ball['shell']) if bits & (1 << j)]))
        need(ids == selected, 'coface interior/shell binding')
        actual[ids] = beta
    need(actual == expected_cofaces, 'exhaustive Gabriel cofaces and exact levels')
    need(data['stats']['cardinality_candidates'] == total_masks, 'candidate count')
    need(data['stats']['rejected_center_masks'] == rejected, 'center rejection count')
    births = {ids: oracle.meb(ids)[1] for ids in combinations(range(oracle.n), k)}
    cuts = {F(0), *births.values(), *all_cofaces.values()}
    cuts.update(rational(node['level']) for node in data['native']['nodes'])
    cuts.update(rational(row['level']) for row in data['native']['contributions'])
    queries = 0
    for cut in sorted(cuts):
        for closed in (False, True):
            cech = graph_coverage(k, all_cofaces, cut, closed, births)
            gabriel = graph_coverage(k, expected_cofaces, cut, closed)
            full = native_coverage(data['native'], cut, closed)
            need(cech == gabriel, f'Cech/Gabriel nontrivial coverage at {cut}, closed={closed}')
            need(gabriel == full, f'Gabriel/FULL nontrivial coverage at {cut}, closed={closed}')
            queries += 1
    return dict(name=fixture['name'], n_points=oracle.n, k=k,
                support_candidates=len(oracle.candidates), catalogue_balls=len(observed),
                extra_balls=sum(len(row['shell']) > row['q_min'] for row in observed.values()),
                coface_subsets=len(all_cofaces), gabriel_cofaces=len(expected_cofaces),
                rejected_center_masks=rejected, cut_queries=queries,
                strict_and_closed=True, full_nontrivial_coverage_equal=True)


def scope_summary(results):
    fields = ('catalogue_balls', 'extra_balls', 'coface_subsets', 'gabriel_cofaces',
              'rejected_center_masks', 'cut_queries')
    total = {key: sum(row[key] for row in results) for key in fields}
    need(total['extra_balls'] > 0 and total['rejected_center_masks'] > 0, 'nonregular cases genuinely exercised')
    return dict(cases=len(results), **total)


def pins_for(build_receipt, binary):
    build = json.loads(build_receipt.read_text())
    need(build['status'] == 'completed', 'completed native build required')
    need(Path(build['binary']).resolve() == binary and build['binary_sha256'] == sha(binary), 'native build/binary binding')
    need(build['pins_before'] == build['pins_after'], 'native build closure')
    pins = dict(build['pins_after'])
    for name in ('qualify_geometry.py', 'test_qualify_geometry.py', 'README_ORACLE.md',
                 'AUDIT_MATH.md', 'weighted_model.py', 'test_weighted_model.py',
                 'weighted_eom.py', 'test_weighted_eom.py', 'test_native_export.py',
                 'native_weighted_export.cpp'):
        path = HERE/name
        need(path.is_file(), 'qualification pin missing: '+name)
        pins[str(path)] = sha(path)
    for path in (build_receipt, binary, Path(sys.executable).resolve()):
        pins[str(path)] = sha(path)
    old = ROOT/'morsehgp3D_v9/audits/b_point_hierarchy_k_20260927/eom.py'
    pins[str(old)] = sha(old)
    need(all(Path(path).is_file() and sha(path) == digest for path,digest in pins.items()), 'build sources no longer live')
    return pins


def gate_commands(binary):
    commands = []
    for test in ('test_qualify_geometry.py', 'test_weighted_model.py',
                 'test_weighted_eom.py', 'test_native_export.py'):
        for optimized in (False, True):
            name = test.removesuffix('.py') + ('_optimized' if optimized else '_normal')
            argv = [sys.executable, '-B', *(['-O'] if optimized else []), str(HERE/test)]
            if test == 'test_native_export.py':
                argv += ['--native', str(binary)]
            commands.append((name, argv))
    commands.append(('native_gate', [str(binary), '--gate']))
    return commands


def capture(args):
    output, binary, build_receipt = args.output.resolve(), args.binary.resolve(), args.build_receipt.resolve()
    need(output.parent.is_dir() and not output.exists(), 'NEW output directory required')
    need(not output.is_relative_to(ROOT/'morsehgp3D_v9'), 'private capture outside versioned source')
    before = pins_for(build_receipt, binary)
    output.mkdir()
    started = time.monotonic()
    receipt = dict(schema=SCHEMA, status='running', native_binary=str(binary),
                   native_binary_sha256=sha(binary), build_receipt=str(build_receipt),
                   build_receipt_sha256=sha(build_receipt),
                   sources_before=before, sources_after={}, commands=[], fixtures=fixtures(), results=[],
                   GCP_used=False, production_geometry_rerun=False,
                   scope='tiny_exact_catalogue_and_nontrivial_cover_not_weighted_label_quality')
    save(output/'receipt.json', receipt)
    def run(name, argv, timeout=60):
        stdout, stderr = output/(name+'.stdout'), output/(name+'.stderr')
        command = dict(name=name, argv=list(map(str,argv)), cwd=str(ROOT), timeout_seconds=timeout)
        save(output/(name+'.command.json'), command)
        t = time.monotonic()
        try:
            with stdout.open('wb') as out, stderr.open('wb') as err:
                result = subprocess.run(command['argv'], cwd=ROOT, stdout=out, stderr=err,
                                        timeout=timeout, check=False)
            command['returncode'] = result.returncode
        except subprocess.TimeoutExpired:
            command.update(returncode=None, timed_out=True)
        command.update(elapsed_seconds=time.monotonic()-t, stdout_sha256=sha(stdout), stderr_sha256=sha(stderr))
        save(output/(name+'.command.json'), command)
        receipt['commands'].append(command)
        save(output/'receipt.json', receipt)
        need(command['returncode'] == 0, 'command failed: '+name)
        return stdout
    try:
        for name, argv in gate_commands(binary):
            run(name, argv)
        for fixture in receipt['fixtures']:
            name = fixture['name']
            input_path = output/(name+'.u32le')
            input_path.write_bytes(b''.join(struct.pack('<III', *p) for p in fixture['points']))
            fixture['input_sha256'] = sha(input_path)
            stdout = run(name, [binary, '--input', input_path, '--k', str(fixture['k']),
                                '--workers', '1', '--verify-coverage'])
            receipt['results'].append(check_export(json.loads(stdout.read_text()), fixture))
            save(output/'receipt.json', receipt)
        receipt['summary'] = scope_summary(receipt['results'])
        receipt['status'] = 'passed'
    except BaseException as exc:
        receipt.update(status='failed', error=f'{type(exc).__name__}: {exc}')
        raise
    finally:
        receipt['sources_after'] = {path:sha(path) if Path(path).is_file() else None for path in before}
        if receipt['sources_after'] != before:
            receipt.update(status='failed', error='qualification source/binary closure changed')
        receipt['elapsed_seconds'] = time.monotonic()-started
        receipt['artifacts'] = {p.name:sha(p) for p in sorted(output.iterdir()) if p.is_file() and p.name != 'receipt.json'}
        save(output/'receipt.json', receipt)
    need(receipt['status'] == 'passed', receipt.get('error', 'qualification failed'))
    print(json.dumps(dict(status='passed', output=str(output), **receipt['summary']), sort_keys=True))


def readback(directory):
    directory = directory.resolve()
    receipt = json.loads((directory/'receipt.json').read_text())
    need(receipt['schema'] == SCHEMA and receipt['status'] == 'passed', 'passed qualification receipt required')
    need(receipt['sources_before'] == receipt['sources_after'], 'source closure')
    need(all(Path(p).is_file() and sha(p) == h for p,h in receipt['sources_before'].items()), 'LIVE source/binary pins')
    need(sha(receipt['native_binary']) == receipt['native_binary_sha256'], 'binary hash')
    need(sha(receipt['build_receipt']) == receipt['build_receipt_sha256'], 'build receipt hash')
    need(pins_for(Path(receipt['build_receipt']), Path(receipt['native_binary'])) == receipt['sources_before'],
         'build/qualification pin inventory')
    actual_artifacts = {p.name:sha(p) for p in sorted(directory.iterdir()) if p.is_file() and p.name != 'receipt.json'}
    need(actual_artifacts == receipt['artifacts'], 'artifact closure')
    expected_fixtures = fixtures()
    need(len(receipt['fixtures']) == len(expected_fixtures), 'fixture count')
    commands = receipt['commands']
    gates = gate_commands(receipt['native_binary'])
    need(len(commands) == len(expected_fixtures)+len(gates), 'command inventory')
    for i, command in enumerate(commands):
        need(command['returncode'] == 0 and not command.get('timed_out'), 'successful recorded command')
        name = command['name']
        need(json.loads((directory/(name+'.command.json')).read_text()) == command, 'command receipt binding')
        need(sha(directory/(name+'.stdout')) == command['stdout_sha256'] and
             sha(directory/(name+'.stderr')) == command['stderr_sha256'], 'command streams')
        need(command['cwd'] == str(ROOT), 'command working directory')
        if i < len(gates):
            gate_name, expected = gates[i]
            need(name == gate_name, 'gate command order')
        else:
            fixture = expected_fixtures[i-len(gates)]
            need(name == fixture['name'], 'native command order')
            expected = [receipt['native_binary'], '--input', str(directory/(name+'.u32le')),
                        '--k', str(fixture['k']), '--workers', '1', '--verify-coverage']
        need(command['argv'] == expected, 'command exact argv')
    results = []
    for fixture, expected in zip(receipt['fixtures'], expected_fixtures):
        need({k:v for k,v in fixture.items() if k != 'input_sha256'} == expected, 'fixed fixture identity')
        input_path = directory/(fixture['name']+'.u32le')
        need(sha(input_path) == fixture['input_sha256'], 'input file hash')
        need(input_path.read_bytes() == b''.join(struct.pack('<III', *p) for p in fixture['points']), 'input coordinates')
        data = json.loads((directory/(fixture['name']+'.stdout')).read_text())
        results.append(check_export(data, fixture))
    need(results == receipt['results'] and scope_summary(results) == receipt['summary'], 'independent replay results')
    print(json.dumps(dict(status='passed', readback=str(directory), **receipt['summary']), sort_keys=True))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--binary', type=Path)
    parser.add_argument('--build-receipt', type=Path)
    parser.add_argument('--output', type=Path)
    parser.add_argument('--readback', type=Path)
    args = parser.parse_args()
    if args.readback:
        need(not any((args.binary, args.build_receipt, args.output)), 'readback/capture options exclusive')
        readback(args.readback)
    else:
        need(all((args.binary, args.build_receipt, args.output)), 'capture requires binary/build-receipt/output')
        capture(args)
