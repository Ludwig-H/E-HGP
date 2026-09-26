#!/usr/bin/env python3
"""Rejudge the S2 tile-cache capture and print its paired analysis as JSON.

Accepts either a raw gpu_filter_v9_host directory or a published directory
with host/ and vm/. No cloud command, benchmark, mutation or FULL claim is
made. The current protocol must be the protocol pinned by the capture.
For a published package without its snapshot/manifest, pass their paths.
"""

import argparse
import json
from pathlib import Path
from statistics import median
import sys


HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
sys.path.insert(0, str(ROOT / 'gcp-migration'))
import gpu_filter_session_v9 as session  # noqa: E402
import gpu_filter_worker_v9 as worker  # noqa: E402

need, sha = worker.need, worker.sha
TIMES = worker.GPU_PASS_TIMES


def read(path):
    return worker.strict_json(path.read_bytes())


def statistics(values):
    return dict(count=len(values), minimum=min(values), median=median(values), maximum=max(values)) if values else None


def timing_summary(passes):
    return {selection: {key: statistics([row[key] for row in rows]) for key in TIMES}
            for selection, rows in (('all_passes', passes), ('warm_passes_1_onward', passes[1:]))}


def ratio(reference, candidate):
    return reference / candidate if candidate else None


def analyze_rows(cases, probes, command_rows):
    """Arithmetic after protocol validation; still refuse unpaired populations."""
    need(len(cases) == len(probes) == len(command_rows), 'analysis row lengths')
    rows, groups = [], {}
    for index, (case, probe, command) in enumerate(zip(cases, probes, command_rows)):
        gpu = probe['gpu']
        need(probe['schema'] == worker.PROBE_SCHEMA, 'all-pass analysis requires probe v3')
        need(worker.validate_probe(probe, case, command['exit_code']) == 'complete',
             'only complete exact comparisons enter performance analysis')
        cache = case['tile_cache']
        row = dict(index=index, scene=case['scene'], sites=case['n'], K=case['k'], s=case['s'],
                   repeat=case['repeat'], arm='tile_cache' if cache else 'reference',
                   input=probe['input'], population=probe['population'], cpu=probe['cpu'],
                   gpu_counts={key: gpu[key] for key in worker.GPU_COUNTS + worker.GPU_V3_COUNTS},
                   passes=gpu['passes'], timings_ms=timing_summary(gpu['passes']),
                   selected_minimum_total_pass_ms={key: gpu[key] for key in TIMES},
                   process_elapsed_seconds=command['elapsed_seconds'], peak_host_rss_kb=probe['peak_rss_kb'],
                   pair_geometry_tests=gpu['pair_visits'] + gpu['cache_node_tests'],
                   # 88-byte trace plus one-byte mask; two u64 rectangle arrays.
                   # This excludes CUB scratch, capacities, base buffers and allocations.
                   tile_cache_logical_storage_bytes=(89 * gpu['tiles'] +
                                                     16 * probe['population']['rectangles']) if cache else 0)
        rows.append(row)
        key = (case['scene'], case['k'], case['s'], case['workers'])
        groups.setdefault(key, []).append(row)

    comparisons = []
    for (scene, k, s, workers), group in groups.items():
        reference = [row for row in group if row['arm'] == 'reference']
        candidate = [row for row in group if row['arm'] == 'tile_cache']
        need(reference and candidate, 'both arms required for every group')
        identity = (reference[0]['input'], reference[0]['population'],
                    {key: reference[0]['cpu'][key] for key in worker.CPU_COUNTS})
        need(all((row['input'], row['population'], {key: row['cpu'][key] for key in worker.CPU_COUNTS}) == identity
                 for row in group), 'paired populations or CPU work differ')
        for arm in (reference, candidate):
            need(all(row['gpu_counts'] == arm[0]['gpu_counts'] for row in arm),
                 'GPU work differs between independent processes of one arm')
        def arm_times(arm):
            # A process remains the experimental unit. Pooling internal passes
            # must not pretend they are independent process repetitions.
            return {selection: {key: statistics([row['timings_ms'][selection][key]['median'] for row in arm])
                                for key in TIMES}
                    for selection in ('all_passes', 'warm_passes_1_onward')}
        ref_times, new_times = arm_times(reference), arm_times(candidate)
        ratios = {selection: {key: ratio(ref_times[selection][key]['median'], new_times[selection][key]['median'])
                              for key in TIMES}
                  for selection in ref_times}
        matched = []
        for ref in reference:
            matches = [row for row in candidate if row['repeat'] == ref['repeat']]
            need(len(matches) == 1, 'one cache process required per reference repetition')
            new = matches[0]
            matched.append(dict(reference_index=ref['index'], cache_index=new['index'], repeat=ref['repeat'],
                                warm_pair_delta_ms=new['timings_ms']['warm_passes_1_onward']['pair_ms']['median'] -
                                ref['timings_ms']['warm_passes_1_onward']['pair_ms']['median'],
                                warm_total_delta_ms=new['timings_ms']['warm_passes_1_onward']['total_ms']['median'] -
                                ref['timings_ms']['warm_passes_1_onward']['total_ms']['median']))
        need(len(matched) == len(candidate), 'unmatched cache process')
        comparisons.append(dict(scene=scene, K=k, s=s, workers=workers,
                                process_count=dict(reference=len(reference), tile_cache=len(candidate)),
                                order=[row['arm'] for row in group], paired_processes=matched,
                                process_median_timings_ms=dict(reference=ref_times, tile_cache=new_times),
                                timing_speedup_reference_over_cache=ratios,
                                pair_geometry_tests=dict(reference=reference[0]['pair_geometry_tests'],
                                                         tile_cache=candidate[0]['pair_geometry_tests']),
                                geometry_reduction_reference_over_cache=ratio(reference[0]['pair_geometry_tests'],
                                                                             candidate[0]['pair_geometry_tests']),
                                tile_cache_logical_storage_bytes=candidate[0]['tile_cache_logical_storage_bytes']))
    return rows, comparisons


