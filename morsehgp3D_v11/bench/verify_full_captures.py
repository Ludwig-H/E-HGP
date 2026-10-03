#!/usr/bin/env python3
"""Offline QUAL/PAIRED receipt checker. Stdlib only; never launches a process or cloud call."""
import argparse
import copy
import csv
import hashlib
import io
import json
import math
from pathlib import Path, PurePosixPath
import re
import statistics
import sys
import tarfile
import xml.etree.ElementTree as ET

SOURCE = 'c40f40798375a0fc37917499401f16876cccbd2a'
HISTORICAL_PROTO_SOURCE = '6503c95abeb7822a5efd61e25babc2c0b60b79cc'
BASELINE = '895680ff866fbe41c450c87b2498ebff2ac7408b'
BASELINE_ARCHIVE = 'd9f8765c7284887d54bc248223fac3d00b9602aedbba1b8566e4727bdfbc8eef'
PAIRED_MATRIX_NAME = 'paired_bits21_matrix_corrected.json'
PAIRED_MATRIX_SHA256 = 'ed84337bce6223cde1b5cc69ee96a18a3777f51818baa38db8f223dd0913c75e'
COUNTS = dict(lidar_ng00=39885, lidar_ng01=35551, lidar_ng02=45845)
REUSE1_INPUTS = {
    'lidar_ng00': ('0baa4de14c95838ef7bd18d5a98551ca513ed830ec1eeee84f649fa97c95abaf',
                   'c73a41965f6f2e1042b5f3ba876d12ae0811b7f24aa73c886aa07830974e33c6'),
    'lidar_ng01': ('ba15adc6907d58e50bf28bca92305210c1efdde6efdf46c782aa1eec2318036f',
                   'bb699c2511a87618813a32657399539fe90e8f1e4f1187656ac88c03f2fee6a4'),
    'lidar_ng02': ('a4bbc86d00f92627b869fdc34aa260353bf1b821eff7c992ad93beb2a13308af',
                   '121e76f3ef1fcedb05cb65da9a95bfb84fe4b8305076a7485483fff113d128cc'),
}
VARIANTS = (('baseline', 2047), ('current2047', 2047), ('current16379', 16379))
MANDATORY = {
    'mhgp11_tower_population_concurrent_lemma', 'mhgp11_tower_population_concurrent_equivalence',
    'mhgp11_tower_population_concurrent_refusals', 'mhgp11_tower_full_bench_io',
    'mhgp11_tower_population_contract_contexts', 'mhgp11_tower_population_contract_each_step',
    'mhgp11_tower_population_contract_owned_admission', 'mhgp11_tower_population_contract_ledger',
    'mhgp11_catalogue_sort_fenv_key_modes', 'mhgp11_catalogue_sort_fenv_permutations',
    'mhgp11_catalogue_sort_fault_allocations',
}
IDENTITY = ('build_variant', 'case', 'coord_bits', 'kmax', 'workers', 'repetition', 'optimizations')


def need(value, reason):
    if not value:
        raise ValueError(reason)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def digest(path):
    h = hashlib.sha256()
    with Path(path).open('rb') as stream:
        for chunk in iter(lambda: stream.read(1 << 20), b''):
            h.update(chunk)
    return h.hexdigest()


def is_sha(value):
    return type(value) is str and re.fullmatch('[0-9a-f]{64}', value) is not None


def pairs(rows):
    result = {}
    for key, value in rows:
        need(key not in result, 'duplicate JSON key: ' + key)
        result[key] = value
    return result


def load(data):
    def invalid(value):
        raise ValueError('nonfinite JSON: ' + value)
    return json.loads(data, object_pairs_hook=pairs, parse_constant=invalid)


def number(value, reason, integer=False):
    need(type(value) is int if integer else type(value) in (int, float), reason + ': type')
    need(math.isfinite(value) and value >= 0 and (not integer or value <= 2**64 - 1), reason + ': domain')
    return value


def canonical(path):
    need(type(path) is str and '\\' not in path, 'archive path type/separator')
    clean = path.removeprefix('./').rstrip('/')
    parts = PurePosixPath(clean).parts
    need(clean and not path.startswith('/') and '..' not in parts and '.' not in parts and
         '/'.join(parts) == clean, 'unsafe/noncanonical path: ' + path)
    return clean


class Archive:
    """Read regular members in place. Never extract an archive onto the filesystem."""
    def __init__(self, source, result=False):
        self.tar = tarfile.open(fileobj=io.BytesIO(source), mode='r:gz') if isinstance(source, bytes) else tarfile.open(source, 'r:gz')
        self.files = {}
        seen = set()
        total = 0
        for member in self.tar.getmembers():
            path = canonical(member.name)
            need(path not in seen, 'duplicate archive path: ' + path)
            seen.add(path)
            need(member.isfile() or member.isdir(), 'archive link/special member: ' + path)
            total += member.size
            need(total <= 2 * 1024**3, 'archive expanded size exceeds offline checker cap')
            if member.isfile():
                self.files[path] = member
        self.manifest_hash = None
        if result:
            manifest = 'results/MANIFEST.sha256'
            raw = self.read(manifest)
            listed = {}
            for line in raw.decode().splitlines():
                h, path = line.split('  ', 1)
                path = 'results/' + canonical(path)
                need(is_sha(h) and path not in listed, 'result manifest entry')
                listed[path] = h
            need(set(listed) == set(self.files) - {manifest}, 'result manifest is not exhaustive (nested inventories included)')
            for path, expected in listed.items():
                need(self.hash(path) == expected, 'result payload differs from manifest: ' + path)
            self.manifest_hash = sha(raw)

    def read(self, path, cap=32 * 1024**2):
        member = self.files[path]
        need(member.size <= cap, 'member too large for parser: ' + path)
        return self.tar.extractfile(member).read()

    def hash(self, path):
        h = hashlib.sha256()
        with self.tar.extractfile(self.files[path]) as stream:
            for chunk in iter(lambda: stream.read(1 << 20), b''):
                h.update(chunk)
        return h.hexdigest()

    def json(self, path):
        return load(self.read(path))

    def close(self):
        self.tar.close()


