"""Lecteur autonome des seules copies : aucun binaire, CTest produit ou cloud execute."""
import hashlib
import json
from pathlib import Path
import tarfile
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parent


def require(condition, message):
    if not condition:
        raise ValueError(message)


def strict_json(data):
    def pairs(items):
        result = {}
        for name, value in items:
            require(name not in result, 'duplicate JSON key')
            result[name] = value
        return result

    def nonfinite(word):
        raise ValueError('nonfinite JSON constant ' + word)

    return json.loads(data, object_pairs_hook=pairs, parse_constant=nonfinite)


def main():
    for entry in strict_json((ROOT / 'sources_before.json').read_bytes())['files']:
        content = (ROOT / 'sources' / entry['path']).read_bytes()
        require(hashlib.sha256(content).hexdigest() == entry['sha256'], 'frozen source SHA')
    capture = ROOT / 'developer_receipts_provisional/catalogue1'
    receipt = strict_json((capture / 'receipt.json').read_bytes())
    archive = capture / 'results.tar.gz'
    require(hashlib.sha256(archive.read_bytes()).hexdigest() == receipt['results_sha256'], 'archive SHA')
    require(receipt['generation'] == receipt['closing_generation'] ==
            receipt['observed_after']['lastStartTimestamp'], 'generation')
    require(receipt['observed_after']['status'] == 'TERMINATED' and
            receipt['targeted_shutdown_certified'] is True, 'closed session declaration')
    with tarfile.open(archive) as stream:
        data = {member.name: stream.extractfile(member).read()
                for member in stream.getmembers() if member.isfile()}
    for line in data['results/MANIFEST.sha256'].decode().splitlines():
        digest, path = line.split('  ', 1)
        require(hashlib.sha256(data['results/' + path.removeprefix('./')]).hexdigest() == digest, 'manifest SHA')
    base = 'results/cmd/000_matrice/files/matrix/'
    require(data[base + 'summary.json'] == (capture / 'matrix.json').read_bytes(), 'matrix bytes')
    matrix = strict_json(data[base + 'summary.json'])
    selected_total = passed_total = 0
    for config in matrix['configurations']:
        if config['status'] == 'absent':
            require(config['name'] == 'clang_release', 'only optional Clang absent')
            continue
        prefix = base + config['name'] + '/'
        selected = strict_json(data[prefix + 'tests.json'])
        names = [row['name'] for row in selected]
        tests = ET.fromstring(data[prefix + 'junit.xml']).findall('testcase')
        require(bool(names) and len(names) == len(set(names)) == len(tests), 'nonvacuous unique inventory')
        require(set(names) == {test.get('name') for test in tests}, 'inventory/JUnit')
        passed = sum(test.get('status') == 'run' and test.find('failure') is None and
                     test.find('skipped') is None for test in tests)
        require(passed == config['tests']['passed'] and len(names) == config['tests']['selected'], 'gate counts')
        selected_total += len(names)
        passed_total += passed
        provenance = strict_json(data[prefix + 'build_provenance.json'])
        require(provenance['complete'] is True and not provenance['errors'], 'build provenance errors')
        files = provenance['files']
        require(len(files) == len({row['path'] for row in files}), 'provenance unique names')
        for row in files:
            if 'text' in row:
                content = row['text'].encode()
                require(len(content) == row['size'] and hashlib.sha256(content).hexdigest() == row['sha256'],
                        'effective cache/flags text SHA')
        if config['name'] != 'style':
            require({'mhgp11_catalogue_bench', 'libmhgp11.a'} <= {row['path'] for row in files}, 'binary hashes')
    require((passed_total, selected_total) == (947, 948), 'actual campaign counts')
    require(matrix['conforming'] is False and matrix['exit_code'] == 1, 'failure preserved')
    log = data[base + 'mutants/LastTest.log'].decode(errors='replace')
    require('TEMOIN ROUGE module=catalogue : aucun mutant juge' in log, 'red witness preserved')
    require('bench/catalogue_probe.cpp' in log, 'missing clone source preserved')
    require('results/cmd/001_catalogue/files/catalogue.json' not in data, 'no fabricated benchmark')
    print('review_ok source=f391bf13e campagne=ECHEC commit=643fe47d7 portes=947/948 mutants_catalogue_juges=0 banc=0/36')
    return 0


if __name__ == '__main__':
    try:
        raise SystemExit(main())
    except (ValueError, KeyError, TypeError, OSError, tarfile.TarError, ET.ParseError) as error:
        print('review_refuse:', error)
        raise SystemExit(1)
