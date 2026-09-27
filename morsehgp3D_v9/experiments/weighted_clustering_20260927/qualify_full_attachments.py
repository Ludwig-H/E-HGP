#!/usr/bin/env python3
"""New FULL-attachment qualification, superseding only the naive graph claim.

Explicit port of qualify_geometry.py capture/readback and catalogue checks.
The frozen R1 source and receipt stay unchanged. Fraction geometry remains
independent of C++; connectivity now uses ALL Cech cofaces (tiny clouds only).
"""
from __future__ import annotations
import argparse
from collections import defaultdict
from fractions import Fraction as F
from itertools import combinations
import json
import math
from pathlib import Path
import struct
import subprocess
import sys
import time

from qualify_geometry import (ROOT, HERE, need, sha, save, integer, rational,
    oracle_for, point_ids, mask_of, validate_forest, graph_coverage,
    native_coverage, fixtures as old_fixtures)
from full_attachment_oracle import build_reference, reference_cut
from full_weighted_tree import build_full_weighted_tree, tree_cut

SCHEMA = 'mhgp9_weighted_full_attachment_qualification_v1'


def fixtures():
    rows = old_fixtures()
    e5 = [[0,0,7], [0,9,6], [1,4,0], [0,0,1], [4,1,2]]
    for k in (1,2,3,4):
        rows.append(dict(name=f'e5_silent_k{k}', points=e5, k=k))
    rows.append(dict(name='square_empty_weighted_k4',
                     points=[[0,0,0], [2,0,0], [2,2,0], [0,2,0]], k=4))
    return rows


def check_catalogue(data, fixture):
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
    queries, naive_disagreements = 0, 0
    for cut in sorted(cuts):
        for closed in (False, True):
            cech = graph_coverage(k, all_cofaces, cut, closed, births)
            gabriel = graph_coverage(k, expected_cofaces, cut, closed)
            full = native_coverage(data['native'], cut, closed)
            naive_disagreements += cech != gabriel
            need(cech == full, f'Cech/FULL nontrivial coverage at {cut}, closed={closed}')
            queries += 1
    return dict(name=fixture['name'], n_points=oracle.n, k=k,
                support_candidates=len(oracle.candidates), catalogue_balls=len(observed),
                extra_balls=sum(len(row['shell']) > row['q_min'] for row in observed.values()),
                coface_subsets=len(all_cofaces), gabriel_cofaces=len(expected_cofaces),
                rejected_center_masks=rejected, cut_queries=queries,
                strict_and_closed=True, full_nontrivial_coverage_equal=True,
                naive_disagreements=naive_disagreements)



def check_export(payload, fixture):
    need(payload['schema'] == 'mhgp9_weighted_full_attachment_export_v1' and
         payload['status'] == 'completed', 'attachment export schema/status')
    data = payload['weighted']
    result = check_catalogue(data, fixture)
    reference = build_reference(fixture['points'], fixture['k'], exp_z=2)
    facets = [tuple(face) for face in reference['facets']]
    rows = payload['attachments']
    need([point_ids(row['vertices'], len(fixture['points']), fixture['k'])
          for row in rows] == facets, 'every weighted facet attached once in canonical order')
    native = data['native']
    nodes = native['nodes']
    anchors = payload['anchors']
    if fixture['k'] == 1:
        need(anchors == [] and payload['validation']['anchors_available'] is False,
             'K1 direct point leaves, no captured ball anchors')
    else:
        need(len(anchors) == len(data['catalogue']) and payload['validation']['anchors_available'] is True,
             'one closed anchor slot per catalogue ball')
    for ball, node in zip(data['catalogue'], anchors):
        if node is None:
            continue
        integer(node)
        beta = rational(ball['beta'])
        need(node < len(nodes) and rational(nodes[node]['level']) <= beta, 'ball anchor after birth')
        successor = nodes[node]['successor']
        need(successor is None or beta < rational(nodes[successor]['level']), 'ball anchor normalized at ball level')
    for facet, row, birth in zip(facets, rows, reference['leaf_birth_betas']):
        beta = rational(row['beta'])
        need(beta == birth, 'facet attachment at exact MEB birth')
        node = integer(row['node'])
        need(node < len(nodes) and rational(nodes[node]['level']) <= beta, 'attachment after node birth')
        successor = nodes[node]['successor']
        need(successor is None or beta < rational(nodes[successor]['level']), 'attachment normalized closed')
        anchor = integer(row['anchor_node'])
        need(anchor < len(nodes), 'attachment anchor node domain')
        if fixture['k'] == 1:
            need(row['terminal_ball'] is None and beta == 0 and anchor == node, 'K1 direct point birth')
            covered = set()
            for contribution in native['contributions']:
                if contribution['segment'] != node or rational(contribution['level']) != 0:
                    continue
                population = native['populations'][contribution['population']]
                if contribution['include_interior']:
                    covered.update(population['interior'])
                covered.update(x for bit,x in enumerate(population['shell'])
                               if contribution['shell_mask'] & (1 << bit))
            need(covered == set(facet), 'K1 point/leaf identity')
        else:
            terminal = integer(row['terminal_ball'])
            need(terminal < len(anchors) and anchors[terminal] == anchor, 'terminal/closed anchor binding')
            need(rational(data['catalogue'][terminal]['beta']) <= beta, 'terminal not above facet birth')
            while nodes[anchor]['successor'] is not None:
                following = nodes[anchor]['successor']
                if rational(nodes[following]['level']) > beta:
                    break
                anchor = following
            need(anchor == node, 'closed terminal normalization')
    weighted_tree = build_full_weighted_tree(nodes, native['roots'], reference['masses'],
                     [dict(node=row['node'], beta=row['beta']) for row in rows])
    oracle = oracle_for(tuple(map(tuple, fixture['points'])))
    all_cofaces, _ = oracle.cofaces(fixture['k'])
    cuts = {F(0), *all_cofaces.values(), *reference['leaf_birth_betas'],
            *(rational(node['level']) for node in nodes)}
    checks = 0
    for cut in sorted(cuts):
        for closed in (False, True):
            admitted = lambda beta: beta <= cut if closed else beta < cut
            groups = defaultdict(list)
            for index, row in enumerate(rows):
                if not admitted(rational(row['beta'])):
                    continue
                node = row['node']
                while nodes[node]['successor'] is not None:
                    successor = nodes[node]['successor']
                    if not admitted(rational(nodes[successor]['level'])):
                        break
                    node = successor
                groups[node].append(index)
            actual = sorted(tuple(ids) for ids in groups.values())
            expected = sorted(tuple(group['facet_ids'])
                              for group in reference_cut(reference, cut, closed=closed)
                              if group['facet_ids'])
            need(actual == expected, f'FULL facet partition at {cut}, closed={closed}')
            augmented = tree_cut(weighted_tree, cut, closed=closed)
            need([tuple(block) for block in augmented['blocks']] == expected,
                 f'augmented weighted tree partition at {cut}, closed={closed}')
            checks += 1
    result.update(attached_facets=len(facets), attachment_cut_queries=checks)
    return result


