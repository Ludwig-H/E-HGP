#!/usr/bin/env python3
"""Read pinned S2 edge traces and test whether nominal q3/q4 centre disks fit the actual cloud box.

This is a necessary opportunity screen for a single clipped-box guard.  It
does not search guards, prove lane closure, or estimate runtime savings.
"""
import argparse
import hashlib
import json
import struct
import time
from collections import Counter
from pathlib import Path

REC = struct.Struct('<IIII')
XYZ = struct.Struct('<III')
ID = struct.Struct('<I')
EXPECTED_MANIFEST = '6e64125abddbfcd589ba61cf747e493e455adec4beeaea81c2739ce98fe16095'
EXPECTED_SOURCE_COMMIT = 'c265a5dae4dd92059fc78acc0a1d7f52de9c1435'
AUDIT_REL = Path('morsehgp3D_v9/audits/edge_matched_core_20260923')


def sha(blob):
    return hashlib.sha256(blob).hexdigest()


def read_verified(path, digest):
    blob = path.read_bytes()
    if sha(blob) != digest:
        raise ValueError(f'SHA256 mismatch: {path}')
    return blob


def projection(row):
    return {key: row[key] for key in ('input', 'generator', 'catalogue', 'orders', 'tower_digest')}


def load_input(inputs, manifest, density, sector, summary):
    meta = manifest['datasets'][density][sector]
    points_blob = read_verified(inputs / meta['points_file'], meta['points_sha256'])
    ids_blob = read_verified(inputs / meta['raw_return_ids_file'], meta['raw_return_ids_sha256'])
    if summary['input_sha256'] != meta['points_sha256'] or \
       summary['raw_ids_sha256'] != meta['raw_return_ids_sha256'] or \
       summary['input_sites'] != meta['sites'] or \
       len(points_blob) != 12 * meta['sites'] or len(ids_blob) != 4 * meta['sites']:
        raise ValueError(f'{density}/{sector}: input mismatch')
    coords = [None] * manifest['raw_returns']
    lo = [2**64] * 3
    hi = [0] * 3
    for (rid,), xyz in zip(ID.iter_unpack(ids_blob), XYZ.iter_unpack(points_blob), strict=True):
        if rid >= len(coords) or coords[rid] is not None:
            raise ValueError(f'{density}/{sector}: duplicate or invalid return ID')
        coords[rid] = xyz
        for i, v in enumerate(xyz):
            lo[i] = min(lo[i], v)
            hi[i] = max(hi[i], v)
    if sum(p is not None for p in coords) != meta['sites']:
        raise ValueError(f'{density}/{sector}: incomplete input map')
    return coords, lo, hi


