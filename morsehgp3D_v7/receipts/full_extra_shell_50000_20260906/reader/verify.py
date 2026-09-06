#!/usr/bin/env python3
"""Portable byte/association check, no subprocess, no product import."""
import hashlib
import json
from pathlib import Path


def need(ok, reason):
    if not ok:
        raise ValueError(reason)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    base = Path(__file__).resolve().parent
    manifest = json.loads((base/'FILES.json').read_text())
    for field in ('files', 'references'):
        need(manifest[field], 'nonempty file inventory')
        for relative, expected in manifest[field].items():
            need(sha(base/relative) == expected, 'changed bytes: ' + relative)
    receipt = json.loads((base/'checks/receipt.json').read_text())
    need(receipt['status'] == 'completed' and receipt['sources_stable'] is True and
         receipt['outputs_byte_equal'] is True, 'reader closure')
    expected = 'c4a066e620b7850b6b3f1937f5b6d92b027f763012a554f9d1fbbf5512cc3c81'
    for name in ('normal', 'optimized'):
        output, error = base/('checks/'+name+'.stdout'), base/('checks/'+name+'.stderr')
        command = json.loads((base/('checks/'+name+'.command.json')).read_text())
        need(command['exit_code'] == 0 and command['stdout_sha256'] == sha(output) == expected and
             command['stderr_sha256'] == sha(error) and error.read_bytes() == b'', 'reader command/streams')
    result = json.loads((base/'checks/normal.stdout').read_text())
    need(result['source_jsonl_sha256'] == sha(base/'../run_r3/n50000_k10.stderr') and
         len(result['records']) == 4 and all(row['all_input_points_scanned'] == 50000 and
         row['global_parents_reconstructed'] is False for row in result['records']), 'trace/reader association')
    print(json.dumps(dict(status='passed', verified_files=len(manifest['files']),
        verified_references=len(manifest['references']), pure_reader_records=4, diagnostic_only=True,
        scope='byte integrity and preserved reader associations, not a new geometric replay'), sort_keys=True))


if __name__ == '__main__':
    main()
