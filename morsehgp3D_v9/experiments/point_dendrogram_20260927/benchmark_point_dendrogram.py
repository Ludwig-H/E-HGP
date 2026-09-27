#!/usr/bin/env python3
"""Fixed, known Gaussian K5 pilot after FULL; no geometry or HDBSCAN fit.

Scores are dyadic lifts of the pinned rounded S_tau, even at z=2. The
projection receives no truth. Selection is common point-cardinality EOM.
All 52 new selections and 182 unchanged inherited rows must be retained.
New output only; partial units/failures are not silently resumed/promoted.
"""
from __future__ import annotations

import argparse
from collections import Counter
from fractions import Fraction
import gc
import gzip
import hashlib
import importlib.metadata
import json
import math
from pathlib import Path
import platform
import signal
import sys
import time
import traceback

HERE = Path(__file__).resolve().parent
V9 = HERE.parents[1]
WEIGHTED = HERE.parent / 'weighted_clustering_20260927'
FIRST = V9 / 'audits/b_point_hierarchy_k_20260927'
GAUSSIAN = V9 / 'audits/b_gaussian_point_clustering_20260927'
for directory in (FIRST, GAUSSIAN, WEIGHTED):
    sys.path.insert(0, str(directory))

from point_routing_reference import route_points, cut as routing_cut
from point_tree import build_point_tree, validate_point_tree, cut, to_jsonable
from point_eom import cluster_point_tree
from benchmark import metrics, clean
from evaluation import evaluate_labels
import report_full_weighted_parallel as inherited
from threadpoolctl import threadpool_limits

CAPTURE_SHA = '577930f26f4f97fd4f13fb99bb1e356e232a8479055c7f6e09a1bab87bbea720'
POST_SHA = 'b048e8d34e8e3548e4e05340f333301c276ef04a0cacbbde4ad99814e010ee00'
MANIFEST_SHA = '1cab6404055aebf0dfa31b76f16f8a4c84e4acf9678b2b910dabd9f367f46c67'
QUALIFICATION_SHA = '1507b1117092574ad2a2466bdabe6cab78517171d66ba48b7b05cea3d6e6414c'
SCHEMA = 'mhgp9_exact_routed_point_gaussian_pilot_v1'
METHOD = 'hgp_exclusive_point_routing'
CASES = tuple(inherited.formatting.CASES)
KEYS = ('case', 'k', 'min_cluster_size', 'exp_z', 'method')


def need(ok, message):
    if not ok:
        raise ValueError(message)


sha = inherited.sha


def unique_object(pairs):
    result = {}
    for key, value in pairs:
        need(key not in result, 'duplicate JSON key: ' + key)
        result[key] = value
    return result


def read(path):
    opener = gzip.open if str(path).endswith('.gz') else open
    with opener(path, 'rt') as stream:
        return json.load(stream, object_pairs_hook=unique_object)


def save(path, value):
    # Receipts within this NEW capture are checkpoints; payloads are exclusive.
    with Path(path).open('w') as stream:
        json.dump(clean(value), stream, sort_keys=True, indent=2, allow_nan=False)
        stream.write('\n')


def payload(path, value):
    with gzip.open(path, 'xt', compresslevel=1) as stream:
        json.dump(clean(to_jsonable(value)), stream, sort_keys=True,
                  separators=(',', ':'), allow_nan=False)
        stream.write('\n')


def integer_map(values):
    result = {}
    for key, value in values.items():
        need(isinstance(key, str) and key.isascii() and key.isdecimal() and
             str(int(key)) == key, 'canonical integer map key')
        result[int(key)] = value
    return result


def source_inventory():
    paths = [HERE / name for name in ('benchmark_point_dendrogram.py', 'point_tree.py',
              'point_eom.py', 'test_point_tree.py', 'test_point_eom_audit.py', 'PLAN.md')]
    # Frozen pure-Python dependencies include dynamically imported EOM/oracles.
    for directory in (WEIGHTED, FIRST, GAUSSIAN):
        paths.extend(sorted(directory.glob('*.py')))
    paths.append(Path(sys.executable).resolve())
    return {str(path): sha(path) for path in paths}


def check_pins(pins):
    for path, digest in pins.items():
        need(sha(path) == digest, 'LIVE pin changed: ' + path)


def validate_grid(rows):
    expected = {(case, 5, m, z, method) for case in CASES for m in (20, 50)
                for z in (1, 2) for method in (*inherited.formatting.METHODS, METHOD)}
    expected.update((case, 5, m, 1, 'hdbscan_standard') for case in CASES for m in (20, 50))
    keys = [tuple(row[key] for key in KEYS) for row in rows]
    need(len(keys) == 234 and set(keys) == expected, 'complete unique 234-row grid')


