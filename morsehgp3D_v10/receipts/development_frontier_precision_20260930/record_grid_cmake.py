"""Record targeted CMake integration in a fresh receipt directory, no GCP."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess


def hashes(root):
    return {str(p.relative_to(root)): hashlib.sha256(p.read_bytes()).hexdigest()
            for p in sorted(root.rglob('*')) if p.is_file() and '__pycache__' not in p.parts}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--source', type=Path, required=True)
    parser.add_argument('--build', type=Path, required=True)
    parser.add_argument('--out', type=Path, required=True)
    args = parser.parse_args()
    args.out.mkdir(parents=True, exist_ok=False)
    before = hashes(args.source)
    commands = [
        ('configure', ['cmake', '-S', str(args.source), '-B', str(args.build),
                       '-DCMAKE_BUILD_TYPE=Release']),
        ('build', ['cmake', '--build', str(args.build), '--target',
                   'mhgp10_grid32_primitives', 'mhgp10_rank_search', '-j2']),
        ('gates', ['ctest', '--test-dir', str(args.build), '-R',
                   '^(mhgp10_grid32_primitives|mhgp10_rank_search|mhgp10_cover_band_structural|mhgp10_dev_quotas)$',
                   '--output-on-failure']),
    ]
    results = []
    for name, argv in commands:
        run = subprocess.run(argv, capture_output=True, text=True, check=False)
        (args.out / (name + '.stdout.txt')).write_text(run.stdout)
        (args.out / (name + '.stderr.txt')).write_text(run.stderr)
        results.append({'name': name, 'argv': argv, 'returncode': run.returncode})
        if run.returncode != 0:
            break
    after = hashes(args.source)
    status = 'PASS' if (len(results) == 3 and all(r['returncode'] == 0 for r in results)
                        and before == after) else 'FAIL'
    receipt = {'status': status, 'commands': results, 'source_before': before,
               'source_after': after, 'GCP_used': False,
               'scope': 'four_targeted_CMake_gates_only'}
    (args.out / 'receipt.json').write_text(json.dumps(receipt, indent=2, sort_keys=True) + '\n')
    entries = hashes(args.out)
    (args.out / 'SHA256SUMS').write_text(''.join(d + '  ' + p + '\n' for p, d in entries.items()))
    print(json.dumps({'status': status, 'commands': len(results), 'sources': len(before)}))
    return 0 if status == 'PASS' else 1


if __name__ == '__main__':
    raise SystemExit(main())
