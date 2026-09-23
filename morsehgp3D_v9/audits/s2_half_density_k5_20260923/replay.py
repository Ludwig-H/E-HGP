#!/usr/bin/env python3
"""Local, pinned S2 CPU K5 panel. Raw attempts are retained before validation."""
import argparse
import datetime
import hashlib
import json
import math
import os
import pathlib
import struct
import subprocess
import time

if not __debug__:
    raise SystemExit('Assertions must remain enabled')

ROOT = pathlib.Path('/tmp/mhgp9-s2-scaling-20260923-run')
INPUTS = pathlib.Path('/tmp/mhgp9-s2-scaling-20260923-inputs')
BINARY = pathlib.Path('/tmp/mhgp9-v17-batch-density-replay-1533/build/mhgp9_tower_probe')
MANIFEST_SHA = '6e64125abddbfcd589ba61cf747e493e455adec4beeaea81c2739ce98fe16095'
BINARY_SHA = '076312fb9502dee5a7f0a5fbe549b68c942c511892b4c0ba5583770b511758d7'
DENSITIES = ('quarter', 'half', 'full')
SECTORS = ('half_x_neg', 'half_x_nonneg', 'full')
FNV_PRIME = 1099511628211
FNV_MASK = (1 << 64) - 1


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def fnv_input(path):
    blob = path.read_bytes()
    if len(blob) % 12:
        raise ValueError('invalid u32le payload length')
    n = len(blob) // 12
    h = 14695981039346656037
    for word in (n, *(v for xyz in struct.iter_unpack('<III', blob) for v in xyz)):
        for byte in word.to_bytes(8, 'little'):
            h = ((h ^ byte) * FNV_PRIME) & FNV_MASK
    return n, f'{h:016x}'


def projection(result):
    return {key: result[key] for key in ('input', 'generator', 'catalogue', 'orders', 'tower_digest')}


def write_json(path, row):
    tmp = path.with_suffix(path.suffix + '.tmp')
    tmp.write_text(json.dumps(row, sort_keys=True, indent=2) + '\n')
    tmp.replace(path)


def run_one(manifest, density, sector, mode):
    meta = manifest['datasets'][density][sector]
    point = INPUTS / meta['points_file']
    ids = INPUTS / meta['raw_return_ids_file']
    if sha(point) != meta['points_sha256'] or sha(ids) != meta['raw_return_ids_sha256']:
        raise RuntimeError('manifest/payload SHA mismatch')
    n, fnv = fnv_input(point)
    if n != meta['sites'] or len(ids.read_bytes()) != 4 * n:
        raise RuntimeError('payload lengths disagree with manifest')
    if sha(BINARY) != BINARY_SHA:
        raise RuntimeError('pinned binary changed before run')
    stem = f'k5_{density}_{sector}_{mode}'
    stdout_file = ROOT / f'{stem}.stdout'
    stderr_file = ROOT / f'{stem}.stderr'
    if stdout_file.exists() or stderr_file.exists():
        raise RuntimeError(f'refusing existing attempt {stem}')
    argv = ['nice', '-n', '19', str(BINARY), str(point), '5', '8', '--s=8', '--static=8',
            '--grid=1mm', f'--lever=q34_batch_filter={int(mode == "batch")}',
            '--lever=q34_gpu_filter=0']
    started = datetime.datetime.now(datetime.timezone.utc).isoformat()
    begin = time.monotonic()
    exit_code = None
    error = None
    with stdout_file.open('wb') as out, stderr_file.open('wb') as err:
        try:
            exit_code = subprocess.run(argv, stdout=out, stderr=err, timeout=1800, check=False).returncode
        except subprocess.TimeoutExpired:
            exit_code = 124
            error = 'timeout_1800s'
    elapsed = time.monotonic() - begin
    row = {'density': density, 'sector': sector, 'mode': mode, 'source_commit': 'a6d81f9ce',
           'argv': argv, 'started_utc': started, 'elapsed_s': elapsed, 'exit_code': exit_code,
           'error': error, 'binary_sha256': BINARY_SHA, 'input_sha256': meta['points_sha256'],
           'raw_ids_sha256': meta['raw_return_ids_sha256'], 'sites': n, 'input_fnv': fnv,
           'stdout_file': stdout_file.name, 'stdout_sha256': sha(stdout_file),
           'stderr_file': stderr_file.name, 'stderr_sha256': sha(stderr_file),
           'binary_immutable': sha(BINARY) == BINARY_SHA,
           'input_immutable': sha(point) == meta['points_sha256'] and sha(ids) == meta['raw_return_ids_sha256']}
    if exit_code == 0:
        result = json.loads(stdout_file.read_text())
        row.update({'status': result['status'], 'schema': result['schema'],
                    'input': result['input'], 'options': result['options'],
                    'generator': result['generator'], 'catalogue': result['catalogue'],
                    'orders': result['orders'], 'tower_digest': result['tower_digest'],
                    'ledger': result['ledger'], 'chain_cpu_s': result['chain_cpu_s'],
                    'chain_wall_ms': result['times_ms']['chain_total'],
                    'peak_rss_kb': result['peak_rss_kb'], 'q34_batch': result['q34_batch']})
    with (ROOT / 'ATTEMPTS.jsonl').open('a') as stream:
        stream.write(json.dumps(row, sort_keys=True) + '\n')
        stream.flush()
        os.fsync(stream.fileno())
    if exit_code != 0 or not row['binary_immutable'] or not row['input_immutable']:
        raise RuntimeError(f'failed run {stem}, exit={exit_code}')
    if row['status'] != 'complete_relative' or row['input']['hash'] != fnv or row['input']['sites'] != n:
        raise RuntimeError(f'failed output gate {stem}')
    ledger = row['ledger']
    if ledger['core_sites'] != ledger['dead_core_form_sites'] + 2 * ledger['dead_core_loads']:
        raise RuntimeError(f'core form identity fails for {stem}')
    return row, result