def check_capture(root, summary, density, sector, format_after, receipt):
    if summary['case'] != sector or summary.get('density', 'full') != density or \
       summary['trace']['duplicate_records'] != 0 or \
       summary['input_manifest_sha256'] != EXPECTED_MANIFEST:
        raise ValueError(f'{root}: incompatible capture summary')
    patch = json.loads((root / 'PATCH_MANIFEST.json').read_bytes())
    if patch['source_commit'] != EXPECTED_SOURCE_COMMIT:
        raise ValueError(f'{root}: source commit differs')
    role = 'after_trace' if format_after else 'before_trace'
    pinned_trace = receipt[role]
    patch_metadata_sha = sha(json.dumps({k: v for k, v in patch.items() if k != 'source_dir'},
                                       sort_keys=True, separators=(',', ':')).encode())
    if patch_metadata_sha != pinned_trace['patch_metadata_sha256'] or \
       patch['injected_sha256'] != pinned_trace['injected_sha256'] or \
       patch['trace_header_sha256'] != pinned_trace['header_sha256'] or \
       patch['source_sha256'] != receipt['source_file_sha256'] or \
       patch['git_archive_sha256'] != receipt['source_archive_sha256']:
        raise ValueError(f'{root}: patch provenance differs from versioned RESULTS')
    source = Path(patch['source_dir'])
    if sha((source / patch['source_file']).read_bytes()) != patch['injected_sha256'] or \
       sha((source / 'src/gen/pipeline/edge_trace_audit.hpp').read_bytes()) != patch['trace_header_sha256']:
        raise ValueError(f'{root}: temporary source differs from patch manifest')
    cache = (root / 'build/CMakeCache.txt').read_text()
    if 'CMAKE_BUILD_TYPE:STRING=Release' not in cache or 'MHGP9_ENABLE_CUDA:BOOL=OFF' not in cache:
        raise ValueError(f'{root}: expected Release CPU-only binary')
    binary_sha = sha((root / 'build/mhgp9_tower_probe').read_bytes())
    if binary_sha != pinned_trace['binary_sha256']:
        raise ValueError(f'{root}: binary differs from versioned RESULTS')
    pinned_case = (receipt['post_full'] if format_after else
                   next((c for c in receipt['cases'] if c['density'] == density and c['sector'] == sector), None))
    if pinned_case is None or \
       pinned_case['input_sha256'] != summary['input_sha256'] or \
       pinned_case['raw_ids_sha256'] != summary['raw_ids_sha256'] or \
       pinned_case['core_loads'] != summary['trace']['records'] or \
       pinned_case['core_forms'] != summary['trace']['sum_core_sites'] or \
       pinned_case['engine_stdout_sha256'] != summary['engine']['stdout_sha256'] or \
       pinned_case['batch_stdout_sha256'] != summary['batch']['stdout_sha256'] or \
       pinned_case['trace_parts'] != summary['trace']['parts']:
        raise ValueError(f'{root}: capture case differs from versioned RESULTS')
    if format_after and pinned_case['core_closed_edges'] != summary['trace']['core_closed_edges']:
        raise ValueError(f'{root}: post-core closed count differs from versioned RESULTS')
    if not format_after and (pinned_case['sites'] != summary['input_sites'] or
                             pinned_case['input_fnv'] != summary['input_fnv'] or
                             pinned_case['tower_digest'] != summary['logical_result']['tower_digest']):
        raise ValueError(f'{root}: before-core case identity differs from versioned RESULTS')
    outputs = {}
    case_dir = root / (sector if density == 'full' else f'{density}/{sector}')
    for mode in ('engine', 'batch'):
        meta = summary[mode]
        if meta['binary_sha256'] != binary_sha or meta['returncode'] != 0:
            raise ValueError(f'{case_dir}: binary or run differs')
        out = read_verified(case_dir / f'{mode}.stdout', meta['stdout_sha256'])
        err = read_verified(case_dir / f'{mode}.stderr', meta['stderr_sha256'])
        if err:
            raise ValueError(f'{case_dir}: nonempty stderr')
        outputs[mode] = json.loads(out)
        if outputs[mode]['status'] != 'complete_relative':
            raise ValueError(f'{case_dir}: incomplete run')
    if projection(outputs['engine']) != projection(outputs['batch']) or \
       projection(outputs['batch']) != summary['logical_result']:
        raise ValueError(f'{case_dir}: logical projection differs')
    for key in ('core_builds', 'core_sites', 'dead_core_loads',
                'dead_core_form_sites', 'core_closed_edges'):
        if outputs['engine']['ledger'][key] != outputs['batch']['ledger'][key] or \
           (key in summary['core_ledger'] and
            summary['core_ledger'][key] != outputs['batch']['ledger'][key]):
            raise ValueError(f'{case_dir}: core ledger differs: {key}')
    if format_after != (summary['trace'].get('trace_format') == 'before_after'):
        raise ValueError(f'{case_dir}: trace format differs')
    return case_dir, binary_sha


def add_score(scores, stage, lane, contained, forms, failed_axes):
    row = scores[stage][lane]
    row['edges'] += 1
    row['forms'] += forms
    if contained:
        row['contained_edges'] += 1
        row['contained_forms'] += forms
    else:
        row['clipped_edges'] += 1
        row['clipped_forms'] += forms
        for i, name in enumerate('xyz'):
            if failed_axes & (1 << i):
                row[f'clipped_{name}_edges'] += 1
                row[f'clipped_{name}_forms'] += forms


