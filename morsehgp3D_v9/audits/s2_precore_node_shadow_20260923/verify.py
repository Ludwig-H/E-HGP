#!/usr/bin/env python3
"""Static exact proof/ledger reader; --capture also checks the live S2 bytes."""

import argparse
import hashlib
import json
import struct
from pathlib import Path

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[2]
PANEL = REPO / 'morsehgp3D_v9/audits/s2_segment_panel_20260923/PANEL.json'
TRACE_PARENT = REPO / 'morsehgp3D_v9/audits/edge_matched_core_20260923/RESULTS.json'
INPUT_PARENT = REPO / 'morsehgp3D_v9/audits/lidar_raw_physical_scaling_20260923/MANIFEST.json'
DEFAULT_RECEIPT = HERE / 'RECEIPT.json'
CASES = {
    'full_near64': ('full', 'full', 64, 1, 1),
    'full_near256': ('full', 'full', 256, 1, 1),
    'sparse_pre64': ('quarter', 'quarter_x_nonneg_y_nonneg', 64, 0, 0),
    'sparse_near64': ('quarter', 'quarter_x_nonneg_y_nonneg', 64, 1, 0),
    'sparse_near4096': ('quarter', 'quarter_x_nonneg_y_nonneg', 4096, 1, 0),
}
HEAD = ('sites', 'front_rectangles', 'front_pair_mass', 'open_rectangles',
        'open_pair_mass', 'rectangle_node_visits', 'survivor_edges', 'core_forms',
        'q3_survivors', 'q4_survivors', 'empty_open_rectangles',
        'positive_segments', 'segment16_q3', 'segment16_q4')
SHADOW = ('segments', 'edges', 'forms', 'closed_segments', 'closed_edges',
          'closed_forms', 'proved_cells', 'budget_cells', 'exhausted_cells',
          'q_terms', 'node_visits', 'node_tests', 'vertex_tests',
          'excluded_node_visits', 'excluded_subtree_skips', 'credited_nodes',
          'credited_sites', 'obstructed_cells', 'obstructed_segments',
          'obstructed_forms', 'post_zero_edges', 'post_nonzero_edges',
          'no_post_edges', 'shadow_ns', 'total_ns')


def check(ok, message):
    if not ok:
        raise ValueError(message)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def parse_output(path):
    sections = {}
    cells = {}
    anomalies = {}
    proofs = {}
    for line in path.read_text().splitlines():
        words = line.split()
        check(words, f'{path.name}: empty line')
        tag = words[0]
        if tag in ('config', 'head', 'shadow'):
            check(tag not in sections, f'{path.name}: duplicate {tag}')
            sections[tag] = list(map(int, words[1:]))
        elif tag == 'cells':
            check(len(words) == 4, f'{path.name}: malformed cells row')
            c, count, forms = map(int, words[1:])
            check(c not in cells and 0 <= c <= 8, f'{path.name}: duplicate cell bucket')
            cells[c] = (count, forms)
        elif tag == 'anomaly':
            check(len(words) == 6, f'{path.name}: malformed anomaly')
            a, b, pre, post, forms = map(int, words[1:])
            key = tuple(sorted((a, b)))
            check(a != b and key not in anomalies and pre in (2, 4, 6) and
                  post in (2, 4, 6) and post & ~pre == 0 and forms >= 2,
                  f'{path.name}: invalid anomaly')
            anomalies[key] = dict(pre=pre, post=post, forms=forms)
        elif tag == 'proof':
            check(len(words) == 14, f'{path.name}: malformed proof')
            a, b, cell, *rest = map(int, words[1:])
            key = (tuple(sorted((a, b))), cell)
            check(key not in proofs and 0 <= cell < 8 and len(rest) == 10,
                  f'{path.name}: duplicate/invalid proof')
            proofs[key] = dict(low=rest[:3], high=rest[3:6], guards=rest[6:])
        else:
            raise ValueError(f'{path.name}: unknown line {tag}')
    check(set(sections) == {'config', 'head', 'shadow'} and
          len(sections['config']) == 3 and len(sections['head']) == len(HEAD) and
          len(sections['shadow']) == len(SHADOW) and set(cells) == set(range(9)),
          f'{path.name}: incomplete main sections')
    result = dict(config=sections['config'], head=dict(zip(HEAD, sections['head'], strict=True)),
                  shadow=dict(zip(SHADOW, sections['shadow'], strict=True)),
                  cells=cells, anomalies=anomalies, proofs=proofs)
    s = result['shadow']
    check(sum(n for n, _ in cells.values()) == s['segments'] and
          sum(f for _, f in cells.values()) == s['forms'] and
          cells[8] == (s['closed_segments'], s['closed_forms']) and
          s['proved_cells'] + s['budget_cells'] + s['exhausted_cells'] == 8 * s['segments'] and
          s['q_terms'] == 27 * s['edges'] and
          s['post_zero_edges'] + s['post_nonzero_edges'] + s['no_post_edges'] == s['closed_edges'] and
          len(anomalies) == s['post_nonzero_edges'] and
          len(proofs) == 8 * len(anomalies) and
          s['node_visits'] <= result['config'][0] * 8 * s['segments'] and
          s['closed_forms'] <= s['forms'] and s['closed_edges'] <= s['edges'],
          f'{path.name}: shadow conservation failed')
    return result


