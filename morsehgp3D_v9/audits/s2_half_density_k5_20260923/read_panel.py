#!/usr/bin/env python3
"""Revalidate retained S2 attempts and compute spatial/density finite diagnostics."""
import argparse
import hashlib
import json
import math
import struct
import sys
from pathlib import Path

if not __debug__:
    raise SystemExit('Reader requires assertions enabled; do not use -O')

ROOT = Path('/tmp/mhgp9-s2-scaling-20260923-run')
INPUTS = Path('/tmp/mhgp9-s2-scaling-20260923-inputs')
BINARY = Path('/tmp/mhgp9-v17-batch-density-replay-1533/build/mhgp9_tower_probe')
DENSITIES = ('quarter', 'half', 'full')
SECTORS = ('full', 'half_x_neg', 'half_x_nonneg')


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def fnv_points(path):
    blob = path.read_bytes()
    if len(blob) % 12:
        raise SystemExit('invalid input byte length')
    h = 14695981039346656037
    words = (len(blob) // 12, *struct.unpack(f'<{len(blob) // 4}I', blob))
    for word in words:
        for byte in word.to_bytes(8, 'little'):
            h = (h ^ byte) * 1099511628211 & ((1 << 64) - 1)
    return f'{h:016x}'


def slope(a, b):
    return math.log(b[1] / a[1]) / math.log(b[0] / a[0])


def main():
    global ROOT, INPUTS, BINARY
    ap = argparse.ArgumentParser()
    ap.add_argument('--repo', type=Path, required=True)
    ap.add_argument('--inputs', type=Path, required=True)
    ap.add_argument('--receipt', type=Path, required=True)
    ap.add_argument('--binary', type=Path,
                    help='optional in offline verification; when supplied SHA must match pinned binary')
    summary_mode = ap.add_mutually_exclusive_group()
    summary_mode.add_argument('--partial', action='store_true',
                              help='monitor a still-running panel; do not read or write SUMMARY.json')
    summary_mode.add_argument('--write-summary', action='store_true',
                              help='create SUMMARY.json once for a new complete capture; refuse if it exists')
    args = ap.parse_args()
    ROOT, INPUTS = args.receipt.resolve(), args.inputs.resolve()
    BINARY = args.binary.resolve() if args.binary else None
    panel = json.loads((ROOT / 'PANEL.json').read_text())
    manifest = json.loads((INPUTS / 'MANIFEST.json').read_text())
    versioned_manifest = (args.repo.resolve() /
                          'morsehgp3D_v9/audits/lidar_raw_physical_scaling_20260923/MANIFEST.json')
    if sha(versioned_manifest) != panel['input_manifest_sha256'] or \
       sha(INPUTS / 'MANIFEST.json') != panel['input_manifest_sha256'] or \
       (BINARY is not None and sha(BINARY) != panel['binary_sha256']):
        raise SystemExit('manifest or binary SHA changed')
    build_receipt_path = (args.repo.resolve() /
                          'morsehgp3D_v9/audits/q34_batch_density_quarter_20260923/receipt.json')
    build_receipt = json.loads(build_receipt_path.read_text())
    if build_receipt['published_source_commit'] != panel['source_commit'] or \
       build_receipt['binary_sha256'] != panel['binary_sha256'] or \
       build_receipt['input_manifest_sha256'] != panel['input_manifest_sha256'] or \
       build_receipt['build']['CMAKE_BUILD_TYPE'] != 'Release' or \
       build_receipt['build']['MHGP9_ENABLE_CUDA'] != 'OFF':
        raise SystemExit('versioned source/build receipt disagrees')
    records = [json.loads(line) for line in (ROOT / 'ATTEMPTS.jsonl').read_text().splitlines()]
    by_case = {(r['density'], r['sector'], r['mode']): r for r in records}
    if len(by_case) != len(records) or len(records) != 2 * len(panel['cases']):
        raise SystemExit('attempt count/uniqueness mismatch')
    if not args.partial and (panel.get('complete') is not True or len(panel['cases']) != 9 or len(records) != 18):
        raise SystemExit('final panel is incomplete')
    outputs = {}
    for (density, sector, mode), row in by_case.items():
        meta = manifest['datasets'][density][sector]
        src = INPUTS / meta['points_file']
        rid = INPUTS / meta['raw_return_ids_file']
        if len(row['argv']) != 12 or Path(row['argv'][3]).name != 'mhgp9_tower_probe' or \
           Path(row['argv'][4]).name != meta['points_file']:
            raise SystemExit('recorded input/binary path does not match case')
        expected_argv = ['nice', '-n', '19', row['argv'][3], row['argv'][4], '5', '8', '--s=8',
                         '--static=8', '--grid=1mm',
                         f'--lever=q34_batch_filter={int(mode == "batch")}',
                         '--lever=q34_gpu_filter=0']
        if row['argv'] != expected_argv or row['source_commit'] != panel['source_commit'] or \
           row['binary_sha256'] != panel['binary_sha256'] or \
           row['input_sha256'] != meta['points_sha256'] or \
           row['raw_ids_sha256'] != meta['raw_return_ids_sha256'] or \
           not row['binary_immutable'] or not row['input_immutable']:
            raise SystemExit('attempt provenance mismatch')
        stem = f'k5_{density}_{sector}_{mode}'
        if row['stdout_file'] != stem + '.stdout' or row['stderr_file'] != stem + '.stderr':
            raise SystemExit('attempt output path mismatch')
        if sha(src) != meta['points_sha256'] or sha(rid) != meta['raw_return_ids_sha256']:
            raise SystemExit('input SHA changed')
        if fnv_points(src) != row['input_fnv']:
            raise SystemExit('independent input FNV mismatch')
        if sha(ROOT / row['stdout_file']) != row['stdout_sha256'] or \
           sha(ROOT / row['stderr_file']) != row['stderr_sha256']:
            raise SystemExit('output SHA changed')
        if row['exit_code'] != 0 or row['error'] is not None:
            raise SystemExit('failed attempt cannot qualify')
        out = json.loads((ROOT / row['stdout_file']).read_text())
        outputs[density, sector, mode] = out
        copied = {
            'status': out['status'], 'schema': out['schema'], 'input': out['input'],
            'options': out['options'], 'generator': out['generator'],
            'catalogue': out['catalogue'], 'orders': out['orders'],
            'tower_digest': out['tower_digest'], 'ledger': out['ledger'],
            'chain_cpu_s': out['chain_cpu_s'],
            'chain_wall_ms': out['times_ms']['chain_total'],
            'peak_rss_kb': out['peak_rss_kb'], 'q34_batch': out['q34_batch']}
        if any(row[key] != value for key, value in copied.items()):
            raise SystemExit('attempt metadata disagrees with raw output')
        if row['exit_code'] != 0 or out['status'] != 'complete_relative' or \
           out['input']['hash'] != row['input_fnv'] or out['input']['sites'] != row['sites'] or \
           out['options']['K'] != 5 or out['options']['s'] != 8 or \
           out['options']['workers'] != 8 or out['options']['tower_static_threads'] != 8 or \
           out['options']['levers']['q34_batch_filter'] != (mode == 'batch') or \
           out['options']['levers']['q34_gpu_filter'] or \
           out['q34_batch']['used'] != (mode == 'batch') or \
           (mode == 'batch' and out['q34_batch']['backend'] != 'cpu'):
            raise SystemExit('output mode/status/geometry mismatch')
        if row['ledger']['core_sites'] != row['ledger']['dead_core_form_sites'] + \
           2 * row['ledger']['dead_core_loads']:
            raise SystemExit('core forms identity mismatch')
    case_map = {(r['density'], r['sector']): r for r in panel['cases']}
    if len(case_map) != len(panel['cases']):
        raise SystemExit('duplicate panel case')
    expected_cases = {(density, sector) for density in DENSITIES for sector in SECTORS}
    if not set(case_map).issubset(expected_cases) or \
       (not args.partial and set(case_map) != expected_cases):
        raise SystemExit('panel case set differs from fixed 3x3 matrix')
    v12_path = (args.repo.resolve() /
                'morsehgp3D_v9/audits/lidar_raw_physical_scaling_20260923/SUMMARY.json')
    v12_metrics = json.loads(v12_path.read_text())['metrics']
    v12_fields = ('sites', 'core_sites', 'dead_core_loads', 'catalogue_balls',
                  'expanded_pairs', 'dead_core_uniform_tests')
    for (density, sector), case in case_map.items():
        engine = by_case[density, sector, 'engine']
        batch = by_case[density, sector, 'batch']
        for key in ('status', 'reason', 'input', 'generator', 'catalogue',
                    'tower_work', 'orders', 'tower_digest'):
            if outputs[density, sector, 'engine'][key] != outputs[density, sector, 'batch'][key]:
                raise SystemExit(f'paired projection mismatch {density}/{sector}/{key}')
        for field in ('core_builds', 'core_sites', 'dead_core_loads',
                      'dead_core_form_sites', 'core_closed_edges'):
            if engine['ledger'][field] != batch['ledger'][field]:
                raise SystemExit(f'paired core differs for {density}/{sector}/{field}')
        expected_case = {
            'density': density, 'sector': sector, 'sites': batch['sites'],
            'input_sha256': batch['input_sha256'], 'input_fnv': batch['input_fnv'],
            'tower_digest': batch['tower_digest'],
            'catalogue_balls': batch['catalogue']['balls'],
            'core_loads': batch['ledger']['dead_core_loads'],
            'core_forms_all': batch['ledger']['core_sites'],
            'core_forms_off_endpoints': batch['ledger']['dead_core_form_sites'],
            'engine_chain_cpu_s': engine['chain_cpu_s'],
            'batch_chain_cpu_s': batch['chain_cpu_s'],
            'engine_chain_wall_ms': engine['chain_wall_ms'],
            'batch_chain_wall_ms': batch['chain_wall_ms'],
            'engine_elapsed_s': engine['elapsed_s'],
            'batch_elapsed_s': batch['elapsed_s'],
            'engine_rss_kb': engine['peak_rss_kb'],
            'batch_rss_kb': batch['peak_rss_kb'],
            'engine_stdout_sha256': engine['stdout_sha256'],
            'batch_stdout_sha256': batch['stdout_sha256'],
            'paired_projection_equal': True, 'paired_core_equal': True}
        if case != expected_case:
            raise SystemExit('panel case disagrees with attempts')
        historical = v12_metrics[f'{density}_{sector}']
        now = {'sites': batch['sites'], 'core_sites': batch['ledger']['core_sites'],
               'dead_core_loads': batch['ledger']['dead_core_loads'],
               'catalogue_balls': batch['catalogue']['balls'],
               'expanded_pairs': batch['ledger']['expanded_pairs'],
               'dead_core_uniform_tests': batch['ledger']['dead_core_uniform_tests']}
        if any(now[field] != historical[field] for field in v12_fields):
            raise SystemExit(f'v12 cross-check differs for {density}/{sector}')
    metrics = {
        'forms_all': lambda r: r['ledger']['core_sites'],
        'forms_off_endpoints': lambda r: r['ledger']['dead_core_form_sites'],
        'core_loads': lambda r: r['ledger']['dead_core_loads'],
        'expanded_pairs': lambda r: r['ledger']['expanded_pairs'],
        'core_cover_node_visits': lambda r: r['ledger']['core_cover_node_visits'],
        'catalogue_balls': lambda r: r['catalogue']['balls'],
        'chain_cpu_s': lambda r: r['chain_cpu_s'],
        'chain_wall_ms': lambda r: r['chain_wall_ms'],
    }
    spatial = {}
    for density in DENSITIES:
        if any((density, sector) not in case_map for sector in SECTORS):
            continue
        full = by_case[density, 'full', 'batch']
        halves = [by_case[density, sec, 'batch'] for sec in SECTORS[1:]]
        n = full['sites']
        if sum(r['sites'] for r in halves) != n:
            raise SystemExit('half sites do not reconstruct full')
        spatial[density] = {'sites': n, 'quadratic_reference': sum((r['sites']/n)**2 for r in halves),
                            'ratios_half_sum_over_full': {
                                name: sum(fn(r) for r in halves) / fn(full)
                                for name, fn in metrics.items()}}
    density_slopes = {}
    for sector in SECTORS:
        if any((density, sector) not in case_map for density in DENSITIES):
            continue
        rows = [by_case[d, sector, 'batch'] for d in DENSITIES]
        density_slopes[sector] = {
            name: [slope((rows[0]['sites'], fn(rows[0])), (rows[1]['sites'], fn(rows[1]))),
                   slope((rows[1]['sites'], fn(rows[1])), (rows[2]['sites'], fn(rows[2])))]
            for name, fn in metrics.items()}
    result = {'schema': 'mhgp9_s2_cpu_k5_half_density_summary_v1',
              'source_commit': panel['source_commit'], 'binary_sha256': panel['binary_sha256'],
              'input_manifest_sha256': panel['input_manifest_sha256'],
              'cases': len(case_map), 'attempts': len(records),
              'complete': panel.get('complete') is True, 'paired': True,
              'v12_summary_sha256': sha(v12_path),
              'v12_equal_cases': len(case_map), 'v12_equal_fields': list(v12_fields),
              's2_build_receipt_sha256': sha(build_receipt_path),
              'spatial': spatial, 'density_slopes': density_slopes}
    summary_path = ROOT / 'SUMMARY.json'
    expected_bytes = (json.dumps(result, sort_keys=True, indent=2) + '\n').encode('utf-8')
    if args.write_summary:
        if summary_path.exists():
            raise SystemExit('SUMMARY.json already exists; refusing overwrite')
        with summary_path.open('xb') as stream:
            stream.write(expected_bytes)
    elif not args.partial:
        if not summary_path.exists():
            raise SystemExit('archived SUMMARY.json is missing')
        if summary_path.read_bytes() != expected_bytes:
            raise SystemExit('archived SUMMARY.json differs from recomputed summary')
    print(f'binary_live_verified={BINARY is not None}', file=sys.stderr)
    print(json.dumps(result, sort_keys=True, indent=2))


if __name__ == '__main__':
    main()
