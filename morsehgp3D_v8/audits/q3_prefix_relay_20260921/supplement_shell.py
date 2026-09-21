#!/usr/bin/env python3
"""Exercise all 30 shell contacts using unchanged, already qualified binaries."""
import json
from pathlib import Path
import sys

import campaign as c
from read_closed import empty_counts


def run(folder):
    c.require(not folder.exists(), 'Receipt exists')
    folder.mkdir(parents=True)
    prior = c.BASE/'receipts/r1'
    old = json.loads((prior/'MANIFEST.json').read_text())
    c.require(old['status'] == 'completed', 'Prior receipt incomplete')
    pins = dict(old['sources'])
    for path in (Path(__file__).resolve(), c.BASE/'read_closed.py', prior/'MANIFEST.json', prior/'COMPLETION.json'):
        pins[str(path.relative_to(c.ROOT))] = c.sha(path)
    c.require(all(c.sha(c.ROOT/name) == value for name, value in pins.items()), 'Changed source')
    c.require(all(c.sha(c.ROOT/name) == value for name, value in old['binaries'].items()), 'Changed binary')
    manifest = dict(status='started', sources=pins, binaries=old['binaries'], commands=0,
                    scope='Additional shell30 non-vacuity; no source changes or rebuild')
    c.write(folder/'MANIFEST.json', manifest)
    for options in ([], ['-O']):
        c.append(folder, manifest, c.execute([sys.executable, '-B', *options, str(c.BASE/'forest_model.py')]))
    points = dict(c.cases())['shell30']
    source = c.BASE/'.inputs/shell30.u16le'
    counts = empty_counts()
    for name in old['binaries']:
        record = dict(case='shell30', n=len(points), k=3, edge=[5, 29], sample_limit=0,
                      input_sha256=c.sha(source),
                      **c.execute([str(c.ROOT/name), str(source), '3', '5', '29', '0']))
        c.append(folder, manifest, record)
        data = json.loads(record['stdout'])
        c.check(data, points, 5, 29, 3, 0, counts)
        selected = next(r for r in data['records'] if r['x'] == 8)
        c.require(selected['reference']['shell'] == list(range(30)), 'Missing full shell')
        c.require(all(r['shell'] == list(range(30)) for r in selected['relays']), 'Lost contact after handoff')
    c.require(counts['max_shell'] == 30, 'Large shell not exercised')
    c.require(all(c.sha(c.ROOT/name) == value for name, value in pins.items()), 'Source changed during run')
    c.require(all(c.sha(c.ROOT/name) == value for name, value in old['binaries'].items()), 'Binary changed during run')
    manifest.update(status='completed', counts=counts, commands_sha256=c.sha(folder/'COMMANDS.jsonl.gz'))
    c.write(folder/'MANIFEST.json', manifest)
    c.write(folder/'COMPLETION.json', dict(status='completed', manifest_sha256=c.sha(folder/'MANIFEST.json')))
    print(json.dumps(counts, sort_keys=True))


if __name__ == '__main__':
    c.require(len(sys.argv) == 2, 'usage: supplement_shell.py new_receipt_folder')
    run(Path(sys.argv[1]).resolve())