def fields(raw):
    rows = [line.split('=', 1) for line in raw.decode().splitlines() if '=' in line]
    return pairs(rows)


def cache_entries(text):
    return pairs((line.split('=', 1)[0].split(':', 1)[0], line.split('=', 1)[1])
                 for line in text.splitlines() if ':' in line and '=' in line and not line.startswith(('#', '//')))


def generation_and_target(receipt, target):
    generation = receipt['generation']
    need(type(generation) is str and generation and generation == receipt['closing_generation'] ==
         receipt['observed_after']['lastStartTimestamp'] == target['lastStartTimestamp'], 'target generation mismatch')
    need(receipt['closure'] == 'stopped' and receipt['targeted_shutdown_certified'] is True and
         receipt['observed_after']['status'] == 'TERMINATED', 'targeted stop is not certified')
    expected = receipt['target']
    need(target['name'] == expected['instance'] and target['status'] == 'RUNNING' and
         target['zone'].endswith('/zones/' + expected['zone']) and
         target['machineType'].endswith('/projects/' + expected['project'] + '/zones/' + expected['zone'] +
                                         '/machineTypes/g4-standard-48'), 'target is not the captured G4-48 generation')
    scheduling = target['scheduling']
    need(scheduling['provisioningModel'] == 'SPOT' and scheduling['instanceTerminationAction'] == 'STOP' and
         scheduling['automaticRestart'] is False and scheduling['maxRunDuration'] == {'nanos': 0, 'seconds': '4200'} and
         receipt['max_run_seconds'] == 4200, 'G4 session lifecycle/cap differs')
    origin_sha = target['capture_source']['sha256'] if 'capture_source' in target else target['original_capture_sha256']
    need(is_sha(origin_sha), 'target describe origin hash missing')


def session(directory, package_hash, expected_source, command_names, inspect_failed=False):
    directory = Path(directory)
    receipt = load((directory / 'receipt.json').read_bytes())
    need(receipt['schema'] == 'ehgp.v11.session_receipt.v1' and receipt['source_kind'] == 'commit' and
         receipt['commit'] == expected_source and receipt['worker_source'] == 'commit:' + expected_source and
         receipt['package_sha256'] == package_hash, 'controller source/package differs')
    generation_and_target(receipt, load((directory / 'target_running_minimal.json').read_bytes()))
    need(receipt['status'] in (('completed', 'failed_remote') if inspect_failed else ('completed',)) and
         receipt['results_verified'] is True and type(receipt['worker_exit_code']) is int and
         receipt['worker_exit_code'] in ((0, 1) if inspect_failed else (0,)) and
         receipt['guest_guard_intact'] is True and receipt['start_certified'] is True and
         receipt['data_verified_remote'] is True and not receipt['errors'] and not receipt['results_skipped_members'],
         'controller receipt failed/incomplete')
    need(not receipt['overflow']['evicted'] and not receipt['overflow']['truncated_streams'], 'receipt overflow/truncation')
    plan_raw = (directory / 'package/plan.json').read_bytes()
    plan = load(plan_raw)
    need(sha(plan_raw) == receipt['plan_sha256'] and [r['name'] for r in plan['commands']] == command_names,
         'session plan hash/command selection')
    plan_sh = (directory / 'package/plan.sh').read_bytes()
    need(sha(plan_sh) == receipt['worker_plan_sha256'], 'rendered worker plan hash differs')
    archive_path = directory / 'results/results.tar.gz'
    need(digest(archive_path) == receipt['results_sha256'] and archive_path.stat().st_size == receipt['results_bytes'],
         'results archive controller hash/size')
    archive = Archive(archive_path, result=True)
    need(len(archive.files) - 1 == receipt['results_manifest_files'], 'results manifest count')
    worker = fields(archive.read('results/worker.txt'))
    need(worker == receipt['worker'] and worker['schema'] == 'ehgp.v11.worker_result.v1' and
         worker['status'] in (('completed', 'failed') if inspect_failed else ('completed',)) and
         not worker['fatal'] and worker['data'] == 'ok' and
         worker['source'] == receipt['worker_source'] and worker['package_sha256'] == package_hash and
         worker['generation'] == receipt['generation'] and worker['plan_sha256'] == receipt['worker_plan_sha256'],
         'worker/controller provenance or verdict differs')
    need(all(worker[k] == '0' for k in ('interrupted', 'overflow_files', 'overflow_unresolved', 'truncated_streams')) and
         worker['commands_total'] == str(len(command_names)) and worker['nproc'] == '48',
         'worker counts/interruption/overflow/CPU')
    need(archive.hash('results/plan.sh') == receipt['worker_plan_sha256'], 'returned rendered plan differs')
    commands = list(csv.DictReader(io.StringIO(archive.read('results/commands.tsv').decode()), delimiter='\t'))
    need(commands == receipt['commands'] and [r['name'] for r in commands] == command_names and
         [r['index'] for r in commands] == [str(i) for i in range(len(commands))] and
         (inspect_failed or all(r['status'] == 'ok' and r['exit_code'] == '0' for r in commands)),
         'command table failed/different')
    successes = sum(r['status'] == 'ok' and r['exit_code'] == '0' for r in commands)
    need(worker['commands_ok'] == str(successes) and (inspect_failed or successes == len(commands)) and
         (worker['status'] == 'completed') == (successes == len(commands)) and
         receipt['worker_exit_code'] == (0 if successes == len(commands) else 1), 'worker command accounting/verdict')
    for row, wanted in zip(commands, plan['commands']):
        need(row['timeout_seconds'] == str(wanted['timeout_seconds']), 'command timeout changed')
        number(float(row['wall_seconds']), 'command wall')
    return receipt, archive, plan


