import csv
import hashlib
import json
import pathlib

ROOT = pathlib.Path(__file__).resolve().parent

def fail(message):
    raise RuntimeError(message)

def main():
    entries = []
    for line in (ROOT / 'SHA256SUMS').read_text().splitlines():
        expected, relative = line.split('  ', 1)
        path = ROOT / relative
        if path.resolve().parent != ROOT and ROOT not in path.resolve().parents:
            fail('escape')
        if hashlib.sha256(path.read_bytes()).hexdigest() != expected:
            fail('hash mismatch: ' + relative)
        entries.append(relative)
    if len(entries) < 25 or len(entries) != len(set(entries)):
        fail('manifest vacuity or duplication')
    record = json.loads((ROOT / 'receipt.json').read_text())
    if record['source_before'] != record['source_after'] or record['source_copy'] != record['source_before']:
        fail('source provenance')
    if record['native_calls'] != 0 or record['gcp_used']:
        fail('scope')
    tests = record['tests']
    if {(t['case'], t['mode']) for t in tests} != {(c, m) for c in ['positive', 'empty_new', 'truncated_new', 'extra_new'] for m in ['normal', 'optimized']} or len(tests) != 8:
        fail('inventory')
    good, bad = 0, 0
    for test in tests:
        folder = ROOT / test['case']
        with (folder / 'old.csv').open(newline='') as handle:
            old = list(csv.DictReader(handle))
        with (folder / 'new.csv').open(newline='') as handle:
            new = list(csv.DictReader(handle))
        if (len(old), len(new)) != (test['old_count'], test['new_count']):
            fail('fixture counts')
        valid = old == new and bool(old)
        if valid != test['expected_valid']:
            fail('oracle')
        output = json.loads((folder / (test['mode'] + '.stdout')).read_text())
        if test['code'] != 0 or output['ecarts'] or not output['entetes_egaux']:
            fail('tool acceptance was not observed')
        if output['lignes_ancien'] != len(old) or output['lignes_nouveau'] != len(new):
            fail('stdout counts')
        if (folder / (test['mode'] + '.stderr')).read_text():
            fail('unexpected stderr')
        if not all(c[3] for c in output['lignes_coherentes_avec_json_natifs']) or output['appels'] != 2 * len(new):
            fail('native-shaped coherence')
        if valid:
            good += 1
        else:
            bad += 1
    if (good, bad) != (2, 6):
        fail('non-vacuity')
    print('PASS ARCHIVE_READER_FALSE_ACCEPTANCE_OBSERVED files=%d valid=%d invalid_accepted=%d source_locked=true native=0 GCP=false' % (len(entries), good, bad))

if __name__ == '__main__':
    main()
