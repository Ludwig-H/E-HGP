import datetime
import hashlib
import importlib.util
import json
import pathlib
import re
import subprocess
import sys

sys.dont_write_bytecode = True
ROOT = pathlib.Path(__file__).resolve().parent
CASES = ['positive', 'empty_new', 'truncated_new', 'extra_new']
INNER = {'capture.py', 'compare_scale.py', 'redecide_refusion.py', 'receipt.json', 'seal.py', 'verify.py',
         'subset_provenance.json', 'mut_scale_python3.log', 'mut_scale_python3.10.log',
         'SHA256SUMS', 'verify_normal.txt', 'verify_optimized.txt'} | {
         case + '/' + name for case in CASES for name in ['old.csv', 'new.csv', 'new.csv.calls.jsonl',
                                                        'normal.stdout', 'normal.stderr', 'optimized.stdout', 'optimized.stderr']}
EXPECTED = {'original/' + p for p in INNER} | {'strictread.py', 'verify.py', 'CAPTURE_LIVE.json', 'MUTATIONS.json', 'README.md'}

def need(test, reason):
    if not test:
        raise RuntimeError(reason)

def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def main():
    need(len(sys.argv) == 2 and re.fullmatch('[0-9a-f]{64}', sys.argv[1]), 'explicit authoritative manifest SHA required')
    need(sha(ROOT / 'SHA256SUMS') == sys.argv[1], 'outer manifest pin')
    listed = {}
    for line in (ROOT / 'SHA256SUMS').read_text().splitlines():
        expected, relative = line.split('  ', 1)
        need(relative in EXPECTED and relative not in listed, 'outer inventory')
        p = ROOT / relative
        need(not p.is_symlink() and sha(p) == expected, 'outer file hash ' + relative)
        listed[relative] = expected
    actual = {p.relative_to(ROOT).as_posix() for p in ROOT.rglob('*') if p.is_file() or p.is_symlink()}
    need(len(listed) == 45 and set(listed) == EXPECTED and actual == EXPECTED | {'SHA256SUMS'}, 'complete45/no extras')
    before = {relative: sha(ROOT / relative) for relative in actual}
    spec = importlib.util.spec_from_file_location('strict_readonly_import', ROOT / 'strictread.py')
    reader = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(reader)  # import only; never call its mutable --replay CLI
    record = reader.archive_check(ROOT / 'original')
    archived = json.loads((ROOT / 'CAPTURE_LIVE.json').read_text())
    need(len(archived['runs']) == 8 and archived['native_calls'] == 0 and archived['gcp_used'] is False, 'capture scope')
    started = datetime.datetime.now(datetime.timezone.utc).isoformat()
    for i, test in enumerate(record['tests']):
        case, mode = test['case'], test['mode']
        argv = [sys.executable, '-B'] + (['-O'] if mode == 'optimized' else []) + [
            str(ROOT / 'original/compare_scale.py'), str(ROOT / 'original' / case / 'old.csv'),
            str(ROOT / 'original' / case / 'new.csv')]
        r = subprocess.run(argv, capture_output=True, text=True, timeout=10)
        need(r.returncode == 0 and r.stderr == '', 'LIVE code/stderr')
        out = json.loads(r.stdout)
        reference = json.loads((ROOT / 'original' / case / (mode + '.stdout')).read_text())
        need(out == reference == archived['runs'][i]['stdout'] and archived['runs'][i]['code'] == 0 and
             archived['runs'][i]['case'] == case and archived['runs'][i]['mode'] == mode, 'LIVE semantic replay')
    reader.archive_check(ROOT / 'original')
    after = {relative: sha(ROOT / relative) for relative in actual}
    need(before == after and actual == {p.relative_to(ROOT).as_posix() for p in ROOT.rglob('*') if p.is_file() or p.is_symlink()}, 'verification wrote into archive')
    print(json.dumps({'status': 'PASS_READONLY_SNAPSHOT_REPLAY', 'started_utc': started,
                      'ended_utc': datetime.datetime.now(datetime.timezone.utc).isoformat(),
                      'outer_manifest_sha': sys.argv[1], 'files': 45, 'CLI_replays': 8,
                      'false_acceptances': 6, 'archive_before_after_equal': True,
                      'native_calls': 0, 'gcp_used': False}, sort_keys=True))

if __name__ == '__main__':
    main()
