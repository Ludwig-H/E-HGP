#!/usr/bin/env python3
"""Read-only 15-case segment join over the pinned S2 traces."""

import argparse
import hashlib
import json
import struct
import subprocess
import time
from pathlib import Path

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[2]
MANIFEST = REPO / 'morsehgp3D_v9/audits/lidar_raw_physical_scaling_20260923/MANIFEST.json'
TRACE_RECEIPT = REPO / 'morsehgp3D_v9/audits/edge_matched_core_20260923/RESULTS.json'
CASES = ('full', 'quarter_x_neg_y_neg', 'quarter_x_neg_y_nonneg',
         'quarter_x_nonneg_y_neg', 'quarter_x_nonneg_y_nonneg')
DENSITIES = ('quarter', 'half', 'full')
REC = struct.Struct('<IIII')


def require(ok, explanation):
    if not ok:
        raise ValueError(explanation)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def parse_measure(text):
    lines = text.splitlines()
    heads = [line for line in lines if line.startswith('head ')]
    require(len(heads) == 1, 'sidecar head missing/duplicated')
    values = tuple(map(int, heads[0].split()[1:]))
    require(len(values) == 21, 'sidecar head length differs')
    names = ('sites', 'front_rectangles', 'front_pair_mass', 'open_rectangles',
             'open_pair_mass', 'rectangle_node_visits', 'survivor_edges', 'core_forms',
             'q3_survivors', 'q4_survivors', 'empty_open_rectangles', 'positive_segments',
             'singleton_rectangles', 'singleton_survivors', 'singleton_forms',
             'large_product_forms', 'large_product_edges', 'large_product_q3',
             'large_product_q4', 'segment16_q3', 'segment16_q4')
    head = dict(zip(names, values, strict=True))
    histograms = {}
    for label in ('product', 'segment'):
        hist = {}
        for line in lines:
            if line.startswith(label + ' '):
                words = line.split()
                require(len(words) == 6, f'invalid {label} row')
                power = int(words[1])
                require(power not in hist and 0 <= power < 32, f'duplicate/out-of-range {label} bucket')
                rects, pairs, edges, forms = map(int, words[2:])
                require(rects > 0 and pairs >= edges and forms >= 2 * edges,
                        f'invalid {label} bucket totals')
                if label == 'product':
                    require(rects * (1 << power) <= pairs < rects * (1 << (power + 1)),
                            'product bucket bounds failed')
                else:
                    require(rects * (1 << power) <= edges < rects * (1 << (power + 1)),
                            'segment bucket bounds failed')
                hist[str(power)] = dict(rectangles=rects, product_pairs=pairs,
                                       survivor_edges=edges, core_forms=forms)
        require(hist, f'no {label} rows')
        histograms[label] = hist
    product = histograms['product']
    segment = histograms['segment']
    require(tuple(sum(row[field] for row in product.values()) for field in
                  ('rectangles', 'product_pairs', 'survivor_edges', 'core_forms')) ==
            tuple(head[field] for field in ('open_rectangles', 'open_pair_mass',
                                            'survivor_edges', 'core_forms')),
            'product histogram does not partition work')
    require(tuple(sum(row[field] for row in segment.values()) for field in
                  ('rectangles', 'survivor_edges', 'core_forms')) ==
            tuple(head[field] for field in ('positive_segments', 'survivor_edges', 'core_forms')),
            'segment histogram does not partition positive segments')
    require(head['positive_segments'] + head['empty_open_rectangles'] == head['open_rectangles'],
            'empty/positive segment count differs')
    segment16_edges = sum(row['survivor_edges'] for power, row in segment.items() if int(power) >= 4)
    require(head['segment16_q3'] <= segment16_edges and head['segment16_q4'] <= segment16_edges
            and head['segment16_q3'] + head['segment16_q4'] >= segment16_edges,
            'segment>=16 lane masks do not cover its survivors')
    require(sum(row['core_forms'] for power, row in product.items() if int(power) >= 10) ==
            head['large_product_forms'] and
            sum(row['survivor_edges'] for power, row in product.items() if int(power) >= 10) ==
            head['large_product_edges'], 'large-product subtotal differs')
    require(product['0']['rectangles'] == head['singleton_rectangles'] and
            product['0']['survivor_edges'] == head['singleton_survivors'] and
            product['0']['core_forms'] == head['singleton_forms'], 'singleton subtotal differs')
    top = []
    for line in lines:
        if line.startswith('top '):
            words = line.split()
            require(len(words) == 7, 'invalid top row')
            num, den, count, forms, edges, pairs = map(int, words[1:])
            require(count == (head['positive_segments'] * num + den - 1) // den and
                    forms <= head['core_forms'] and edges <= head['survivor_edges'] and
                    pairs <= head['open_pair_mass'], 'top row invalid')
            top.append(dict(numerator=num, denominator=den, segments=count,
                            core_forms=forms, survivor_edges=edges, product_pairs=pairs))
    require([(row['numerator'], row['denominator']) for row in top] ==
            [(1, 1000), (1, 100), (5, 100), (10, 100)], 'top fractions differ')
    require([row['core_forms'] for row in top] == sorted(row['core_forms'] for row in top),
            'top form prefixes not monotone')
    return dict(head=head, histograms=histograms, top=top)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--inputs', type=Path, required=True)
    parser.add_argument('--traces', type=Path, required=True)
    parser.add_argument('--archive', type=Path, required=True)
    parser.add_argument('--binary', type=Path, required=True)
    parser.add_argument('--out', type=Path, required=True)
    args = parser.parse_args()
    require(not args.out.exists(), 'refusing to overwrite receipt')
    manifest = json.loads(MANIFEST.read_text())
    parent = json.loads(TRACE_RECEIPT.read_text())
    require(parent['source_commit'] == 'c265a5dae4dd92059fc78acc0a1d7f52de9c1435',
            'trace source commit differs')
    require(sha(args.inputs / 'MANIFEST.json') == sha(MANIFEST), 'input manifest mismatch')
    require(sha(args.archive) == '208aabb30a764bad25ab1c99d74885bd405e84a13dcb8375622d66aa6203f70a',
            'linked library archive differs')
    by_case = {(row['density'], row['sector']): row for row in parent['cases']}
    require(len(parent['cases']) == len(by_case) == 15,
            'trace receipt is not exactly the 15-case matrix')
    output = {
        'schema': 'mhgp9_s2_segment_panel_v1',
        'scope': 'raw SemanticKITTI 08/000000, 1 mm, K5/s8, q3+q4, CPU sidecar',
        'trace_source_commit': parent['source_commit'],
        'hashes': dict(manifest=sha(MANIFEST), trace_receipt=sha(TRACE_RECEIPT),
                       sidecar_source=sha(HERE / 'measure.cpp'), archive=sha(args.archive),
                       binary=sha(args.binary)),
        'cases': [],
    }
    for density in DENSITIES:
        for sector in CASES:
            name = f'{density}/{sector}'
            meta = manifest['datasets'][density][sector]
            trace_meta = by_case[density, sector]
            require(meta['sites'] == trace_meta['sites'] and
                    meta['points_sha256'] == trace_meta['input_sha256'] and
                    meta['raw_return_ids_sha256'] == trace_meta['raw_ids_sha256'],
                    f'{name}: published input provenance differs')
            point_path = args.inputs / meta['points_file']
            raw_path = args.inputs / meta['raw_return_ids_file']
            require(sha(point_path) == meta['points_sha256'] and
                    sha(raw_path) == meta['raw_return_ids_sha256'], f'{name}: live input differs')
            case_root = args.traces / (sector if density == 'full' else f'{density}/{sector}')
            stdout = case_root / 'batch.stdout'
            require(sha(stdout) == trace_meta['batch_stdout_sha256'], f'{name}: batch stdout differs')
            batch = json.loads(stdout.read_text())
            require(batch['status'] == 'complete_relative' and
                    batch['tower_digest'] == trace_meta['tower_digest'] and
                    batch['ledger']['dead_core_loads'] == trace_meta['core_loads'] and
                    batch['ledger']['core_sites'] == trace_meta['core_forms'],
                    f'{name}: batch ledger differs')
            folder = case_root / 'trace'
            require(len(list(folder.glob('part_*.bin'))) == 8, f'{name}: trace part count differs')
            trace_edges = trace_forms = q3 = q4 = 0
            for part in trace_meta['trace_parts']:
                data = (folder / part['file']).read_bytes()
                require(len(data) == 16 * part['records'] and hashlib.sha256(data).hexdigest() == part['sha256'],
                        f'{name}: trace part {part["file"]} differs')
                for a, b, n, mask in REC.iter_unpack(data):
                    require(a < b and n >= 2 and mask in (2, 4, 6), f'{name}: invalid trace record')
                    trace_edges += 1
                    trace_forms += n
                    q3 += bool(mask & 2)
                    q4 += bool(mask & 4)
            require((trace_edges, trace_forms) == (trace_meta['core_loads'], trace_meta['core_forms']),
                    f'{name}: trace core totals differ')
            start = time.perf_counter()
            process = subprocess.run((str(args.binary), str(point_path), str(raw_path), str(folder)),
                                     check=True, capture_output=True, text=True)
            elapsed = time.perf_counter() - start
            require(not process.stderr, f'{name}: sidecar wrote stderr')
            measured = parse_measure(process.stdout)
            h = measured['head']
            l = batch['ledger']
            require((h['sites'], h['front_rectangles'], h['front_pair_mass'],
                     h['open_rectangles'], h['open_pair_mass'], h['rectangle_node_visits']) ==
                    (meta['sites'], l['q34_input_rectangles'], l['witness_input_pair_mass'],
                     l['q34_input_rectangles'] - l['witness_rejected_rectangles'],
                     l['expanded_pairs'], l['witness_rect_node_visits']),
                    f'{name}: five front/rectangle-filter counters differ')
            require((h['survivor_edges'], h['core_forms'], h['q3_survivors'], h['q4_survivors']) ==
                    (trace_edges, trace_forms, q3, q4), f'{name}: traced S2 survivors differ')
            ledger_names = ('q34_input_rectangles', 'witness_input_pair_mass',
                            'witness_rejected_rectangles', 'expanded_pairs',
                            'witness_rect_node_visits', 'dead_core_loads', 'core_sites')
            measured.update(density=density, sector=sector, input_sha256=meta['points_sha256'],
                            raw_ids_sha256=meta['raw_return_ids_sha256'],
                            batch_stdout_sha256=trace_meta['batch_stdout_sha256'],
                            batch_ledger={key: l[key] for key in ledger_names},
                            sidecar_stdout_sha256=hashlib.sha256(process.stdout.encode()).hexdigest(),
                            sidecar_elapsed_s=round(elapsed, 3))
            output['cases'].append(measured)
            threshold = [row for power, row in measured['histograms']['segment'].items()
                         if int(power) >= 4]
            threshold_forms = sum(row['core_forms'] for row in threshold)
            print(f'{name}: {h["survivor_edges"]:,} edges, {h["core_forms"]:,} forms, '
                  f'F(segment>=16)={threshold_forms / h["core_forms"]:.2%}, {elapsed:.1f} s', flush=True)
    args.out.write_text(json.dumps(output, indent=2, sort_keys=True) + '\n')
    print(f'wrote {args.out}', flush=True)


if __name__ == '__main__':
    main()
