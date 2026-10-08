"""Contrelecture de sources Git et de traces existantes ; aucun binaire exécuté."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess


def need(ok, message):
    if not ok:
        raise ValueError(message)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo', required=True)
    parser.add_argument('--evidence', help='copie extérieure des trois journaux ; facultative')
    args = parser.parse_args()
    capture = json.loads(Path(__file__).with_name('capture.json').read_text())
    sources = {}
    for name, expected in capture['sources'].items():
        data = subprocess.check_output(['git', '-C', args.repo, 'show',
                                        capture['commit'] + ':morsehgp3D_v12/' + name])
        need(sha(data) == expected, 'source ' + name)
        sources[name] = data.decode()
    test = sources['tests/io/sha256_test.cpp']
    lengths = re.search(r'for \(std::size_t length : \{([^}]+)\}', test).group(1)
    need([int(x.strip().rstrip('u')) for x in lengths.split(',')] ==
         [0, 1, 55, 56, 63, 64, 65, 127, 128, 129, 1000, 4096, 4100], 'longueurs')
    need('MHGP12_TEST(sha256_voies, 136)' in test, 'plancher')
    need('for (std::size_t chunk : {1u, 63u, 64u, 65u, 1000u})' in test, 'decoupages')
    need(4 + 13 * 5 * 2 + 1 + 1 == 136, 'decompte statique')
    entries = json.loads(sources['tests/mutants/io.json'])['mutants'][-4:]
    need([m['id'] for m in entries] == [m['id'] for m in capture['mutants']], 'mutants')
    for mutant in entries:
        need(sources['src/io/sha256.cpp'].count(mutant['cherche']) == 1, mutant['id'])
    need(capture['live_sources_equal_git'] and capture['logs_stable'], 'capture stable')
    need(capture['fast_counts'] == {'passed': 680, 'skipped': 1, 'failed': 0}, 'fast')
    gate = capture['gates']['mhgp12_io_unit_sha256_voies']
    need(gate['passed'] and 'sha256_voies : voie materielle presente\n' in gate['output'], 'voie observee')
    need('test sha256_voies controles=136 echecs=0 plancher=136\n' in gate['output'], 'controles observes')
    if args.evidence:
        raw = {}
        for name, item in capture['logs'].items():
            data = (Path(args.evidence) / (name + '.txt')).read_bytes()
            need(sha(data) == item['sha256'] and len(data) == item['bytes'], 'journal ' + name)
            raw[name] = data.decode()
        for name, item in capture['gates'].items():
            block = re.search(r'^\d+/\d+ Testing: ' + name +
                              r'\n.*?(?=\n\d+/\d+ Testing:|\Z)', raw['last_test'], re.M | re.S).group()
            need(sha(block.encode()) == item['block_sha256'], 'bloc ' + name)
            need(item['output'] in block and '\nTest Passed.\n' in block, 'sortie ' + name)
    print(json.dumps({'source_pin': capture['commit'], 'hardware_trace': True,
                      'checks': 136, 'lengths': 13, 'chunks': 5,
                      'fast': capture['fast_counts'], 'mutant_locations': 4,
                      'mutant_runs_observed': False, 'timing_verified': False,
                      'native_calls': 0}, sort_keys=True))


if __name__ == '__main__':
    main()
