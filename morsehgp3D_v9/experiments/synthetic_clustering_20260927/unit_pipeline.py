"""One fresh synthetic scene, one K5 export, eighteen newly computed rows.

The parent owns qualification/build bindings, process-group cleanup and the
predeclared scene plan. This worker closes its own runtime/input/artifact pins.
No historical labels or scores are imported. Frozen modules remain unchanged.
"""
from __future__ import annotations

from collections import Counter
from fractions import Fraction
import gzip
import hashlib
import json
import math
from pathlib import Path
import subprocess
import sys
import time
import traceback

HERE = Path(__file__).resolve().parent
V9 = HERE.parents[1]
WEIGHTED = HERE.parent / 'weighted_clustering_20260927'
POINT = HERE.parent / 'point_dendrogram_20260927'
FIRST = V9 / 'audits/b_point_hierarchy_k_20260927'
GAUSSIAN = V9 / 'audits/b_gaussian_point_clustering_20260927'
for directory in (FIRST, GAUSSIAN, WEIGHTED, POINT):
    sys.path.insert(0, str(directory))

import numpy as np
from threadpoolctl import threadpool_limits
from benchmark import clean, metrics
from condensed import point_clusterer_from_tree
from eom import fit_hdbscan, equivalent_labels, sklearn_provenance
from evaluation import evaluate_labels
from projection import SourceTree
from weighted_model import build_facet_model, vote_points
from weighted_eom import weighted_condense_eom
from full_weighted_tree import build_full_weighted_tree, eom_input
from point_routing_reference import route_points, cut as routing_cut
from point_tree import build_point_tree, validate_point_tree, cut, to_jsonable
from point_eom import cluster_point_tree

SCHEMA = 'mhgp9_synthetic_clustering_unit_v1'
NATIVE_SHA = 'c7afdae542074ee90bb8c1ba5a7dfe679ef9851f91bf32bbcbc067b15853576e'
K, SIZES, EXPONENTS = 5, (20, 50), (1, 2)
METHODS = ('hgp_exclusive_point_routing', 'hgp_weighted_full_vote',
           'hgp_first_coverage', 'hdbscan_common')
INPUT_KEYS = ('points_u32le', 'points_npy', 'labels_json')


def need(condition, reason):
    if not condition:
        raise ValueError(reason)


def sha(path):
    digest = hashlib.sha256()
    with Path(path).open('rb') as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b''):
            digest.update(chunk)
    return digest.hexdigest()


def source_paths():
    """Explicit runtime dependency inventory, also available to the parent."""
    return [Path(__file__).resolve()] + [directory / name for directory, names in (
        (WEIGHTED, ('weighted_model.py', 'weighted_eom.py', 'full_weighted_tree.py',
                    'point_routing_reference.py')),
        (POINT, ('point_tree.py', 'point_eom.py')),
        (FIRST, ('benchmark.py', 'eom.py', 'projection.py')),
        (GAUSSIAN, ('condensed.py', 'evaluation.py'))) for name in names]


def verify(pins, description):
    for path, expected in pins.items():
        need(sha(path) == expected, description + ': ' + str(path))


def save(path, value, *, exclusive=True):
    with Path(path).open('x' if exclusive else 'w') as stream:
        json.dump(clean(to_jsonable(value)), stream, sort_keys=True, indent=2, allow_nan=False)
        stream.write('\n')


def payload(path, value):
    # Same JSON meaning as previous pilots; bulk gzip level 1 only affects IO.
    encoded = (json.dumps(clean(to_jsonable(value)), sort_keys=True,
                          separators=(',', ':'), allow_nan=False) + '\n').encode()
    with Path(path).open('xb') as stream:
        with gzip.GzipFile(fileobj=stream, mode='wb', compresslevel=1, mtime=0) as zipped:
            zipped.write(encoded)