def provenance(archive, path, options):
    value = archive.json(path)
    need(value['schema'] == 'ehgp.v11.build_provenance.v1' and value['complete'] is True and not value['errors'],
         'build provenance incomplete: ' + path)
    rows = {r['path']: r for r in value['files']}
    need(len(rows) == len(value['files']), 'build provenance duplicate path')
    for row in rows.values():
        need(is_sha(row['sha256']) and number(row['size'], 'build artifact size', True) > 0, 'build artifact pin')
        if 'text' in row:
            raw = row['text'].encode()
            need(len(raw) == row['size'] and sha(raw) == row['sha256'], 'embedded build flags/cache hash')
    cache = cache_entries(rows['CMakeCache.txt']['text'])
    for option in options:
        if option.startswith('-D') and '=' in option:
            key, wanted = option[2:].split('=', 1)
            need(cache.get(key) == wanted, 'actual CMake cache differs from requested option: ' + key)
    return rows, cache


def matrix(archive, root, spec, require_guards=False, inspect_failed=False):
    summary = archive.json(root + '/summary.json')
    need(summary['schema'] == 'ehgp.v11.g4_matrix_summary.v1' and summary['complete'] is True and
         type(summary['conforming']) is bool and type(summary['exit_code']) is int and
         summary['exit_code'] in ((0, 1, 3) if inspect_failed else (0,)) and
         summary['conforming'] == (summary['exit_code'] == 0) and
         not summary['signals'], 'matrix incomplete/failed/interrupted: ' + root)
    configs = summary['configurations']
    names = [r['name'] for r in configs]
    need(names == summary['requested'] == [r['name'] for r in spec['configurations']] and len(set(names)) == len(names) and
         summary['statuses'] == {r['name']: r['status'] for r in configs}, 'matrix selection/status inventory')
    result = []
    for actual, planned in zip(configs, spec['configurations']):
        name = actual['name']
        if actual['status'] == 'absent':
            need(planned.get('optional') is True and actual['status'] == 'absent',
                 'mandatory matrix configuration did not pass: ' + name)
            result.append(dict(configuration=name, status='absent_optional', qualified=False))
            continue
        failed = actual['status'] == 'failed'
        floor = actual['status'] == 'floor_violated'
        bad = failed or floor
        need(actual['status'] == 'ok' or (inspect_failed and bad), 'mandatory configuration did not complete: ' + name)
        need(actual['conforming'] is (not bad) and (failed or (not actual['failures'] and not actual['not_run'])) and
             actual['cmake_options'] == planned['cmake_options'] and actual['ctest_args'] == planned['ctest_args'] and
             all(type(s['exit_code']) is int and (failed or (s['status'] == 'ok' and s['exit_code'] == 0))
                 for s in actual['steps']),
             'matrix steps/options/verdict: ' + name)
        inventory = archive.json(root + '/' + name + '/tests.json')
        test_names = [r['name'] for r in inventory]
        counts = actual['tests']
        need(len(set(test_names)) == len(test_names) and not any(r['disabled'] for r in inventory) and
             type(counts['selected']) is int and (floor or counts['selected'] >= planned.get('min_tests', 1)) and
             counts['selected'] == counts['passed'] + counts['failed'] + counts['not_run'] == len(test_names) and
             (failed or counts['failed'] == counts['not_run'] == 0),
             'matrix selected/passed inventory: ' + name)
        xml = ET.fromstring(archive.read(root + '/' + name + '/junit.xml'))
        cases = list(xml.iter('testcase'))
        passed_cases = [r for r in cases if r.get('status') == 'run' and r.find('failure') is None and
                        r.find('skipped') is None and r.find('error') is None]
        failed_cases = [r for r in cases if r.find('failure') is not None or r.find('error') is not None]
        not_run_cases = [r for r in cases if r.find('skipped') is not None or r.get('status') == 'notrun']
        need({r.get('name') for r in cases} == set(test_names) and len(cases) == len(test_names) and
             len(passed_cases) == counts['passed'] and len(failed_cases) == counts['failed'] and
             len(not_run_cases) == counts['not_run'], 'JUnit execution/verdict differs: ' + name)
        passed_names = {r.get('name') for r in passed_cases}
        labels = {}
        for test in inventory:
            if test['name'] in passed_names:
                for label in test['labels']:
                    labels[label] = labels.get(label, 0) + 1
        need(labels == actual['passed_labels'], 'matrix labels differ from passed inventory: ' + name)
        required = set(planned.get('require_labels', [])) | set(planned.get('require_labels_if_data', []))
        missing = sorted(label for label in required if labels.get(label, 0) == 0)
        need((floor and (missing or counts['selected'] < planned.get('min_tests', 1))) or
             (not floor and (failed or not missing)), 'matrix label/floor verdict inconsistent: ' + name)
        guards = MANDATORY if require_guards is True else (require_guards or set())
        if guards and name not in ('style', 'mutants'):
            need(guards <= set(test_names), 'new native refusal/FENV guards not all played: ' + name)
        options = [o.replace('{threads}', str(actual['threads'])) for o in planned['cmake_options']]
        rows, cache = provenance(archive, root + '/' + name + '/build_provenance.json', options)
        result.append(dict(configuration=name, status=actual['status'], selected=len(test_names), passed=counts['passed'],
                           failed=counts['failed'], failed_tests=[r.get('name') for r in failed_cases],
                           coord_bits=cache['MHGP11_COORD_BITS'],
                           required_guards_played=sorted(guards & set(test_names)),
                           passed_labels=labels, missing_required_labels=missing,
                           executable_pins=sum(p.startswith('mhgp11_') for p in rows),
                           seconds=number(actual['seconds'], 'matrix seconds')))
    states = {r['status'] for r in configs}
    expected_code = 1 if 'failed' in states else 3 if 'floor_violated' in states else 0
    need(summary['exit_code'] == expected_code, 'matrix global exit differs from configuration verdicts')
    return summary, result


