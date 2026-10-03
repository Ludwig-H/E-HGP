"""Read-only checkpoint of independent cleanup capsules; no deletions or edits."""
from pathlib import Path
import hashlib
import json
import sys
root = Path('/workspaces/.ehgp-backups/cleanup-20261003')
def digest(path):
    h=hashlib.sha256()
    with path.open('rb') as reader:
        for block in iter(lambda: reader.read(1048576),b''):
            h.update(block)
    return h.hexdigest()
rows=[]
for checksum_file in sorted(root.glob('*/SHA256SUMS')):
    capsule=checksum_file.parent
    entries={}
    for line in checksum_file.read_text().splitlines():
        if not line: continue
        checksum, separator, relative = line.partition('  ')
        if not separator or len(checksum)!=64:
            raise SystemExit('Bad checksum line in '+str(checksum_file))
        path=Path(relative)
        if path.is_absolute() or '..' in path.parts or relative in entries:
            raise SystemExit('Invalid/duplicate inventory path in '+str(checksum_file))
        entries[relative]=checksum
    actual={str(p.relative_to(capsule)) for p in capsule.rglob('*') if p.is_file() and p!=checksum_file}
    missing=sorted(set(entries)-actual)
    extra=sorted(actual-set(entries))
    bad=[name for name, checksum in entries.items() if name in actual and digest(capsule/name)!=checksum]
    rows.append({'capsule':capsule.name,'checksum_sha256':digest(checksum_file),'files':len(entries),'bytes':sum((capsule/name).stat().st_size for name in entries if name in actual),'missing':missing,'uninventoried':extra,'bad_hashes':bad,'ok':not(missing or extra or bad)})
print(json.dumps({'ok':bool(rows) and all(r['ok'] for r in rows),'capsules':rows},indent=2))
if not rows or any(not r['ok'] for r in rows):
    raise SystemExit(1)