def read_geometry(case):
    """Read coordinates, never decode the truth labels before all selections."""
    n = case['n']
    need(type(n) is int and n >= K, 'whole scene size >= K required')
    expected = case['prepared_sha256']
    need(set(expected) == set(INPUT_KEYS), 'three prepared input pins required')
    pins = {str(Path(case[key]).resolve()): expected[key] for key in INPUT_KEYS}
    need(len(pins) == 3, 'distinct input files required')
    verify(pins, 'prepared input changed')
    points = np.load(case['points_npy'], allow_pickle=False)
    raw = np.fromfile(case['points_u32le'], dtype='<u4')
    need(raw.size == 3*n, 'whole binary geometry length')
    binary = raw.reshape(n, 3)
    need(points.dtype == np.float64 and points.shape == (n, 3) and
         np.array_equal(points, binary), 'same exact float64/u32 geometry required')
    need(binary.max() < 2**18 and len(np.unique(binary, axis=0)) == n,
         'distinct u18 sites; no hidden collision merge')
    return points, binary, pins


def beta(value):
    need(isinstance(value, dict) and set(value) == {'num', 'den'}, 'exact beta schema')
    values = []
    for name in ('num', 'den'):
        raw = value[name]
        if isinstance(raw, str):
            need(raw.isascii() and raw.isdecimal() and str(int(raw)) == raw,
                 'canonical unsigned rational')
            raw = int(raw)
        need(type(raw) is int and raw >= 0, 'unsigned rational coefficient')
        values.append(raw)
    need(values[1] > 0, 'positive rational denominator')
    return Fraction(*values)


def validate_export(export, binary):
    """Check command/geometry/anchor bindings, not an exhaustive geometry oracle."""
    need(export.get('schema') == 'mhgp9_weighted_full_attachment_export_v1' and
         export.get('status') == 'completed', 'completed attachment export required')
    weighted = export['weighted']; native = weighted['native']
    need(weighted['schema'] == 'mhgp9_weighted_catalogue_export_v1' and
         weighted['status'] == 'completed' and
         weighted['catalog_universe'] == 'gabriel_complete_boundary' and
         weighted['shell_cap'] == 12 and weighted['beta_unit'] == 'squared_radius_grid_units',
         'qualified catalogue contract')
    need(native['k'] == K and native['point_count'] == len(binary) and
         np.array_equal(native['points'], binary), 'native command/geometry mismatch')
    proof = export['validation']
    need(all(proof[key] is True for key in ('observed_native_every_field_equal',
         'all_capture_slots_admitted', 'anchors_available')), 'native/observed validation flags')
    need(proof['native_tower_digest'] == proof['observed_tower_digest'] == native['tower_digest'],
         'native/observed digest binding')
    anchors, balls, nodes = export['anchors'], weighted['catalogue'], native['nodes']
    node_levels = [beta(row['level']) for row in nodes]
    ball_levels = [beta(row['beta']) for row in balls]
    need(len(anchors) == len(balls), 'one anchor slot per catalogue ball')
    for ball, anchor in zip(balls, anchors):
        if anchor is None:
            continue
        need(type(anchor) is int and 0 <= anchor < len(nodes), 'anchor node domain')
        at = beta(ball['beta']); successor = nodes[anchor]['successor']
        need(node_levels[anchor] <= at and
             (successor is None or at < node_levels[successor]), 'closed ball anchor')
    # Binary lifting keeps attachment validation O((V+F) log V), not F*depth.
    up = [[row['successor'] for row in nodes]]
    for _ in range(len(nodes).bit_length()):
        previous = up[-1]
        up.append([None if parent is None else previous[parent] for parent in previous])
    for row in export['attachments']:
        terminal, anchor, node = row['terminal_ball'], row['anchor_node'], row['node']
        need(type(terminal) is int and 0 <= terminal < len(balls) and
             type(anchor) is int and 0 <= anchor < len(nodes) and anchors[terminal] == anchor,
             'terminal ball/anchor binding')
        at = beta(row['beta'])
        need(ball_levels[terminal] <= at, 'terminal above facet birth')
        current = anchor
        for jump in reversed(up):
            following = jump[current]
            if following is not None and node_levels[following] <= at:
                current = following
        need(type(node) is int and node == current, 'attachment closed successor normalization')
    return native


