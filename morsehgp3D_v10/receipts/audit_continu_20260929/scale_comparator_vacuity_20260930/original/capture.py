import csv
import hashlib
import json
import pathlib
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parent
SOURCE = pathlib.Path('/workspaces/E-HGP/build/v10-integration-r2/src/morsehgp3D_v10/receipts/raccord_r2_20260930/bancs/outils/compare_scale.py')
FIELDS = ['file', 'k', 'status', 'balls', 'cat_judged', 'tower_nodes_all', 'tower_steps_all', 'tower_s']

def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def rows(names):
    return [dict(zip(FIELDS, [name, '5', 'ok', '7', 'True', '3', '4', '0.05'])) for name in names]

def write_csv(path, names):
    with path.open('w', newline='') as handle:
        writer = csv.DictWriter(handle, fieldnames=FIELDS)
        writer.writeheader()
        writer.writerows(rows(names))

def write_calls(path, names):
    values = []
    for name in names:
        for call, output in [('catalogue', {'balls': 7, 'judged': True}),
                             ('tower', {'balls': 7, 'orders': [{'nodes': 3, 'steps': 4}], 'tower_s': 0.05})]:
            values.append({'file': name, 'k': 5, 'call': call, 'code': 0, 'stdout': json.dumps(output), 'stderr': ''})
    path.write_text(''.join(json.dumps(value) + '\n' for value in values))

def main():
    before = digest(SOURCE)
    if before != '6b5364272a3f412afb3a5ffbf46e4912ec4dbf99e8caeecace744f0c2c3aac63' or before != digest(ROOT / 'compare_scale.py'):
        raise RuntimeError('unexpected source')
    cases = [('positive', ['a.u32le'], ['a.u32le']),
             ('empty_new', ['a.u32le'], []),
             ('truncated_new', ['a.u32le', 'b.u32le'], ['a.u32le']),
             ('extra_new', ['a.u32le'], ['a.u32le', 'b.u32le'])]
    outputs = []
    for name, old, new in cases:
        folder = ROOT / name
        folder.mkdir()
        write_csv(folder / 'old.csv', old)
        write_csv(folder / 'new.csv', new)
        write_calls(folder / 'new.csv.calls.jsonl', new)
        for mode, flags in [('normal', []), ('optimized', ['-O'])]:
            argv = [sys.executable, '-B'] + flags + [str(ROOT / 'compare_scale.py'), str(folder / 'old.csv'), str(folder / 'new.csv')]
            result = subprocess.run(argv, capture_output=True, text=True, timeout=10)
            stem = folder / mode
            stem.with_suffix('.stdout').write_text(result.stdout)
            stem.with_suffix('.stderr').write_text(result.stderr)
            outputs.append({'case': name, 'mode': mode, 'argv': argv, 'code': result.returncode,
                            'expected_valid': old == new and bool(old), 'old_count': len(old), 'new_count': len(new)})
    record = {'schema': 'causal_compare_scale_static_source_v1', 'source_path': str(SOURCE),
              'source_before': before, 'source_after': digest(SOURCE), 'source_copy': digest(ROOT / 'compare_scale.py'),
              'redecide_source_copy': digest(ROOT / 'redecide_refusion.py'),
              'tests': outputs, 'native_calls': 0, 'gcp_used': False,
              'scope': 'reader accepts missing or extra rows; fabricated bounded CSV/JSON, no engine result falsified',
              'redecide_scope': 'static codeflow only: unconditional return 0 at line 118; not executed'}
    (ROOT / 'receipt.json').write_text(json.dumps(record, indent=2, sort_keys=True) + '\n')
    if record['source_before'] != record['source_after']:
        raise RuntimeError('source changed during capture')
    paths = sorted(path for path in ROOT.rglob('*') if path.is_file() and path.name not in ['SHA256SUMS', 'verify_normal.txt', 'verify_optimized.txt'])
    (ROOT / 'SHA256SUMS').write_text(''.join(digest(path) + '  ' + path.relative_to(ROOT).as_posix() + '\n' for path in paths))
    print('CAPTURE files=%d commands=%d positive=2 bad_accepted=6 native=0 GCP=false' % (len(paths), len(outputs)))

if __name__ == '__main__':
    main()
