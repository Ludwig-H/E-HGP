#!/usr/bin/env python3
"""Static consistency check for the 15-case S2 segment receipt."""

import hashlib
import json
from pathlib import Path

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[2]
MANIFEST = REPO / 'morsehgp3D_v9/audits/lidar_raw_physical_scaling_20260923/MANIFEST.json'
TRACE_RECEIPT = REPO / 'morsehgp3D_v9/audits/edge_matched_core_20260923/RESULTS.json'
SINGLE = REPO / 'morsehgp3D_v9/audits/s2_segment_mass_20260923/measure.stdout'
CASES = ('full', 'quarter_x_neg_y_neg', 'quarter_x_neg_y_nonneg',
         'quarter_x_nonneg_y_neg', 'quarter_x_nonneg_y_nonneg')
DENSITIES = ('quarter', 'half', 'full')


def require(ok, message):
    if not ok:
        raise ValueError(message)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def check_hist(case, label):
    h = case['head']
    product = case['histograms']['product']
    segment = case['histograms']['segment']
    require(product and segment, f'{label}: missing histogram')
    for kind, hist in (('product', product), ('segment', segment)):
        for power_str, row in hist.items():
            power = int(power_str)
            r, p, s, f = (row[k] for k in ('rectangles', 'product_pairs',
                                           'survivor_edges', 'core_forms'))
            require(r > 0 and 0 <= power < 32 and 0 <= s <= p and f >= 2 * s,
                    f'{label}: {kind} bucket invalid')
            if kind == 'product':
                require(r * (1 << power) <= p < r * (1 << (power + 1)),
                        f'{label}: product bucket bounds invalid')
            else:
                require(r * (1 << power) <= s < r * (1 << (power + 1)),
                        f'{label}: segment bucket bounds invalid')
    require(tuple(sum(row[key] for row in product.values()) for key in
                  ('rectangles', 'product_pairs', 'survivor_edges', 'core_forms')) ==
            tuple(h[key] for key in ('open_rectangles', 'open_pair_mass',
                                      'survivor_edges', 'core_forms')),
            f'{label}: product conservation invalid')
    require(tuple(sum(row[key] for row in segment.values()) for key in
                  ('rectangles', 'survivor_edges', 'core_forms')) ==
            tuple(h[key] for key in ('positive_segments', 'survivor_edges', 'core_forms')),
            f'{label}: segment conservation invalid')
    require(h['empty_open_rectangles'] + h['positive_segments'] == h['open_rectangles'],
            f'{label}: empty/positive conservation invalid')
    s16 = sum(row['survivor_edges'] for power, row in segment.items() if int(power) >= 4)
    require(0 <= h['segment16_q3'] <= s16 and 0 <= h['segment16_q4'] <= s16 and
            h['segment16_q3'] + h['segment16_q4'] >= s16,
            f'{label}: segment>=16 masks do not cover survivors')
    for key, field in (('large_product_forms', 'core_forms'),
                       ('large_product_edges', 'survivor_edges')):
        require(h[key] == sum(row[field] for power, row in product.items() if int(power) >= 10),
                f'{label}: {key} differs')
    require((h['singleton_rectangles'], h['singleton_survivors'], h['singleton_forms']) ==
            (product['0']['rectangles'], product['0']['survivor_edges'], product['0']['core_forms']),
            f'{label}: singleton subtotal differs')
    require(0 <= h['large_product_q3'] <= h['large_product_edges'] and
            0 <= h['large_product_q4'] <= h['large_product_edges'] and
            h['large_product_q3'] + h['large_product_q4'] >= h['large_product_edges'],
            f'{label}: large-product masks do not cover survivors')
    require(0 <= h['q3_survivors'] <= h['survivor_edges'] and
            0 <= h['q4_survivors'] <= h['survivor_edges'] and
            h['q3_survivors'] + h['q4_survivors'] >= h['survivor_edges'],
            f'{label}: all masks do not cover survivors')
    require([(row['numerator'], row['denominator']) for row in case['top']] ==
            [(1, 1000), (1, 100), (5, 100), (10, 100)], f'{label}: top fractions differ')
    for row in case['top']:
        require(row['segments'] == (h['positive_segments'] * row['numerator'] +
                                     row['denominator'] - 1) // row['denominator'] and
                row['core_forms'] <= h['core_forms'] and
                row['survivor_edges'] <= h['survivor_edges'],
                f'{label}: top prefix exceeds total')
    require([row['core_forms'] for row in case['top']] ==
            sorted(row['core_forms'] for row in case['top']),
            f'{label}: top forms not monotone')
    return s16, sum(row['core_forms'] for power, row in segment.items() if int(power) >= 4)


