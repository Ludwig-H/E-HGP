"""Small pinned provenance check; no engine build, large allocation, GPU or GCP."""
import argparse, hashlib, json, subprocess, sys, tempfile
from pathlib import Path

REV = '4b7d7042226e4299e57e1257f4034d45d82c8b77'
BASE = 'morsehgp3D_v10/'
PROOF = 'receipts/development_frontier_precision_20260930/'
R2 = ['src/sched/pool.cpp', 'src/head/head.cpp', 'src/points/dendrogram.cpp',
      'cli/mhgp10_catalogue.cpp', 'cli/mhgp10_tower.cpp', 'cli/mhgp10_cluster.cpp']

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--repo', type=Path, required=True)
    ap.add_argument('--out', type=Path, required=True)
    args = ap.parse_args()
    if args.out.exists():
        raise RuntimeError('fresh output required')
    def blob(path, rev=REV):
        return subprocess.check_output(['git', 'show', rev + ':' + BASE + path], cwd=args.repo)
    def digest(b): return hashlib.sha256(b).hexdigest()
    sources = {}
    def pinned(path):
        b = blob(path)
        sources[path] = digest(b)
        return b
    unchanged = []
    for path in R2:
        now = pinned(path)
        before = blob(path, '777406b82')
        unchanged.append({'path': path, 'sha256': digest(now), 'equal_to_777406b82': now == before})
    closure = []
    for path in ['qualification/execution.json', 'grid32_cmake/receipt.json']:
        d = json.loads(pinned(PROOF + path))
        differences = []
        for f, expected in d['source_before'].items():
            actual = digest(pinned(f))
            if actual != expected:
                differences.append({'path': f, 'captured_sha256': expected, 'commit_sha256': actual})
        closure.append({'receipt': path, 'files': len(d['source_before']),
                        'before_after_equal': d['source_before'] == d['source_after'],
                        'matches_commit': len(d['source_before']) - len(differences),
                        'differences': differences,
                        'commands': [{'name': c['name'], 'returncode': c['returncode'],
                                      'argv': c['argv']} for c in d['commands']]})
    ledger = pinned(PROOF + 'SHA256SUMS')
    lines = ledger.decode().splitlines()
    bad = []
    for line in lines:
        h, f = line.split('  ', 1)
        if digest(blob(PROOF + f)) != h:
            bad.append(f)
    tests = []
    with tempfile.TemporaryDirectory(prefix='mhgp10-cover-band-provenance-') as td:
        root = Path(td)
        for f in ['bench/frontier/cover_band.py', 'tests/points/test_cover_band.py']:
            dest = root / f
            dest.parent.mkdir(parents=True, exist_ok=True)
            dest.write_bytes(pinned(f))
        for optimize in (0, 1):
            argv = [sys.executable, '-B'] + (['-O'] if optimize else []) + [str(root / 'tests/points/test_cover_band.py')]
            done = subprocess.run(argv, capture_output=True, text=True, timeout=10)
            tests.append({'optimize_flag': optimize, 'returncode': done.returncode,
                          'stdout': done.stdout, 'stderr': done.stderr})
    ok = (all(x['equal_to_777406b82'] for x in unchanged) and not bad and
          len(lines) == 105 and all(x['before_after_equal'] for x in closure) and
          closure[0]['matches_commit'] == 50 and closure[0]['files'] == 51 and
          [x['path'] for x in closure[0]['differences']] == ['CMakeLists.txt'] and
          closure[1]['matches_commit'] == closure[1]['files'] == 78 and
          all(x['returncode'] == 0 and 'Ran 5 tests' in x['stderr'] and 'OK' in x['stderr'] for x in tests))
    report = {'status': 'PASS' if ok else 'FAIL', 'source_commit': REV,
              'comparison_commit': '777406b82', 'sources_sha256': sources,
              'unchanged_r2_product_sources': unchanged, 'captured_source_closure': closure,
              'archive_ledger': {'payloads': len(lines), 'sha256': digest(ledger), 'mismatches': bad},
              'pure_structural_tests': tests,
              'scope': 'Read-only pinned Git provenance plus two pure structural test executions; no live engine qualification',
              'notes': [
                  'First capture links a native u18 exporter to a pre-existing core archive with RankIndex fix; the source hashes do not contain separate R2 changes.',
                  'Second CMake capture builds only the standalone grid32_primitives and rank_search targets; four targeted gates do not qualify the core, head or CLI integration.',
                  'The native receipt reports six clouds with n=5..7, K3/K5 and 815 checks per mode; it is not a quality, scaling, full CTest, GPU or G4 gate.',
                  'No new product defect is demonstrated by this follow-up.'],
              'GCP_used': False, 'GPU_used': False, 'engine_executed': False, 'build_executed': False}
    args.out.write_text(json.dumps(report, indent=2, sort_keys=True) + '\n')
    print(json.dumps({'status': report['status'], 'archive_payloads': len(lines),
                      'source_closures': [[x['matches_commit'], x['files']] for x in closure],
                      'pure_tests': [x['returncode'] for x in tests]}, sort_keys=True))
    return 0 if ok else 1

if __name__ == '__main__':
    sys.exit(main())
