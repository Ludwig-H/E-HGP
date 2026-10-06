#!/usr/bin/env python3
"""Rejeu stdlib de la proposition DSU sur lecteur capture, sans produit natif."""
import difflib
import hashlib
import json
import sys
import tempfile
import types
from pathlib import Path

ROOT = Path(__file__).resolve().parent
P = json.loads((ROOT / 'proposal.json').read_text())
CAPTURE = ROOT.parent / 'formats'
META = json.loads((CAPTURE / 'sources.json').read_text())
for item in META['sources']:
    raw = (CAPTURE / item['capture_path']).read_bytes()
    if hashlib.sha256(raw).hexdigest() != item['sha256']:
        raise ValueError('dependency hash')
SOURCE = (CAPTURE / 'sources/mhgp11_formats.py').read_text()
if hashlib.sha256(SOURCE.encode()).hexdigest() != P['captured_reader_sha256']:
    raise ValueError('reader hash')
if SOURCE.count(P['call_needle']) != 1 or SOURCE.count('def _check_canonical(f):') != 1:
    raise ValueError('patch anchors')
FIXED = SOURCE.replace(P['call_needle'], P['call_replacement']).replace(
    'def _check_canonical(f):', P['function_inserted'] + 'def _check_canonical(f):')
if hashlib.sha256(FIXED.encode()).hexdigest() != P['proposed_reader_sha256']:
    raise ValueError('fixed hash')
patch = ''.join(difflib.unified_diff(SOURCE.splitlines(keepends=True), FIXED.splitlines(keepends=True),
    fromfile='a/morsehgp3D_v11/bench/mhgp11_formats.py', tofile='b/morsehgp3D_v11/bench/mhgp11_formats.py'))
if patch != (ROOT / 'read_supports_spanning.patch').read_text() or hashlib.sha256(patch.encode()).hexdigest() != P['patch_sha256']:
    raise ValueError('patch mismatch')
sys.path.insert(0, str(CAPTURE / 'sources'))
import mhgp11_formats as before
import supports_spanning_reader_gate as gate
after = types.ModuleType('formats_proposed_spanning')
after.__file__ = str(CAPTURE / 'sources/mhgp11_formats.py')
exec(compile(FIXED, 'formats_proposed_spanning.py', 'exec'), after.__dict__)
first, second = gate.outcomes(before), gate.outcomes(after)
if [row['accepted'] for row in first] != [True, True, True]:
    raise ValueError('before not reproduced')
if any(row['accepted'] != row['expected_accept'] for row in second):
    raise ValueError('proposal not causal')


def directory_outcome(reader, binary):
    # Internally consistent hashes/counts of the malformed fixture: the structure is what must be refused.
    f = before.read_supports(binary, 21)
    manifest = dict(schema=before.SCHEMA, output='supports', status='complete', public_status='not_claimed',
        coord_bits=21, k=1, parameters=dict(budget_bytes=4096, grid_step=None, origin=None),
        inputs=[dict(name='points', bytes=12*f.n, sha256='0'*64), dict(name='ids', bytes=4*f.n, sha256='0'*64)],
        files=[dict(name=before.SUPPORTS_NAME, format='MHGP11SP', version=2, bytes=len(binary),
                    sha256=hashlib.sha256(binary).hexdigest())], tree_k_sha256=f.tree_signature(), counts=f.manifest_counts())
    with tempfile.TemporaryDirectory(prefix='audit-spanning-reader-') as folder:
        d = Path(folder) / 'D'
        d.mkdir()
        (d / before.SUPPORTS_NAME).write_bytes(binary)
        (d / before.MANIFEST).write_text(json.dumps(manifest, separators=(',', ':')) + '\n')
        try:
            reader.check_directory(str(d), 21)
            return {'accepted': True}
        except ValueError as error:
            return {'accepted': False, 'reason': str(error)}


directories = []
for name, binary, expected in gate.cases():
    a, b = directory_outcome(before, binary), directory_outcome(after, binary)
    if not a['accepted'] or b['accepted'] != expected:
        raise ValueError('directory outcome')
    directories.append(dict(case=name, before=a, proposed=b))

# A published hyperedge may supply one useful union together with redundant links.
# Two hyperedge supports connect four children; do not impose B = children - 1.
f = types.SimpleNamespace(N=1, kind=[after.KIND_MERGE], children=[[0, 1, 2, 3]], first_ball=[0],
    ball_count=[2], prior_at=[0, 3], prior_count=[3, 3], prior=[0, 1, 2, 0, 2, 3])
after._check_spanning(f)
result = dict(schema='audit_sp_v2_reader_spanning_replay_v1', base_pin=P['base_pin'],
    captured_reader_sha256=P['captured_reader_sha256'], proposed_reader_sha256=P['proposed_reader_sha256'],
    reader_before=first, reader_proposed=second, directory_cases=directories,
    helper_hyperedge_with_redundant_links='accepted', native_runs=0, cloud_actions=0,
    limit=P['limit'])
output = json.dumps(result, indent=2, sort_keys=True) + '\n'
if '--capture' in sys.argv:
    (ROOT / 'summary.json').write_text(output)
else:
    if json.loads((ROOT / 'summary.json').read_text()) != result:
        raise ValueError('summary changed')
print(output, end='')
