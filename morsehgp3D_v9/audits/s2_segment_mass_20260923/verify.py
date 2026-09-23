#!/usr/bin/env python3
"""Read the pinned segment-mass receipt; optionally verify ephemeral inputs."""

import argparse
import hashlib
import json
from pathlib import Path

AUDIT = Path(__file__).resolve().parent
REPO = AUDIT.parents[2]


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def require(condition, message):
    if not condition:
        raise ValueError(message)


def rows_of_kind(rows, kind):
    result = {}
    for row in rows:
        words = row.split()
        if words[0] == kind:
            require(len(words) == 6, f'invalid {kind} row: {row}')
            key = int(words[1])
            require(key not in result, f'duplicate {kind} bucket {key}')
            result[key] = tuple(map(int, words[2:]))
    return result


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--points', type=Path)
    parser.add_argument('--raw-ids', type=Path)
    parser.add_argument('--trace', type=Path)
    parser.add_argument('--archive', type=Path)
    parser.add_argument('--binary', type=Path)
    args = parser.parse_args()
    provenance = json.loads((AUDIT / 'PROVENANCE.json').read_text())
    hashes = provenance['hashes']
    require(provenance['trace_source_commit'] == 'c265a5dae4dd92059fc78acc0a1d7f52de9c1435',
            'trace source commit differs')
    stable = {
        'measure_cpp_sha256': AUDIT / 'measure.cpp',
        'measure_stdout_sha256': AUDIT / 'measure.stdout',
        'edge_trace_receipt_sha256': REPO / 'morsehgp3D_v9/audits/edge_matched_core_20260923/RESULTS.json',
        'rectangle_histogram_receipt_sha256': REPO / 'morsehgp3D_v9/audits/q34_raw_rectangle_mass_20260923/k5.stdout',
        'input_manifest_sha256': REPO / 'morsehgp3D_v9/audits/lidar_raw_physical_scaling_20260923/MANIFEST.json',
    }
    for name, path in stable.items():
        require(digest(path) == hashes[name], f'{name}: hash mismatch')
    manifest = json.loads(stable['input_manifest_sha256'].read_text())
    full = manifest['datasets']['full']['full']
    require(full['sites'] == 123389 and full['points_sha256'] == hashes['input_points_sha256']
            and full['raw_return_ids_sha256'] == hashes['input_raw_ids_sha256'], 'input manifest differs')
    original = json.loads(stable['edge_trace_receipt_sha256'].read_text())
    matches = [case for case in original['cases'] if case['density'] == 'full' and case['sector'] == 'full']
    require(len(matches) == 1, 'not exactly one original full case')
    original_full = matches[0]
    require(original['source_commit'] == provenance['trace_source_commit']
            and original_full['sites'] == full['sites']
            and original_full['input_sha256'] == full['points_sha256']
            and original_full['raw_ids_sha256'] == full['raw_return_ids_sha256'],
            'original trace provenance differs')
    require(len(original_full['trace_parts']) == 8, 'trace part count differs')
    require(sum(p['records'] for p in original_full['trace_parts']) == original_full['core_loads'],
            'trace part records do not sum to core loads')
    historical = stable['rectangle_histogram_receipt_sha256'].read_text().splitlines()[0].split()
    require(historical[0] == 'sites' and historical[2] == 'K' and int(historical[1]) == 123389
            and int(historical[3]) == 5, 'rectangle histogram case differs')
    old = dict(zip(historical[::2], historical[1::2], strict=True))

    lines = (AUDIT / 'measure.stdout').read_text().splitlines()
    heads = [line for line in lines if line.startswith('head ')]
    require(len(heads) == 1, 'head row missing/duplicated')
    head = list(map(int, heads[0].split()[1:]))
    require(len(head) == 19, 'head column count differs')
    (n, front_rect, front_mass, open_rect, open_mass, visits, survivors,
     forms, q3, q4, empty, positive, singleton_rect, singleton_survivors,
     singleton_forms, large_forms, large_edges, large_q3, large_q4) = head
    require((n, front_rect, front_mass, open_rect, open_mass, visits) ==
            tuple(int(old[k]) for k in ('sites', 'front_rect', 'front_mass', 'open_rect',
                                        'open_mass', 'rect_visits')), 'five front/filter counters differ')
    require((survivors, forms) == (original_full['core_loads'], original_full['core_forms']),
            'core totals differ from original trace receipt')
    require(positive + empty == open_rect and q3 <= survivors and q4 <= survivors
            and large_q3 <= q3 and large_q4 <= q4 and large_edges <= survivors,
            'head conservation failed')
    product = rows_of_kind(lines, 'product')
    segment = rows_of_kind(lines, 'segment')
    require(product and segment, 'histogram missing')
    for kind, table in (('product', product), ('segment', segment)):
        for power, (rects, pairs, edges, subtotal) in table.items():
            require(rects > 0 and 0 <= power < 32 and 0 <= edges <= pairs and subtotal >= 2 * edges,
                    f'{kind} bucket invariant failed')
            if kind == 'product':
                require(rects * (1 << power) <= pairs < rects * (1 << (power + 1)),
                        'product bucket bounds failed')
            else:
                require(rects * (1 << power) <= edges < rects * (1 << (power + 1)),
                        'segment bucket bounds failed')
    require(tuple(sum(row[col] for row in product.values()) for col in range(4)) ==
            (open_rect, open_mass, survivors, forms), 'product histogram does not partition open work')
    require(sum(row[0] for row in segment.values()) == positive
            and sum(row[2] for row in segment.values()) == survivors
            and sum(row[3] for row in segment.values()) == forms,
            'segment histogram does not partition positive work')
    require(product[0] == (singleton_rect, singleton_rect, singleton_survivors, singleton_forms),
            'singleton row differs')
    require(sum(row[2] for power, row in product.items() if power >= 10) == large_edges
            and sum(row[3] for power, row in product.items() if power >= 10) == large_forms,
            'large-product subtotal differs')
    require(q3 + q4 >= survivors and large_q3 + large_q4 >= large_edges,
            'lane mask union is incomplete')
    tops = []
    for row in lines:
        if row.startswith('top '):
            words = row.split()
            require(len(words) == 7, 'top row shape differs')
            num, den, take, amount, edges, pairs = map(int, words[1:])
            require(take == (positive * num + den - 1) // den and amount <= forms
                    and edges <= survivors and pairs <= open_mass, 'top row invalid')
            tops.append((num / den, amount))
    require(len(tops) == 4 and [x[0] for x in tops] == [.001, .01, .05, .1]
            and [x[1] for x in tops] == sorted(x[1] for x in tops), 'top prefix order differs')

    live = (args.points, args.raw_ids, args.trace, args.archive, args.binary)
    require(all(x is None for x in live) or all(x is not None for x in live),
            'give all five live paths or none')
    if args.points is not None:
        for name, path in (('input_points_sha256', args.points),
                           ('input_raw_ids_sha256', args.raw_ids),
                           ('linked_archive_sha256', args.archive),
                           ('compiled_binary_sha256', args.binary)):
            require(digest(path) == hashes[name], f'{name}: live hash mismatch')
        for part in original_full['trace_parts']:
            path = args.trace / part['file']
            require(path.stat().st_size == 16 * part['records'] and digest(path) == part['sha256'],
                    f'trace part {part["file"]}: live mismatch')
    print(f'PASS: {survivors:,} joined S2 survivors, {forms:,} core forms, '
          f'{open_rect:,} open rectangles, {positive:,} positive segments')


if __name__ == '__main__':
    main()