def paired_matrix_plan(template, actual, receipt, matrix_path=None):
    """Admit exactly the recorded runtime correction, never arbitrary external configurations."""
    wanted = copy.deepcopy(template)
    spec_path = 'morsehgp3D_v11/bench/full_paired_bits21_matrix.json'
    argv = actual['commands'][1]['argv']
    matrix_arg = argv[argv.index('--matrix') + 1]
    source_arg = '{src}/' + spec_path
    override = None
    if matrix_arg != source_arg:
        need(matrix_arg == '{data}/' + PAIRED_MATRIX_NAME and matrix_path is not None,
             'paired matrix override is not the approved captured input')
        raw = Path(matrix_path).read_bytes()
        rows = [r for r in receipt['data_files'] if r['name'] == PAIRED_MATRIX_NAME]
        need(len(rows) == 1 and sha(raw) == rows[0]['sha256'] == PAIRED_MATRIX_SHA256 and len(raw) == rows[0]['size'],
             'paired runtime matrix input hash/size')
        override = load(raw)
        plan_argv = wanted['commands'][1]['argv']
        plan_argv[plan_argv.index('--matrix') + 1] = matrix_arg
    # Only prose changes are additional to the two prior archive substitutions and the admitted matrix argument.
    normalized = copy.deepcopy(actual)
    if 'note' in wanted:
        need(type(normalized['note']) is str, 'paired plan note type')
        normalized['note'] = wanted['note']
    need(wanted == normalized, 'paired plan changed beyond approved substitutions')
    return override


def checked_runtime_matrix(original, override):
    if override is None:
        return original
    corrected = copy.deepcopy(override)
    need(len(corrected['configurations']) == 1 and corrected['configurations'][0]['require_labels'] == ['unit', 'oracle'] and
         original['configurations'][0]['require_labels'] == ['unit', 'concurrency'], 'runtime matrix correction labels')
    corrected['configurations'][0]['require_labels'] = original['configurations'][0]['require_labels']
    need(type(corrected['note']) is str, 'runtime matrix correction note')
    corrected['note'] = original['note']
    need(corrected == original, 'runtime matrix changed filters/options/floors/threads beyond labels')
    return override


def mutant_summary(archive, root, package, inspect_failed=False):
    raw = archive.read(root + '/mutants/LastTest.log').decode()
    result = []
    for module in ('core', 'num', 'sched', 'cloud', 'index', 'catalogue', 'tower'):
        plan = package.json('morsehgp3D_v11/tests/mutants/' + module + '.json')
        wanted = {r['id'] for r in plan['mutants']}
        need(len(wanted) == len(plan['mutants']), 'source mutant IDs duplicate')
        blocks = re.findall(r'^\d+/\d+ Testing: mhgp11_mutants_' + module +
                            r'\n(.*?)(?=^\d+/\d+ Testing:|\Z)', raw, re.MULTILINE | re.DOTALL)
        need(len(blocks) == 1, 'mutant CTest log block missing/duplicate: ' + module)
        block = blocks[0]
        summary = re.findall(r'mutants_ok module=' + module +
            r' mutants=(\d+) tues=(\d+) dont_signal=(\d+) dont_delai=(\d+) dont_construction=(\d+) plancher=(\d+)', block)
        lines = re.findall(r'^([^\s]+)\s+(TUE|SURVIT|INVALIDE)\s+(.*)$', block, re.MULTILINE)
        observed = [(ident, verdict, detail) for ident, verdict, detail in lines if ident in wanted]
        need(len(observed) == len(wanted) and {i for i, _, _ in observed} == wanted,
             'individual mutant verdict inventory: ' + module)
        unsuccessful = [dict(id=ident, verdict=verdict, detail=detail) for ident, verdict, detail in observed if verdict != 'TUE']
        if inspect_failed and unsuccessful:
            need(not summary, 'failed mutant module falsely has mutants_ok summary')
            result.append(dict(module=module, mutants=len(wanted), killed=sum(v == 'TUE' for _, v, _ in observed),
                               unsuccessful=unsuccessful, causes_scope='no successful module summary; causes not inferred'))
            continue
        need(len(summary) == 1, 'mutant summary missing/duplicate: ' + module)
        total, killed, signal, timeout, construction, floor = map(int, summary[0])
        expected_construction = sum(r.get('attendu', 'porte') == 'construction' for r in plan['mutants'])
        need(total == killed == len(wanted) and floor == plan['plancher'] and killed >= floor and
             signal == timeout == 0 and construction == expected_construction, 'mutant causes/counts: ' + module)
        need(not unsuccessful, 'individual mutant verdict inventory: ' + module)
        result.append(dict(module=module, mutants=total, judge_kills=total-construction,
                           expected_compile_refusals=construction, signal_kills=signal, timeout_kills=timeout))
    return result


