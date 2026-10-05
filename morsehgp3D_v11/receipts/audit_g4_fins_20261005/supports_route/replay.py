#!/usr/bin/env python3
"""Read-only replay of six archived ASan/u24 failures; no native or cloud invocation."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import tarfile
ROOT = Path(__file__).resolve().parent
parser = argparse.ArgumentParser()
parser.add_argument('--archive', type=Path)
args = parser.parse_args()
manifest = json.loads((ROOT / 'source_manifest.json').read_text())
for name, meta in manifest['files'].items():
    if hashlib.sha256((ROOT / 'sources' / name).read_bytes()).hexdigest() != meta['sha256']:
        raise SystemExit('source SHA mismatch: ' + name)
def source(name):
    return (ROOT / 'sources' / 'morsehgp3D_v11' / name).read_text()
probe = source('tests/api/supports_route.cpp')
judge = source('cmake/run_expect.cmake')
if probe.index('if (!agree) return kMismatch;') > probe.index('std::printf("supports_route_verdict conforme'):
    raise SystemExit('probe guard no longer precedes success verdict')
if judge.index('if(NOT "${rc}" STREQUAL "${EXPECTED}")') > judge.index('run_expect_say("run_expect_verdict ligne_absente")'):
    raise SystemExit('exact code no longer checked before LINE')
cmake = source('tests/api/tests.cmake')
body = cmake[cmake.index('foreach(case "scale8000;'):].split('list(GET case 0 label)')[0]
expected = {}
for entry in re.findall(r'"([^"\n]+;[^"\n]+)"', body):
    label, n, request, counts, balls, cells = entry.split(';')
    suffix = 'lidar_' + request.split('data=lidar_')[1] + '_k5' if label == 'lidar' else label
    expected['mhgp11_api_supports_route_' + suffix] = 'supports_route_verdict conforme k=5 n=' + n + ' ' + counts + ' fils=1,4'
def tokens(line):
    return dict(re.findall(r'([a-z0-9_]+)=([^ ]+)', line))
logs = json.loads((ROOT / 'log_manifest.json').read_text())
rows = []
for case in logs['cases']:
    data = (ROOT / case['excerpt']).read_bytes()
    if hashlib.sha256(data).hexdigest() != case['excerpt_sha256']:
        raise SystemExit('excerpt SHA mismatch')
    text = data.decode()
    actual = re.findall(r'(?m)^supports_route_verdict conforme .+$', text)
    verdicts = re.findall(r'(?m)^run_expect_verdict ([a-z_]+)$', text)
    if len(actual) != 1 or verdicts != ['ligne_absente'] or '"-DEXPECTED=0"' not in text or 'Test Failed.' not in text:
        raise SystemExit('unexpected native/wrapper status: ' + case['test'])
    if any(mark in text for mark in ['ECART W', 'PLANCHER', 'supports_route_verdict refus']):
        raise SystemExit('extra runtime mismatch/refusal')
    want, got = tokens(expected[case['test']]), tokens(actual[0])
    differences = sorted(k for k in set(want) | set(got) if want.get(k) != got.get(k))
    if differences != ['fichier', 'manifeste']:
        raise SystemExit('differences beyond the two profile hashes')
    json_rows = [json.loads(line) for line in text.splitlines() if line.startswith('{"route":')]
    by_route = {(row['route'], row.get('workers')): row for row in json_rows}
    if set(by_route) != {('compute', None), ('order_tree', 1), ('order_tree', 4), ('full_tower', 1), ('full_tower', 4)}:
        raise SystemExit('missing/extra route diagnostics')
    compute = by_route[('compute', None)]
    order_peak = by_route[('order_tree', 1)]['tree_peak']
    full_peak = by_route[('full_tower', 1)]['tree_peak']
    if not (compute['tree_peak'] == compute['full_tower_peak'] == full_peak and compute['order_tree_peak'] == order_peak and full_peak != order_peak):
        raise SystemExit('public/full/order peak mismatch')
    rows.append(dict(test=case['test'], profile=case['profile'], native_code=0, wrapper_verdict=verdicts[0],
        only_differences=differences, observed_u24_hashes={k: got[k] for k in differences},
        expected_u21_hashes={k: want[k] for k in differences},
        W1_tree_peaks=dict(compute=compute['tree_peak'], full_tower=full_peak, order_tree=order_peak),
        native_routes_and_public_api_conform=True))
if len(rows) != 6:
    raise SystemExit('six cases required')
scan = json.loads((ROOT / 'sanitizer_scan.json').read_text())
if len(scan['logs']) != 45 or any(entry['hits'] for entry in scan['logs']):
    raise SystemExit('archived sanitizer scan differs')
if args.archive:
    data = args.archive.read_bytes()
    if hashlib.sha256(data).hexdigest() != scan['archive_sha256']:
        raise SystemExit('archive SHA mismatch')
    with tarfile.open(args.archive, 'r:gz') as t:
        for entry in scan['logs']:
            data = t.extractfile(entry['member']).read()
            if hashlib.sha256(data).hexdigest() != entry['sha256']:
                raise SystemExit('runtime log SHA mismatch')
            if any(marker in data.decode(errors='replace') for marker in scan['markers']):
                raise SystemExit('runtime sanitizer marker found')
    archive_replayed = True
else:
    archive_replayed = False
scope = logs['profile_results']
sums = {}
for family in ('gcc_asan_', 'gcc_tsan_'):
    selected = [v['tests'] for name, v in scope.items() if name.startswith(family)]
    sums[family] = {key: sum(row[key] for row in selected) for key in ['selected', 'passed', 'failed', 'not_run']}
print(json.dumps(dict(pin=manifest['pin'], cases=rows, CTest_scope=sums,
    archived_runtime_logs_scanned=len(scan['logs']), sanitizer_markers_observed=0,
    full_archive_scan_replayed=archive_replayed,
    verdict='all_six_ASan_u24_failures_are_expected_LINE_hash_mismatches_after_native_route_conformance',
    CTest_failures_reclassified_as_passed=False, engine_defect_established=False), sort_keys=True, indent=2))
