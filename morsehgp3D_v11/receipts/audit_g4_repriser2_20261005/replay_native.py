from pathlib import Path
import argparse
import hashlib
import json
import re
import subprocess
import tarfile
import xml.etree.ElementTree as ET

SESSION = Path('/workspaces/.ehgp-sessions/v11.20261005.clauderepriser2')
PIN = '98a00955083d483306c4f92b9031e382e81b0e59'
REPO = Path('/workspaces/E-HGP')
parser = argparse.ArgumentParser(description='Read-only comparison of R2 supports_route archive to a frozen audit JSON.')
parser.add_argument('--check', type=Path, default=Path(__file__).with_name('r2_supports_route_review_20261005.json'))
OUT = parser.parse_args().check
BASE = 'results/cmd/000_matrice/files/matrix/'

def ensure(condition, message):
    if not condition:
        raise RuntimeError(message)

def sha(data):
    return hashlib.sha256(data).hexdigest()

def source(suffix):
    return subprocess.check_output(['git', 'show', PIN + ':morsehgp3D_v11/' + suffix], cwd=REPO)

cmake = source('tests/api/tests.cmake')
probe = source('tests/api/supports_route.cpp')
profiles = {}
current = None
for line in cmake.decode().splitlines():
    branch = re.match(r'(?:if|elseif)\(MHGP11_COORD_BITS EQUAL (18|21|24)\)', line)
    if branch:
        current = branch.group(1)
        profiles[current] = {}
    variable = re.search(r'set\((mhgp11_route_\w+) ([0-9a-f]{16})\)', line)
    if variable:
        ensure(current is not None, 'unscoped CMake fingerprint')
        profiles[current][variable.group(1)] = variable.group(2)
ensure(len(profiles) == 3 and all(len(v) == 12 for v in profiles.values()), '36 fingerprints not present')
rows_section = cmake.decode().split('foreach(case "scale8000;', 1)[1].split('\n  list(GET case', 1)[0]
rows_section = '"scale8000;' + rows_section
rows = re.findall(r'"([^"]+)"', rows_section)
ensure(len(rows) == 6, 'six route cases not found')
expected = {}
for bits, variables in profiles.items():
    expected[bits] = {}
    for row in rows:
        label, n, input_spec, counts, min_balls, min_cells = row.split(';')
        counts = re.sub(r'\$\{(mhgp11_route_\w+)\}', lambda m: variables[m.group(1)], counts)
        ensure('${' not in counts, 'unexpanded fingerprint')
        name = ('mhgp11_api_supports_route_lidar_' + input_spec.removeprefix('data=lidar_') + '_k5') if label == 'lidar' else ('mhgp11_api_supports_route_' + label)
        expected[bits][name] = {'line': 'supports_route_verdict conforme k=5 n=' + n + ' ' + counts + ' fils=1,4', 'min_balls': int(min_balls), 'min_cells': int(min_cells)}

archive = SESSION / 'results/results.tar.gz'
archive_bytes = archive.read_bytes()
archive_hash = sha(archive_bytes)
receipt_path = SESSION / 'receipt.json'
receipt = json.loads(receipt_path.read_text()) if receipt_path.exists() else None
closed = bool(receipt and (SESSION / 'DONE').exists() and receipt.get('status') == 'completed' and receipt.get('worker_outcome') == 'exited' and receipt.get('results_verified') is True and receipt.get('targeted_shutdown_certified') is True)
if receipt:
    ensure(receipt.get('commit') == PIN and receipt.get('worker_source') == 'commit:' + PIN, 'receipt source mismatch')
    ensure(receipt.get('results_sha256') == archive_hash, 'archive/receipt SHA mismatch')

