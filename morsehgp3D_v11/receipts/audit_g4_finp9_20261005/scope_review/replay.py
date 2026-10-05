#!/usr/bin/env python3
"""Check frozen source anchors and direct P9 archive; never execute a native program."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import tarfile


def need(ok, what):
    if not ok:
        raise ValueError(what)


def digest(raw):
    return hashlib.sha256(raw).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo', required=True)
    parser.add_argument('--archive', required=True)
    args = parser.parse_args()
    scope = json.loads(Path(__file__).with_name('scope.json').read_text())
    pin = scope['pin']
    for source in scope['sources']:
        raw = subprocess.check_output(['git', '-C', args.repo, 'show', pin + ':' + source['path']])
        need(digest(raw) == source['sha256'], source['path'] + ': source hash')
        lines = raw.decode().splitlines()
        for anchor in source['anchors']:
            need(lines[anchor['line'] - 1] == anchor['text'], source['path'] + ': anchor')
    archive = Path(args.archive)
    need(digest(archive.read_bytes()) == scope['observed']['archive_sha256'], 'archive hash')
    with tarfile.open(archive) as tar:
        for test in scope['observed']['tests']:
            raw = tar.extractfile(test['stdout_member']).read()
            need(digest(raw) == test['stdout_sha256'], test['test'] + ': stdout hash')
            out = raw.decode()
            need(test['test'] in out and 'Passed' in out and '100% tests passed, 0 tests failed out of 1' in out,
                 test['test'] + ': direct CTest PASS')
            need('points_vs_python_verdict' not in out, test['test'] + ': business counter unexpectedly present')
            meta = tar.extractfile(test['meta_member']).read().decode()
            need('exit_code=0\n' in meta and 'status=ok\n' in meta, test['test'] + ': direct exit')
    print(json.dumps({'source_files_verified': len(scope['sources']), 'direct_ctest_passes': 4,
                      'business_counter_lines_archived': 0, 'native_runs_by_this_replay': 0,
                      'interpretation': 'source contract and archive review; no rerun of the differential'},
                     sort_keys=True))


if __name__ == '__main__':
    main()
