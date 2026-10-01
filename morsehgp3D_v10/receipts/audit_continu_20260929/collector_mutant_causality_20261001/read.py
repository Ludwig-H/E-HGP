"""Hash-first read-only archive reader. Live Python controlflow replay is opt-in."""
import hashlib
import json
from pathlib import Path
import subprocess
import sys

SOURCE_SHA = '2555ba3c31eec86b62f3f11798ecc03a8afb2061ed8c921567dc670c4a892d51'
CASES = ('judge_1', 'signal_11', 'timeout', 'unknown_id')
IDS = tuple(mode + '/' + case for mode in ('normal', 'O') for case in CASES)
BASE_FILES = {'README.md', 'controlflow.py', 'read.py', 'capture.py', 'receipt.json', 'mutations.json',
              'sources/mutants_entrees_cli.py'}
FILES = BASE_FILES | {f'captures/{mode}_{case}.{suffix}' for mode in ('normal', 'O') for case in CASES
                      for suffix in ('stdout', 'stderr')}


def require(condition, message):
    if not condition:
        raise ValueError(message)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def judge_stdout(raw, mode, case):
    obj = json.loads(raw)
    require(obj['case'] == case and obj['optimized'] == (mode == 'O'), 'case/mode')
    require(obj['returncode'] == 0, 'collector returncode')
    require(all(obj[key] == 0 for key in ('native_build_calls', 'native_judge_calls',
                                         'collector_filesystem_writes')), 'scope')
    unknown = case == 'unknown_id'
    require(obj['judges_calls'] == (2 if unknown else 3), 'judge call count')
    events = obj['events']
    require([event['id'] for event in events] ==
            (['temoin_avant', 'temoin_apres', 'bilan'] if unknown else
             ['temoin_avant', 'MA1', 'temoin_apres', 'bilan']), 'event inventory')
    zero = dict.fromkeys(('unit', 'fault', 'produit', 'temoin', 'fils', 'fast'), '0')
    require(events[0]['verdicts'] == zero and events[-2]['verdicts'] == zero and
            events[-2]['build'] == 0, 'witnesses')
    expected_bilan = ('mutants_entrees_cli tues=%d equivalents=0 survivants=0 equivalents_tues=0 '
                      'harnais=0 temoin_apres=ok') % (0 if unknown else 1)
    require(events[-1]['line'] == expected_bilan and expected_bilan in obj['collector_stdout'], 'bilan')
    if not unknown:
        expected_code = {'judge_1': '1', 'signal_11': '-11', 'timeout': 'delai_1500s'}[case]
        expected = dict.fromkeys(('unit', 'fault', 'produit', 'temoin', 'fils'), '0')
        expected['produit'] = expected_code
        require(events[1]['verdicts'] == expected and events[1]['killed_by'] == ['produit'], 'kill model')


def check(root, expected_manifest):
    root = Path(root).resolve()
    manifest = root / 'MANIFEST.sha256'
    require(sha(manifest) == expected_manifest, 'external manifest pin')
    entries = {}
    for line in manifest.read_text().splitlines():
        digest, name = line.split('  ', 1)
        require(len(digest) == 64 and all(c in '0123456789abcdef' for c in digest), 'digest spelling')
        require(name not in entries and name in FILES, 'manifest duplicate/extra')
        entries[name] = digest
    require(set(entries) == FILES, 'manifest inventory')
    actual = {str(path.relative_to(root)) for path in root.rglob('*') if path.is_file()}
    require(actual == FILES | {'MANIFEST.sha256', 'MANIFEST_SHA256'}, 'physical inventory')
    require(not any(path.is_symlink() for path in root.rglob('*')), 'symlink')
    require((root / 'MANIFEST_SHA256').read_text() == expected_manifest + '\n', 'manifest sidecar')
    for name, digest in entries.items():
        require(sha(root / name) == digest, 'file hash ' + name)
    require(entries['sources/mutants_entrees_cli.py'] == SOURCE_SHA, 'source pin')
    receipt = json.loads((root / 'receipt.json').read_text())
    require(receipt['source_before'] == SOURCE_SHA == receipt['source_after'] == receipt['source_snapshot'],
            'source before/after')
    require(receipt['local_transitive_dependencies'] == [], 'local transdeps')
    require([row['id'] for row in receipt['calls']] == list(IDS), 'case inventory')
    for row in receipt['calls']:
        mode, case = row['id'].split('/')
        expected_argv = [receipt['python'], '-B'] + (['-O'] if mode == 'O' else []) + \
                        [receipt['capture_root'] + '/controlflow.py', case]
        require(row['argv'] == expected_argv and row['exit'] == 0, 'argv/exit')
        raw = (root / row['stdout']).read_text()
        require(row['stdout'] == f'captures/{mode}_{case}.stdout' and
                row['stderr'] == f'captures/{mode}_{case}.stderr', 'capture paths')
        require((root / row['stderr']).read_bytes() == b'', 'stderr')
        judge_stdout(raw, mode, case)
    mutations = json.loads((root / 'mutations.json').read_text())
    require([row['id'] for row in mutations] == ['empty_manifest', 'omitted_case', 'wrong_exit'] and
            all(row['exit'] != 0 for row in mutations), 'reader mutation receipts')
    return entries, receipt


def main():
    if len(sys.argv) not in (3, 4) or (len(sys.argv) == 4 and sys.argv[3] != '--replay-controlflow'):
        return 2
    root, expected = Path(sys.argv[1]).resolve(), sys.argv[2]
    entries, receipt = check(root, expected)
    if len(sys.argv) == 4:
        require(sha(Path(receipt['python'])) == receipt['python_sha256'], 'live Python pin')
        for row in receipt['calls']:
            mode, case = row['id'].split('/')
            argv = [receipt['python'], '-B'] + (['-O'] if mode == 'O' else []) + \
                   [str(root / 'controlflow.py'), case]
            result = subprocess.run(argv, capture_output=True, timeout=10)
            require(result.returncode == row['exit'] and result.stderr == b'' and
                    result.stdout == (root / row['stdout']).read_bytes(), 'live controlflow ' + row['id'])
        check(root, expected)
    require(all(sha(root / name) == digest for name, digest in entries.items()), 'after-read hashes')
    print('archive_check_ok cases=8 files=23 readonly=1 replay=' + str(len(sys.argv) == 4))
    return 0


if __name__ == '__main__':
    try:
        raise SystemExit(main())
    except (ValueError, KeyError, OSError, json.JSONDecodeError, subprocess.TimeoutExpired) as error:
        print('REFUS ' + str(error), file=sys.stderr)
        raise SystemExit(2)