def validate_qualification(path):
    need(sha(path) == QUALIFICATION_SHA, 'point-tree/EOM qualification')
    qualification = read(path)
    need(qualification['status'] == 'passed' and
         qualification['sources_before'] == qualification['sources_after'] and
         len(qualification['commands']) == 4 and
         all(command['returncode'] == 0 for command in qualification['commands']),
         'four passed closed point-tree/EOM gates')
    pins = dict(qualification['sources_after'])
    pins[str(path)] = QUALIFICATION_SHA
    pins.update({str(path.parent/name): digest for name, digest in qualification['artifacts'].items()})
    check_pins(pins)
    return pins


def build_unit(case, exponent, measure_path, directory):
    times = {}; started = time.perf_counter()
    measure = read(measure_path)
    facets = measure['facets']; scores = measure['scores']
    need(all(len(facet) == 5 for facet in facets), 'fixed K5 facets')
    need(all(type(score) is float and math.isfinite(score) and score > 0 for score in scores),
         'positive finite rounded binary64 source scores')
    children = integer_map(measure['children'])
    levels = integer_map(measure['squared_levels'])
    nodes = set(range(len(facets))) | set(children)
    roots = sorted(nodes - {child for kids in children.values() for child in kids})
    rational_scores = [Fraction.from_float(score) for score in scores]
    times['load_and_lift'] = 1000*(time.perf_counter()-started)
    started = time.perf_counter()
    routing = route_points(case['n'], facets, rational_scores, children, levels, roots,
                           measure['leaf_birth_betas'])
    times['routing_reference'] = 1000*(time.perf_counter()-started)
    started = time.perf_counter()
    tree = build_point_tree(routing)
    validate_point_tree(tree)
    times['point_tree_and_validation'] = 1000*(time.perf_counter()-started)
    started = time.perf_counter()
    dates = sorted(set(tree['squared_levels'].values()))
    selected = {Fraction(0)}
    if dates:
        selected.update(dates[q*(len(dates)-1)//4] for q in range(5))
        selected.add(dates[-1]+1)
    checks = []
    for beta in sorted(selected):
        for closed in (False, True):
            blocks = cut(tree, beta, closed=closed)
            need(blocks == routing_cut(routing, beta, closed=closed), 'exact sample cut differs')
            checks.append(dict(beta=to_jsonable(beta), closed=closed, blocks=len(blocks),
                partition_sha256=hashlib.sha256(json.dumps(blocks, separators=(',', ':')).encode()).hexdigest()))
    times['differential_cut_checks'] = 1000*(time.perf_counter()-started)
    routing_statistics = routing['statistics']
    attachment_reasons = dict(Counter(row['reason'] for row in routing['attachments']))
    del measure, facets, scores, children, levels, rational_scores, routing
    started = time.perf_counter()
    tree_path = directory / 'point_tree.json.gz'
    payload(tree_path, tree)
    times['save_point_tree_with_source_provenance'] = 1000*(time.perf_counter()-started)
    results = []
    # Truth is read only AFTER routing, the point tree and all selections.
    for minimum in (20, 50):
        started = time.perf_counter()
        result = cluster_point_tree(tree, min_cluster_size=minimum, exp_z=exponent)
        selection_ms = 1000*(time.perf_counter()-started)
        selection_path = directory / f'selection_m{minimum}.json.gz'
        payload(selection_path, result)
        results.append((minimum, result, selection_path, selection_ms))
    truth = read(case['labels_json'])
    need(len(truth) == case['n'] == 1200 and all(type(label) is int for label in truth), 'whole scene truth')
    rows = []
    for minimum, result, selection_path, selection_ms in results:
        prediction = result['result']['selection']['labels']
        row = {key: case[key] for key in ('regime', 'communities', 'separation', 'seed', 'n')}
        row.update(case=case['id'], k=5, min_cluster_size=minimum, exp_z=exponent, method=METHOD,
            metrics=metrics(truth, prediction), extra=evaluate_labels(truth, prediction, min_cluster_size=minimum),
            times_ms=dict(**times, selection=selection_ms), routing_statistics=routing_statistics,
            point_tree_statistics=tree['statistics'], attachment_reasons=attachment_reasons,
            labels_payload=str(selection_path), labels_payload_sha256=sha(selection_path))
        rows.append(row)
    return dict(case=case['id'], k=5, exp_z=exponent, status='completed', rows=rows,
                source_measure=str(measure_path), source_measure_sha256=sha(measure_path),
                point_tree=str(tree_path), point_tree_sha256=sha(tree_path),
                times_ms=times, checks=checks, routing_statistics=routing_statistics,
                point_tree_statistics=tree['statistics'], attachment_reasons=attachment_reasons)


def run(args):
    output = args.output.resolve()
    need(not output.exists(), 'NEW capture directory required')
    output.mkdir(parents=True)
    receipt = dict(schema=SCHEMA, status='running', argv=sys.argv,
        primary=dict(k=5, min_cluster_size=20, exp_z=1),
        plan=dict(cases=list(CASES), k=[5], min_cluster_size=[20, 50], exp_z=[1, 2]),
        scope='known_gaussian_diagnostic_after_FULL_not_heldout_or_GPU',
        weights='Fraction.from_float_pinned_rounded_S_tau_both_z',
        EOM_arithmetic='binary64_radii_lambda_stability', mass_policy='one_per_point',
        root_policy='single_real_root_excluded', GCP_used=False, GPU_used=False,
        geometry_rerun=False, hdbscan_fits_rerun=False, units=[], rows=[], artifacts={},
        sources_before=source_inventory(),
        runtime=dict(python=sys.version, executable=sys.executable, platform=platform.platform(),
            packages={name: importlib.metadata.version(name) for name in ('numpy', 'scipy', 'scikit-learn', 'threadpoolctl')}))
    started = time.perf_counter()
    save(output / 'receipt.json', receipt)
    try:
        qualification_pins = validate_qualification(args.qualification)
        receipt['qualification'] = str(args.qualification)
        receipt['qualification_sha256'] = QUALIFICATION_SHA
        receipt['qualification_pins'] = qualification_pins
        need(sha(args.capture/'receipt.json') == CAPTURE_SHA and sha(args.post_audit) == POST_SHA,
             'exact completed inherited capture/post-audit')
        old, _, _, pins, _ = inherited.validated_inputs(args.capture.resolve(), args.post_audit.resolve())
        receipt['inherited_pins'] = pins
        receipt['inherited_receipt'] = str(args.capture/'receipt.json')
        receipt['inherited_receipt_sha256'] = CAPTURE_SHA
        manifest_path = Path(old['manifest'])
        need(sha(manifest_path) == MANIFEST_SHA, 'fixed input manifest')
        manifest = {case['id']: case for case in read(manifest_path)['cases']}
        receipt['manifest'] = str(manifest_path); receipt['manifest_sha256'] = MANIFEST_SHA
        receipt['rows'] = [row for row in old['rows'] if row['k'] == 5]
        need(len(receipt['rows']) == 182, 'all inherited K5 rows')
        for case_id in CASES:
            case = manifest[case_id]
            for exponent in (1, 2):
                directory = output / f'{case_id}_k5_z{exponent}'
                directory.mkdir()
                source = args.capture / f'{case_id}_k5' / f'measure_z{exponent}.json.gz'
                need(sha(source) == old['artifacts'][str(source)], 'bound inherited measure')
                print(f'START {case_id} K5 z{exponent}', flush=True)
                with threadpool_limits(limits=1):
                    unit = build_unit(case, exponent, source, directory)
                save(directory/'unit.json', unit)
                receipt['units'].append(unit)
                receipt['rows'].extend(unit['rows'])
                for artifact in directory.iterdir():
                    receipt['artifacts'][str(artifact)] = sha(artifact)
                save(output/'receipt.json', receipt)
                print(json.dumps(dict(case=case_id, z=exponent, status='completed',
                    routing_ms=unit['times_ms']['routing_reference'],
                    scores=[dict(m=row['min_cluster_size'], ari=row['metrics']['ari_all'],
                                 clusters=row['metrics']['clusters']) for row in unit['rows']])), flush=True)
                del unit
                gc.collect()
        validate_grid(receipt['rows'])
        check_pins(pins)
        check_pins(qualification_pins)
        check_pins(receipt['artifacts'])
        receipt['sources_after'] = source_inventory()
        need(receipt['sources_after'] == receipt['sources_before'], 'new source closure')
        receipt['status'] = 'completed'
    except BaseException as error:
        receipt['status'] = 'failed'; receipt['error'] = repr(error)
        receipt['traceback'] = traceback.format_exc()
        receipt['sources_after'] = source_inventory()
        raise
    finally:
        receipt['elapsed_seconds'] = time.perf_counter()-started
        save(output/'receipt.json', receipt)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--capture', type=Path, required=True)
    parser.add_argument('--post-audit', type=Path, required=True)
    parser.add_argument('--qualification', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    def interrupted(signum, _frame):
        raise InterruptedError('received signal ' + str(signum))
    signal.signal(signal.SIGTERM, interrupted)
    run(args)