def expected_case(name, panel):
    density, sector, budget, near, post = CASES[name]
    row = next(x for x in panel['cases'] if x['density'] == density and x['sector'] == sector)
    hist = row['histograms']['segment']
    s16 = {field: sum(b[field] for power, b in hist.items() if int(power) >= 4)
           for field in ('rectangles', 'survivor_edges', 'core_forms')}
    return row, s16, [budget, near, post]


def check_case(name, parsed, panel):
    row, s16, config = expected_case(name, panel)
    h, s = parsed['head'], parsed['shadow']
    check(parsed['config'] == config, f'{name}: configuration changed')
    check(all(h[field] == row['head'][field] for field in HEAD),
          f'{name}: front/filter/S2 counts differ from panel')
    check((s['segments'], s['edges'], s['forms']) ==
          (s16['rectangles'], s16['survivor_edges'], s16['core_forms']),
          f'{name}: segment>=16 work differs from panel')
    check(s['obstructed_cells'] <= 8 * s['obstructed_segments'] and
          s['obstructed_segments'] <= s['segments'] and
          s['obstructed_forms'] <= s['forms'], f'{name}: obstruction counts invalid')
    check((s['no_post_edges'] == 0) == (config[2] == 1) or s['closed_edges'] == 0,
          f'{name}: post-flag/closed-edge mismatch')


def squared_at4(point, vertex4):
    return sum((4 * point[i] - vertex4[i]) ** 2 for i in range(3))


def radius(d):
    lo, hi = 0, 262144
    while hi - lo > 1:
        mid = (lo + hi) // 2
        if 8 * mid * mid >= d:
            hi = mid
        else:
            lo = mid
    return hi


def check_proofs(parsed, point_map, cloud_box, name):
    for key, row in parsed['anomalies'].items():
        a, b = (point_map[str(raw)] for raw in key)
        grid = [set() for _ in range(3)]
        for cell in range(8):
            p = parsed['proofs'][(key, cell)]
            low, high = p['low'], p['high']
            check(all(low[i] <= high[i] for i in range(3)) and
                  len(set(p['guards'])) == 4 and not set(p['guards']) & set(key),
                  f'{name}: malformed cell/guard set {key}/{cell}')
            for i in range(3):
                grid[i].add(low[i]); grid[i].add(high[i])
            for raw in p['guards']:
                check(str(raw) in point_map, f'{name}: missing guard coordinate {raw}')
                g = point_map[str(raw)]
                for x in (low[0], high[0]):
                    for y in (low[1], high[1]):
                        for z in (low[2], high[2]):
                            v = (x, y, z)
                            check(2 * squared_at4(g, v) < squared_at4(a, v) + squared_at4(b, v),
                                  f'{name}: non-strict guard inequality {key}/{cell}/{raw}')
        axis = [sorted(values) for values in grid]
        check(all(len(x) == 3 and 2 * x[1] == x[0] + x[2] for x in axis),
              f'{name}: cells do not form a dyadic 2x2x2 grid')
        for cell in range(8):
            x, y, z = (cell >> 2) & 1, (cell >> 1) & 1, cell & 1
            p = parsed['proofs'][(key, cell)]
            check(p['low'] == [axis[0][x], axis[1][y], axis[2][z]] and
                  p['high'] == [axis[0][x + 1], axis[1][y + 1], axis[2][z + 1]],
                  f'{name}: cell cover has gap or overlap mismatch')
        d = sum((a[i] - b[i]) ** 2 for i in range(3))
        r = radius(d)
        for i in range(3):
            mid4 = 2 * (a[i] + b[i])
            clipped_low = max(mid4 - 4 * r, 4 * cloud_box['low'][i])
            clipped_high = min(mid4 + 4 * r, 4 * cloud_box['high'][i])
            check(axis[i][0] <= clipped_low <= clipped_high <= axis[i][2],
                  f'{name}: nominal q4 disk not covered by the grid')
        check(row['pre'] in (2, 4, 6) and row['post'] & ~row['pre'] == 0,
              f'{name}: invalid post mask')


