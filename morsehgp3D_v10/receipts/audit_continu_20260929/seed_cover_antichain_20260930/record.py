"""Capture two Python-only runs; source/input pins before and after."""
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parent
UPSTREAM = Path('/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v10/receipts/audit_independant_20260930/fixed_k_antichain')
UPSTREAM_MANIFEST = '9ab1e991f360331a9bcfe91a84920bb1708d8370c44ac6a603fd90d161f1806e'


def need(ok, message):
    if not ok:
        raise ValueError(message)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def now():
    return datetime.now(timezone.utc).isoformat()


def main():
    need(not (ROOT / 'receipt.json').exists() and not (ROOT / 'SHA256SUMS').exists(),
         'capture already exists; immutable packet')
    need(sha(UPSTREAM / 'SHA256SUMS') == UPSTREAM_MANIFEST == sha(ROOT / 'upstream_SHA256SUMS'),
         'wrong upstream closed manifest')
    upstream = {}
    for line in (ROOT / 'upstream_SHA256SUMS').read_text().splitlines():
        digest, name = line.split('  ', 1)
        need(name not in upstream, 'duplicate upstream name')
        upstream[name] = digest
    need(len(upstream) == 37, 'wrong upstream manifest count')
    inputs = {}
    for path in sorted((ROOT / 'fixtures').glob('*.json')):
        name = str(path.relative_to(ROOT))
        need(sha(path) == upstream[name] == sha(UPSTREAM / name), 'source copy changed')
        inputs[name] = dict(sha256=sha(path), upstream_path=str(UPSTREAM / name))
    need(len(inputs) == 6, 'fixture floor')
    provenance = dict(upstream_manifest_sha256=UPSTREAM_MANIFEST,
                      upstream_manifest_path=str(UPSTREAM / 'SHA256SUMS'),
                      copied_existing_exports_only=True, native_invocations=0, inputs=inputs)
    (ROOT / 'input_sources.json').write_text(json.dumps(provenance, sort_keys=True, indent=2) + '\n')
    source_names = ['prototype.py', 'record.py', 'verify.py', 'README.md', 'input_sources.json',
                    'upstream_SHA256SUMS'] + sorted(inputs)
    before = {name: sha(ROOT / name) for name in source_names}
    receipt = dict(started_utc=now(), started_unix_ns=time.time_ns(), sources_before=before,
                   scope='Fraction structural seed compression; existing closed native JSON inputs only',
                   native_invocations=0, GCP_used=False, commands=[])
    for mode in ('normal', 'optimized'):
        argv = [sys.executable, '-B'] + (['-O'] if mode == 'optimized' else []) + ['prototype.py']
        start_ns, start_utc = time.time_ns(), now()
        command = subprocess.run(argv, cwd=ROOT, capture_output=True, timeout=60)
        row = dict(mode=mode, argv=argv, started_utc=start_utc, started_unix_ns=start_ns,
                   ended_utc=now(), ended_unix_ns=time.time_ns(), returncode=command.returncode,
                   stdout=mode + '.stdout', stderr=mode + '.stderr')
        (ROOT / row['stdout']).write_bytes(command.stdout)
        (ROOT / row['stderr']).write_bytes(command.stderr)
        row['stdout_sha256'] = sha(ROOT / row['stdout'])
        row['stderr_sha256'] = sha(ROOT / row['stderr'])
        receipt['commands'].append(row)
    receipt['sources_after'] = {name: sha(ROOT / name) for name in source_names}
    receipt['upstream_inputs_after'] = {name: sha(UPSTREAM / name) for name in inputs}
    receipt['upstream_manifest_after'] = sha(UPSTREAM / 'SHA256SUMS')
    receipt['ended_utc'], receipt['ended_unix_ns'] = now(), time.time_ns()
    ok = (receipt['sources_after'] == before and receipt['upstream_manifest_after'] == UPSTREAM_MANIFEST and
          receipt['upstream_inputs_after'] == {name: row['sha256'] for name, row in inputs.items()} and
          all(row['returncode'] == 0 and (ROOT / row['stderr']).read_bytes() == b''
              for row in receipt['commands']) and
          (ROOT / 'normal.stdout').read_bytes() == (ROOT / 'optimized.stdout').read_bytes())
    receipt['status'] = 'CAPTURE_PASS' if ok else 'CAPTURE_FAIL'
    (ROOT / 'receipt.json').write_text(json.dumps(receipt, sort_keys=True, indent=2) + '\n')
    print(json.dumps(dict(status=receipt['status'], commands=len(receipt['commands']),
                          native_invocations=0, GCP_used=False), sort_keys=True))
    return 0 if ok else 1


if __name__ == '__main__':
    sys.exit(main())
