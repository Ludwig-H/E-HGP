"""Freeze only explicitly named, small inputs; never execute the developer exporter."""
from pathlib import Path
import hashlib
import json
import subprocess
import sys

ROOT = Path(__file__).resolve().parent
CHECKOUT = Path('/workspaces/E-HGP/build/v9-open-worktree')
PRODUCT = CHECKOUT / 'morsehgp3D_v10'
FILES = [
    (Path('/tmp/deux_triangles') / (name + suffix), 'inputs/' + name + suffix)
    for name in ('aretes_plus_courtes', 'pont_plus_court', 'pont_plus_long')
    for suffix in ('.u32le', '.json')
] + [
    (PRODUCT / 'docs/SPEC_V10.md', 'sources/SPEC_V10.md.source.txt'),
    (PRODUCT / 'audits/REPONSE_CLAUDE_AUDIT_GEANT_20260930.md', 'sources/REPONSE_CLAUDE_AUDIT_GEANT_20260930.md.source.txt'),
    (Path('/workspaces/E-HGP/build/v10-verrou-points/SYNTHESE/code/vx.py'), 'sources/vx.py'),
]

def digest(b):
    return hashlib.sha256(b).hexdigest()

mode = sys.argv[1]
if mode not in ('open', 'close'):
    raise SystemExit('expected open or close')
entries = []
for src, relative in FILES:
    content = src.read_bytes()
    target = ROOT / relative
    if mode == 'open':
        if target.exists():
            raise SystemExit('snapshot already exists: ' + str(target))
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(content)
    snapshot = target.read_bytes()
    entries.append({'source': str(src), 'snapshot': relative, 'bytes': len(content),
                    'sha256': digest(content), 'snapshot_sha256': digest(snapshot),
                    'equal_snapshot': content == snapshot})
out = {
    'mode': mode,
    'checkout_head_readonly': subprocess.check_output(['git', '-C', str(CHECKOUT), 'rev-parse', 'HEAD'], text=True).strip(),
    'inputs_and_named_sources_stable': all(e['equal_snapshot'] for e in entries),
    'files': entries,
    'scope': 'Archived six-site exports, exact Fraction verifier; no exporter execution, no native engine qualification, no campaign/GCP.'
}
(ROOT / ('SOURCE_BEFORE.json' if mode == 'open' else 'SOURCE_AFTER.json')).write_text(json.dumps(out, indent=2) + '\n')
print(json.dumps({'mode': mode, 'files': len(entries), 'stable': out['inputs_and_named_sources_stable']}))
if not out['inputs_and_named_sources_stable']:
    raise SystemExit(1)
