"""Lecture autonome des copies catalogue2, sans natif ni appel cloud."""
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
    folder = ROOT / 'catalogue2'
    receipt = js((folder / 'receipt.json').read_bytes())
    archive = folder / 'results.tar.gz'
    need(hashlib.sha256(archive.read_bytes()).hexdigest() == receipt['results_sha256'], 'archive SHA')
    need(receipt['commit'] == 'f391bf13e1a9a982025bde86fc9219b5b7430afc', 'source commit')
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
    need(selected == passed == 948, '948 selected/passed, including repetitions')
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
    need(len(observed) == 7 and len(omitted) == 29 and len(set(observed + omitted)) == 36
         and set(observed + omitted) == expected, '7 attempts +29 omissions exactly cover schedule')
    need(report['complete'] is True and report['full_schedule_completed'] is False
         and report['all_attempted_ok'] is False and report['full_contract'] == 'not_testable_missing_tower', 'benchmark failure scope')
    first = report['runs'][0]
    need(unit(first) == ('uniform_u18_n8000', 5, 0) and first['status'] == 'ok' and first['count'] == 8000, 'one complete success')
    cloud, catalogue, exit_event = first['events']
    need(cloud['points'] == cloud['sites'] == 8000 and catalogue['coord_bits'] == 18 and catalogue['kmax'] == 5, 'whole input/profile')
    need(catalogue['wall_ns'] == 15478187473 and first['catalogue_ms'] == catalogue['wall_ns'] / 1e6, 'exact API duration')
    need((catalogue['balls'], catalogue['incidences'], catalogue['peak_reserved_bytes'])
         == (597998, 2895136, 133416208), 'counts and reserved peak')
    need(all(row['status'] == 'timeout' and row['exit_code'] is None and row['timeout_seconds'] == 30
             and 'canonical_sha256' not in row for row in report['runs'][1:]), 'six timeouts preserved')
    observations = js((ROOT / 'observations.json').read_bytes())
    mutants = observations['catalogue_mutant_verdicts']
    need(len(mutants) == len({row['id'] for row in mutants}) == 8
         and all(row['verdict'] == 'TUE' and row['cause'] == 'code' for row in mutants), 'eight causal code verdicts')
    log = data[BASE + 'mutants/LastTest.log'].decode()
    need('mutants_ok module=catalogue mutants=8 tues=8 dont_signal=0 dont_delai=0 dont_construction=0 plancher=8' in log,
         'native mutant summary')
    need(receipt['status'] == 'failed_remote' and receipt['worker_exit_code'] == 1, 'overall failure retained')
    print('review_ok source=f391bf13e matrice=948/948 mutants_catalogue=8/code banc=1_succes+6_timeouts omissions=29 FULL=absent')
    return 0


if __name__ == '__main__':
    try:
        raise SystemExit(main())
    except (ValueError, KeyError, TypeError, OSError, tarfile.TarError, ET.ParseError) as error:
        print('review_refuse:', error)
        raise SystemExit(1)