def calendar():
    rows = []
    for repetition in range(3):
        for case in sorted(COUNTS):
            for workers in (1, 8, 48):
                for name, mode in VARIANTS[repetition:] + VARIANTS[:repetition]:
                    rows.append(dict(build_variant=name, case=case, coord_bits=21, kmax=5, workers=workers,
                                     repetition=repetition, optimizations=mode))
    return rows


def identity(row):
    return tuple(row[k] for k in IDENTITY)


def input_manifest(value, pins):
    need(value['schema'] == 'mhgp11.catalogue_benchmark_inputs.v1', 'input manifest schema')
    cases = {r['name']: r for r in value['cases']}
    need(len(cases) == len(value['cases']), 'duplicate input cases')
    files = {r['name']: r for r in pins}
    need(len(files) == len(pins), 'duplicate controller data files')
    for name, count in COUNTS.items():
        row = cases[name]
        need(type(row['count']) is int and row['count'] == count and row['unit_site_weights'] is True and
             row['profile'] == 'quantized_u18_input_only' and row['duplicate_sites'] == 0, 'whole same-coordinate unit input')
        need((row['sha256'], row['ids_sha256']) == REUSE1_INPUTS[name], 'inputs are not the pinned reuse1 XYZ/IDs: ' + name)
        for field, hash_field, width in (('coordinates', 'sha256', 12), ('point_ids', 'ids_sha256', 4)):
            need(files[row[field]]['sha256'] == row[hash_field] and files[row[field]]['size'] == width * count,
                 'controller input hash/size differs from benchmark: ' + name)
    return cases


def paired_rows(report, cases, commit, targeted_pins):
    need(report['schema'] == 'ehgp.v11.full_paired_campaign.v1' and report['complete'] is True and
         report['conforming'] is True and report['build_status'] == 'ok' and not report.get('failure') and
         not report['not_run'] and report['requested_runs'] == 81 and report['requested'] == calendar(),
         'paired schedule incomplete/nonconforming/not exactly 81')
    rows, intents = report['runs'], report['launch_intents']
    need(len(rows) == len(intents) == 81 and [identity(r) for r in rows] == [identity(r) for r in intents] ==
         [identity(r) for r in calendar()], 'paired invocations missing/duplicate/out of calendar order')
    builds = report['builds']
    need(len(builds) == 2 and builds[0]['source_commit'] == BASELINE and builds[1]['source_commit'] == commit,
         'paired comparator/current sources')
    seen = {}
    origins = {}
    timings = {}
    for row, intent in zip(rows, intents):
        case = cases[row['case']]
        current = row['build_variant'] != 'baseline'
        pin = row['build_pin']
        build = builds[1 if current else 0]
        need(pin == row['build_pin_after'] == intent['build_pin'] and all(pin[k] == build[k] for k in pin),
             'attempt binary/source/cache pin drift')
        need(pin['source_commit'] == (commit if current else BASELINE) and
             all(is_sha(pin[k]) for k in ('sha256', 'cache_sha256', 'provenance_sha256', 'source_manifest_sha256')),
             'attempt source/binary pins')
        if current:
            need(pin['sha256'] == targeted_pins['binary']['sha256'] and pin['bytes'] == targeted_pins['binary']['size'] and
                 pin['cache_sha256'] == targeted_pins['cache']['sha256'] and
                 pin['provenance_sha256'] == targeted_pins['provenance_sha256'], 'attempt not the newly targeted binary')
        need(row['status'] == 'ok' and type(row['exit_code']) is int and row['exit_code'] == 0 and
             not row['stderr'] and not row['errors'] and row['whole_input'] is True and row['count'] == case['count'] and
             row['argv'] == intent['argv'] and intent['input_sha256'] == case['sha256'] and
             intent['ids_sha256'] == case['ids_sha256'] and intent['whole_input'] is True, 'paired attempt verdict/input/argv')
        argv = row['argv']
        need(len(argv) == 12 and argv[0] == pin['path'] and Path(argv[1]).name == case['coordinates'] and
             Path(argv[2]).name == case['point_ids'] and argv[4] == '5' and argv[10] == str(row['workers']) and
             argv[11] == str(row['optimizations']), 'native argv parameters')
        events = [load(line) for line in row['stdout'].splitlines() if line.strip()]
        need(events == row['events'] and [r['phase'] for r in events] == ['cloud', 'domain', 'full', 'exit'],
             'native complete stream/retained stdout differs')
        cloud, domain, full, end = events
        need(full['status'] == end['status'] == 'ok' and full['reason'] == end['reason'] == 'none' and
             cloud['sites'] == cloud['points'] == case['count'] and
             all(type(full[k]) is int and full[k] == row[k] for k in ('coord_bits', 'kmax', 'workers', 'optimizations')),
             'native FULL params/cardinalities/verdict')
        for key in ('wall_ns', 'index_ns', 'domain_ns', 'forest_ns', 'peak_reserved_bytes', 'reserved_after_bytes'):
            number(full[key], key, True)
        need(domain['index_ns'] == full['index_ns'] and domain['domain_ns'] == full['domain_ns'] and
             sum(full[k] for k in ('index_ns', 'domain_ns', 'forest_ns')) <= full['wall_ns'] and
             full['reserved_after_bytes'] <= full['peak_reserved_bytes'], 'disjoint FULL phase wall/reservations')
        number(full['cpu_seconds'], 'native CPU')
        for key in ('process_wall_seconds', 'semantic_wall_seconds', 'full_ms'):
            number(row[key], key)
        need(row['full_ms'] == full['wall_ns']/1e6 and len(full['orders']) == len(row['semantic']['orders']) == 5,
             'reported wall/orders differ')
        need(all(number(v, 'order work', True) >= 0 for o in full['orders'] for v in o['work'].values()), 'order work domain')
        if current:
            need(type(full['concurrent_orders']) is bool and type(full['population_lookup']) is bool and
                 sum(number(v, 'global phase', True) for v in full['phases'].values()) <= full['forest_ns'],
                 'current disjoint global phases')
        semantic = row['semantic']
        need(is_sha(semantic['sha256']) and is_sha(semantic['raw_sha256']) and
             number(semantic['bytes'], 'FULL artifact size', True) > 0, 'FULL artifact identity')
        artifact = row['artifact_comparison']
        need(artifact['raw_sha256'] == semantic['raw_sha256'] and
             artifact['status'] == ('equal_bytes' if row['case'] in seen else 'reference'), 'byte comparison record')
        same = (semantic['sha256'], semantic['raw_sha256'], semantic['bytes'])
        need(row['case'] not in seen or seen[row['case']] == same, 'old/new FULL output hashes/bytes differ')
        seen[row['case']] = same
        reuse = row['semantic_reuse']
        attempt = list(identity(row))
        need(reuse['current_attempt'] == attempt and reuse['raw_sha256'] == semantic['raw_sha256'] and
             reuse['bytes'] == semantic['bytes'] and reuse['mode'] in ('decoded', 'reused'), 'semantic reuse context/current identity')
        context = reuse['context']
        need(reuse['schema'] == 'ehgp.v11.semantic_reuse.v1' and context['format'] == 'MHGP11FUL1' and
             context['decoder_version'] == 'ehgp.v11.full_semantic.v1' and
             context['decoder_sha256'] == targeted_pins['decoder_sha256'] and
             context['coord_bits'] == 21 and context['kmax'] == 5 and context['count'] == case['count'] and
             context['xyz_sha256'] == case['sha256'] and context['ids_sha256'] == case['ids_sha256'],
             'semantic reuse exact input/decoder context')
        first = row['case'] not in origins
        origins.setdefault(row['case'], attempt)
        need(reuse['source_attempt'] == origins[row['case']] and reuse['mode'] == ('decoded' if first else 'reused'),
             'semantic origin is not the first validated matching attempt')
        timings.setdefault((row['case'], row['build_variant'], row['workers']), []).append(row['full_ms'])
    return [dict(case=k[0], build_variant=k[1], workers=k[2], full_ms=v, median_full_ms=statistics.median(v))
            for k, v in sorted(timings.items())]


