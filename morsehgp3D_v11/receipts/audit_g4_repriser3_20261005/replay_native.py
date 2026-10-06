from pathlib import Path
import argparse
import collections
import hashlib
import json
import re
import subprocess
import tarfile
import xml.etree.ElementTree as ET

PIN = '98a00955083d483306c4f92b9031e382e81b0e59'
REPO = Path('/workspaces/E-HGP')
SESSION = Path('/workspaces/.ehgp-sessions/v11.20261005.clauderepriser3')
BASE = 'results/cmd/000_matrice/files/matrix/mutants/'

def ensure(condition, message):
    if not condition:
        raise RuntimeError(message)

def sha(data):
    return hashlib.sha256(data).hexdigest()

def review():
    receipt = json.loads((SESSION / 'receipt.json').read_text())
    ensure((SESSION / 'DONE').exists() and receipt.get('status') == 'completed' and receipt.get('worker_outcome') == 'exited' and receipt.get('results_verified') is True and receipt.get('targeted_shutdown_certified') is True, 'R3 not closed/certified')
    ensure(receipt['commit'] == PIN and receipt['worker_source'] == 'commit:' + PIN, 'receipt source mismatch')
    archive = SESSION / 'results/results.tar.gz'
    archive_bytes = archive.read_bytes()
    ensure(sha(archive_bytes) == receipt['results_sha256'], 'archive SHA mismatch')
    source_cache = {}
    def source(suffix):
        if suffix not in source_cache:
            source_cache[suffix] = subprocess.check_output(['git', 'show', PIN + ':morsehgp3D_v11/' + suffix], cwd=REPO)
        return source_cache[suffix]
    result = {'schema': 'ehgp.v11.audit.r3_api_cli_review.v1', 'source_pin': PIN, 'archive': {'session': SESSION.name, 'sha256': sha(archive_bytes), 'bytes': len(archive_bytes)}, 'closure_status': 'closed_certified', 'method': 'Read-only Git/archive/stdlib review, no auditor build/native/cloud execution.', 'modules': {}, 'important_findings': []}
    with tarfile.open(archive) as tar:
        get = lambda suffix: tar.extractfile(BASE + suffix).read()
        raw = get('LastTest.log'); text = raw.decode()
        sections = {m.group(1): m.group(2) for m in re.finditer(r'^\d+/\d+ Testing: (\S+)\n(.*?)(?=^\d+/\d+ Testing: |\Z)', text, re.M | re.S)}
        cases = {x.get('name'): x for x in ET.fromstring(get('junit.xml')).iter('testcase')}
        ctest = get('ctest.log').decode()
        passed = re.findall(r'\d+/\d+\s+Test\s+#\s*\d+:\s+(\S+)\s+\.+\s+Passed\s+[0-9.]+\s+sec', ctest)
        inventory = {x['name']: x for x in json.loads(get('tests.json'))}
        bp = json.loads(get('build_provenance.json'))
        ensure(bp['complete'] and not bp['errors'], 'build provenance incomplete')
        lib = next(f for f in bp['files'] if f['path'] == 'CMakeFiles/mhgp11.dir/flags.make')
        ensure(sha(lib['text'].encode()) == lib['sha256'], 'flags content mismatch')
        fields = {k: re.search(r'^' + k + r' = (.*)$', lib['text'], re.M).group(1) for k in ['CXX_DEFINES', 'CXX_FLAGS']}
        ensure(fields['CXX_DEFINES'] == '-DMHGP11_COORD_BITS=18', 'base library domain differs')
        result['base_library_flags'] = {'sha256': lib['sha256'], **fields}
        result['lasttest'] = {'member': BASE + 'LastTest.log', 'sha256': sha(raw), 'bytes': len(raw)}
        for module, floor in [('api', 23), ('cli', 28)]:
            manifest_raw = source('tests/mutants/' + module + '.json')
            manifest = json.loads(manifest_raw)
            mutants = manifest['mutants']
            ensure(manifest['plancher'] == floor and len(mutants) == floor, 'floor/count changed')
            ensure(all(not m.get('options', []) and m.get('attendu', 'porte') == 'porte' for m in mutants), 'local profile/expected type differs')
            gate = 'mhgp11_mutants_' + module
            target_gates = [gate, gate + '_manifest', gate + '_manifest_opt']
            verdicts = []
            for name in target_gates:
                case = cases[name]
                ensure(name in inventory and not inventory[name].get('disabled') and name in passed, 'gate absent/disabled/unpassed')
                ensure(case.get('status') == 'run' and not any(case.find(x) is not None for x in ['failure', 'error', 'skipped']), 'JUnit non-PASS')
                block = sections[name]
                ensure('run_expect_verdict conforme' in block and 'Test Passed.' in block, 'LastTest gate non-PASS')
                if name != gate:
                    ensure('manifeste_ok module=' + module + ' mutants=' + str(floor) + ' plancher=' + str(floor) in block, 'manifest verdict differs')
                verdicts.append({'name': name, 'status': 'PASS', 'lasttest_block_sha256': sha(block.encode())})
            block = sections[gate]
            parsed = [(m.group(1), m.group(2), m.group(3)) for m in re.finditer(r'^([a-z0-9_]+)\s+(TUE|SURVIT|INVALIDE)\s+(\S+)', block, re.M)]
            ensure([row[0] for row in parsed] == [m['id'] for m in mutants] and len(set(row[0] for row in parsed)) == floor, 'individual IDs differ/missing/duplicated')
            ensure(all(status == 'TUE' and cause == 'code' for _, status, cause in parsed), 'non-code/surviving/invalid individual')
            final = 'mutants_ok module=' + module + ' mutants=' + str(floor) + ' tues=' + str(floor) + ' dont_signal=0 dont_delai=0 dont_construction=0 plancher=' + str(floor)
            ensure(final in block, 'campaign final counters differ')
            ensure(not any(x in block for x in ['TEMOIN ROUGE', 'SURVIVANTS module=', 'INVALIDES module=']), 'reference or candidate failure')
            command = next(l for l in block.splitlines() if l.startswith('Command:'))
            definitions = re.findall(r'--cmake-arg=([^"\s]+)', command)
            ensure('-DCMAKE_BUILD_TYPE=Release' in definitions and '-DMHGP11_COORD_BITS=18' in definitions and '-DMHGP11_POISON=OFF' in definitions, 'actual campaign base args differ')
            individuals = []
            for mutant, (identifier, status, cause) in zip(mutants, parsed):
                body = source(mutant['fichier']).decode()
                ensure(body.count(mutant['cherche']) == 1 and mutant['cherche'] != mutant['remplace'], 'mutation not unique/effective at pin')
                individuals.append({'id': identifier, 'recorded_verdict': status, 'recorded_cause': cause, 'gate_from_pinned_manifest': mutant['porte'], 'mutation_source': 'morsehgp3D_v11/' + mutant['fichier'], 'local_cmake_options': [], 'effective_domain': 'u18_release'})
            result['modules'][module] = {'manifest_sha256': sha(manifest_raw), 'floor': floor, 'mutants': floor, 'actual_campaign_base_cmake_args': definitions, 'local_option_groups': [{'options': [], 'mutants': floor}], 'effective_domain': 'u18_release', 'witness': {'status': 'passed_by_successful_pinned_runner_control_flow', 'gates_required_to_pass': sorted({m['porte'] for m in mutants}), 'evidence': 'The campaign reached individual judgments and successful final output. Pinned runner executes every distinct nonmutated gate in its options group before any mutation judgment, and aborts red on any non-PASS. Separate witness outputs are not retained.'}, 'ctest_gates': verdicts, 'actual_final_line': final, 'individuals': individuals, 'observed_cause_counts': dict(collections.Counter(cause for _, _, cause in parsed)), 'signals': 0, 'delays': 0, 'expected_construction_rejections': 0}
    result['sp_masque_16379'] = {'recorded_verdict': 'TUE', 'recorded_cause': 'code', 'selected_gate': 'mhgp11_cli_points', 'source_inference': 'Removing only order_params.concurrent_orders=false leaves the true value inherited from full_params. PointsRequest invokes points_parts -> order_tree(default order_tree) -> build_order(order_params). build_order refuses concurrent_orders with parameter_out_of_range before order_forest. cli_points checks code0/status=ok for positive calls, so the counterfactual violates that gate.', 'runtime_boundary': 'Actual archived judgment is a checker-code mismatch. Native variant exit value/stderr is not retained: run_mutants.py returns an empty output for TUE at line308; no claim of a separately archived parameter_out_of_range message.'}
    anchors = {'CMakeLists.txt': [[300, 317]], 'tests/mutants/run_mutants.py': [[238, 249], [260, 308], [311, 324], [435, 468]], 'src/api/compute.cpp': [[38, 55], [118, 133], [163, 171], [208, 221]], 'src/tower/order_tree.cpp': [[94, 105]], 'tests/cli/tests.cmake': [[126, 128]], 'tests/cli/cli_points.py': [[49, 62]]}
    result['source_anchors'] = [{'path': 'morsehgp3D_v11/' + p, 'sha256': sha(source(p)), 'ranges': ranges} for p, ranges in anchors.items()]
    result['scope'] = {'api_cli_individuals': 51, 'recorded_code_detections': 51, 'signals': 0, 'delays': 0, 'construction_kills': 0, 'domain': 'u18_release_only', 'native_cloud_actions_by_auditor': False}
    result['verdict'] = 'favorable_51_individual_code_detections_closed'
    result['limitations'] = ['This targeted review covers API23 and CLI28 only. Other R3 modules may override base u18 with local u21/u24/poison options and are outside this JSON.', 'Cause code is the executed checker verdict recorded by the runner; outputs of successful kills and standalone reports mutants_api.json/mutants_cli.json are not archived. No exact native variant exit status or stderr is invented.', 'The nonmutated witnesses passed by the enforced execution path; their individual successful outputs were not separately preserved in the archive.', 'No historical long/scale/sanitizer qualification is transferred from this mutation campaign, and no timing contract is claimed.']
    ensure(sha(archive.read_bytes()) == receipt['results_sha256'], 'archive changed while reading')
    return result

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Read-only replay of frozen API/CLI R3 review.')
    parser.add_argument('--check', type=Path, default=Path(__file__).with_name('r3_api_cli_review_20261005.json'))
    expected = parser.parse_args().check
    generated = (json.dumps(review(), ensure_ascii=False, indent=2) + '\n').encode()
    ensure(expected.read_bytes() == generated, 'frozen JSON differs')
    print(json.dumps({'verdict': 'PASS_read_only', 'json_sha256': sha(generated), 'bytes': len(generated), 'api_cli_code_detections': 51}))
