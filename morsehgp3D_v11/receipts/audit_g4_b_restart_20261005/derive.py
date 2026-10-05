#!/usr/bin/env python3
"""Ensembles de reprise exacts depuis le résumé sûr de la capsule B fermée."""
import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def describe(names):
    ordered = sorted(names)
    modules = sorted({name.split('_')[1] for name in ordered})
    return {'count': len(ordered), 'tests': ordered,
            'by_module': {module: [name for name in ordered if name.split('_')[1] == module]
                          for module in modules},
            'module_counts': dict(sorted(Counter(name.split('_')[1] for name in ordered).items()))}


def derive(summary_path):
    raw = summary_path.read_bytes()
    data = json.loads(raw)
    source = 'b319efc8477fec234afc0b31e86f8a43e3023641'
    require(data['source_commit'] == source, 'wrong source pin')
    session = data['sessions'][0]
    require(session['session'] == 'v11.20261005.claudequalb', 'wrong session')
    configurations = {entry['name']: entry for entry in session['configurations']}
    asan = set(configurations['gcc_asan_ubsan']['missing_tests'])
    tsan = set(configurations['gcc_tsan']['missing_tests'])
    require(len(asan) == 50 and len(tsan) == 65, 'unexpected missing counts')
    require(asan <= tsan, 'unexpected inclusion relation')
    require(all(not entry['failed_tests'] for entry in configurations.values()),
            'failed results must be addressed separately')
    return {'schema': 'ehgp.v11.audit.g4_b_restart_sets.v1',
            'source_commit': source, 'session': session['session'],
            'input_summary_sha256': hashlib.sha256(raw).hexdigest(),
            'archive_sha256': session['archive_sha256'],
            'gcc_asan_ubsan': describe(asan), 'gcc_tsan': describe(tsan),
            'union': describe(asan | tsan), 'intersection': describe(asan & tsan),
            'asan_only': describe(asan - tsan), 'tsan_only': describe(tsan - asan),
            'native_runs': 0, 'gcp_calls': 0}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--summary', type=Path, required=True)
    args = parser.parse_args()
    here = Path(__file__).resolve().parent
    result = derive(args.summary)
    expected = json.loads((here / 'restart_sets.json').read_bytes())
    require(result == expected, 'derivation differs from pinned restart sets')
    for line in (here / 'SHA256SUMS').read_text().splitlines():
        digest, filename = line.split('  ', 1)
        require(hashlib.sha256((here / filename).read_bytes()).hexdigest() == digest,
                'addendum SHA: ' + filename)
    print('restart_sets_verdict conforme ASan50 TSan65 union65 intersection50 TSan_seul15 ASan_seul0 sourceb319')


if __name__ == '__main__':
    main()
