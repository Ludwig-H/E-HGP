#!/usr/bin/env python3
"""LIVE contract/label readback of the new 34-scene synthetic capture.

No native geometry, routing, fit, condensation or EOM is run. Fraction ARI
and integer contingencies are independent of production metrics. Hungarian
optimization is shared; NMI is not recomputed. Existing point-tree/selection
validators are reused explicitly, not their old hardcoded capture reader.
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
import sys

HERE = Path(__file__).resolve().parent
POINT = HERE.parent/'point_dendrogram_20260927'
POINT_READER = POINT/'post_audit_point_dendrogram.py'
POINT_READER_SHA = '09d2fa0f3439ccc73109be0691783ac4c9d3cba607dcdf9d0c9f4410c9cc9204'
sys.path[:0] = [str(POINT), str(HERE)]
import run as driver
import unit_pipeline as unit
from plan import PLAN

SCHEMA = 'mhgp9_synthetic_clustering_post_audit_v1'
KEYS = ('case', 'k', 'min_cluster_size', 'exp_z', 'method')
need, sha = unit.need, unit.sha


def helpers():
    need(sha(POINT_READER) == POINT_READER_SHA, 'frozen point reader source identity')
    spec = importlib.util.spec_from_file_location('synthetic_point_reader_helpers', POINT_READER)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module, module.arithmetic()


def add_pins(target, values):
    for path, digest in values.items():
        need(path not in target or target[path] == digest, 'conflicting pin: '+path)
        target[path] = digest


def metadata(row, case):
    expected = {key:case[key] for key in ('phase', 'axis', 'replicate', 'spec', 'n', 'groups')}
    expected.update({key:case['spec'][key] for key in
                     ('family', 'groups', 'separation', 'seed', 'noise_fraction')})
    need(all(row.get(key) == value for key,value in expected.items()), 'row/manifest metadata')


def verify_scores(truth, prediction, row, arithmetic):
    """All saved ARIs, coverage, counts, noise and matched P/R/F1; no NMI."""
    truth, prediction = arithmetic.labels(truth), arithmetic.labels(prediction)
    need(len(truth) == len(prediction) == row['n'] and
         all(x == -1 or x > 0 for x in truth) and all(x >= -1 for x in prediction),
         'whole point labels and explicit noise domain')
    minimum = None if row['method'] == 'hgp_weighted_full_vote' else row['min_cluster_size']
    semantic = 'facet_mass_before_vote' if minimum is None else 'point_cardinality'
    need(row['threshold_semantics'] == semantic, 'mass/cardinality threshold semantics')
    metrics, extra = row['metrics'], row['extra']
    kept = [i for i,x in enumerate(prediction) if x >= 0]
    actual = [x for x in truth if x >= 0]
    assigned = [prediction[i] for i,x in enumerate(truth) if x >= 0]
    singles = [x if x >= 0 else ('noise', i) for i,x in enumerate(assigned)]
    exact = dict(ari_all=arithmetic.ari(truth, prediction),
        ari_true_inliers=arithmetic.ari(actual, assigned) if len(actual) >= 2 else None,
        ari_inliers_noise_singletons=arithmetic.ari(actual, singles) if len(actual) >= 2 else None,
        ari_classified=arithmetic.ari([truth[i] for i in kept], [prediction[i] for i in kept]) if len(kept) >= 2 else None)
    for key,value in exact.items():
        if value is None:
            need(metrics[key] is None, 'undefined '+key)
        else:
            arithmetic.close_float(metrics[key], value, key)
    counts = Counter(prediction[i] for i in kept)
    need(metrics['clusters'] == len(counts) and metrics['noise_count'] == len(truth)-len(kept), 'cluster/noise counts')
    arithmetic.close_float(metrics['coverage'], Q(len(kept),len(truth)), 'coverage')
    if minimum is not None:
        need(all(size >= minimum for size in counts.values()), 'final point cardinalities')
    if any(x < 0 for x in truth):
        tp = sum(a < 0 and b < 0 for a,b in zip(truth,prediction))
        fp = sum(a >= 0 and b < 0 for a,b in zip(truth,prediction))
        fn = sum(a < 0 and b >= 0 for a,b in zip(truth,prediction))
        for key,value in dict(noise_precision=Q(tp,tp+fp) if tp+fp else Q(0),
                noise_recall=Q(tp,tp+fn), noise_f1=Q(2*tp,2*tp+fp+fn)).items():
            arithmetic.close_float(metrics[key], value, key)
    else:
        need(all(metrics[key] is None for key in ('noise_precision','noise_recall','noise_f1')),
             'undefined noise scores without injected noise')
    classes = sorted(set(actual)); predicted = sorted(counts); sizes = Counter(truth)
    intersections = Counter(zip(truth,prediction))
    matrix = [[intersections[a,b] for b in predicted] for a in classes]
    matches = {}
    if classes and predicted:
        left,right = arithmetic.linear_sum_assignment(matrix, maximize=True)
        matches = {int(i):int(j) for i,j in zip(left,right) if matrix[int(i)][int(j)] > 0}
    need(extra['min_cluster_size'] == minimum and len(extra['per_class']) == len(classes), 'eligibility threshold')
    need(extra['scope'] == 'labels_all_points_true_noise_excluded_as_class', 'true noise is not a class')
    precision,recall,f1,correct,recovered = [],[],[],0,0
    eligible = lambda size: None if minimum is None else size >= minimum
    for i,a in enumerate(classes):
        cluster = predicted[matches[i]] if i in matches else None
        tp = intersections[a,cluster] if cluster is not None else 0
        found = counts[cluster] if cluster is not None else 0
        is_exact = any(intersections[a,b] == sizes[a] == counts[b] for b in predicted)
        saved = extra['per_class'][i]
        for key,value in dict(truth_label=a,size=sizes[a],matched_label=cluster,matched_size=found,
                              intersection=tp,exact=is_exact,eligible=eligible(sizes[a])).items():
            need(saved[key] == value, 'per-class '+key)
        p = Q(tp,found) if found else Q(0); r = Q(tp,sizes[a]); f = Q(2*tp,sizes[a]+found)
        for key,value in (('precision',p),('recall',r),('f1',f)):
            arithmetic.close_float(saved[key], value, 'per-class '+key)
        precision.append(p); recall.append(r); f1.append(f); correct += tp; recovered += is_exact
    for key,value in dict(truth_classes=len(classes),predicted_clusters=len(counts),matched_correct_points=correct,
        exact_classes=recovered,cluster_count_error=len(counts)-len(classes),
        cluster_count_absolute_error=abs(len(counts)-len(classes)),
        eligible_classes=None if minimum is None else sum(sizes[a] >= minimum for a in classes),
        classes_below_min_cluster_size=None if minimum is None else sum(sizes[a] < minimum for a in classes)).items():
        need(extra[key] == value, 'extra summary '+key)
    need(extra['truth_class_sizes'] == [dict(truth_label=a,size=sizes[a],eligible=eligible(sizes[a])) for a in classes],
         'truth eligibility inventory')
    for key,value in dict(exact_class_fraction=Q(recovered,len(classes)) if classes else None,
        matched_macro_precision=sum(precision)/len(classes) if classes else None,
        matched_macro_recall=sum(recall)/len(classes) if classes else None,
        matched_macro_f1=sum(f1)/len(classes) if classes else None,
        matched_micro_precision=Q(correct,len(kept)) if kept else Q(0),
        matched_micro_recall=Q(correct,len(actual)) if actual else None,
        matched_micro_f1=Q(2*correct,len(actual)+len(kept)) if actual else None).items():
        if value is None:
            need(extra[key] is None, 'undefined '+key)
        else:
            arithmetic.close_float(extra[key], value, key)
    return {key:None if value is None else [value.numerator,value.denominator] for key,value in exact.items()}


def lifecycle(receipt, case_ids, capture):
    """Recorded start/join ownership and cleanup, not a new kill/ps operation."""
    commands = receipt['worker_commands']; active = {}; starts = []; joins = []; maximum = 0
    need(type(receipt['workers']) is int and 1 <= receipt['workers'] <= 2, 'worker limit')
    for event in receipt['events']:
        name = event['case']
        need(name in case_ids and event['k'] == 5 and type(event['pid']) is int and event['pid'] > 0,
             'worker event domain')
        argv = event['argv']
        need(len(argv) == 5 and Path(argv[0]).resolve() == Path(sys.executable).resolve() and
             argv[1:] == ['-B',str(HERE/'run.py'),'--worker',str(capture/(name+'.spec.json'))], 'worker argv binding')
        if event['event'] == 'start':
            need(name not in starts and name not in active and set(event) == {'event','case','k','pid','argv'},
                 'one owned start per case')
            starts.append(name); active[name] = event
            maximum = max(maximum,len(active)); need(len(active) <= receipt['workers'], 'worker concurrency limit')
        else:
            need(event['event'] == 'joined' and name in active, 'join without owned start')
            need(event['pid'] == active[name]['pid'] and event['argv'] == active[name]['argv'] and
                 event['returncode'] == 0, 'successful matching join')
            joins.append({key:value for key,value in event.items() if key != 'event'})
            del active[name]
    need(not active and starts == case_ids and joins == commands and len(commands) == len(case_ids),
         'complete ordered starts and exact joins/commands')
    need(len(receipt['cleanup']) == len(commands), 'one cleanup per joined group')
    for cleanup,command in zip(receipt['cleanup'],commands):
        need(cleanup['pgid'] == command['pid'] and cleanup['status'] == 'closed' and cleanup['returncode'] == 0,
             'closed successful owned process group')
        signals = cleanup['signals']
        need(type(cleanup['residual_group_before_cleanup']) is bool and
             signals in ([],[2],[2,15],[2,15,9]) and
             (cleanup['residual_group_before_cleanup'] or not signals), 'cleanup signal trace')
    need(not receipt.get('cleanup_errors') and not receipt.get('closure_errors'), 'capture closure errors')
    return maximum


def check_point_cuts(tree, checks, helper):
    dates = sorted(set(tree['squared_levels'].values())); selected = {Q(0)}
    if dates:
        selected.update(dates[q*(len(dates)-1)//4] for q in range(5)); selected.add(dates[-1]+1)
    expected = [dict(beta=helper.to_jsonable(at),closed=closed,
                     blocks=len(helper.cut(tree,at,closed=closed)))
                for at in sorted(selected) for closed in (False,True)]
    need(checks == expected, 'complete sampled point cuts')
    return len(expected)


def common_labels(result, n, minimum, exponent, helper):
    """Validate saved common condensation and its label ancestry, no selection."""
    condensed = deepcopy(result['condensed_tree'])
    for key in ('birth_lambda','death_lambda','own_stability','point_exit_lambda'):
        condensed[key] = [math.inf if value == '+infinity' else value for value in condensed[key]]
    validation = helper.common_module().validate_condensed_tree(condensed)
    need(result['validation'] == validation and condensed['n_points'] == n and
         condensed['min_cluster_size'] == minimum and condensed['exp_z'] == exponent, 'common condensed binding')
    selection = result['selection']; chosen = selection['selected']; count = len(condensed['parent'])
    need(all(type(c) is int and 0 < c < count for c in chosen) and len(set(chosen)) == len(chosen), 'common nonroot selection')
    need(selection['allow_single_cluster'] is False and selection['atomize_ties'] is True and
         selection['cluster_selection_epsilon'] == 0 and selection['method'] == 'shared_EOM', 'common EOM policy')
    chosen_ids = {c:i for i,c in enumerate(chosen)}; assigned = [-1]*count
    for c in range(1,count):
        parent_label = assigned[condensed['parent'][c]]
        need(c not in chosen_ids or parent_label == -1, 'common antichain')
        assigned[c] = chosen_ids.get(c,parent_label)
    labels = [assigned[c] for c in condensed['point_exit_parent']]
    need(selection['labels'] == labels and selection['selected_legacy_ids'] ==
         [condensed['legacy_cluster_ids'][c] for c in chosen], 'common selected ancestry')
    return labels


def validate_capture(receipt_path):
    receipt_path = Path(receipt_path).resolve(); capture = receipt_path.parent
    helper,independent = helpers(); read = helper.read
    receipt = read(receipt_path)
    need(receipt_path.name == 'receipt.json' and receipt['schema'] == driver.SCHEMA and
         receipt['status'] == 'completed', 'completed synthetic capture receipt required')
    need(receipt['plan'] == PLAN and receipt['sources_before'] == receipt['sources_after'] == driver.sources(),
         'fixed plan and exact LIVE runtime inventory')
    argv = receipt['argv']
    need(len(argv) == 7 and (Path(receipt['cwd'])/argv[0]).resolve() == HERE/'run.py' and
         dict(zip(argv[1::2],argv[2::2])) == {'--manifest':receipt['manifest'],'--output':str(capture),
                                            '--workers':str(receipt['workers'])}, 'recorded parent invocation')
    for key in ('GCP_used','GPU_used','growth_executed'):
        need(receipt[key] is False, 'quality-only local capture: '+key)
    need(receipt['full_observer_workers'] == 2 and receipt['native_requested_workers'] == 1 and
         receipt['worker_environment'] == driver.THREAD_ENV, 'instrumented native/worker scope')
    need(receipt['native_binary'] == str(driver.NATIVE) and receipt['native_binary_sha256'] == unit.NATIVE_SHA,
         'exact qualified binary binding')
    manifest_path = Path(receipt['manifest'])
    need(sha(manifest_path) == receipt['manifest_sha256'], 'manifest receipt binding')
    manifest,inherited = driver.preflight(manifest_path)
    need(inherited == receipt['inherited_pins'], 'exact inherited proof/preparation inventory')
    pins = {str(receipt_path):sha(receipt_path),str(Path(__file__).resolve()):sha(__file__),
            str(POINT_READER):POINT_READER_SHA,str(helper.ARITHMETIC_PATH):helper.ARITHMETIC_SHA}
    add_pins(pins, inherited); add_pins(pins,receipt['sources_after'])
    cases = [case for case in manifest['cases'] if case['phase'] == 'quality']
    ids = [case['id'] for case in cases]
    need(len(ids) == len(set(ids)) == 34, '34 complete new quality scenes')
    rows = receipt['rows']; keys = [tuple(row[key] for key in KEYS) for row in rows]
    need(len(keys) == 612 and set(keys) == driver.expected_grid(), 'exact unique 612-row grid')
    maximum = lifecycle(receipt,ids,capture)
    need([u['case'] for u in receipt['units']] == ids and len(receipt['units']) == 34, '34 ordered unit receipts')
    intent = read(capture/'intent.json')
    need(intent['status'] == 'running' and not intent['rows'] and not intent['units'] and
         all(intent[key] == receipt[key] for key in ('schema','argv','cwd','plan','manifest','manifest_sha256',
             'sources_before','workers','worker_environment','native_binary','native_binary_sha256')), 'original run intent')
    actual_artifacts = {str(path):sha(path) for path in capture.rglob('*') if path.is_file() and path != receipt_path}
    need(receipt['artifacts'] == actual_artifacts, 'exact complete capture artifact inventory')
    add_pins(pins,actual_artifacts)
    row_union = []; counters = dict(rows=612,point_trees=0,point_selection_replays=0,
        common_condensed_replays=0,hdbscan_fits_checked=0,worker_commands=34,native_commands=0,
        fraction_ari_replays=0,archived_cut_replays=0,maximum_observed_workers=maximum)
    exact_ari = []
    expected_entries = {receipt_path,capture/'intent.json'}
    command_by_case = {command['case']:command for command in receipt['worker_commands']}
    for case,saved in zip(cases,receipt['units']):
        name = case['id']; folder = capture/name
        expected_entries.update((folder,capture/(name+'.spec.json'),capture/(name+'.stdout'),capture/(name+'.stderr')))
        worker = command_by_case[name]
        for suffix in ('stdout','stderr'):
            need(worker[suffix+'_sha256'] == actual_artifacts[str(capture/(name+'.'+suffix))], 'worker stream binding')
        need(read(capture/(name+'.spec.json')) == dict(case=case,output=str(folder),
             native_binary=receipt['native_binary'],source_pins=receipt['sources_before']), 'exact worker specification')
        need(read(folder/'receipt.json') == saved, 'parent/worker receipt identity')
        driver.validate_unit(saved,case); unit.validate_rows(saved['rows'],name)
        need(saved['sources_before'] == saved['sources_after'] == {str(p):sha(p) for p in unit.source_paths()},
             'unit runtime closure')
        add_pins(pins,saved['sources_after']); add_pins(pins,saved['sklearn']['sha256'])
        need(saved['sklearn'] == unit.sklearn_provenance(), 'LIVE sklearn package/binary provenance')
        need(saved['native_binary'] == receipt['native_binary'] and saved['native_binary_sha256'] == unit.NATIVE_SHA,
             'unit/native binary binding')
        need(saved['catalogue'] == 'Gabriel_complete_boundary_not_historical_order_cell_catalogue' and
             saved['arithmetic'] == 'exact_geometry_and_point_dates_binary64_weights_and_EOM_dyadic_routing' and
             saved['root_policy'] == 'exclude_real_root_no_artificial_forest_root' and
             saved['truth_access'] == 'decode_only_after_all_18_predictions', 'unit scientific contract')
        need(all(saved[key] is False for key in ('GCP_used','GPU_used','engine_modified')), 'unit local unchanged engine')
        _,_,input_pins = unit.read_geometry(case)
        need(saved['input_hashes'] == input_pins, 'exact three whole-input bindings')
        add_pins(pins,input_pins)
        truth = read(case['labels_json'])
        need(len(truth) == case['n'] and dict(sorted(Counter(map(str,truth)).items())) ==
             dict(sorted(case['true_counts'].items())), 'manifest truth counts')
        artifacts = {p:h for p,h in actual_artifacts.items() if Path(p).parent == folder and Path(p).name != 'receipt.json'}
        need(saved['artifacts'] == artifacts and len(artifacts) == 28, '28 unit artifacts plus receipt')
        need(len(saved['commands']) == 1, 'one native export per whole case')
        command = saved['commands'][0]
        need(command == read(folder/'command.json') and command['returncode'] == 0 and
             command['argv'] == [receipt['native_binary'],'--input',str(Path(case['points_u32le']).resolve()),
                                 '--k','5','--workers','1'] and command['binary_sha256'] == unit.NATIVE_SHA and
             command['input_sha256'] == input_pins[str(Path(case['points_u32le']).resolve())], 'native command binding')
        for suffix,filename in (('stdout','native.json'),('stderr','native.stderr')):
            need(command[suffix+'_sha256'] == artifacts[str(folder/filename)], 'native stream binding')
        counters['native_commands'] += 1
        fits = {}
        need([fit['min_cluster_size'] for fit in saved['hdbscan_fits']] == [20,50], 'two HDB fits')
        for fit,minimum in zip(saved['hdbscan_fits'],(20,50)):
            fitted = read(folder/f'hdbscan_m{minimum}.json.gz'); fits[minimum] = fitted
            tree = fitted['tree']; tree['children'] = helper.pilot.integer_map(tree['children'])
            tree['heights'] = helper.pilot.integer_map(tree['heights'])
            digest = hashlib.sha256(json.dumps(tree,sort_keys=True,allow_nan=False).encode()).hexdigest()
            need(digest == fit['tree_sha256'] and tree['n'] == case['n'], 'whole HDB source fingerprint')
            need(fitted['provenance'] == saved['sklearn'] and fit['warnings'] == fitted['warnings'] and
                 fit['common_z1_matches_standard'] == fitted['common_z1_matches_standard'], 'HDB proof/warnings binding')
            parameters = fitted['parameters']
            need(parameters == dict(k_self_included=5,min_samples=5,min_cluster_size=minimum,metric='euclidean',
                 alpha=1.0,algorithm='kd_tree',n_jobs=1,max_cluster_size=None,cluster_selection_epsilon=0.0,
                 allow_single_cluster=False,max_points=100000), 'common HDB parameter contract')
            counters['hdbscan_fits_checked'] += 1
        need(saved['hdbscan_fits'][0]['tree_sha256'] == saved['hdbscan_fits'][1]['tree_sha256'], 'minimum-size-independent HDB tree')
        point_trees = {}
        need([part['exp_z'] for part in saved['units']] == [1,2], 'two ordered routed point trees')
        for part in saved['units']:
            z = part['exp_z']; tree_path = folder/f'point_tree_z{z}.json.gz'
            need(part['point_tree'] == str(tree_path) and part['measure'] == str(folder/f'measure_z{z}.json.gz') and
                 part['score_arithmetic'] == 'binary64_S_lifted_exactly_as_dyadic_even_z2', 'tree/measure/dyadic bindings')
            tree = helper.decode_tree(read(tree_path)); point_trees[z] = tree
            need(tree['n_points'] == case['n'] and len(tree['roots']) == 1, 'whole single-root point dendrogram')
            need(part['point_tree_statistics'] == tree['statistics'] and part['attachment_reasons'] ==
                 dict(Counter(row['reason'] for row in tree['attachments'])), 'point statistics/provenance')
            counters['archived_cut_replays'] += check_point_cuts(tree,part['checks'],helper)
            counters['point_trees'] += 1
        for row in saved['rows']:
            metadata(row,case); minimum,z,method = (row[key] for key in ('min_cluster_size','exp_z','method'))
            path = folder/f'{method}_m{minimum}_z{z}.json.gz'
            need(row['labels_payload'] == str(path) and row['labels_payload_sha256'] == artifacts[str(path)], 'label payload binding')
            wire = read(path); prediction = wire['labels']; result = wire['result']
            if method == 'hgp_exclusive_point_routing':
                need(prediction == helper.verify_selection(result,point_trees[z],minimum,z), 'exclusive point selection')
                counters['point_selection_replays'] += 1
            elif method in ('hgp_first_coverage','hdbscan_common'):
                need(prediction == common_labels(result,case['n'],minimum,z,helper), 'common point label ancestry')
                counters['common_condensed_replays'] += 1
                if method == 'hdbscan_common' and z == 1:
                    need(unit.equivalent_labels(prediction,fits[minimum]['common_z1_labels']), 'saved HDB common z1 labels')
            elif method == 'hdbscan_standard':
                need(prediction == fits[minimum]['standard_labels_z1'] and result ==
                     dict(parameters=fits[minimum]['parameters'],warnings=fits[minimum]['warnings']), 'HDB standard labels')
            else:
                need(method == 'hgp_weighted_full_vote' and prediction == result['vote']['labels'] and
                     result['vote']['noise_policy'] == 'ignore_noise_facets_no_1NN_fill' and
                     result['vote']['tie_policy'] == 'smallest_cluster_id' and
                     result['selection']['allow_single_cluster'] is False, 'weighted vote payload/root policy')
            exact_ari.append(dict(key=list(tuple(row[key] for key in KEYS)),
                values=verify_scores(truth,prediction,row,independent)))
            counters['fraction_ari_replays'] += 1
        row_union.extend(saved['rows'])
    need(rows == row_union and set(capture.iterdir()) == expected_entries, 'exact parent row ledger/capture entries')
    need(counters['point_trees'] == counters['hdbscan_fits_checked'] == 68 and
         counters['point_selection_replays'] == 136 and counters['common_condensed_replays'] == 272 and
         counters['fraction_ari_replays'] == 612 and counters['native_commands'] == 34, 'complete replay inventory')
    counters['exact_ari'] = exact_ari
    # One deduplicated final closure over all consulted evidence. Preflight also
    # checked inherited proof pins before reading this capture's payloads.
    driver.check_pins(pins)
    return receipt,pins,counters


def audit(receipt_path, output):
    output = Path(output).resolve(); need(not output.exists(), 'NEW audit output required')
    need(not output.is_relative_to(Path(receipt_path).resolve().parent), 'audit must stay outside capture')
    receipt,pins,counters = validate_capture(receipt_path)
    summary = dict(schema=SCHEMA,status='passed',receipt=str(Path(receipt_path).resolve()),
        receipt_sha256=sha(receipt_path),audit_source_sha256=sha(__file__),
        pins_sha256=pins,pins_checked=len(pins),**counters,
        Fraction_ARI=True,Hungarian_solver_shared=True,NMI_recomputed=False,
        point_cut_check_scope='same_sample_dates_and_block_counts_no_original_partition_hashes_recorded',
        native_geometry_revalidated=False,weighted_S_or_vote_recomputed=False,
        routing_recomputed=False,condensation_or_EOM_recomputed=False,
        fits_or_geometry_rerun=False,GCP_used=False,
        scope='LIVE evidence and complete label metrics; archived point/condensed structures; not an independent algorithm proof')
    output.mkdir(parents=True,exist_ok=False)
    with (output/'audit.json').open('x') as stream:
        json.dump(summary,stream,sort_keys=True,indent=2,allow_nan=False);stream.write('\n')
    print(json.dumps({key:summary[key] for key in ('status','rows','point_trees','point_selection_replays',
                                                 'hdbscan_fits_checked','fraction_ari_replays','pins_checked')},sort_keys=True))
    return summary


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('receipt',type=Path)
    parser.add_argument('--output',type=Path,required=True)
    args = parser.parse_args()
    audit(args.receipt,args.output)
