"""Fresh proof assembly; never overwrites a closed packet or copies binaries."""
import hashlib
import json
from pathlib import Path
import shutil

root = Path(__file__).resolve().parent
destination = Path('/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v10/receipts/audit_continu_20260929/level_order_20260930')
destination.mkdir()
origins = {'prototype':Path('/tmp/mhgp10-level-comparator-0930.jpN8t6og'),'independent':root}
copied = {}
for label,origin in origins.items():
    shutil.copytree(origin,destination/label)
    copied[label] = {}
    for path in sorted(origin.rglob('*')):
        if path.is_symlink():
            raise ValueError('linked source in packet')
        if path.is_file():
            name = str(path.relative_to(origin))
            digest = hashlib.sha256(path.read_bytes()).hexdigest()
            if digest != hashlib.sha256((destination/label/name).read_bytes()).hexdigest():
                raise ValueError('assembly changed bytes')
            copied[label][name] = digest
    if label=='independent':
        (destination/label/'SHA256SUMS').write_text(''.join(
            digest+'  '+name+'\n' for name,digest in copied[label].items()))
(destination/'assembly.json').write_text(json.dumps(dict(schema='mhgp10_level_order_assembly_v1',
    origins={k:str(v) for k,v in origins.items()},copied_sha256=copied,
    engine_source_changes=0,GCP_used=False),sort_keys=True,indent=2)+'\n')
print(json.dumps(dict(status='ASSEMBLED',files=sum(map(len,copied.values())),destination=str(destination)),sort_keys=True))