def run_case(root, inputs, manifest, density, sector, format_after, receipt):
    start = time.monotonic()
    case_dir = root / (sector if density == 'full' else f'{density}/{sector}')
    summary = json.loads((case_dir / 'SUMMARY.json').read_text())
    case_dir, binary_sha = check_capture(root, summary, density, sector, format_after, receipt)
    coords, lo, hi = load_input(inputs, manifest, density, sector, summary)
    scores = {'before_core': {'q3': Counter(), 'q4': Counter(), 'edge_union': Counter()}}
    if format_after:
        scores['after_core'] = {'q3': Counter(), 'q4': Counter(), 'edge_union': Counter()}
    masks_before = Counter()
    masks_after = Counter()
    seen = set()
    records = forms_sum = 0
    for part in summary['trace']['parts']:
        data = read_verified(case_dir / 'trace' / part['file'], part['sha256'])
        if len(data) != REC.size * part['records']:
            raise ValueError(f'{case_dir}: truncated trace part')
        for a, b, forms, packed in REC.iter_unpack(data):
            before = packed & 0xff
            after = (packed >> 8) & 0xff if format_after else 0
            if a >= b or forms < 2 or before not in (2, 4, 6) or \
               (format_after and (packed >> 16 or after not in (0, 2, 4, 6) or after & ~before)) or \
               (not format_after and packed != before):
                raise ValueError(f'{case_dir}: invalid trace record')
            key = (a << 32) | b
            if key in seen:
                raise ValueError(f'{case_dir}: duplicate trace edge')
            seen.add(key)
            pa, pb = coords[a], coords[b]
            if pa is None or pb is None:
                raise ValueError(f'{case_dir}: edge endpoint not in input')
            d2 = [(pb[i] - pa[i]) ** 2 for i in range(3)]
            D = sum(d2)
            if not D:
                raise ValueError(f'{case_dir}: degenerate edge')
            q = [min(pa[i] + pb[i] - 2 * lo[i], 2 * hi[i] - pa[i] - pb[i]) for i in range(3)]
            if min(q) < 0:
                raise ValueError(f'{case_dir}: edge outside actual input box')
            failed = {2: 0, 4: 0}
            for i in range(3):
                perp = D - d2[i]
                if 3 * q[i] * q[i] < perp:
                    failed[2] |= 1 << i
                if 2 * q[i] * q[i] < perp:
                    failed[4] |= 1 << i
            if failed[4] == 0 and failed[2] != 0:
                raise ValueError(f'{case_dir}: q4 containment not nested in q3')
            masks_before[before] += 1
            if format_after:
                masks_after[after] += 1
            for stage, mask in (('before_core', before), ('after_core', after)) if format_after else (('before_core', before),):
                lane_failed = []
                for bit, lane in ((2, 'q3'), (4, 'q4')):
                    if mask & bit:
                        lane_failed.append(failed[bit])
                        add_score(scores, stage, lane, failed[bit] == 0, forms, failed[bit])
                if lane_failed:
                    # A single-cell closure of this whole edge requires all open
                    # lanes' nominal disks to clip the box.  This is necessary only.
                    all_clipped = all(x != 0 for x in lane_failed)
                    add_score(scores, stage, 'edge_union', not all_clipped, forms,
                              failed[2] | failed[4] if all_clipped else 0)
            records += 1
            forms_sum += forms
    trace = summary['trace']
    if records != trace['records'] or len(seen) != trace['unique_edges'] or \
       forms_sum != trace['sum_core_sites'] or \
       records != summary['core_ledger']['core_builds'] or \
       forms_sum != summary['core_ledger']['core_sites'] or \
       forms_sum != summary['core_ledger']['dead_core_form_sites'] + 2 * records or \
       {str(k): v for k, v in masks_before.items()} != trace['mask_before_core']:
        raise ValueError(f'{case_dir}: trace aggregate differs')
    if format_after and ({str(k): v for k, v in masks_after.items()} != trace['mask_after_core'] or
                         masks_after[0] != summary['core_ledger']['core_closed_edges']):
        raise ValueError(f'{case_dir}: after-core mask aggregate differs')
    out = {'density': density, 'sector': sector, 'sites': summary['input_sites'],
           'box_lo': lo, 'box_hi': hi, 'trace_role': 'after_trace' if format_after else 'before_trace',
           'trace_summary_sha256': sha((case_dir / 'SUMMARY.json').read_bytes()),
           'binary_sha256': binary_sha, 'records': records, 'forms': forms_sum,
           'mask_before_core': dict(sorted(masks_before.items())),
           'scores': {stage: {lane: dict(counter) for lane, counter in lanes.items()}
                      for stage, lanes in scores.items()}}
    if format_after:
        out['mask_after_core'] = dict(sorted(masks_after.items()))
    return out, time.monotonic() - start


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--repo', type=Path, default=Path.cwd())
    ap.add_argument('--inputs', type=Path, default=Path('/tmp/mhgp9-s2-scaling-20260923-inputs'))
    ap.add_argument('--before-root', type=Path, default=Path('/tmp/mhgp9-edge-core-audit-20260923'))
    ap.add_argument('--after-root', type=Path, default=Path('/tmp/mhgp9-edge-core-after-audit-20260923'))
    ap.add_argument('--out', type=Path, default=Path('/tmp/mhgp9_single_cell_disk_20260923.json'))
    ap.add_argument('--sectors', nargs='+', default=['full'])
    ap.add_argument('--densities', nargs='+', default=['full', 'half', 'quarter'])
    ap.add_argument('--case', action='append', default=[], metavar='DENSITY:SECTOR',
                    help='Select an individual density/sector pair; repeat as needed')
    args = ap.parse_args()
    if args.out.exists():
        raise ValueError(f'refusing existing output: {args.out}')
    source = args.repo / 'morsehgp3D_v9/audits/lidar_raw_physical_scaling_20260923/MANIFEST.json'
    manifest_blob = read_verified(source, EXPECTED_MANIFEST)
    if (args.inputs / 'MANIFEST.json').read_bytes() != manifest_blob:
        raise ValueError('regenerated input manifest differs from versioned manifest')
    manifest = json.loads(manifest_blob)
    audit = args.repo / AUDIT_REL
    checksum_rows = {}
    for line in (audit / 'SHA256SUMS').read_text().splitlines():
        digest, name = line.split('  ', 1)
        if name in checksum_rows:
            raise ValueError('duplicate audit checksum')
        checksum_rows[name] = digest
    receipt_blob = read_verified(audit / 'RESULTS.json', checksum_rows['RESULTS.json'])
    receipt = json.loads(receipt_blob)
    if receipt['schema'] != 'mhgp9_edge_matched_core_receipt_v1' or \
       receipt['source_commit'] != EXPECTED_SOURCE_COMMIT:
        raise ValueError('versioned RESULTS schema/source differs')
    post_blob = read_verified(audit / 'POST_CORE_FULL.json', checksum_rows['POST_CORE_FULL.json'])
    post = json.loads(post_blob)
    if not all(post['checks'].values()) or not post['edge_sets_equal'] or \
       not post['core_sizes_and_pre_masks_equal'] or \
       post['before_binary_sha256'] != receipt['before_trace']['binary_sha256'] or \
       post['after_binary_sha256'] != receipt['after_trace']['binary_sha256']:
        raise ValueError('versioned post-core edge matching differs')
    rows = []
    pairs = ([tuple(value.split(':', 1)) for value in args.case] if args.case else
             [(d, s) for d in args.densities for s in args.sectors])
    if len(set(pairs)) != len(pairs):
        raise ValueError('duplicate density/sector case')
    for density, sector in pairs:
        if density not in manifest['datasets'] or sector not in manifest['datasets'][density]:
            raise ValueError(f'unknown case: {density}/{sector}')
        root = args.after_root if density == 'full' and sector == 'full' else args.before_root
        row, elapsed = run_case(root, args.inputs, manifest, density, sector,
                                root == args.after_root, receipt)
        rows.append(row)
        lane = row['scores']['before_core']['edge_union']
        print(json.dumps({'density': density, 'sector': sector, 'records': row['records'],
                          'all_open_disks_clip_edges': lane['clipped_edges'],
                          'all_open_disks_clip_forms': lane['clipped_forms'],
                          'analysis_wall_s': elapsed}), flush=True)
    result = {'schema': 'mhgp9_single_box_nominal_disk_screen_v1',
              'source_commit': EXPECTED_SOURCE_COMMIT,
              'input_manifest_sha256': EXPECTED_MANIFEST,
              'versioned_results_sha256': sha(receipt_blob),
              'versioned_post_core_sha256': sha(post_blob),
              'criterion': 'For each axis i, q=min(a_i+b_i-2lo_i,2hi_i-a_i-b_i); q3 contained iff 3q^2>=D-d_i^2 on all axes; q4 contained iff 2q^2>=D-d_i^2 on all axes.',
              'interpretation': 'For q3/q4, clipped means that the nominal disk extends outside the actual input box; tangency to the box is contained. For edge_union, clipped means all still-open lane disks extend outside the box; contained means at least one still-open lane disk is fully contained. A fully contained nominal disk makes a one-cell universal guard redundant with S2; clipping is necessary but not sufficient for a new proof. Core forms are a reference weight already paid in the trace, not proved savings.',
              'cases': rows}
    args.out.write_text(json.dumps(result, indent=2, sort_keys=True) + '\n')


if __name__ == '__main__':
    main()
