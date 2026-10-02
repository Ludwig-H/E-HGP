"""Relecture pure de copies closes : aucun build, CTest produit, signal ou cloud."""
from pathlib import Path
import collections
import hashlib
import json
import re
import tarfile
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parent
SOURCE = ROOT / 'sources/morsehgp3D_v11'


def require(condition, message):
    if not condition:
        raise ValueError(message)


def main():
    observations = json.loads((ROOT / 'observations.json').read_text())
    for session in observations['sessions']:
        number = session['reprise']
        receipt_dir = SOURCE / 'receipts/developpement_20261002' / ('reprise%d' % number)
        receipt = json.loads((receipt_dir / 'receipt.json').read_text())
        archive = (receipt_dir / 'results.tar.gz').read_bytes()
        require(hashlib.sha256(archive).hexdigest() == receipt['results_sha256'], 'archive SHA')
        with tarfile.open(receipt_dir / 'results.tar.gz') as stream:
            data = {member.name: stream.extractfile(member).read()
                    for member in stream.getmembers() if member.isfile()}
        for line in data['results/MANIFEST.sha256'].decode().splitlines():
            digest, path = line.split('  ', 1)
            key = 'results/' + path.removeprefix('./')
            require(hashlib.sha256(data[key]).hexdigest() == digest, 'manifest ' + key)
        matrix = json.loads((receipt_dir / 'matrix.json').read_text())
        base = 'results/cmd/000_matrice/files/matrix/'
        require(data[base + 'summary.json'] == (receipt_dir / 'matrix.json').read_bytes(), 'summary bytes')
        for config in matrix['configurations']:
            if config['status'] == 'absent':
                continue
            directory = base + config['name'] + '/'
            selected = json.loads(data[directory + 'tests.json'])
            names = [row['name'] for row in selected]
            tests = ET.fromstring(data[directory + 'junit.xml']).findall('testcase')
            require(len(names) == len(set(names)) == len(tests), 'unique inventory')
            require(set(names) == {row.get('name') for row in tests}, 'inventory/JUnit')
        require(matrix['conforming'] is (number == 3), 'historical failure preserved')
    final = observations['sessions'][2]
    require([row['tests']['selected'] for row in final['configurations']
             if row.get('tests')] == [205, 9, 130, 130, 130, 130, 131, 2], 'counts')
    rows = final['mutant_verdicts']
    require(len(rows) == len({row['id'] for row in rows}) == 103, '103 unique mutants')
    require(all(row['verdict'] == 'TUE' for row in rows), 'all mutant verdicts')
    require(collections.Counter(row['detail'] for row in rows)
            == {'code': 98, 'ligne': 3, 'construction': 2}, 'mutant causes')
    for module in ('core', 'num', 'cloud'):
        manifest = json.loads((SOURCE / 'tests/mutants' / (module + '.json')).read_text())
        ids = {row['id'] for row in manifest['mutants']}
        require(len(ids) == manifest['plancher'], 'manifest floor ' + module)
        require(ids <= {row['id'] for row in rows}, 'manifest IDs ' + module)
    relationships = final['inventory_relationships']
    require(len(relationships['Release_minus_ASan']) == 75
            and relationships['Release_minus_ASan_all_reference'], '75 Python reference gates')
    print('review_ok captures=3 echecs_conserves=2 release=205 base=130 mutants=103 juges=101 construction=2')
    return 0


if __name__ == '__main__':
    try:
        raise SystemExit(main())
    except (ValueError, KeyError, TypeError, OSError) as error:
        print('review_refuse:', error)
        raise SystemExit(1)