def build(folder, manifest_path=None, snapshot_path=None):
    host = folder if (folder / 'received/output/receipt.json').is_file() else folder / 'host'
    output = host / 'received/output' if (host / 'received/output/receipt.json').is_file() else folder / 'vm'
    host_receipt = read(host / 'receipt.json')
    receipt = read(output / 'receipt.json')
    manifest_path = manifest_path or (host / 'source_manifest.json' if (host / 'source_manifest.json').is_file()
                                     else folder / 'source_manifest.json')
    snapshot_path = snapshot_path or host / 'snapshot.tar.gz'
    need(manifest_path.is_file() and snapshot_path.is_file(), 'original manifest and snapshot required')
    need(sha(manifest_path) == host_receipt.get('manifest_sha256') and
         sha(snapshot_path) == host_receipt.get('snapshot_sha256'), 'host manifest/snapshot pins')
    manifest = read(manifest_path)
    cases, provenance = session.validate_snapshot(snapshot_path, manifest)
    session.validate_protocol_runtime(manifest)
    need(cases == read(HERE / 'plan.json')['cases'] and len(cases) == 14,
         'capture differs from the fixed 14-process ABBA/AB plan')
    need(host_receipt.get('worker_sha256') == manifest['gcp-migration/gpu_filter_worker_v9.py'] and
         host_receipt.get('controller_sha256') == manifest['gcp-migration/gpu_filter_session_v9.py'],
         'host executing protocol pins')
    status = session.validate_received(output, manifest, host_receipt['worker_sha256'], cases,
                                       host_receipt['generation'], provenance, host_receipt['verified_guard'])
    need(status == host_receipt.get('status') == 'completed' and host_receipt.get('targeted_shutdown_certified') is True
         and host_receipt.get('target') == worker.TARGET and host_receipt.get('provenance') == provenance and
         host_receipt.get('GPU_executed') is True and host_receipt.get('FULL_executed') is False,
         'completed exact capture and targeted shutdown required')
    # This is archived closure, not an independent live GCE read.
    stops = [row for row in host_receipt['commands'] if row.get('name') == 'guarded_stop']
    need(len(stops) == 1 and stops[0]['exit_code'] == 0 and stops[0]['group_closed'] is True and
         stops[0]['argv'][-2:] == ['--expected-last-start-timestamp', host_receipt['generation']],
         'targeted stop command/generation')
    stop_raw_verified = (host / 'guarded_stop.stdout').is_file() and (host / 'guarded_stop.stderr').is_file()
    if stop_raw_verified:
        need(sha(host / 'guarded_stop.stdout') == stops[0]['stdout_sha256'] and
             sha(host / 'guarded_stop.stderr') == stops[0]['stderr_sha256'], 'raw stop command hashes')
    probes = [read(output / ('probe_%d.stdout' % index)) for index in range(len(cases))]
    commands = [read(output / ('probe_%d.command.json' % index)) for index in range(len(cases))]
    rows, comparisons = analyze_rows(cases, probes, commands)
    return dict(schema='mhgp9_g4_tile_cache_analysis_v1', public_status='not_claimed',
                scope='S2_pair_filter_whole_frame_u18_1mm_nonground_three_frames_one_sequence',
                FULL_executed=False, contract_certified=False, global_subquadratic_claim=False,
                plan_sha256=sha(HERE / 'plan.json'), snapshot_sha256=sha(snapshot_path),
                manifest_sha256=sha(manifest_path), host_receipt_sha256=sha(host / 'receipt.json'),
                worker_receipt_sha256=sha(output / 'receipt.json'), source_commit=provenance['commit'],
                protocol_replayed=True, host_status=host_receipt['status'], worker_status=receipt['status'],
                targeted_shutdown_certified=True, generation=host_receipt['generation'],
                shutdown_raw_hashes_verified=stop_raw_verified, live_GCE_state_read_by_analyzer=False,
                case_count=len(rows), gpu_pass_count=sum(len(row['passes']) for row in rows),
                all_pair_masks_equal_to_cpu=True, all_repeat_masks_and_work_equal=True,
                memory_scope='logical incremental cache arrays only; not peak VRAM',
                timing_scope='CUDA event filter phases; excludes CPU front, census, catalogue and FULL tower',
                warm_definition='drop pass 0 separately in each process; median of remaining passages',
                grouping='process is the independent unit; compare medians of process medians',
                rows=rows, comparisons=comparisons)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('receipt', type=Path)
    parser.add_argument('--manifest', type=Path)
    parser.add_argument('--snapshot', type=Path)
    args = parser.parse_args()
    try:
        result = build(args.receipt, args.manifest, args.snapshot)
    except (ValueError, KeyError, OSError, TypeError) as error:
        print(json.dumps(dict(status='refused', reason=str(error), public_status='not_claimed'), sort_keys=True))
        return 2
    print(json.dumps(result, ensure_ascii=False, sort_keys=True, indent=2))
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