def main():
    panel = json.loads((HERE / 'PANEL.json').read_text())
    require(panel['schema'] == 'mhgp9_s2_segment_panel_v1' and
            panel['trace_source_commit'] == 'c265a5dae4dd92059fc78acc0a1d7f52de9c1435',
            'panel identity differs')
    h = panel['hashes']
    require(h['manifest'] == sha(MANIFEST) and h['trace_receipt'] == sha(TRACE_RECEIPT)
            and h['sidecar_source'] == sha(HERE / 'measure.cpp') and
            h['archive'] == '208aabb30a764bad25ab1c99d74885bd405e84a13dcb8375622d66aa6203f70a',
            'pinned source/receipt/archive SHA differs')
    original = json.loads(TRACE_RECEIPT.read_text())
    manifest = json.loads(MANIFEST.read_text())
    require(original['source_commit'] == panel['trace_source_commit'], 'trace commit differs')
    parent = {(x['density'], x['sector']): x for x in original['cases']}
    cases = {(x['density'], x['sector']): x for x in panel['cases']}
    expected = {(d, s) for d in DENSITIES for s in CASES}
    require(len(original['cases']) == len(panel['cases']) == len(expected),
            '15-case matrix has duplicate or missing rows')
    require(set(parent) == set(cases) == expected, '15-case matrix incomplete/duplicated')
    for density in DENSITIES:
        for sector in CASES:
            label = f'{density}/{sector}'
            case = cases[density, sector]
            meta = manifest['datasets'][density][sector]
            old = parent[density, sector]
            head = case['head']
            ledger = case['batch_ledger']
            require((case['input_sha256'], case['raw_ids_sha256'], case['batch_stdout_sha256']) ==
                    (meta['points_sha256'], meta['raw_return_ids_sha256'], old['batch_stdout_sha256']),
                    f'{label}: parent file SHA differs')
            require((head['sites'], head['survivor_edges'], head['core_forms']) ==
                    (meta['sites'], old['core_loads'], old['core_forms']),
                    f'{label}: source/core totals differ')
            require((head['front_rectangles'], head['front_pair_mass'],
                     head['open_rectangles'], head['open_pair_mass'], head['rectangle_node_visits']) ==
                    (ledger['q34_input_rectangles'], ledger['witness_input_pair_mass'],
                     ledger['q34_input_rectangles'] - ledger['witness_rejected_rectangles'],
                     ledger['expanded_pairs'], ledger['witness_rect_node_visits']),
                    f'{label}: five front/rectangle-filter counts differ')
            require((head['survivor_edges'], head['core_forms']) ==
                    (ledger['dead_core_loads'], ledger['core_sites']),
                    f'{label}: parent core ledger differs')
            require(sum(part['records'] for part in old['trace_parts']) == head['survivor_edges'],
                    f'{label}: trace records do not match edges')
            check_hist(case, label)
    # Independent earlier full-frame sidecar used a separately compiled source.
    first = SINGLE.read_text().splitlines()
    dense = cases['full', 'full']
    old_head = tuple(map(int, first[0].split()[1:]))
    keys = ('sites', 'front_rectangles', 'front_pair_mass', 'open_rectangles',
            'open_pair_mass', 'rectangle_node_visits', 'survivor_edges', 'core_forms',
            'q3_survivors', 'q4_survivors', 'empty_open_rectangles', 'positive_segments',
            'singleton_rectangles', 'singleton_survivors', 'singleton_forms',
            'large_product_forms', 'large_product_edges', 'large_product_q3',
            'large_product_q4')
    require(old_head == tuple(dense['head'][key] for key in keys),
            'dense full case differs from earlier independent sidecar')
    print('PASS: 15/15 trace, five front/filter counts, core ledgers, segment partitions; '
          'dense full agrees with first sidecar')


if __name__ == '__main__':
    main()
