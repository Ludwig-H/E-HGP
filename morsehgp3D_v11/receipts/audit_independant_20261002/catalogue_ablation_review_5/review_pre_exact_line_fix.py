"""Lecture autonome des copies catalogue3, sans natif ni appel cloud."""
from pathlib import Path
import hashlib
import json
import tarfile
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parent
BASE = 'results/cmd/000_matrice/files/matrix/'


def need(condition, message):
    if not condition:
        raise ValueError(message)


def js(data):
    def pairs(items):
        answer = {}
        for key, value in items:
            need(key not in answer, 'duplicate JSON key')
            answer[key] = value
        return answer

    def reject(value):
        raise ValueError('nonfinite JSON constant ' + value)

    return json.loads(data, object_pairs_hook=pairs, parse_constant=reject)


def main():
    folder = ROOT / 'catalogue3'
    receipt = js((folder / 'receipt.json').read_bytes())
    archive = folder / 'results.tar.gz'
    need(hashlib.sha256(archive.read_bytes()).hexdigest() == receipt['results_sha256'], 'archive SHA')
    need(receipt['commit'] == 'e6fe34cb082f19d0041c829dfb38ea249319ab19', 'source commit')
    need(receipt['generation'] == receipt['closing_generation'] == receipt['observed_after']['lastStartTimestamp']
         and receipt['observed_after']['status'] == 'TERMINATED', 'generation stopped')
    with tarfile.open(archive) as stream:
        data = {member.name: stream.extractfile(member).read()
                for member in stream.getmembers() if member.isfile()}
    for line in data['results/MANIFEST.sha256'].decode().splitlines():
        digest, name = line.split('  ', 1)
        need(hashlib.sha256(data['results/' + name.removeprefix('./')]).hexdigest() == digest, 'manifest SHA')
    need(data[BASE + 'summary.json'] == (folder / 'matrix.json').read_bytes(), 'matrix copy')
    matrix = js(data[BASE + 'summary.json'])
    need(matrix['conforming'] is True and matrix['exit_code'] == 0 and not matrix['signals'], 'qualification')
    selected = passed = 0
    for config in matrix['configurations']:
        if config['status'] == 'absent':
            need(config['name'] == 'clang_release' and config['optional'] is True, 'optional Clang')
            continue
        need(config['status'] == 'ok', 'configuration qualified')
        prefix = BASE + config['name'] + '/'
        inventory = js(data[prefix + 'tests.json'])
        names = [row['name'] for row in inventory]
        tests = ET.fromstring(data[prefix + 'junit.xml']).findall('testcase')
        need(bool(names) and len(names) == len(set(names)) == len(tests), 'unique nonempty inventory')
        need(set(names) == {test.get('name') for test in tests}, 'JUnit names')
        need(all(test.get('status') == 'run' and test.find('failure') is None and test.find('skipped') is None
                 for test in tests), 'all gates executed and passed')
        selected += len(names)
        passed += len(tests)
    need(selected == passed == 960, '960 selected/passed, including repetitions')
    provenance = js(data[BASE + 'gcc_release/build_provenance.json'])
    for row in provenance['files']:
        if 'text' in row:
            content = row['text'].encode()
            need(len(content) == row['size'] and hashlib.sha256(content).hexdigest() == row['sha256'], 'flags/cache SHA')
    source = data['results/cmd/001_catalogue/files/catalogue.json']
    need(source == (folder / 'catalogue.json').read_bytes(), 'benchmark copy')
    report = js(source)
    need(report['executable_sha256'] == next(row['sha256'] for row in provenance['files']
                                            if row['path'] == 'mhgp11_catalogue_bench'), 'measured binary identity')
    need(report['qualification_sha256'] == hashlib.sha256(data[BASE + 'summary.json']).hexdigest(), 'qualified summary')
    inputs = (folder / 'inputs.json').read_bytes()
    need(report['manifest'] == js(inputs) and report['manifest_sha256'] == hashlib.sha256(inputs).hexdigest(), 'input manifest')
    expected = {(case['name'], k, r) for case in report['manifest']['cases'] for k in (5, 10) for r in range(3)}
    unit = lambda row: (row['case'], row['kmax'], row['repetition'])
    observed, omitted = [unit(row) for row in report['runs']], [unit(row) for row in report['not_run']]
    need(len(observed) == 13 and len(omitted) == 23 and len(set(observed + omitted)) == 36
         and set(observed + omitted) == expected, '13 attempts +23 omissions exactly cover schedule')
    need(report['complete'] is True and report['full_schedule_completed'] is False
         and report['all_attempted_ok'] is False and report['full_contract'] == 'not_testable_missing_tower', 'benchmark failure scope')
    need(report['leaf_size'] == 16, 'leaf16 report')
    prior = js((ROOT / 'comparison_catalogue2/catalogue.json').read_bytes())
    need(report['manifest'] == prior['manifest'] and report['manifest_sha256'] == prior['manifest_sha256'], 'same exact inputs')
    need(report['executable_sha256'] == prior['executable_sha256'], 'same compiled benchmark binary')
    complete = [row for row in report['runs'] if row['status'] == 'ok']
    timeouts = [row for row in report['runs'] if row['status'] == 'timeout']
    need(len(complete) == 7 and len(timeouts) == 6, '7 successes and6 timeouts')
    for row in report['runs']:
        need(row['argv'][5:] == ['16', '256', '0', '4294967295', '8589934592'], 'only leaf size16')
        case = next(case for case in report['manifest']['cases'] if case['name'] == row['case'])
        need(row['count'] == case['count'] and row['whole_input'] is True, 'whole declared input')
        if row['status'] == 'ok':
            cloud, catalogue, exit_event = row['events']
            need(cloud['points'] == cloud['sites'] == row['count'] and catalogue['coord_bits'] == 18
                 and catalogue['kmax'] == row['kmax'] and catalogue['status'] == exit_event['status'] == 'ok', 'complete phases')
            need(row['catalogue_ms'] == catalogue['wall_ns'] / 1e6, 'exact completed API duration')
        else:
            need(row['status'] == 'timeout' and row['exit_code'] is None and row['timeout_seconds'] == 30
                 and 'canonical_sha256' not in row, 'no completed API/hash fabricated at timeout')
    small = [row for row in complete if row['case'] == 'uniform_u18_n8000']
    need([row['repetition'] for row in small] == [0, 1, 2], 'three successful8k repetitions')
    need([row['events'][1]['wall_ns'] for row in small] == [8239569411, 8240274530, 8210329939], 'exact8k durations')
    old = prior['runs'][0]
    for row in small:
        need(row['canonical_sha256'] == old['canonical_sha256'] and row['canonical_bytes'] == old['canonical_bytes'], 'same declared canonical output')
        need(all(row['events'][1][key] == old['events'][1][key] for key in ['balls', 'levels', 'incidences']), 'same catalogue quantities')
    observations = js((ROOT / 'observations.json').read_bytes())
    mutations = observations['catalogue_mutants']
    need(len(mutations) == len({row['id'] for row in mutations}) == 9
         and all(row['verdict'] == 'TUE' and row['cause'] == 'code' for row in mutations), 'nine causal code verdicts')
    log = data[BASE + 'mutants/LastTest.log'].decode()
    need('mutants_ok module=catalogue mutants=9 tues=9 dont_signal=0 dont_delai=0 dont_construction=0 plancher=9' in log, 'native mutant summary')
    release = data[BASE + 'gcc_release/LastTest.log'].decode()
    need(release.count('projection_contracts_ok faits=5') == 2, 'reference projection normal/-O')
    need(receipt['status'] == 'failed_remote' and receipt['worker_exit_code'] == 1, 'overall incomplete benchmark preserved')
    print('review_ok source=e6fe34cb0 matrice=960/960 mutants_catalogue=9/code leaf16=7_succes+6_timeouts omissions=23 sorties8k_identiques FULL=absent')
    return 0


if __name__ == '__main__':
    try:
        raise SystemExit(main())
    except (ValueError, KeyError, TypeError, OSError, tarfile.TarError, ET.ParseError) as error:
        print('review_refuse:', error)
        raise SystemExit(1)
