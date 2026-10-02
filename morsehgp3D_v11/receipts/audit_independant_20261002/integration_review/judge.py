"""Rejudge the frozen core build receipt; no compiler or engine run."""
from pathlib import Path
import hashlib
import json
import re

ROOT = Path(__file__).resolve().parent

def require(condition, text):
    if not condition:
        raise RuntimeError(text)


def main():
    before = json.loads((ROOT / 'SOURCE_BEFORE.json').read_text())
    for item in before['files']:
        digest = hashlib.sha256((ROOT / 'sources' / item['path']).read_bytes()).hexdigest()
        require(digest == item['sha256'], item['path'])
    runs = json.loads((ROOT / 'RUN.json').read_text())
    require(len(runs) == 6, 'six captured commands')
    require(all(run['returncode'] == 0 for run in runs), 'captured command failure')
    tests = json.loads((ROOT / 'gates.json').read_text())['tests']
    require(len(tests) == 72, 'registered gate floor')
    log = (ROOT / 'run_04.stdout').read_text()
    done = re.findall(r'\d+/71 Test\s+#\d+:\s+(\S+).*?(Passed|\*\*\*Skipped)', log)
    require(len(done) == 71 and len({name for name, _ in done}) == 71, 'executed inventory')
    passed = [name for name, outcome in done if outcome == 'Passed']
    skipped = [name for name, outcome in done if outcome == '***Skipped']
    require(len(passed) == 70, 'passed inventory')
    require(skipped == ['mhgp11_support_lidar_sentinel'], 'only expected skip')
    require('0 tests failed out of 71' in log, 'terminal CTest result')
    require('mhgp11_mutants_core' not in {name for name, _ in done}, 'long campaign excluded')
    require('mhgp11_mutants_core_manifest' in passed, 'manifest only checked')
    ledger = ROOT / 'SHA256SUMS'
    if ledger.is_file():
        for line in ledger.read_text().splitlines():
            digest, name = line.split(maxsplit=1)
            require(hashlib.sha256((ROOT / name).read_bytes()).hexdigest() == digest, name)
    print(json.dumps({'status': 'PASS', 'registered': 72, 'passed': 70, 'skipped': 1,
                      'excluded_long': 1, 'sources': len(before['files']),
                      'scope': 'Frozen WIP core GCC release only; no FULL, LiDAR, sanitizer or mutant campaign'}, sort_keys=True))

if __name__ == '__main__':
    main()
