#!/usr/bin/env python3
"""Bounded real-reader checks on the integrated source; no native execution."""
import argparse
import ast
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import types


def need(value, reason):
    if not value:
        raise ValueError(reason)


parser = argparse.ArgumentParser()
parser.add_argument('--repo', default='/workspaces/E-HGP')
args = parser.parse_args()
base = Path(__file__).resolve().parent
meta = json.loads((base / 'sources.json').read_text())
blobs = {}
for path, digest in meta['sources'].items():
    raw = subprocess.check_output(['git', '-C', args.repo, 'show', meta['pin'] + ':' + path])
    need(hashlib.sha256(raw).hexdigest() == digest, 'source hash: ' + path)
    blobs[path] = raw
for entry in ('fixture_builder', 'selector_proposal'):
    item = meta[entry]
    raw = subprocess.check_output(['git', '-C', args.repo, 'show', meta['pin'] + ':' + item['path']])
    need(hashlib.sha256(raw).hexdigest() == item['sha256'], 'frozen ' + entry)
    if entry == 'selector_proposal':
        need(raw == blobs['morsehgp3D_v11/src/supports/hierarchy.cpp'], 'native selector equals proposed file')
    else:
        tree = ast.parse(raw)
        nodes = [node for node in tree.body if isinstance(node, ast.FunctionDef)
                 and node.name in ('column', 'point_file')]
        need(len(nodes) == 2, 'real frozen fixture builder')
        fixtures = {'struct': struct, 'need': need, 'NONE': (1 << 32) - 1}
        exec(compile(ast.Module(body=nodes, type_ignores=[]), 'frozen fixture builder', 'exec'), fixtures)
for name in ('catalogue_semantic', 'full_semantic', 'mhgp11_formats'):
    module = types.ModuleType(name)
    module.__file__ = meta['pin'] + ':' + name + '.py'
    sys.modules[name] = module
    exec(compile(blobs['morsehgp3D_v11/bench/' + name + '.py'], module.__file__, 'exec'), module.__dict__)
reader = sys.modules['mhgp11_formats']
cases = [('before_parent', 0, 0, True), ('at_parent', 0, 1, False),
         ('after_parent', 0, 2, False), ('root_at_birth', 2, 1, True),
         ('root_later', 2, 2, True), ('before_block_birth', 2, 0, False)]
results = []
for name, target, when, expected in cases:
    for bits in (18, 21, 24):
        raw = fixtures['point_file'](target, when, bits)
        try:
            reader.read_points(raw, bits, exact=True)
            accepted, reason = True, None
        except ValueError as error:
            accepted, reason = False, str(error)
        need(accepted == expected, name + '/' + str(bits))
        results.append(dict(case=name, bits=bits, accepted=accepted, refusal=reason))
with tempfile.TemporaryDirectory(prefix='mhgp11_integrated_reader_') as directory:
    temp = Path(directory)
    for name in ('catalogue_semantic', 'full_semantic', 'mhgp11_formats'):
        (temp / (name + '.py')).write_bytes(blobs['morsehgp3D_v11/bench/' + name + '.py'])
    gate = temp / 'supports_spanning_reader_gate.py'
    gate.write_bytes(blobs['morsehgp3D_v11/tests/cli/supports_spanning_reader_gate.py'])
    options = ['-O'] if sys.flags.optimize else []
    run = subprocess.run([sys.executable, *options, '-B', str(gate), '--bench', str(temp)],
                         capture_output=True, text=True)
    need(run.returncode == 0, 'official reader gate: ' + run.stderr)
    need('supports_spanning_reader_verdict conforme cas3' in run.stdout, 'official gate verdict')
print(json.dumps(dict(pin=meta['pin'], chronology=results, official_spanning_gate=run.stdout.strip(),
                     selector_matches_frozen_proposal=True, native_runs=0, cloud_actions=0),
                 indent=2, sort_keys=True))