def parse_exact(path):
    rows = {}
    total = None
    for line in path.read_text().splitlines():
        words = line.split()
        if words[0] == 'edge':
            check(len(words) == 7, 'bad exact edge row')
            a, b, sites, visits, q3, q4 = map(int, words[1:])
            check((a, b) not in rows and a < b and sites >= 2 and visits > 0 and q3 == q4 == 0,
                  'invalid exact edge result')
            rows[a, b] = (sites, visits)
        elif words[0] == 'total':
            check(total is None and len(words) == 6, 'bad exact total')
            total = list(map(int, words[1:]))
        else:
            raise ValueError('unknown exact edge row')
    check(total is not None and total == [len(rows), sum(x[0] for x in rows.values()),
                                            sum(x[1] for x in rows.values()), 0, 0],
          'exact edge totals differ')
    return rows


def live_hashes(args, panel, trace_parent, input_parent):
    check(args.inputs and args.traces and args.after_traces and args.archive and
          args.shadow_binary and args.exact_binary,
          'capture needs inputs, both trace roots, archive and both binaries')
    check(sha(args.inputs / 'MANIFEST.json') == sha(INPUT_PARENT), 'input manifest differs')
    check(sha(args.archive) == '208aabb30a764bad25ab1c99d74885bd405e84a13dcb8375622d66aa6203f70a',
          'linked library differs')
    metadata = {}
    for density, sector in {CASE[0:2] for CASE in CASES.values()}:
        name = f'{density}/{sector}'
        input_row = input_parent['datasets'][density][sector]
        trace_row = next(x for x in trace_parent['cases'] if x['density'] == density and x['sector'] == sector)
        point_path = args.inputs / input_row['points_file']
        raw_path = args.inputs / input_row['raw_return_ids_file']
        check(sha(point_path) == input_row['points_sha256'] == trace_row['input_sha256'] and
              sha(raw_path) == input_row['raw_return_ids_sha256'] == trace_row['raw_ids_sha256'],
              f'{name}: input mismatch')
        trace_root = args.traces / (sector if density == 'full' else f'{density}/{sector}') / 'trace'
        for part in trace_row['trace_parts']:
            path = trace_root / part['file']
            check(sha(path) == part['sha256'] and path.stat().st_size == 16 * part['records'],
                  f'{name}: trace differs')
        metadata[name] = dict(points=input_row['points_sha256'], raw_ids=input_row['raw_return_ids_sha256'],
                              trace_parts=[x['sha256'] for x in trace_row['trace_parts']])
    post = trace_parent['post_full']
    for part in post['trace_parts']:
        path = args.after_traces / 'full/trace' / part['file']
        check(sha(path) == part['sha256'] and path.stat().st_size == 16 * part['records'],
              'post trace differs')
    metadata['post_full'] = [x['sha256'] for x in post['trace_parts']]
    return metadata


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--capture', action='store_true')
    ap.add_argument('--inputs', type=Path)
    ap.add_argument('--traces', type=Path)
    ap.add_argument('--after-traces', type=Path)
    ap.add_argument('--archive', type=Path)
    ap.add_argument('--shadow-binary', type=Path)
    ap.add_argument('--exact-binary', type=Path)
    ap.add_argument('--outputs', type=Path, default=HERE)
    ap.add_argument('--receipt', type=Path, default=DEFAULT_RECEIPT)
    args = ap.parse_args()
    panel = json.loads(PANEL.read_text())
    trace_parent = json.loads(TRACE_PARENT.read_text())
    input_parent = json.loads(INPUT_PARENT.read_text())
    parsed = {}
    for name in CASES:
        parsed[name] = parse_output(args.outputs / f'{name}.stdout')
        check_case(name, parsed[name], panel)
    exact = parse_exact(args.outputs / 'exact_edges.stdout')
    anomalies = set().union(*(set(x['anomalies']) for x in parsed.values()))
    check(set(exact) == anomalies, 'exact edge gate does not cover anomaly union')
    if args.capture:
        check(not args.receipt.exists(), 'refusing to overwrite receipt')
        live = live_hashes(args, panel, trace_parent, input_parent)
        point_file = args.inputs / input_parent['datasets']['full']['full']['points_file']
        raw_file = args.inputs / input_parent['datasets']['full']['full']['raw_return_ids_file']
        point_bytes, raw_bytes = point_file.read_bytes(), raw_file.read_bytes()
        check(len(point_bytes) % 12 == 0 and len(raw_bytes) * 3 == len(point_bytes),
              'full point/raw map alignment')
        point_map = {str(raw): list(p) for (raw,), p in zip(struct.iter_unpack('<I', raw_bytes),
                                                            struct.iter_unpack('<III', point_bytes), strict=True)}
        check(len(point_map) == len(point_bytes) // 12, 'duplicate raw IDs')
        needed = set()
        for item in parsed.values():
            for key in item['anomalies']:
                needed.update(key)
            for p in item['proofs'].values():
                needed.update(p['guards'])
        check(all(str(raw) in point_map for raw in needed), 'proof ID outside full input')
        points = {str(raw): point_map[str(raw)] for raw in sorted(needed)}
        coords = list(struct.iter_unpack('<III', point_bytes))
        cloud_box = dict(low=[min(p[i] for p in coords) for i in range(3)],
                         high=[max(p[i] for p in coords) for i in range(3)])
        receipt = dict(schema='mhgp9_s2_precore_node_shadow_v1',
                       trace_source_commit=trace_parent['source_commit'],
                       scope='08/000000 raw 1mm u18 K5/s8 CPU shadow',
                       reference_sha256=dict(panel=sha(PANEL), trace_receipt=sha(TRACE_PARENT),
                                             input_manifest=sha(INPUT_PARENT)),
                       source_sha256=dict(measure=sha(HERE / 'measure.cpp'),
                                          exact_gate=sha(HERE / 'check_exact_edges.cpp')),
                       binaries_sha256=dict(shadow=sha(args.shadow_binary), exact=sha(args.exact_binary),
                                            archive=sha(args.archive)),
                       live_sha256=live,
                       outputs_sha256={file.name: sha(file) for file in args.outputs.glob('*.stdout')},
                       cloud_box=cloud_box, proof_points=points)
        for name, item in parsed.items():
            check_proofs(item, points, cloud_box, name)
        args.receipt.write_text(json.dumps(receipt, indent=2, sort_keys=True) + '\n')
    else:
        receipt = json.loads(args.receipt.read_text())
        check(receipt['schema'] == 'mhgp9_s2_precore_node_shadow_v1' and
              receipt['trace_source_commit'] == trace_parent['source_commit'] and
              receipt['reference_sha256'] ==
              dict(panel=sha(PANEL), trace_receipt=sha(TRACE_PARENT), input_manifest=sha(INPUT_PARENT)) and
              receipt['source_sha256'] == dict(measure=sha(HERE / 'measure.cpp'),
                                                exact_gate=sha(HERE / 'check_exact_edges.cpp')),
              'static receipt provenance differs')
        for file, expected in receipt['outputs_sha256'].items():
            check(sha(args.outputs / file) == expected, f'{file}: output hash differs')
        for name, item in parsed.items():
            check_proofs(item, receipt['proof_points'], receipt['cloud_box'], name)
    print(json.dumps({name: {key: parsed[name]['shadow'][key] for key in
                             ('segments', 'closed_segments', 'closed_edges', 'closed_forms',
                              'proved_cells', 'node_visits', 'q_terms', 'post_nonzero_edges')}
                      for name in CASES}, sort_keys=True))
    print(f'PASS {len(anomalies)} distinct post-core-open edges: 8 cells x 4 exact guards each; '
          f'exact edge path q3=q4=0 on all {len(exact)}')


if __name__ == '__main__':
    main()