def main():
    global ROOT, INPUTS, BINARY
    ap = argparse.ArgumentParser()
    ap.add_argument('--repo', type=pathlib.Path, required=True,
                    help='checkout containing the versioned raw physical LiDAR manifest')
    ap.add_argument('--inputs', type=pathlib.Path, required=True,
                    help='directory regenerated by lidar_raw_physical_scaling_20260923/generate.py')
    ap.add_argument('--receipt', type=pathlib.Path, required=True,
                    help='new/empty output directory for all attempts')
    ap.add_argument('--binary', type=pathlib.Path, required=True,
                    help='pinned CPU Release v17 probe from a6d81f9ce')
    args = ap.parse_args()
    ROOT, INPUTS, BINARY = args.receipt.resolve(), args.inputs.resolve(), args.binary.resolve()
    versioned_manifest = (args.repo.resolve() /
                          'morsehgp3D_v9/audits/lidar_raw_physical_scaling_20260923/MANIFEST.json')
    if sha(versioned_manifest) != MANIFEST_SHA:
        raise RuntimeError('versioned input manifest changed')
    build_receipt_path = (args.repo.resolve() /
                          'morsehgp3D_v9/audits/q34_batch_density_quarter_20260923/receipt.json')
    build_receipt = json.loads(build_receipt_path.read_text())
    if build_receipt['published_source_commit'] != 'a6d81f9ce' or \
       build_receipt['binary_sha256'] != BINARY_SHA or \
       build_receipt['input_manifest_sha256'] != MANIFEST_SHA or \
       build_receipt['build']['CMAKE_BUILD_TYPE'] != 'Release' or \
       build_receipt['build']['MHGP9_ENABLE_CUDA'] != 'OFF':
        raise RuntimeError('versioned source/build receipt disagrees')
    if ROOT.exists() and any(ROOT.iterdir()):
        raise RuntimeError('refusing nonempty receipt directory')
    ROOT.mkdir(parents=True, exist_ok=True)
    manifest_path = INPUTS / 'MANIFEST.json'
    if sha(manifest_path) != MANIFEST_SHA or sha(BINARY) != BINARY_SHA:
        raise RuntimeError('pinned input or binary mismatch')
    manifest = json.loads(manifest_path.read_text())
    if (ROOT / 'ATTEMPTS.jsonl').exists() or (ROOT / 'PANEL.json').exists():
        raise RuntimeError('refusing existing campaign')
    panel = {'schema': 'mhgp9_s2_cpu_k5_half_density_panel_v1', 'source_commit': 'a6d81f9ce',
             'binary_sha256': BINARY_SHA, 'input_manifest_sha256': MANIFEST_SHA, 'cases': []}
    for density in DENSITIES:
        for sector in SECTORS:
            engine, engine_raw = run_one(manifest, density, sector, 'engine')
            batch, batch_raw = run_one(manifest, density, sector, 'batch')
            if projection(engine_raw) != projection(batch_raw):
                raise RuntimeError(f'engine/batch output projection differs for {density}/{sector}')
            for field in ('core_builds', 'core_sites', 'dead_core_loads', 'dead_core_form_sites', 'core_closed_edges'):
                if engine['ledger'][field] != batch['ledger'][field]:
                    raise RuntimeError(f'engine/batch core ledger differs: {density}/{sector}/{field}')
            case = {'density': density, 'sector': sector, 'sites': batch['sites'],
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
                    'engine_elapsed_s': engine['elapsed_s'], 'batch_elapsed_s': batch['elapsed_s'],
                    'engine_rss_kb': engine['peak_rss_kb'], 'batch_rss_kb': batch['peak_rss_kb'],
                    'engine_stdout_sha256': engine['stdout_sha256'],
                    'batch_stdout_sha256': batch['stdout_sha256'],
                    'paired_projection_equal': True, 'paired_core_equal': True}
            panel['cases'].append(case)
            write_json(ROOT / 'PANEL.json', panel)
            print(json.dumps({'density': density, 'sector': sector, 'sites': case['sites'],
                              'forms_all': case['core_forms_all'], 'digest': case['tower_digest'],
                              'cpu_batch_s': case['batch_chain_cpu_s'],
                              'wall_batch_s': case['batch_chain_wall_ms'] / 1000}, sort_keys=True), flush=True)
    panel['complete'] = True
    write_json(ROOT / 'PANEL.json', panel)


if __name__ == '__main__':
    main()