def validate_rows(rows, case_id):
    expected = {(case_id, K, m, z, method) for m in SIZES for z in EXPONENTS for method in METHODS}
    expected |= {(case_id, K, m, 1, 'hdbscan_standard') for m in SIZES}
    actual = [tuple(row[key] for key in ('case', 'k', 'min_cluster_size', 'exp_z', 'method')) for row in rows]
    need(len(actual) == 18 and set(actual) == expected, 'unique complete 18-row unit')


def _compute(case, directory, binary_path, receipt):
    points, binary, pins = read_geometry(case)
    receipt['input_hashes'] = pins
    argv = [str(binary_path), '--input', str(Path(case['points_u32le']).resolve()),
            '--k', str(K), '--workers', '1']
    started = time.perf_counter()
    with (directory/'native.json').open('xb') as out, (directory/'native.stderr').open('xb') as err:
        completed = subprocess.run(argv, stdout=out, stderr=err, check=False)
    command = dict(argv=argv, returncode=completed.returncode, wall_ms=1000*(time.perf_counter()-started),
        stdout_sha256=sha(directory/'native.json'), stderr_sha256=sha(directory/'native.stderr'),
        binary_sha256=NATIVE_SHA, input_sha256=pins[str(Path(case['points_u32le']).resolve())])
    receipt['commands'].append(command)
    save(directory/'command.json', command)
    for name in ('native.json', 'native.stderr', 'command.json'):
        receipt['artifacts'][str(directory/name)] = sha(directory/name)
    need(completed.returncode == 0, 'native exporter failed')
    started = time.perf_counter()
    export = json.loads((directory/'native.json').read_text())
    # SourceTree validates topology before successor-based binding checks.
    source = SourceTree(export['weighted']['native'])
    native = validate_export(export, binary)
    receipt['native_read_and_validate_ms'] = 1000*(time.perf_counter()-started)
    receipt['native_statistics'] = export['weighted']['stats']
    receipt['attachment_statistics'] = export['stats']
    receipt['native_times_ms'] = native['times_ms']
    predictions = []

    def keep(name, value):
        path = directory/name
        payload(path, value)
        receipt['artifacts'][str(path)] = sha(path)
        return str(path)

    def selection(method, minimum, z, labels, details, elapsed_ms):
        need(len(labels) == case['n'] and all(type(x) is int and x >= -1 for x in labels),
             'one integer prediction per original point')
        path = keep(f'{method}_m{minimum}_z{z}.json.gz', dict(labels=labels, result=details))
        predictions.append(dict(method=method, min_cluster_size=minimum, exp_z=z,
            labels=labels, labels_payload=path, labels_payload_sha256=receipt['artifacts'][path],
            selection_ms=elapsed_ms))

    started = time.perf_counter()
    first = source.project('first_coverage', 1)
    receipt['first_coverage_ms'] = 1000*(time.perf_counter()-started)
    keep('first_coverage.json.gz', first)
    del source
    hdb_digest = None
    for minimum in SIZES:
        started = time.perf_counter()
        fitted = fit_hdbscan(points, k=K, min_cluster_size=minimum, exp_z=1)
        fit_ms = 1000*(time.perf_counter()-started)
        encoded = json.dumps(fitted['tree'], sort_keys=True, allow_nan=False).encode()
        digest = hashlib.sha256(encoded).hexdigest()
        need(hdb_digest is None or hdb_digest == digest, 'HDB source changed with minimum size only')
        hdb_digest = digest
        keep(f'hdbscan_m{minimum}.json.gz', fitted)
        receipt['hdbscan_fits'].append(dict(min_cluster_size=minimum, wall_ms=fit_ms,
            tree_sha256=digest, warnings=fitted['warnings'],
            common_z1_matches_standard=fitted['common_z1_matches_standard']))
        for z in EXPONENTS:
            for method, tree in (('hgp_first_coverage', first['tree']), ('hdbscan_common', fitted['tree'])):
                started = time.perf_counter()
                clustered = point_clusterer_from_tree(tree, min_cluster_size=minimum, exp_z=z)
                elapsed = 1000*(time.perf_counter()-started)
                labels = clustered['selection']['labels']
                if method == 'hdbscan_common' and z == 1:
                    need(equivalent_labels(labels, fitted['common_z1_labels']), 'common HDB z1 consistency')
                selection(method, minimum, z, labels, clustered, elapsed)
        selection('hdbscan_standard', minimum, 1, fitted['standard_labels_z1'],
                  dict(parameters=fitted['parameters'], warnings=fitted['warnings']), 0.0)
    del first, fitted
    cofaces = [dict(vertices=row['vertices'], beta=row['beta']) for row in export['weighted']['cofaces']]
    for z in EXPONENTS:
        times = {}; started = time.perf_counter()
        model = build_facet_model(case['n'], K, cofaces, exp_z=z)
        need(model['facets'] == [tuple(row['vertices']) for row in export['attachments']],
             'canonical model/attachment facet order')
        times['measure_and_naive_graph'] = 1000*(time.perf_counter()-started)
        started = time.perf_counter()
        full = build_full_weighted_tree(native['nodes'], native['roots'], model['masses'],
            [dict(node=row['node'], beta=row['beta']) for row in export['attachments']])
        times['full_facet_tree'] = 1000*(time.perf_counter()-started)
        tree = full['tree']
        measure = dict(facets=model['facets'], scores=model['scores'], point_totals=model['point_totals'],
            masses=model['masses'], children=tree['children'], squared_levels=tree['squared_levels'],
            leaf_birth_betas=full['leaf_birth_betas'], attachments=full['attachments'])
        measure_path = keep(f'measure_z{z}.json.gz', measure)
        need(all(type(score) is float and math.isfinite(score) and score > 0 for score in model['scores']),
             'positive binary64 rounded scores')
        started = time.perf_counter()
        routing = route_points(case['n'], model['facets'], [Fraction.from_float(s) for s in model['scores']],
            tree['children'], tree['squared_levels'], tree['roots'], full['leaf_birth_betas'])
        times['routing_reference'] = 1000*(time.perf_counter()-started)
        started = time.perf_counter()
        point_tree = build_point_tree(routing)
        validate_point_tree(point_tree)
        times['point_tree_and_validate'] = 1000*(time.perf_counter()-started)
        dates = sorted(set(point_tree['squared_levels'].values()))
        selected = {Fraction(0)}
        if dates:
            selected.update(dates[q*(len(dates)-1)//4] for q in range(5))
            selected.add(dates[-1]+1)
        checks = []
        for at in sorted(selected):
            for closed in (False, True):
                blocks = cut(point_tree, at, closed=closed)
                need(blocks == routing_cut(routing, at, closed=closed), 'sample exact point cuts differ')
                checks.append(dict(beta=to_jsonable(at), closed=closed, blocks=len(blocks)))
        point_path = keep(f'point_tree_z{z}.json.gz', point_tree)
        receipt['units'].append(dict(exp_z=z, times_ms=times, checks=checks,
            model_statistics=model['statistics'], full_tree_statistics=full['statistics'],
            routing_statistics=routing['statistics'], point_tree_statistics=point_tree['statistics'],
            attachment_reasons=dict(Counter(row['reason'] for row in routing['attachments'])),
            measure=measure_path, point_tree=point_path,
            score_arithmetic='binary64_S_lifted_exactly_as_dyadic_even_z2'))
        del routing, measure
        for minimum in SIZES:
            started = time.perf_counter()
            clustered = cluster_point_tree(point_tree, min_cluster_size=minimum, exp_z=z)
            elapsed = 1000*(time.perf_counter()-started)
            selection('hgp_exclusive_point_routing', minimum, z,
                      clustered['result']['selection']['labels'], clustered, elapsed)
            started = time.perf_counter()
            weighted = weighted_condense_eom(**eom_input(full, min_cluster_size=minimum), exp_z=z)
            vote = vote_points(model, weighted['labels'])
            elapsed = 1000*(time.perf_counter()-started)
            selection('hgp_weighted_full_vote', minimum, z, vote['labels'],
                      dict(selection=weighted, vote=vote), elapsed)
        del model, full, point_tree
    # No decoded truth has entered geometry, routing, tree construction or selection.
    truth = json.loads(Path(case['labels_json']).read_text())
    need(isinstance(truth, list) and len(truth) == case['n'] and
         all(type(label) is int and (label == -1 or label > 0) for label in truth),
         'truth requires positive component IDs or -1 injected noise')
    for prediction in predictions:
        labels = prediction.pop('labels')
        row = {key: case[key] for key in ('phase', 'axis', 'replicate', 'spec', 'n', 'groups') if key in case}
        if 'spec' in case:
            row.update({key: case['spec'][key] for key in
                        ('family', 'groups', 'separation', 'seed', 'noise_fraction')})
        # Facet mass m is NOT a minimum cardinality after the hard point vote.
        cardinal_minimum = None if prediction['method'] == 'hgp_weighted_full_vote' else prediction['min_cluster_size']
        row.update(case=case['id'], k=K, **prediction,
            metrics=metrics(truth, labels),
            extra=evaluate_labels(truth, labels, min_cluster_size=cardinal_minimum),
            threshold_semantics='facet_mass_before_vote' if cardinal_minimum is None else 'point_cardinality')
        receipt['rows'].append(row)
    validate_rows(receipt['rows'], case['id'])


def run_unit(case, output_dir, native_binary):
    """Return a closed receipt; on any failure preserve evidence and re-raise.

    The native child stays in its caller's process group: only the parent
    scheduler owns signal cleanup. There is no retry, resumption or truncation.
    """
    directory = Path(output_dir).absolute()
    directory.mkdir(parents=True, exist_ok=False)
    binary_path = Path(native_binary).resolve()
    started = time.perf_counter()
    receipt = dict(schema=SCHEMA, status='running', case=case['id'], n=case['n'], k=K,
        rows=[], units=[], commands=[], artifacts={}, hdbscan_fits=[],
        native_binary=str(binary_path), input_hashes={}, GCP_used=False, GPU_used=False,
        engine_modified=False, truth_access='decode_only_after_all_18_predictions',
        catalogue='Gabriel_complete_boundary_not_historical_order_cell_catalogue',
        arithmetic='exact_geometry_and_point_dates_binary64_weights_and_EOM_dyadic_routing',
        root_policy='exclude_real_root_no_artificial_forest_root', gzip_compresslevel=1)
    save(directory/'receipt.json', receipt)
    try:
        receipt.update(native_binary_sha256=sha(binary_path),
            sources_before={str(path): sha(path) for path in source_paths()}, sklearn=sklearn_provenance())
        need(receipt['native_binary_sha256'] == NATIVE_SHA, 'qualified attachment exporter required')
        with threadpool_limits(limits=1):
            _compute(case, directory, binary_path, receipt)
        verify(receipt['input_hashes'], 'input closure')
        verify(receipt['artifacts'], 'artifact closure')
        verify(receipt['sklearn']['sha256'], 'sklearn binary closure')
        need(sha(binary_path) == NATIVE_SHA, 'native binary closure')
        receipt['sources_after'] = {str(path): sha(path) for path in source_paths()}
        need(receipt['sources_after'] == receipt['sources_before'], 'runtime source closure')
        receipt['status'] = 'completed'
        return receipt
    except BaseException as error:
        receipt.update(status='failed', error=repr(error), traceback=traceback.format_exc())
        raise
    finally:
        receipt['elapsed_seconds'] = time.perf_counter()-started
        save(directory/'receipt.json', receipt, exclusive=False)
