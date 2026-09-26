#!/usr/bin/env python3
"""Rebuild a small R24-B table from raw, validated G4 probe outputs.

No benchmark is run here. The host/worker protocol, GCE guards and full
command/manifest closure are judged by tower_session_v9.validate_received;
this reader checks the published receipt's case/object arithmetic anew.
"""

import argparse
import hashlib
import json
from pathlib import Path
from statistics import median


HERE = Path(__file__).resolve().parent


def read(path):
    return json.loads(path.read_text())


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def need(ok, why):
    if not ok:
        raise ValueError(why)


def arm(case):
    lever = case['levers']
    if not lever['q34_batch_filter']:
        return 'engine'
    if not lever['tower_overlap_static']:
        return 'C_no_overlap'
    if not lever['tower_pipelined_tail']:
        return 'B_no_tail'
    return 'A_full_overlap'


def build(folder):
    plan_path = HERE / 'plan.json'
    host_path = folder / 'host/receipt.json'
    worker_path = folder / 'vm/receipt.json'
    package_path = folder / 'PACKAGE.json'
    plan, host, worker, package = (read(p) for p in (plan_path, host_path, worker_path, package_path))
    cases = plan['cases']
    need(len(cases) == 36 and worker['cases'] == cases and package['cases'] == cases,
         'plan/receipt/package mismatch')
    need(host['status'] == worker['status'] == 'completed' and host['targeted_shutdown_certified'] is True and
         worker['FULL_executed'] is True and worker['GPU_executed'] is True and
         worker['contract_certified'] is False and worker['public_status'] == 'not_claimed', 'scope/closure')
    need(worker['sources_stable'] is True and worker['compiled_dependencies_stable'] is True and
         worker['binary_stable'] is True and worker['provenance']['commit'] == package['commit'], 'provenance')
    need(len(worker['case_outcomes']) == len(cases) and
         {x['index'] for x in worker['case_outcomes']} == set(range(len(cases))) and
         all(x['outcome'] == 'complete_relative' for x in worker['case_outcomes']), 'case outcomes')
    need(len(worker['cross_worker_comparisons']) == 28 and
         all(x['equal'] is True for x in worker['cross_worker_comparisons']), 'cross comparisons')
    need(len(worker['GPU_completed_cases']) == 28 and not worker['unpaired_batch_cases'], 'GPU/twin coverage')

    rows = []
    objects = {}
    for i, case in enumerate(cases):
        raw_path = folder / 'vm' / ('probe_%d.stdout' % i)
        summary_path = folder / 'vm' / ('probe_%d.summary.json' % i)
        probe, summary = read(raw_path), read(summary_path)
        need(summary['case'] == case and summary['index'] == i and summary['outcome'] == 'complete_relative' and
             probe['status'] == 'complete_relative' and summary['tower_digest'] == probe['tower_digest'],
             'probe/summary mismatch at %d' % i)
        options = probe['options']
        need(options['K'] == case['k'] and options['s'] == case['s'] and options['workers'] == case['workers'] and
             options['tower_static_threads'] == case['static_threads'] and options['levers'] == case['levers'] and
             probe['input']['sites'] == case['n'], 'probe/plan mismatch at %d' % i)
        need(summary['input_file_sha256'] == package['inputs'][case['file']]['sha256'], 'input pin at %d' % i)
        object_key = (probe['tower_digest'], probe['catalogue_digest'], probe['presentation_digest'],
                      probe['catalogue']['balls'])
        group = (case['scene'], case['k'])
        if group in objects:
            need(objects[group] == object_key, 'tower/catalogue/presentation differs at %d' % i)
        else:
            objects[group] = object_key
        times = probe['times_ms']
        rows.append(dict(index=i, scene=case['scene'], sites=case['n'], K=case['k'], s=case['s'],
                         repeat=case['repeat'], arm=arm(case), backend='gpu' if options['levers']['q34_gpu_q3'] else 'engine',
                         chain_ms=times['chain_total'], tower_ms=times['tower'], q2_ms=times['q2'],
                         q34_ms=times['q34'], census_ms=times['census'], digest_ms=times['digest'],
                         elapsed_s=summary['elapsed_seconds'], balls=probe['catalogue']['balls'],
                         tower_digest=probe['tower_digest'], catalogue_digest=probe['catalogue_digest'],
                         presentation_digest=probe['presentation_digest'],
                         tower_phases_ms=probe['tower_phases_ms'], tower_detail=probe['tower_detail'],
                         generator=probe['generator']))

    triplets = []
    for scene in ('00', 'b00'):
        for k in (5, 10):
            group = [r for r in rows if r['scene'] == scene and r['K'] == k and r['s'] == 8 and r['backend'] == 'gpu']
            need(len(group) == 6, 'triplet size')
            by_arm = {a: {r['repeat']: r for r in group if r['arm'] == a}
                      for a in ('A_full_overlap', 'B_no_tail', 'C_no_overlap')}
            need(all(set(reps) == {0, 1} for reps in by_arm.values()), 'triplet repetitions')
            def v(a, key):
                return [by_arm[a][j][key] for j in (0, 1)]
            a, b, c = (v(name, 'chain_ms') for name in by_arm)
            triplets.append(dict(scene=scene, K=k, chain_ms=dict(A=a, B=b, C=c),
                                 medians_ms=dict(A=median(a), B=median(b), C=median(c)),
                                 paired_delta_ms=dict(B_minus_A=[b[j]-a[j] for j in (0, 1)],
                                                      C_minus_B=[c[j]-b[j] for j in (0, 1)]),
                                 tower_medians_ms={name[0]: median(v(name, 'tower_ms')) for name in by_arm}))

    s_comparison = []
    for scene in ('00', 'b00'):
        for s in (8, 10, 12):
            group = [r for r in rows if r['scene'] == scene and r['K'] == 5 and r['s'] == s and
                     r['arm'] in ('A_full_overlap', 'engine')]
            gpu = next((r for r in group if r['backend'] == 'gpu' and r['repeat'] == 0), None)
            engine = next((r for r in group if r['backend'] == 'engine'), None)
            need(gpu is not None and engine is not None, 's comparison pair')
            s_comparison.append(dict(scene=scene, s=s, gpu_chain_ms=gpu['chain_ms'],
                                     gpu_tower_ms=gpu['tower_ms'], engine_chain_ms=engine['chain_ms'],
                                     engine_tower_ms=engine['tower_ms'], tower_digest=gpu['tower_digest'],
                                     catalogue_digest=gpu['catalogue_digest'],
                                     presentation_digest=gpu['presentation_digest']))

    return dict(schema='mhgp9_g4_r24b_analysis_v1', public_status='not_claimed',
                scope='whole_frame_u18_grid_1mm_two_masks_one_sequence',
                plan_sha256=sha(plan_path), package_sha256=sha(package_path),
                host_receipt_sha256=sha(host_path), worker_receipt_sha256=sha(worker_path),
                host_status=host['status'], worker_status=worker['status'],
                targeted_shutdown_certified=host['targeted_shutdown_certified'],
                case_count=len(cases), GPU_cases=len(worker['GPU_completed_cases']),
                equal_cross_worker_comparisons=len(worker['cross_worker_comparisons']),
                contract_certified=False, rows=rows, triplets=triplets, s_comparison=s_comparison)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('receipt', type=Path)
    parser.add_argument('--out', type=Path)
    args = parser.parse_args()
    result = json.dumps(build(args.receipt), ensure_ascii=False, sort_keys=True, indent=2) + '\n'
    if args.out is None:
        print(result, end='')
    else:
        args.out.write_text(result)


if __name__ == '__main__':
    main()