def source_inventory(package, before, after):
    need(before == after, 'paired source changed during benchmark')
    rows = before['files']
    names = [r['path'] for r in rows]
    need(len(set(names)) == len(names), 'paired source inventory duplicate')
    inventory_sha = sha('\n'.join('%s %s %d' % (r['sha256'], r['path'], r['bytes']) for r in rows).encode())
    need(before['sha256'] == inventory_sha, 'paired source aggregate hash')
    expected = {p.removeprefix('morsehgp3D_v11/') for p in package.files if
                p == 'morsehgp3D_v11/CMakeLists.txt' or any(p.startswith('morsehgp3D_v11/' + n + '/')
                for n in ('src', 'cmake', 'tests', 'reference', 'bench', 'tools'))}
    need(set(names) == expected, 'paired source manifest not exhaustive for build/bench/gates')
    for row in rows:
        path = 'morsehgp3D_v11/' + canonical(row['path'])
        need(row['bytes'] == package.files[path].size and row['sha256'] == package.hash(path),
             'paired source blob differs from fixed package: ' + path)


def verify(qualification_dir, paired_dir, package_path, commit, inspect_failed=False, paired_matrix_path=None):
    package_hash = digest(package_path)
    package = Archive(package_path)
    # A Git archive comment is a recorded commit assertion. Hashes protect its bytes; an independently
    # captured Git blob manifest can supplement this check, but the reader never invents one.
    need(package.tar.pax_headers.get('comment') == commit, 'source Git archive commit comment differs/missing')
    main_spec = package.json('morsehgp3D_v11/tools/g4_matrix.json')
    supplement_spec = package.json('morsehgp3D_v11/bench/meb_asan18_matrix.json')
    need('gcp-migration/v11_worker.sh' in package.files, 'worker missing from source package')
    receipt, archive, plan = session(qualification_dir, package_hash, commit, ['matrice', 'asan18'], inspect_failed)
    need(plan == package.json('morsehgp3D_v11/bench/plans/full_qualification_g4.json'), 'qualification plan differs from fixed source')
    main_root = 'results/cmd/000_matrice/files/matrix'
    main, configs = matrix(archive, main_root, main_spec, require_guards=True, inspect_failed=inspect_failed)
    supplement_root = 'results/cmd/001_asan18/files/matrix'
    tower_guards = {n for n in MANDATORY if n.startswith('mhgp11_tower_')}
    _, supplement = matrix(archive, supplement_root, supplement_spec, require_guards=tower_guards)
    mutants = mutant_summary(archive, main_root, package, inspect_failed)
    conforming = receipt['status'] == 'completed' and main['conforming'] is True
    result = dict(schema='ehgp.v11.full_captures_review.v1', conforming=conforming, integrity_verified=True,
                  source_commit=commit,
                  source_package_sha256=package_hash,
                  qualification=dict(generation=receipt['generation'], closure='targeted_stopped',
                      controller_status=receipt['status'], matrix_conforming=main['conforming'],
                      results_sha256=receipt['results_sha256'], manifest_sha256=archive.manifest_hash,
                      configurations=configs, asan18=supplement, mutants=mutants),
                  scope='offline receipt/protocol recoupe; native tests were executed by the captured worker only',
                  limits=['CPU on G4 host; no GPU or points/head qualification',
                          'build hashes are worker provenance; binaries absent from results cannot be rehashed offline',
                          'source Git comment records a commit assertion; package SHA protects the exact captured bytes'])
    if paired_dir is not None:
        need(conforming, 'a failed full qualification cannot qualify a paired campaign')
        current_receipt, current, current_plan = session(paired_dir, package_hash, commit, ['prior', 'bits21', 'paired_full'], inspect_failed)
        template = package.json('morsehgp3D_v11/bench/plans/full_paired_g4.json')
        prior_argv = current_plan['commands'][0]['argv']
        template['commands'][0]['argv'][template['commands'][0]['argv'].index('@PRIOR_RESULTS_ARCHIVE@')] = prior_argv[3]
        template['commands'][0]['argv'][template['commands'][0]['argv'].index('@PRIOR_RESULTS_SHA256@')] = receipt['results_sha256']
        if paired_matrix_path is None:
            paired_matrix_path = Path(paired_dir) / 'package' / PAIRED_MATRIX_NAME
        runtime_override = paired_matrix_plan(template, current_plan, current_receipt, paired_matrix_path)
        need(runtime_override is None or (inspect_failed and commit == HISTORICAL_PROTO_SOURCE and
             current_receipt['status'] == 'failed_remote'), 'runtime override is only admitted for the failed historical650r2 capture')
        targeted_root = 'results/cmd/001_bits21/files/matrix'
        targeted_spec = checked_runtime_matrix(package.json('morsehgp3D_v11/bench/full_paired_bits21_matrix.json'), runtime_override)
        targeted, targeted_counts = matrix(current, targeted_root,
            targeted_spec, require_guards=True, inspect_failed=inspect_failed)
        need(all(targeted['host'][k] == main['host'][k] for k in ('gxx', 'cmake')), 'compiler/CMake version changed between sessions')
        provenance_path = targeted_root + '/bits21/build_provenance.json'
        bins, cache = provenance(current, provenance_path, ['-DMHGP11_COORD_BITS=21'])
        _, first_cache = provenance(archive, main_root + '/bits21/build_provenance.json', ['-DMHGP11_COORD_BITS=21'])
        need(all(cache[k] == first_cache[k] for k in ('CMAKE_CXX_COMPILER', 'CMAKE_CXX_FLAGS',
             'CMAKE_CXX_FLAGS_RELEASE', 'MHGP11_MARCH', 'MHGP11_COORD_BITS')), 'current rebuild flags differ from first qualification')
        root = 'results/cmd/002_paired_full/files/'
        prior_path = 'results/cmd/000_prior/files/prior_context.json'
        prior = current.json(prior_path)
        need(prior['source'] == 'commit:' + commit and prior['archive_sha256'] == receipt['results_sha256'] and
             prior['manifest_sha256'] == archive.manifest_hash, 'prior qualification context binding')
        for name, row in prior['copied'].items():
            copied_path = 'results/cmd/000_prior/files/' + Path(row['path']).name
            need(current.hash(copied_path) == row['sha256'] == archive.hash(name) and
                 current.files[copied_path].size == row['bytes'], 'copied prior payload differs')
        data_before = {r['name']: (r['sha256'], r['size']) for r in receipt['data_files']}
        data_after = {r['name']: (r['sha256'], r['size']) for r in current_receipt['data_files']}
        extra = data_after.pop(PAIRED_MATRIX_NAME, None) if runtime_override is not None else None
        need(data_before == data_after, 'staged inputs changed beyond the approved runtime matrix')
        if inspect_failed and current_receipt['status'] == 'failed_remote' and root + 'full_paired.json' not in current.files:
            refusal = current.read('results/cmd/002_paired_full/stderr').decode().strip()
            expected_refusal = 'full_paired_refused: ' + ('new bits21 targeted gates incomplete/nonconforming'
                if targeted['conforming'] is False else 'evenement JSON non objet')
            need(root + 'full_paired.json' not in current.files and
                 current_receipt['commands'][2]['exit_code'] == '2' and
                 refusal == expected_refusal and commit == HISTORICAL_PROTO_SOURCE,
                 'failed targeted gates did not yield the captured before-benchmark refusal')
            result['conforming'] = False
            result['paired'] = dict(generation=current_receipt['generation'], closure='targeted_stopped',
                controller_status=current_receipt['status'], results_sha256=current_receipt['results_sha256'],
                manifest_sha256=current.manifest_hash, new_targeted_configs=targeted_counts,
                native_gate_counts_passed=all(c.get('failed') == 0 and c['passed'] == c['selected'] for c in targeted_counts),
                benchmark='refused_before_chrono', refusal=refusal, timing_report_present=False,
                runtime_matrix_sha256=extra[0] if extra else None)
            current.close(); archive.close(); package.close()
            return result
        report = current.json(root + 'full_paired.json')
        need(report['source'] == 'commit:' + commit and report['package_sha256'] == package_hash and
             report['generation'] == current_receipt['generation'] and
             report['qualification_sha256'] == current.hash(targeted_root + '/summary.json') and
             report['supplement_sha256'] == archive.hash(supplement_root + '/summary.json') and
             report['prior_context_sha256'] == current.hash(prior_path) and report['prior_qualification'] == prior,
             'paired qualification/generation binding')
        need(report['manifest_sha256'] == data_after['manifest.json'][0], 'paired manifest original hash differs')
        cases = input_manifest(report['manifest'], current_receipt['data_files'])
        source_inventory(package, current.json(root + 'source_before.json'), current.json(root + 'source_after.json'))
        need(report['builds'][1]['source_manifest_sha256'] == current.hash(root + 'source_before.json'), 'current source manifest pin')
        manifest_path = 'morsehgp3D_v11/receipts/qualification_performance_20261003/baseline_source_manifest.json'
        baseline = package.json(manifest_path)
        old_path = 'morsehgp3D_v11/receipts/qualification_performance_20261003/' + baseline['archive']
        need(package.hash(old_path) == baseline['archive_sha256'] == BASELINE_ARCHIVE and
             package.files[old_path].size == baseline['archive_bytes'] and baseline['source_commit'] == BASELINE and
             report['builds'][0]['source_manifest_sha256'] == package.hash(manifest_path), 'baseline source fixture pin')
        old_archive = Archive(package.read(old_path))
        need(set(old_archive.files) == {r['path'] for r in baseline['files']}, 'baseline exact selected Git inventory')
        for row in baseline['files']:
            data = old_archive.read(row['path'])
            blob = hashlib.sha1(b'blob ' + str(len(data)).encode() + b'\0' + data).hexdigest()
            need(len(data) == row['bytes'] and sha(data) == row['sha256'] and blob == row['git_blob'], 'baseline source blob hash')
        old_archive.close()
        old_provenance = current.json(root + 'baseline_build_provenance.json')
        build = report['builds'][0]
        need(all(old_provenance[k] == build[k] for k in old_provenance) and
             build['provenance_sha256'] == current.hash(root + 'baseline_build_provenance.json'), 'baseline build provenance pin')
        for ordinal in (0, 1):
            row = current.json(root + 'baseline_build_%d.json' % ordinal)
            need(type(row['exit_code']) is int and row['exit_code'] == 0 and row['argv'] == build['build_commands'][ordinal],
                 'baseline configure/build failed or command changed')
        configure, build_command = build['build_commands']
        need(all('-D%s=%s' % (k, cache[k]) in configure for k in ('CMAKE_CXX_COMPILER', 'CMAKE_CXX_FLAGS',
             'CMAKE_CXX_FLAGS_RELEASE', 'MHGP11_MARCH')) and '-DMHGP11_COORD_BITS=21' in configure and
             '-DBUILD_TESTING=ON' in configure and '--target' in build_command and
             build_command[build_command.index('--target')+1] == 'mhgp11_full_bench', 'baseline requested flags/target differ')
        pins = dict(binary=bins['mhgp11_full_bench'], cache=bins['CMakeCache.txt'], provenance_sha256=current.hash(provenance_path))
        decoder = hashlib.sha256(b'ehgp.v11.decoder_sources.v1\0')
        for name in ('catalogue_semantic.py', 'full_semantic.py'):
            data = package.read('morsehgp3D_v11/bench/' + name)
            encoded_name = name.encode()
            decoder.update(len(encoded_name).to_bytes(8, 'little')); decoder.update(encoded_name)
            decoder.update(len(data).to_bytes(8, 'little')); decoder.update(data)
        pins['decoder_sha256'] = decoder.hexdigest()
        timings = paired_rows(report, cases, commit, pins)
        need(report['builds'][1]['targeted_qualification_sha256'] == report['qualification_sha256'] and
             report['builds'][1]['targeted_inventory_sha256'] == current.hash(targeted_root + '/bits21/tests.json'),
             'current targeted gate/inventory binding')
        result['paired'] = dict(generation=current_receipt['generation'], closure='targeted_stopped',
            results_sha256=current_receipt['results_sha256'], manifest_sha256=current.manifest_hash,
            requested=81, successful=81, baseline=27, current=54, new_targeted_configs=targeted_counts, timings=timings)
        result['paired']['runtime_matrix_sha256'] = extra[0] if extra else None
        result['limits'] += ['fresh native processes/owners; OS caches are not dropped',
                            'three whole nonground frames from one sequence; K5/u21, unit site weights, same1mm coordinates',
                            'byte comparisons occurred before worker cleanup; missing dumps cannot be decoded again offline',
                            'first session qualifies its own binaries; second session rebuilds and plays targeted gates on its binaries',
                            'baseline cache has recorded hash; comparator flags are recouped from recorded commands and producer checks, not an archived cache text',
                            'native FULL excludes ground/grid preparation, IO/Cloud/Pool, Python decoder, head and projection']
        current.close()
    archive.close()
    package.close()
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--qualification', required=True, type=Path)
    parser.add_argument('--paired', type=Path)
    parser.add_argument('--paired-matrix', type=Path, help='copied input for failed historical650r2 only; defaults to paired/package/paired_bits21_matrix_corrected.json')
    parser.add_argument('--source-package', required=True, type=Path)
    parser.add_argument('--expected-commit', default=SOURCE)
    parser.add_argument('--inspect-failed', action='store_true', help='recoupe a closed failed qualification or paired preflight; remains nonconforming/code1')
    parser.add_argument('--out', type=Path)
    args = parser.parse_args()
    try:
        need(re.fullmatch('[0-9a-f]{40}', args.expected_commit) is not None, 'invalid expected commit')
        result = verify(args.qualification, args.paired, args.source_package, args.expected_commit, args.inspect_failed, args.paired_matrix)
        code = 0 if result['conforming'] else 1
    except (OSError, ValueError, KeyError, TypeError, OverflowError, tarfile.TarError, ET.ParseError) as error:
        result = dict(schema='ehgp.v11.full_captures_review.v1', conforming=False,
                      failure=dict(type=type(error).__name__, message=str(error)))
        code = 1
    rendered = json.dumps(result, indent=2, sort_keys=True, allow_nan=False) + '\n'
    if args.out:
        args.out.write_text(rendered)
    print(rendered, end='')
    return code


if __name__ == '__main__':
    raise SystemExit(main())