review = {
    'schema': 'ehgp.v11.audit.r2_supports_route_review.v1',
    'source_pin': PIN,
    'archive': {'session': SESSION.name, 'sha256': archive_hash, 'bytes': len(archive_bytes)},
    'closure_status': 'closed_certified' if closed else 'preliminary_not_promoted',
    'method': 'Read-only Git/stdlib archive comparison, no native/build/cloud execution by auditor.',
    'sources': {
        'morsehgp3D_v11/tests/api/tests.cmake': {'sha256': sha(cmake), 'anchors': [[68, 107], [108, 129]]},
        'morsehgp3D_v11/tests/api/supports_route.cpp': {'sha256': sha(probe), 'anchors': [[235, 253], [261, 265], [343, 384], [386, 400]]}
    },
    'fingerprint_definitions': profiles,
    'source_contract': [
        'Both routes publish and finish a fresh Session; same() requires byte-identical nonempty support file and manifest, equal complete journal register and equal node/ball/support counts.',
        'At W1 and W4, routes must agree; FULL output/register must also agree between workers.',
        'Public compute at W1 must succeed, publish the same file and manifest as FULL, and have tree peak equal to FULL and distinct from order_tree.',
        'Expected code zero, six full golden verdicts per coordinate domain, common counts/journal and W1,W4, profile-specific file/manifest prefixes, and unchanged ball/cell floors.'
    ],
    'configurations': {}, 'route_occurrences': [], 'important_findings': []
}
seen = {}
markers = ['ERROR: AddressSanitizer', 'SUMMARY: AddressSanitizer', 'AddressSanitizer:DEADLYSIGNAL', 'runtime error:', 'UndefinedBehaviorSanitizer', 'WARNING: ThreadSanitizer', 'SUMMARY: ThreadSanitizer', 'FATAL: ThreadSanitizer', 'ERROR: LeakSanitizer']
with tarfile.open(archive) as tar:
    config_names = sorted(n.removeprefix(BASE).removesuffix('/result.json') for n in tar.getnames() if n.startswith(BASE) and n.endswith('/result.json'))
    ensure(len(config_names) == 8, 'eight configurations absent')
    for config in config_names:
        prefix = BASE + config + '/'
        get = lambda suffix: tar.extractfile(prefix + suffix).read()
        result = json.loads(get('result.json'))
        bp = json.loads(get('build_provenance.json'))
        ensure(result['conforming'] and result['status'] == 'ok' and bp['complete'] and not bp['errors'], 'configuration incomplete ' + config)
        flags = {}
        for f in bp['files']:
            if f['path'] in ['CMakeFiles/mhgp11.dir/flags.make', 'CMakeFiles/mhgp11_api_supports_route_probe.dir/flags.make']:
                ensure(sha(f['text'].encode()) == f['sha256'], 'compiled flag hash mismatch')
                flags[f['path']] = {'sha256': f['sha256']}
                for key in ['CXX_DEFINES', 'CXX_FLAGS']:
                    match = re.search(r'^' + key + r' = (.*)$', f['text'], re.M)
                    ensure(match is not None, 'compile flags missing')
                    flags[f['path']][key] = match.group(1)
        ensure(len(flags) == 2, 'actual library/probe flags absent')
        lib_defines = flags['CMakeFiles/mhgp11.dir/flags.make']['CXX_DEFINES']
        bits = re.search(r'-DMHGP11_COORD_BITS=(18|21|24)(?:\s|$)', lib_defines).group(1)
        poison = '-DMHGP11_POISON' in lib_defines
        domain = 'u' + bits + ('_poison' if poison else '')
        ensure(bits == '21' or not poison, 'unexpected poison domain')
        ensure('-DMHGP11_COORD_BITS=' + bits in flags['CMakeFiles/mhgp11_api_supports_route_probe.dir/flags.make']['CXX_DEFINES'], 'library/probe domain differs')
        inventory = json.loads(get('tests.json'))
        inv_names = [x['name'] for x in inventory]
        junit = ET.fromstring(get('junit.xml'))
        cases = list(junit.iter('testcase'))
        by_name = {x.get('name'): x for x in cases}
        ctest = get('ctest.log').decode()
        ctest_names = re.findall(r'\d+/\d+\s+Test\s+#\s*\d+:\s+(\S+)\s+\.+\s+Passed\s+[0-9.]+\s+sec', ctest)
        ensure(len(by_name) == len(cases) == len(inv_names) == len(set(inv_names)) == len(ctest_names) == len(set(ctest_names)), 'duplicate/missing names')
        ensure(set(inv_names) == set(by_name) == set(ctest_names), 'inventory/JUnit/CTest disagreement')
        route_names = sorted(n for n in inv_names if n in expected[bits])
        last_raw = get('LastTest.log')
        last = last_raw.decode()
        sections = {match.group(1): match.group(2) for match in re.finditer(r'^\d+/\d+ Testing: (\S+)\n(.*?)(?=^\d+/\d+ Testing: |\Z)', last, re.M | re.S)}
        ensure(len(route_names) == 3, 'three routes absent from split configuration')
        runtime_hits = {m: last.count(m) for m in markers if m in last}
        ensure(not runtime_hits, 'runtime diagnostic marker found ' + config)
        review['configurations'][config] = {'domain': domain, 'counts': result['tests'], 'actual_compile_flags': flags, 'route_names': route_names, 'inventory_junit_ctest_sets_equal': True, 'lasttest_sha256': sha(last_raw), 'lasttest_bytes': len(last_raw), 'runtime_diagnostic_marker_hits': runtime_hits}
        for name in route_names:
            case = by_name[name]
            ensure(case.get('status') == 'run' and not any(case.find(x) is not None for x in ['failure', 'error', 'skipped']), 'non-pass route ' + name)
            block = sections[name]
            golden = expected[bits][name]
            actual = [line for line in block.splitlines() if line.startswith('supports_route_verdict conforme ') and line.endswith(' fils=1,4')]
            ensure(len(actual) == 1 and actual[0] == golden['line'], 'native complete golden differs ' + domain + '/' + name)
            ensure('run_expect_verdict conforme' in block and 'Test Passed.' in block, 'successful wrapper verdict absent')
            ensure(not any(x in block for x in ['ECART', 'PLANCHER', 'supports_route_verdict refus', 'run_expect_verdict ligne_absente']), 'route rejection/mismatch present')
            objects = [json.loads(line) for line in block.splitlines() if line.startswith('{"route":')]
            compute = [x for x in objects if x.get('route') == 'compute']
            routes = {(x['route'], x['workers']): x for x in objects if x.get('route') != 'compute'}
            ensure(len(compute) == 1 and set(routes) == {('order_tree', 1), ('order_tree', 4), ('full_tower', 1), ('full_tower', 4)}, 'five actual route/compute lines absent')
            c = compute[0]
            ensure(c['tree_peak'] == c['full_tower_peak'] == routes[('full_tower', 1)]['tree_peak'], 'compute/FULL peak differs')
            ensure(c['order_tree_peak'] == routes[('order_tree', 1)]['tree_peak'] and c['tree_peak'] != c['order_tree_peak'], 'compute does not distinguish order_tree')
            ensure(c['tree_peak'] > 0 and c['order_tree_peak'] > 0, 'empty peak witness')
            seen.setdefault(domain, []).append(name)
            review['route_occurrences'].append({'configuration': config, 'domain': domain, 'name': name, 'status': 'PASS', 'actual_complete_verdict': actual[0], 'golden_matches_profile_counts_journal_workers': True, 'floors': {'balls': golden['min_balls'], 'cells': golden['min_cells']}, 'compute_peak_witness': c, 'route_tree_peaks': {route + '_W' + str(workers): item['tree_peak'] for (route, workers), item in routes.items()}, 'identities_required_by_source_and_successful_gate': True, 'lasttest_block_sha256': sha(block.encode())})
