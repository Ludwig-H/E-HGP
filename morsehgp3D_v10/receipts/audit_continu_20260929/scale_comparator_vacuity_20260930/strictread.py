import csv
import datetime
import hashlib
import json
import pathlib
import subprocess
import sys

BASE = pathlib.Path(__file__).resolve().parent
OLD = '/tmp/mhgp10-compare-scale-causal-20260930.I620fdij'
MANIFEST = 'f1755305ea07e21b9017b6eeddd6613c3bc0735361a3d0c4f826e01fe4947bbc'
COMPARE = '6b5364272a3f412afb3a5ffbf46e4912ec4dbf99e8caeecace744f0c2c3aac63'
REDECIDE = '0d4d3a9f4fa042403257b666554479b14ade8b1e3035ad03d478a2e074ddc07e'
VERIFY_LOG = '4ad8154c97f4fe52cd5886139a0cd3437f0648a95cda4226481c485c58f969f4'
CASES = {'positive': (1, 1, True), 'empty_new': (1, 0, False),
         'truncated_new': (2, 1, False), 'extra_new': (1, 2, False)}
FILES = {'capture.py', 'compare_scale.py', 'redecide_refusion.py', 'receipt.json', 'seal.py', 'verify.py',
         'subset_provenance.json', 'mut_scale_python3.log', 'mut_scale_python3.10.log'} | {
         case + '/' + name for case in CASES for name in ['old.csv', 'new.csv', 'new.csv.calls.jsonl',
                                                        'normal.stdout', 'normal.stderr', 'optimized.stdout', 'optimized.stderr']}

def need(test, reason):
    if not test:
        raise RuntimeError(reason)

def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def utc():
    return datetime.datetime.now(datetime.timezone.utc).isoformat()

def receipt_check(record):
    need(record['schema'] == 'causal_compare_scale_static_source_v1', 'schema')
    need(record['gcp_used'] is False and record['native_calls'] == 0, 'scope')
    need(record['source_before'] == record['source_after'] == record['source_copy'] == COMPARE, 'source pins')
    need(record['redecide_source_copy'] == REDECIDE, 'redecide pin')
    need(isinstance(record['tests'], list) and len(record['tests']) == 8, 'cases inventory length')
    seen = set()
    for test in record['tests']:
        need(set(test) == {'argv', 'case', 'code', 'expected_valid', 'mode', 'old_count', 'new_count'}, 'test keys')
        case, mode = test['case'], test['mode']
        need(case in CASES and mode in ('normal', 'optimized') and (case, mode) not in seen, 'cases inventory')
        seen.add((case, mode))
        need(type(test['code']) is int and test['code'] == 0, 'code exact')
        a, b, valid = CASES[case]
        need(type(test['expected_valid']) is bool and test['expected_valid'] == valid and
             type(test['old_count']) is int and type(test['new_count']) is int and
             (test['old_count'], test['new_count']) == (a, b), 'case oracle')
        argv = ['/home/codespace/.python/current/bin/python3', '-B'] + (['-O'] if mode == 'optimized' else []) + [
            OLD + '/compare_scale.py', OLD + '/' + case + '/old.csv', OLD + '/' + case + '/new.csv']
        need(test['argv'] == argv, 'argv exact')
    need(seen == {(c, m) for c in CASES for m in ('normal', 'optimized')}, 'cases inventory set')

def archive_check(root):
    manifest = root / 'SHA256SUMS'
    need(sha(manifest) == MANIFEST, 'manifest SHA')
    lines = manifest.read_text().splitlines()
    need(len(lines) == 37, 'manifest inventory37')
    entries = {}
    for line in lines:
        expected, relative = line.split('  ', 1)
        need(relative in FILES and relative not in entries, 'manifest inventory exact')
        path = root / relative
        need(path.is_file() and not path.is_symlink() and sha(path) == expected, 'file SHA ' + relative)
        entries[relative] = expected
    need(set(entries) == FILES, 'manifest full set')
    actual = {p.relative_to(root).as_posix() for p in root.rglob('*') if p.is_file() or p.is_symlink()}
    need(actual == FILES | {'SHA256SUMS', 'verify_normal.txt', 'verify_optimized.txt'}, 'extra or missing file')
    for name in ['verify_normal.txt', 'verify_optimized.txt']:
        need(sha(root / name) == VERIFY_LOG, 'archive-only verification log')
    need(sha(root / 'compare_scale.py') == COMPARE and sha(root / 'redecide_refusion.py') == REDECIDE, 'source copies')
    record = json.loads((root / 'receipt.json').read_text())
    receipt_check(record)
    for test in record['tests']:
        folder = root / test['case']
        with (folder / 'old.csv').open(newline='') as f:
            old = list(csv.DictReader(f))
        with (folder / 'new.csv').open(newline='') as f:
            new = list(csv.DictReader(f))
        need((len(old), len(new), old == new and bool(old)) == CASES[test['case']], 'fixture oracle')
        out = json.loads((folder / (test['mode'] + '.stdout')).read_text())
        need(out['lignes_ancien'] == len(old) and out['lignes_nouveau'] == len(new) and
             out['appels'] == 2 * len(new) and out['ecarts'] == [] and out['entetes_egaux'] is True and
             len(out['lignes_coherentes_avec_json_natifs']) == len(new) and
             all(c[3] is True for c in out['lignes_coherentes_avec_json_natifs']), 'captured semantic result')
        need((folder / (test['mode'] + '.stderr')).read_bytes() == b'', 'captured stderr')
    return record

def main():
    root = pathlib.Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else BASE / 'original'
    record = archive_check(root)
    if '--replay' in sys.argv:
        started = utc()
        runs = []
        for test in record['tests']:
            case, mode = test['case'], test['mode']
            argv = [sys.executable, '-B'] + (['-O'] if mode == 'optimized' else []) + [
                str(root / 'compare_scale.py'), str(root / case / 'old.csv'), str(root / case / 'new.csv')]
            r = subprocess.run(argv, capture_output=True, text=True, timeout=10)
            need(type(r.returncode) is int and r.returncode == 0 and not r.stderr, 'LIVE replay code/stderr')
            need(json.loads(r.stdout) == json.loads((root / case / (mode + '.stdout')).read_text()), 'LIVE replay semantics')
            runs.append({'argv': argv, 'code': r.returncode, 'case': case, 'mode': mode,
                         'stdout': json.loads(r.stdout), 'stderr': r.stderr})
        archive_check(root)
        (BASE / 'LIVE_REPLAY.json').write_text(json.dumps({'started_utc': started, 'ended_utc': utc(),
            'timeout_each_s': 10, 'commands': runs, 'source_sha': COMPARE, 'manifest_sha': MANIFEST,
            'scope': 'isolated frozen snapshot replay, not current product validation', 'gcp_used': False,
            'native_calls': 0}, indent=2, sort_keys=True) + '\n')
        print('PASS SNAPSHOT_LIVE_REPLAY commands=8 positive=2 false_acceptances=6 before_after_stable=true')
    print('PASS STRICT_ARCHIVE inventory=37 plus_manifest_and_two_archive_logs source_pins=true')

if __name__ == '__main__':
    main()
