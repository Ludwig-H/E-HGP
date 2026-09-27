#!/usr/bin/env python3
"""LIVE readback of the fixed routed-point pilot, never a new scientific run.

Reuses the frozen independent Fraction ARI arithmetic, not production scores.
Replays archived point cuts and validates condensed mass/lifetime/selection
ancestry. Does NOT route again, rebuild condensation, select by EOM, fit or
run geometry. Hungarian's solver is shared; NMI is not recomputed.
"""
from __future__ import annotations

import argparse
from collections import Counter
from copy import deepcopy
from fractions import Fraction as Q
import hashlib
import importlib.util
import json
import math
from pathlib import Path

import benchmark_point_dendrogram as pilot
from point_tree import validate_point_tree, cut, to_jsonable
from point_eom import beta_value, common_module, SCHEMA as SELECTION_SCHEMA

SCHEMA = 'mhgp9_routed_point_post_capture_audit_v1'
ARITHMETIC_PATH = pilot.WEIGHTED / 'post_audit.py'
ARITHMETIC_SHA = '95cece7c7b74d839495485d8bad018abffb246458f2d4a800e68c479387e6e1d'
need, sha, read, check_pins = pilot.need, pilot.sha, pilot.read, pilot.check_pins


def arithmetic():
    need(sha(ARITHMETIC_PATH) == ARITHMETIC_SHA, 'independent arithmetic source pin')
    spec = importlib.util.spec_from_file_location('routed_point_independent_scores', ARITHMETIC_PATH)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def add_pins(target, values):
    for path, digest in values.items():
        need(path not in target or target[path] == digest, 'conflicting pin: '+path)
        target[path] = digest


def decode_tree(wire):
    tree = deepcopy(wire)
    for name in ('children', 'squared_levels', 'parents', 'point_counts',
                 'source_to_point_node', 'merge_source_nodes'):
        tree[name] = pilot.integer_map(tree[name])
    tree['squared_levels'] = {node: beta_value(beta) for node, beta in tree['squared_levels'].items()}
    tree['leaf_birth_betas'] = [beta_value(x) for x in tree['leaf_birth_betas']]
    for row in tree['attachments']:
        if row['beta'] is not None:
            row['beta'] = beta_value(row['beta'])
    validate_point_tree(tree)
    return tree


