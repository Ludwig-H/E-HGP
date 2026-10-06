from pathlib import Path
import argparse
import hashlib
import json
import re
import shlex
import subprocess
import tarfile
import xml.etree.ElementTree as ET

PIN = '98a00955083d483306c4f92b9031e382e81b0e59'
REPO = Path('/workspaces/E-HGP')
SESSION = Path('/workspaces/.ehgp-sessions/v11.20261005.clauderepriser4')
BASE = 'results/cmd/000_matrice/files/matrix/'

def ensure(condition, message):
    if not condition:
        raise RuntimeError(message)

def sha(data):
    return hashlib.sha256(data).hexdigest()

def review():
    receipt = json.loads((SESSION / 'receipt.json').read_text())
    ensure((SESSION / 'DONE').exists() and receipt.get('status') == 'completed' and receipt.get('worker_outcome') == 'exited' and receipt.get('results_verified') is True and receipt.get('targeted_shutdown_certified') is True, 'R4 not closed/certified')
    ensure(receipt['commit'] == PIN and receipt['worker_source'] == 'commit:' + PIN, 'receipt source mismatch')
    archive = SESSION / 'results/results.tar.gz'
    archive_bytes = archive.read_bytes()
    ensure(sha(archive_bytes) == receipt['results_sha256'], 'archive SHA mismatch')
    get_source = lambda p: subprocess.check_output(['git', 'show', PIN + ':morsehgp3D_v11/' + p], cwd=REPO)
    cmake = get_source('tests/api/tests.cmake')
    probe = get_source('tests/api/supports_route.cpp')
    text = cmake.decode()
    branch = text.split('elseif(MHGP11_COORD_BITS EQUAL 24)', 1)[1].split('endif()', 1)[0]
    prefixes = dict(re.findall(r'set\((mhgp11_route_\w+) ([0-9a-f]{16})\)', branch))
    ensure(len(prefixes) == 12, 'u24 twelve fingerprint prefixes absent')
    rows_section = '"scale8000;' + text.split('foreach(case "scale8000;', 1)[1].split('\n  list(GET case', 1)[0]
    rows = re.findall(r'"([^"]+)"', rows_section)
    ensure(len(rows) == 6, 'six route source rows absent')
    expected = {}
    for row in rows:
        label, n, input_spec, counts, balls, cells = row.split(';')
        counts = re.sub(r'\$\{(mhgp11_route_\w+)\}', lambda m: prefixes[m.group(1)], counts)
        name = 'mhgp11_api_supports_route_lidar_' + input_spec.removeprefix('data=lidar_') + '_k5' if label == 'lidar' else 'mhgp11_api_supports_route_' + label
        expected[name] = {'line': 'supports_route_verdict conforme k=5 n=' + n + ' ' + counts + ' fils=1,4', 'balls_floor': int(balls), 'cells_floor': int(cells)}
    result = {'schema': 'ehgp.v11.audit.r4_routes_sanitizers_review.v1', 'source_pin': PIN, 'archive': {'session': SESSION.name, 'sha256': sha(archive_bytes), 'bytes': len(archive_bytes)}, 'closure_status': 'closed_certified', 'method': 'Read-only Git/archive/stdlib review, no auditor build/native/cloud execution.', 'sources': {'morsehgp3D_v11/tests/api/tests.cmake': {'sha256': sha(cmake), 'anchors': [[94, 113], [126, 129]]}, 'morsehgp3D_v11/tests/api/supports_route.cpp': {'sha256': sha(probe), 'anchors': [[235, 253], [261, 265], [343, 384], [386, 400]]}}, 'configurations': {}, 'routes': [], 'runtime_logs_scan': [], 'important_findings': []}
    markers = ['ERROR: AddressSanitizer', 'SUMMARY: AddressSanitizer', 'AddressSanitizer:DEADLYSIGNAL', 'runtime error:', 'UndefinedBehaviorSanitizer', 'WARNING: ThreadSanitizer', 'SUMMARY: ThreadSanitizer', 'FATAL: ThreadSanitizer', 'ERROR: LeakSanitizer']
    observed_names = []
    inventory_union = set()
    passed_occurrences = 0
    with tarfile.open(archive) as tar:
        configs = sorted(n.removeprefix(BASE).removesuffix('/result.json') for n in tar.getnames() if n.startswith(BASE) and n.endswith('/result.json'))
        ensure(len(configs) == 4, 'four configurations absent')
        for config in configs:
            prefix = BASE + config + '/'
            get = lambda p: tar.extractfile(prefix + p).read()
            meta = json.loads(get('result.json'))
            bp = json.loads(get('build_provenance.json'))
            ensure(meta['status'] == 'ok' and meta['conforming'] and bp['complete'] and not bp['errors'], 'configuration nonconforming')
            files = {f['path']: f for f in bp['files']}
            flags = {}
            for target in ['mhgp11.dir', 'mhgp11_api_supports_route_probe.dir']:
                p = 'CMakeFiles/' + target + '/flags.make'
                f = files[p]
                ensure(sha(f['text'].encode()) == f['sha256'], 'flag text SHA mismatch')
                fields = {k: re.search(r'^' + k + r' = (.*)$', f['text'], re.M).group(1) for k in ['CXX_DEFINES', 'CXX_FLAGS']}
                ensure(fields['CXX_DEFINES'] == '-DMHGP11_COORD_BITS=24' and '-fsanitize=address,undefined' in fields['CXX_FLAGS'] and '-fno-sanitize-recover=all' in fields['CXX_FLAGS'], 'u24 ASan/UBSan actual flags missing')
                flags[p] = {'sha256': f['sha256'], **fields}
            link = files['CMakeFiles/mhgp11_api_supports_route_probe.dir/link.txt']
            ensure(sha(link['text'].encode()) == link['sha256'], 'link SHA mismatch')
            link_options = [x for x in shlex.split(link['text']) if x.startswith(('-f', '-l', '-Wl,'))]
            ensure('-fsanitize=address,undefined' in link_options, 'probe sanitizer link option absent')
            inventory = json.loads(get('tests.json'))
            inames = [x['name'] for x in inventory]
            cases_list = list(ET.fromstring(get('junit.xml')).iter('testcase'))
            cases = {x.get('name'): x for x in cases_list}
            passed = re.findall(r'\d+/\d+\s+Test\s+#\s*\d+:\s+(\S+)\s+\.+\s+Passed\s+[0-9.]+\s+sec', get('ctest.log').decode())
            ensure(len(inames) == len(set(inames)) == len(cases) == len(cases_list) == len(passed) == len(set(passed)) and set(inames) == set(cases) == set(passed), 'inventory/JUnit/CTest disagree')
            ensure(not any(x.get('disabled') for x in inventory) and all(c.get('status') == 'run' and not any(c.find(x) is not None for x in ['failure', 'error', 'skipped']) for c in cases_list), 'non-PASS occurrence')
            inventory_union.update(inames)
            passed_occurrences += len(inames)
            last_raw = get('LastTest.log')
            last = last_raw.decode()
            sections = {m.group(1): m.group(2) for m in re.finditer(r'^\d+/\d+ Testing: (\S+)\n(.*?)(?=^\d+/\d+ Testing: |\Z)', last, re.M | re.S)}
            names = sorted(n for n in inames if n in expected)
            result['configurations'][config] = {'domain': 'u24_ASan_UBSan', 'counts': meta['tests'], 'inventory_junit_ctest_sets_equal': True, 'actual_compile_flags': flags, 'probe_link': {'sha256': link['sha256'], 'options': link_options}, 'route_names': names}
            for name in names:
                block = sections[name]
                actual = [l for l in block.splitlines() if l.startswith('supports_route_verdict conforme ') and l.endswith(' fils=1,4')]
                ensure(len(actual) == 1 and actual[0] == expected[name]['line'], 'complete u24 golden verdict differs')
                ensure('run_expect_verdict conforme' in block and 'Test Passed.' in block and not any(x in block for x in ['ECART', 'PLANCHER', 'supports_route_verdict refus', 'run_expect_verdict ligne_absente']), 'route mismatch/refusal')
                objects = [json.loads(l) for l in block.splitlines() if l.startswith('{"route":')]
                compute = [x for x in objects if x.get('route') == 'compute']
                routes = {(x['route'], x['workers']): x for x in objects if x.get('route') != 'compute'}
                ensure(len(compute) == 1 and set(routes) == {('order_tree', 1), ('order_tree', 4), ('full_tower', 1), ('full_tower', 4)}, 'five route/compute observations absent')
                c = compute[0]
                ensure(c['tree_peak'] == c['full_tower_peak'] == routes[('full_tower', 1)]['tree_peak'] and c['order_tree_peak'] == routes[('order_tree', 1)]['tree_peak'] and 0 < c['order_tree_peak'] != c['tree_peak'], 'compute/FULL witness differs')
                observed_names.append(name)
                result['routes'].append({'configuration': config, 'name': name, 'status': 'PASS', 'actual_complete_verdict': actual[0], 'golden_profile_counts_journal_workers_match': True, 'floors': {'balls': expected[name]['balls_floor'], 'cells': expected[name]['cells_floor']}, 'compute_FULL_peak_witness': c, 'route_tree_peaks': {r + '_W' + str(w): x['tree_peak'] for (r, w), x in routes.items()}, 'lasttest_block_sha256': sha(block.encode())})
        for member in tar.getmembers():
            if member.isfile() and member.name.startswith(BASE) and member.name.endswith(('/ctest.log', '/LastTest.log', '/junit.xml', '/sanitizer.log')):
                raw = tar.extractfile(member).read(); text = raw.decode('utf-8', errors='replace')
                found = {m: text.count(m) for m in markers if m in text}
                ensure(not found, 'runtime diagnostic marker found ' + member.name)
                result['runtime_logs_scan'].append({'member': member.name, 'sha256': sha(raw), 'bytes': len(raw), 'marker_counts': found})
    ensure(len(observed_names) == 6 and set(observed_names) == set(expected), 'six unique exact routes absent')
    ensure(len(inventory_union) == passed_occurrences == 80, '80 unique selected PASS occurrences differ')
    ensure(sha(archive.read_bytes()) == receipt['results_sha256'], 'archive changed while reading')
    result['scope'] = {'six_routes_u24_asan_ubsan': 6, 'compute_FULL_peak_witnesses': 6, 'selected_unique_PASS_occurrences_crosschecked': 80, 'runtime_logs_scanned': len(result['runtime_logs_scan']), 'runtime_diagnostic_marker_hits': 0, 'native_cloud_actions_by_auditor': False}
    result['verdict'] = 'favorable_six_u24_routes_and_runtime_closed'
    result['limitations'] = ['The six route gates require full byte identity between routes/workers, equal complete journal registers and public compute file/manifest/peak equality to FULL. Runtime prints golden prefixes and compute peaks; separate per-route file hashes are not emitted.', 'These are new corrected-source G4 verdicts at98a; the six historical failed fins verdicts at38b remain failed records.', 'Scope is u24 ASan/UBSan and these four scale/LiDAR lots, not TSan/MSan or a global memory-safety proof, and not a timing contract.', 'No source, KITTI bytes or raw native logs were copied. The JSON retains only safe complete verdicts, peaks, flags and source/log fingerprints.']
    return result

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Read-only comparison of frozen R4 route/sanitizer proof.')
    parser.add_argument('--check', type=Path, default=Path(__file__).with_name('r4_routes_sanitizers_review_20261005.json'))
    frozen = parser.parse_args().check
    generated = (json.dumps(review(), ensure_ascii=False, indent=2) + '\n').encode()
    ensure(frozen.read_bytes() == generated, 'frozen JSON differs')
    print(json.dumps({'verdict': 'PASS_read_only', 'json_sha256': sha(generated), 'bytes': len(generated), 'routes': 6, 'runtime_marker_hits': 0}))