ensure(set(seen) == {'u18', 'u21', 'u24', 'u21_poison'}, 'four domains absent')
ensure(all(len(names) == 6 and set(names) == set(expected[domain[1:3]]) for domain, names in seen.items()), 'six exact routes per domain absent')
ensure(len(review['route_occurrences']) == 24, '24 route occurrences absent')
ensure(sha(archive.read_bytes()) == archive_hash, 'archive changed during capture')
review['verdict'] = 'favorable_24_routes_closed' if closed else 'favorable_24_routes_preliminary_unclosed'
review['scope'] = {'route_occurrences': 24, 'domains': {k: len(v) for k, v in seen.items()}, 'golden_file_manifest_prefixes': 36, 'u18_first_G4_pairs_since_prior_capsule': 5, 'u24_replayed_pairs': 6, 'public_compute_FULL_peak_witnesses': 24, 'native_or_cloud_actions_by_auditor': False}
review['limitations'] = [
    'Scope is the 24 supports_route cases, K5, W1/W4, three exact uniform18 sizes and three complete fixed LiDAR inputs; no native input bytes copied.',
    'File/manifest byte identity and full journal-register identity are enforced by the pinned source and successful code-zero gate, not exposed as separate per-route hashes in measurement JSON.',
    'Only rounded-prefix golden file/manifest hashes (16 hexadecimal digits) are printed. The gate separately compares complete file/manifest bytes.',
    'Release and library poison are ordinary domains, not sanitizer qualification; no time-contract conclusion or statistical claim.',
    'Source correction and these new actual G4 verdicts are kept distinct from historical failed verdicts and from independent mutant/long/points/plat lots.'
]
generated = (json.dumps(review, ensure_ascii=False, indent=2) + '\n').encode()
ensure(OUT.read_bytes() == generated, 'frozen JSON differs from independently reconstructed archive/source review')
print(json.dumps({'verdict': 'PASS_read_only', 'json_sha256': sha(generated), 'bytes': len(generated), 'closure_status': review['closure_status'], 'scope': review['scope']}, ensure_ascii=False))
