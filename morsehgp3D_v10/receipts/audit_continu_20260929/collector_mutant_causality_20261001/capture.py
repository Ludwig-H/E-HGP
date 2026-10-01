"""One-time recorder; only an OPEN private packet may run this. Not a reader."""
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
from datetime import datetime, timezone
import read as reader


def manifest(root):
    names = sorted(reader.FILES)
    text = ''.join(reader.sha(root / name) + '  ' + name + '\n' for name in names)
    (root / 'MANIFEST.sha256').write_text(text)
    digest = hashlib.sha256(text.encode()).hexdigest()
    (root / 'MANIFEST_SHA256').write_text(digest + '\n')
    return digest


def main():
    root = Path(__file__).parent.resolve()
    if (root / 'MANIFEST.sha256').exists():
        raise SystemExit('REFUS packet already closed')
    original = Path(sys.argv[1])
    before = reader.sha(original)
    if before != reader.SOURCE_SHA or reader.sha(root / 'sources/mutants_entrees_cli.py') != before:
        raise SystemExit('REFUS source pin')
    (root / 'captures').mkdir()
    calls = []
    for mode in ('normal', 'O'):
        for case in reader.CASES:
            argv = [sys.executable, '-B'] + (['-O'] if mode == 'O' else []) + [str(root / 'controlflow.py'), case]
            start = datetime.now(timezone.utc).isoformat()
            result = subprocess.run(argv, capture_output=True, timeout=10)
            stop = datetime.now(timezone.utc).isoformat()
            out, err = f'captures/{mode}_{case}.stdout', f'captures/{mode}_{case}.stderr'
            (root / out).write_bytes(result.stdout)
            (root / err).write_bytes(result.stderr)
            if result.returncode != 0 or result.stderr:
                raise SystemExit('REFUS capture ' + case)
            reader.judge_stdout(result.stdout.decode(), mode, case)
            calls.append(dict(id=mode + '/' + case, argv=argv, exit=result.returncode, stdout=out, stderr=err,
                              started_utc=start, ended_utc=stop))
    after = reader.sha(original)
    receipt = dict(source_original=str(original), source_before=before, source_after=after,
                   source_snapshot=reader.sha(root / 'sources/mutants_entrees_cli.py'),
                   local_transitive_dependencies=[], stdlib_boundary='Python stdlib, no local helper imports',
                   python=sys.executable, python_sha256=reader.sha(Path(sys.executable)),
                   python_version=sys.version, capture_root=str(root), calls=calls)
    (root / 'receipt.json').write_text(json.dumps(receipt, sort_keys=True, indent=2) + '\n')
    (root / 'mutations.json').write_text(json.dumps([dict(id=name, exit=2) for name in
                                                       ('empty_manifest', 'omitted_case', 'wrong_exit')]) + '\n')
    # Reader mutation copies are PRIVATE and discarded; only their tiny terminal receipts remain.
    initial = manifest(root)
    results = []
    for case in ('empty_manifest', 'omitted_case', 'wrong_exit'):
        with tempfile.TemporaryDirectory(prefix='mhgp10-ecli-reader-mut-') as base:
            copy = Path(base) / 'packet'
            shutil.copytree(root, copy)
            if case == 'empty_manifest':
                (copy / 'MANIFEST.sha256').write_text('')
                pin = reader.sha(copy / 'MANIFEST.sha256')
                (copy / 'MANIFEST_SHA256').write_text(pin + '\n')
            else:
                data = json.loads((copy / 'receipt.json').read_text())
                if case == 'omitted_case':
                    data['calls'].pop()
                else:
                    data['calls'][0]['exit'] = 7
                (copy / 'receipt.json').write_text(json.dumps(data, sort_keys=True, indent=2) + '\n')
                pin = manifest(copy)
            argv = [sys.executable, '-B', str(copy / 'read.py'), str(copy), pin]
            result = subprocess.run(argv, capture_output=True, timeout=10)
            if result.returncode == 0:
                raise SystemExit('REFUS reader mutation survived ' + case)
            results.append(dict(id=case, exit=result.returncode, stdout=result.stdout.decode(),
                                stderr=result.stderr.decode(), external_mutated_pin=pin,
                                argv_template=['{python}', '-B', '{copy}/read.py', '{copy}', pin]))
    (root / 'MANIFEST.sha256').unlink()
    (root / 'MANIFEST_SHA256').unlink()
    (root / 'mutations.json').write_text(json.dumps(results, sort_keys=True, indent=2) + '\n')
    final = manifest(root)
    reader.check(root, final)
    print('closed manifest=' + final + ' files=25 calls=8 native=0')


if __name__ == '__main__':
    main()
