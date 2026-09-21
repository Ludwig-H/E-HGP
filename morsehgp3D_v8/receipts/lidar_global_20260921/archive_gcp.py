"""Archive exactly one CLOSED host session, never its parent/private SSH key."""
import hashlib
import io
import json
from pathlib import Path
import sys
import tarfile

host, destination = map(Path, sys.argv[1:])
host = host.resolve()
if host.name != 'cpu_v8_host' or destination.exists():
    raise ValueError('explicit closed host directory and fresh destination required')
receipt = json.loads((host/'receipt.json').read_text())
if receipt.get('targeted_shutdown_certified') is not True:
    raise ValueError('targeted G4 shutdown not certified')
stop = (host/'guarded_stop.stdout').read_text()
if 'état GCE TERMINATED' not in stop:
    raise ValueError('no raw TERMINATED confirmation')
files = {}
for path in sorted(host.rglob('*')):
    if path.is_symlink():
        raise ValueError('symlink in host capture')
    if path.is_file():
        raw = path.read_bytes()
        if b'PRIVATE KEY-----' in raw or path.name.startswith('session_key'):
            raise ValueError('private key must never enter archive')
        files[str(path.relative_to(host))] = (raw, hashlib.sha256(raw).hexdigest())
for command in receipt['commands']:
    for extension in ('stdout', 'stderr'):
        name = command['name']+'.'+extension
        if name in files and files[name][1] != command[extension+'_sha256']:
            raise ValueError('command log differs')
destination.mkdir(parents=True)
archive = destination/'host_capture.tar.gz'
with tarfile.open(archive, 'x:gz') as target:
    for name, (raw, _) in files.items():
        member = tarfile.TarInfo('cpu_v8_host/'+name)
        member.size, member.mtime, member.mode = len(raw), 0, 0o444
        target.addfile(member, io.BytesIO(raw))
with tarfile.open(archive, 'r:gz') as target:
    observed = {str(Path(member.name).relative_to('cpu_v8_host')):
                hashlib.sha256(target.extractfile(member).read()).hexdigest()
                for member in target.getmembers()}
if observed != {name: pin for name, (_, pin) in files.items()}:
    raise ValueError('archive roundtrip mismatch')
summary = {key: receipt[key] for key in ('status', 'target', 'generation', 'targeted_shutdown_certified',
            'controller_sha256', 'worker_sha256', 'snapshot_sha256', 'manifest_sha256', 'GPU_executed', 'FULL_executed')}
summary.update(archive_sha256=hashlib.sha256(archive.read_bytes()).hexdigest(), files=observed,
               original_host=str(host), private_key_included=False, gcp_used=True,
               public_status='not_claimed', full_contract_qualified=False)
with (destination/'ARCHIVE.json').open('x') as target:
    json.dump(summary, target, indent=2, sort_keys=True)
print(json.dumps({key: value for key, value in summary.items() if key != 'files'}, sort_keys=True))