def verify_cuts(tree, checks):
    dates = sorted(set(tree['squared_levels'].values()))
    selected = {Q(0)}
    if dates:
        selected.update(dates[q*(len(dates)-1)//4] for q in range(5))
        selected.add(dates[-1]+1)
    expected = []
    for beta in sorted(selected):
        for closed in (False, True):
            blocks = cut(tree, beta, closed=closed)
            expected.append(dict(beta=to_jsonable(beta), closed=closed, blocks=len(blocks),
                partition_sha256=hashlib.sha256(json.dumps(blocks, separators=(',', ':')).encode()).hexdigest()))
    need(checks == expected, 'complete archived exact-cut fingerprints')
    return len(expected)


def verify_selection(payload, tree, minimum, exponent):
    need(payload['schema'] == SELECTION_SCHEMA and payload['n_points'] == tree['n_points'] and
         payload['min_cluster_size'] == minimum and payload['exp_z'] == exponent,
         'point selection dimensions/parameters')
    need(payload['source_roots'] == tree['roots'] and
         payload['source_squared_levels'] == to_jsonable(tree['squared_levels']), 'selection exact source dates')
    need(payload['mass_policy'] == 'one_per_original_point' and
         payload['root_policy'] == 'single_real_root_excluded' and
         payload['geometry_recomputed'] is False, 'selection point-mass/root policy')
    result = payload['result']; condensed = deepcopy(result['condensed_tree'])
    for name in ('birth_lambda', 'death_lambda', 'own_stability', 'point_exit_lambda'):
        condensed[name] = [math.inf if value == '+infinity' else value for value in condensed[name]]
    validated = common_module().validate_condensed_tree(condensed)
    need(condensed['n_points'] == tree['n_points'] and condensed['min_cluster_size'] == minimum and
         condensed['exp_z'] == exponent and result['validation'] == validated, 'condensed parameter/validation binding')
    selection = result['selection']; chosen = selection['selected']; count = len(condensed['parent'])
    need(all(type(c) is int and 0 < c < count for c in chosen) and len(chosen) == len(set(chosen)),
         'selected nonroot distinct condensed IDs')
    need(selection['allow_single_cluster'] is False and selection['cluster_selection_epsilon'] == 0.0 and
         selection['atomize_ties'] is True and selection['method'] == 'shared_EOM', 'common EOM policy')
    need(selection['selected_legacy_ids'] == [condensed['legacy_cluster_ids'][c] for c in chosen],
         'selected legacy identity')
    assigned = [-1]*count; index = {c: i for i,c in enumerate(chosen)}
    for c in range(1, count):
        above = assigned[condensed['parent'][c]]
        need(c not in index or above == -1, 'selected nodes are an antichain')
        assigned[c] = index.get(c, above)
    prediction = selection['labels']
    need(all(type(x) is int and x >= -1 for x in prediction) and
         prediction == [assigned[c] for c in condensed['point_exit_parent']], 'labels equal selected ancestry')
    counts = Counter(x for x in prediction if x >= 0)
    need(set(counts) == set(range(len(chosen))) and all(size >= minimum for size in counts.values()),
         'selected final point cardinality threshold')
    for name, expected in dict(selected_clusters=len(chosen), selected_points=sum(counts.values()),
                               noise_points=prediction.count(-1), point_exits=len(prediction),
                               points_removed_from_input=0, condensed_clusters=count).items():
        need(result['stats'][name] == expected, 'selection stats: '+name)
    return prediction


def verify_scores(truth, prediction, row, independent):
    truth, prediction = independent.labels(truth), independent.labels(prediction)
    need(len(truth) == len(prediction) == row['n'], 'whole-point label dimensions')
    minimum = row['min_cluster_size']; metrics = row['metrics']; extra = row['extra']
    kept = [i for i,x in enumerate(prediction) if x >= 0]
    actual = [x for x in truth if x >= 0]
    assigned = [prediction[i] for i,x in enumerate(truth) if x >= 0]
    singles = [x if x >= 0 else ('noise', i) for i,x in enumerate(assigned)]
    exact = dict(ari_all=independent.ari(truth, prediction),
        ari_true_inliers=independent.ari(actual, assigned) if len(actual) >= 2 else None,
        ari_inliers_noise_singletons=independent.ari(actual, singles) if len(actual) >= 2 else None,
        ari_classified=independent.ari([truth[i] for i in kept], [prediction[i] for i in kept]) if len(kept) >= 2 else None)
    for key, value in exact.items():
        if value is None:
            need(metrics[key] is None, 'undefined '+key)
        else:
            independent.close_float(metrics[key], value, key)
    counts = Counter(prediction[i] for i in kept)
    need(metrics['clusters'] == len(counts) and metrics['noise_count'] == len(truth)-len(kept), 'cluster/noise counts')
    independent.close_float(metrics['coverage'], Q(len(kept), len(truth)), 'coverage')
    need(all(size >= minimum for size in counts.values()), 'final cardinalities')
    # Independent integer contingencies/Fraction scores; SciPy optimizer shared.
    classes = sorted(set(actual)); predicted = sorted(counts); sizes = Counter(truth)
    intersections = Counter(zip(truth, prediction))
    matrix = [[intersections[a,b] for b in predicted] for a in classes]
    matches = {}
    if classes and predicted:
        left,right = independent.linear_sum_assignment(matrix, maximize=True)
        matches = {int(i):int(j) for i,j in zip(left,right) if matrix[int(i)][int(j)] > 0}
    need(extra['min_cluster_size'] == minimum and len(extra['per_class']) == len(classes), 'point eligibility threshold')
    precisions, recalls, f1s, correct, recovered = [], [], [], 0, 0
    for i,a in enumerate(classes):
        cluster = predicted[matches[i]] if i in matches else None
        tp = intersections[a,cluster] if cluster is not None else 0
        found = counts[cluster] if cluster is not None else 0
        is_exact = any(intersections[a,b] == sizes[a] == counts[b] for b in predicted)
        saved = extra['per_class'][i]
        for key,value in dict(truth_label=a,size=sizes[a],matched_label=cluster,matched_size=found,
                              intersection=tp,exact=is_exact,eligible=sizes[a] >= minimum).items():
            need(saved[key] == value, 'per-class '+key)
        p = Q(tp,found) if found else Q(0); r = Q(tp,sizes[a]); f = Q(2*tp,sizes[a]+found)
        for key,value in (('precision',p),('recall',r),('f1',f)):
            independent.close_float(saved[key], value, 'per-class '+key)
        precisions.append(p); recalls.append(r); f1s.append(f); correct += tp; recovered += is_exact
    for key,value in dict(truth_classes=len(classes),predicted_clusters=len(counts),matched_correct_points=correct,
        exact_classes=recovered,cluster_count_error=len(counts)-len(classes),
        cluster_count_absolute_error=abs(len(counts)-len(classes)),eligible_classes=sum(sizes[a] >= minimum for a in classes),
        classes_below_min_cluster_size=sum(sizes[a] < minimum for a in classes)).items():
        need(extra[key] == value, 'extra summary '+key)
    need(extra['truth_class_sizes'] == [dict(truth_label=a,size=sizes[a],eligible=sizes[a] >= minimum) for a in classes],
         'truth eligibility inventory')
    for key,value in dict(exact_class_fraction=Q(recovered,len(classes)) if classes else None,
        matched_macro_precision=sum(precisions)/len(classes) if classes else None,
        matched_macro_recall=sum(recalls)/len(classes) if classes else None,
        matched_macro_f1=sum(f1s)/len(classes) if classes else None,
        matched_micro_precision=Q(correct,len(kept)) if kept else Q(0),
        matched_micro_recall=Q(correct,len(actual)) if actual else None,
        matched_micro_f1=Q(2*correct,len(actual)+len(kept)) if actual else None).items():
        if value is None:
            need(extra[key] is None, 'undefined '+key)
        else:
            independent.close_float(extra[key], value, key)
    return {key: None if value is None else [value.numerator,value.denominator] for key,value in exact.items()}


def validate_capture(capture):
    capture = Path(capture).resolve(); receipt_path = capture/'receipt.json'
    receipt = read(receipt_path); pins = {str(receipt_path): sha(receipt_path), str(Path(__file__).resolve()): sha(__file__)}
    need(receipt['schema'] == pilot.SCHEMA and receipt['status'] == 'completed', 'completed routed-point capture required')
    need(receipt['sources_before'] == receipt['sources_after'] == pilot.source_inventory(), 'exact source inventory closure')
    add_pins(pins, receipt['sources_after']); check_pins(pins)
    need(receipt['plan'] == dict(cases=list(pilot.CASES),k=[5],min_cluster_size=[20,50],exp_z=[1,2]) and
         receipt['primary'] == dict(k=5,min_cluster_size=20,exp_z=1), 'fixed whole pilot plan')
    for flag in ('GCP_used','GPU_used','geometry_rerun','hdbscan_fits_rerun'):
        need(receipt[flag] is False, 'pilot scope '+flag)
    need(receipt['mass_policy'] == 'one_per_point' and receipt['root_policy'] == 'single_real_root_excluded', 'pilot policies')
    need(receipt['qualification_sha256'] == pilot.QUALIFICATION_SHA, 'qualification identity')
    qualification_pins = pilot.validate_qualification(Path(receipt['qualification']))
    need(qualification_pins == receipt['qualification_pins'], 'qualification exact pin inventory')
    add_pins(pins, qualification_pins)
    old_path = Path(receipt['inherited_receipt'])
    need(receipt['inherited_receipt_sha256'] == pilot.CAPTURE_SHA == sha(old_path), 'immutable inherited receipt')
    posts = [Path(path) for path,digest in receipt['inherited_pins'].items() if digest == pilot.POST_SHA]
    need(len(posts) == 1, 'unique pinned inherited post-audit')
    old, _, _, inherited_pins, _ = pilot.inherited.validated_inputs(old_path.parent, posts[0])
    need(inherited_pins == receipt['inherited_pins'], 'inherited exact pin inventory')
    add_pins(pins, inherited_pins)
    argv = receipt['argv']
    expected_args = {'--capture':str(old_path.parent),'--post-audit':str(posts[0]),
                     '--qualification':receipt['qualification'],'--output':str(capture)}
    need(len(argv) == 9 and Path(argv[0]).name == 'benchmark_point_dendrogram.py' and
         dict(zip(argv[1::2],argv[2::2])) == expected_args, 'recorded pilot invocation')
    manifest_path = Path(receipt['manifest'])
    need(receipt['manifest'] == old['manifest'] and receipt['manifest_sha256'] == pilot.MANIFEST_SHA == sha(manifest_path),
         'fixed inherited input manifest')
    add_pins(pins, {str(manifest_path):pilot.MANIFEST_SHA})
    cases = {case['id']:case for case in read(manifest_path)['cases']}
    pilot.validate_grid(receipt['rows'])
    old_rows = [row for row in old['rows'] if row['k'] == 5]
    need(len(old_rows) == 182 and receipt['rows'][:182] == old_rows, '182 inherited rows exactly unchanged')
    expected_units = [(case,z) for case in pilot.CASES for z in (1,2)]
    need([(unit['case'],unit['exp_z']) for unit in receipt['units']] == expected_units, '26 ordered complete units')
    artifacts, new_rows = {}, []
    independent = arithmetic(); add_pins(pins, {str(ARITHMETIC_PATH):ARITHMETIC_SHA})
    for unit in receipt['units']:
        case_id,z = unit['case'],unit['exp_z']; case = cases[case_id]
        need(unit['status'] == 'completed' and unit['k'] == 5 and case['n'] == 1200, 'whole completed K5 unit')
        folder = capture/f'{case_id}_k5_z{z}'
        paths = [folder/'unit.json',folder/'point_tree.json.gz',folder/'selection_m20.json.gz',folder/'selection_m50.json.gz']
        need(set(folder.iterdir()) == set(paths) and read(paths[0]) == unit, 'exact unit artifact set/receipt')
        artifacts.update({str(path):sha(path) for path in paths})
        source = old_path.parent/f'{case_id}_k5'/f'measure_z{z}.json.gz'
        need(unit['source_measure'] == str(source) and unit['source_measure_sha256'] == old['artifacts'][str(source)],
             'inherited exact measure binding')
        need(unit['point_tree'] == str(paths[1]) and unit['point_tree_sha256'] == artifacts[str(paths[1])], 'point-tree artifact binding')
        tree = decode_tree(read(paths[1])); need(tree['n_points'] == case['n'] and len(tree['roots']) == 1, 'whole single-root point tree')
        verify_cuts(tree, unit['checks'])
        need(tree['statistics'] == unit['point_tree_statistics'] and
             dict(Counter(row['reason'] for row in tree['attachments'])) == unit['attachment_reasons'], 'tree statistics/attachment counts')
        truth_path = Path(case['labels_json']); need(str(truth_path) in pins, 'truth pinned in inherited inventory')
        truth = read(truth_path); need(len(truth) == 1200, 'whole scene truth')
        need(len(unit['rows']) == 2, 'two cardinality selections per tree')
        for row,minimum,path in zip(unit['rows'],(20,50),paths[2:]):
            need(tuple(row[key] for key in pilot.KEYS) == (case_id,5,minimum,z,pilot.METHOD), 'new exact row key')
            need(all(row[key] == case[key] for key in ('regime','communities','separation','seed','n')), 'manifest metadata')
            need(row['labels_payload'] == str(path) and row['labels_payload_sha256'] == artifacts[str(path)], 'selection payload binding')
            for key in ('routing_statistics','point_tree_statistics','attachment_reasons'):
                need(row[key] == unit[key], 'row/unit '+key)
            prediction = verify_selection(read(path), tree, minimum, z)
            verify_scores(truth, prediction, row, independent)
            new_rows.append(row)
    need(receipt['rows'] == old_rows+new_rows and len(new_rows) == 52, 'exact inherited/new row ledger')
    need(set(capture.iterdir()) == {receipt_path} | {capture/f'{case}_k5_z{z}' for case,z in expected_units}, 'exact capture entries')
    need(receipt['artifacts'] == artifacts and len(artifacts) == 104, '104 exact artifact hashes')
    add_pins(pins, artifacts); check_pins(pins)
    return receipt, receipt['rows'], pins


def audit(capture, output):
    output = Path(output).resolve(); need(not output.exists(), 'NEW audit output required')
    receipt, rows, pins = validate_capture(capture)
    summary = dict(schema=SCHEMA,status='passed',capture=str(Path(capture).resolve()),
        receipt_sha256=sha(Path(capture)/'receipt.json'),audit_source_sha256=sha(__file__),
        arithmetic_source_sha256=ARITHMETIC_SHA,rows=len(rows),inherited_rows_unchanged=182,
        new_label_replays=52,point_trees=26,condensed_trees=52,
        archived_cut_replays=sum(len(unit['checks']) for unit in receipt['units']),
        pins_checked=len(pins),pins_sha256=pins,Fraction_ARI=True,
        Hungarian_solver_shared=True,NMI_recomputed=False,
        routing_recomputed=False,condensation_or_EOM_recomputed=False,
        fits_or_geometry_rerun=False,GCP_used=False,
        scope='LIVE exact provenance; archived-cut replay; point thresholds; independent score arithmetic; not independent routing/geometry/EOM proof')
    output.mkdir(parents=True,exist_ok=False)
    check_pins(pins)
    with (output/'audit.json').open('x') as stream:
        json.dump(summary,stream,sort_keys=True,indent=2,allow_nan=False); stream.write('\n')
    print(json.dumps({key:summary[key] for key in ('status','rows','new_label_replays','point_trees','archived_cut_replays','pins_checked')}))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--capture',required=True,type=Path)
    parser.add_argument('--output',required=True,type=Path)
    args = parser.parse_args()
    audit(args.capture.resolve(),args.output.resolve())