def scope_summary(results):
    fields = ('catalogue_balls', 'extra_balls', 'coface_subsets', 'gabriel_cofaces',
              'rejected_center_masks', 'cut_queries', 'naive_disagreements',
              'attached_facets', 'attachment_cut_queries')
    total = {key: sum(row[key] for row in results) for key in fields}
    need(total['extra_balls'] > 0 and total['rejected_center_masks'] > 0,
         'nonregular cases genuinely exercised')
    need(total['naive_disagreements'] > 0, 'E5 must kill naive Gabriel reconstruction')
    return dict(cases=len(results), **total)


def pins_for(build_receipt, binary):
    build = json.loads(build_receipt.read_text())
    need(build['status'] == 'completed', 'completed native build required')
    need(Path(build['binary']).resolve() == binary and build['binary_sha256'] == sha(binary),
         'native build/binary binding')
    need(build['pins_before'] == build['pins_after'], 'native build closure')
    pins = dict(build['pins_after'])
    wrapper_path = build_receipt.parent.with_name(build_receipt.parent.name+'-adapter')/'receipt.json'
    wrapper = json.loads(wrapper_path.read_text())
    need(wrapper['status'] == 'completed' and wrapper['pins_match'] is True and
         wrapper['pins_before'] == wrapper['pins_after'], 'attachment wrapper closure')
    need(Path(wrapper['binary']).resolve() == binary and wrapper['binary_sha256'] == sha(binary),
         'wrapper/binary binding')
    rename = wrapper['generated_main_rename']
    need(rename == dict(before='int main(int argc,char** argv) {',
                        after='int mhgp9_frozen_weighted_export_main(int argc,char** argv) {', count=1),
         'explicit single main rename')
    original = (HERE/'native_weighted_export.cpp').read_text()
    generated = wrapper_path.parent/'native_weighted_export_renamed.hpp'
    need(original.count(rename['before']) == 1 and
         generated.read_text() == original.replace(rename['before'], rename['after']),
         'generated inclusion differs only by declared main rename')
    need((wrapper_path.parent/'native_weighted_export.cpp').read_text() ==
         '#include "native_attachment_export.cpp"\n', 'generated unit identity')
    pins.update(wrapper['pins_after'])
    pins.update(wrapper['generated_sha256'])
    pins[str(wrapper_path)] = sha(wrapper_path)
    for name in ('qualify_full_attachments.py', 'test_qualify_full_attachments.py',
                 'test_qualify_geometry.py', 'qualify_geometry.py',
                 'full_attachment_oracle.py', 'test_full_attachment_oracle.py',
                 'full_weighted_tree.py', 'test_full_weighted_tree.py',
                 'weighted_model.py', 'test_weighted_model.py',
                 'weighted_eom.py', 'test_weighted_eom.py', 'test_attachment_export.py'):
        path = HERE/name
        need(path.is_file(), 'qualification pin missing: '+name)
        pins[str(path)] = sha(path)
    for path in (build_receipt, binary, Path(sys.executable).resolve()):
        pins[str(path)] = sha(path)
    old = ROOT/'morsehgp3D_v9/audits/b_point_hierarchy_k_20260927/eom.py'
    pins[str(old)] = sha(old)
    need(all(Path(path).is_file() and sha(path) == digest for path,digest in pins.items()),
         'build sources no longer live')
    return pins


def gate_commands(binary):
    commands = []
    for test in ('test_qualify_full_attachments.py', 'test_full_attachment_oracle.py', 'test_full_weighted_tree.py',
                 'test_weighted_model.py', 'test_weighted_eom.py', 'test_attachment_export.py'):
        for optimized in (False, True):
            name = test.removesuffix('.py') + ('_optimized' if optimized else '_normal')
            argv = [sys.executable, '-B', *(['-O'] if optimized else []), str(HERE/test)]
            if test == 'test_attachment_export.py':
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
                   GCP_used=False, large_cloud_geometry_rerun=False, native_geometry_executed=True,
                   scope='tiny_exact_FULL_facet_attachments_not_weighted_label_quality')
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
