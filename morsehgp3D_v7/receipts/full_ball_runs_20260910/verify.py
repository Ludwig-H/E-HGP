#!/usr/bin/env python3
"""Portable integrity/observations check; no C++ execution or GPU promotion."""
import hashlib
import io
import json
import math
from pathlib import Path, PurePosixPath
import re
import tarfile

BASE = Path(__file__).resolve().parent


def need(value, reason):
    if not value:
        raise ValueError(reason)


def unique(pairs):
    out = {}
    for key, value in pairs:
        need(key not in out, 'duplicate JSON field')
        out[key] = value
    return out


def read(name):
    return json.loads((BASE / name).read_text(), object_pairs_hook=unique)


def sha(raw):
    return hashlib.sha256(raw).hexdigest()


def safe(name):
    path = PurePosixPath(name)
    return str(path) == name and not path.is_absolute() and '..' not in path.parts and name != '.'


manifest = read('manifest.json')
observed = {}
for path in BASE.rglob('*'):
    need(not path.is_symlink(), 'symlink')
    if path.is_file() and path.name != 'manifest.json':
        raw = path.read_bytes()
        need(not raw.startswith(b'\x7fELF') and path.suffix not in ('.pem', '.pub'), 'binary/key in packet')
        observed[path.relative_to(BASE).as_posix()] = sha(raw)
need(all(safe(name) and re.fullmatch('[0-9a-f]{64}', pin) for name, pin in manifest.items()), 'manifest domain')
need(observed == manifest, 'manifest differs from files')
source_manifest = read('snapshot/source_manifest.json')
source_observed = {}
with tarfile.open(fileobj=io.BytesIO((BASE / 'snapshot/snapshot.tar.gz').read_bytes()), mode='r:gz') as archive:
    for member in archive.getmembers():
        need(member.isfile() and safe(member.name) and member.name.startswith('morsehgp3D_v7/') and
             member.name not in source_observed, 'unsafe/duplicate source archive member')
        raw = archive.extractfile(member).read()
        need(not raw.startswith(b'\x7fELF'), 'ELF inside snapshot')
        source_observed[member.name] = sha(raw)
need(source_observed == source_manifest, 'source snapshot pin')
pins = read('snapshot/pins.json')
for key, path in [('snapshot', 'snapshot/snapshot.tar.gz'), ('manifest', 'snapshot/source_manifest.json'),
                  ('worker', 'gcp/worker.py'), ('controller', 'gcp/controller.py'),
                  ('start', 'gcp/start_and_verify.sh'), ('stop', 'gcp/stop_and_verify.sh')]:
    need(pins[key]['sha256'] == sha((BASE / path).read_bytes()), 'session input pin: ' + key)
before, after = read('local/sources_before.json'), read('local/sources_after.json')
need(before == after and all(source_observed.get(k) == v for k, v in before.items()), 'local source drift')
receipt = read('local/receipt.json')
need(receipt['status'] == 'completed' and receipt['sources_stable'] is True and receipt['gcp_used'] is False,
     'local campaign status')
commands = receipt['commands']
need([r['name'] for r in commands] == ['git_head', 'git_status', 'compiler_version', 'dependencies', 'compile',
                                     'n400', 'n8000', 'n16000', 'n32000'], 'local command inventory')
need(all(r['closed'] is True and r['exit_code'] == 0 and r['ended_ns'] >= r['started_ns'] for r in commands),
     'local processes not closed')
summaries = []
for n in (400, 8000, 16000, 32000):
    row = read('local/n' + str(n) + '.stdout')
    need(row['schema'] == 'mhgp7-full-ball-tower-probe-v1' and row['status'] == 'completed_relative' and
         row['backend'] == 'cpu_reference' and row['public_status'] == 'not_claimed' and
         row['contract_qualified'] is False, 'local authority')
    for key, value in dict(n=n, s=8, kmax=10, orders=10, threads=1, seed=3, coord=65536).items():
        need(type(row[key]) is int and row[key] == value, 'requested local configuration')
    need(row['raw'] >= row['unique'] >= row['balls'] > 0 and row['nodes'] >= n and
         row['vertical_refs'] > 0 and row['contributions'] >= n, 'nonvacuous retained tower')
    times = [row[k] for k in ('index_s', 'generate_s', 'sort_s', 'prefilter_s', 'census_s', 'tower_s', 'digest_s', 'total_s')]
    need(all(type(t) in (int, float) and math.isfinite(t) and t >= 0 for t in times) and
         sum(times[:-1]) <= times[-1] + 0.000001, 'stage timings')
    need(all(re.fullmatch('[0-9a-f]{64}', row[k]) for k in ('input_digest', 'payload_digest')), 'payload pins')
    summaries.append(dict(n=n, total_s=row['total_s'], tower_s=row['tower_s'], nodes=row['nodes']))
need(read('cmake/sources_before.json') == read('cmake/sources_after.json'), 'CTest source drift')
need(read('cmake/receipt.json')['status'] == 'passed' and
     '100% tests passed, 0 tests failed out of 14' in (BASE / 'cmake/ctest.stdout').read_text(), 'CTest completion')
need(all(row['exit_code'] == 0 for row in read('cmake/commands.json')), 'CTest/worker command failure')
attempt = read('gcp/attempt/receipt.json')
need(attempt['status'] == 'shutdown_uncertified' and attempt['targeted_shutdown_certified'] is False and
     'GPUS_ALL_REGIONS' in (BASE / 'gcp/attempt/guarded_start.stderr').read_text(), 'preserved failed start')
closed = read('gcp/recovery/receipt.json')
need(closed['target'] == attempt['target'] and closed['targeted_shutdown_certified'] is True and
     closed['no_new_generation_observed'] is True and closed['other_instances_modified'] is False and
     closed['gpu_benchmark_executed'] is False, 'targeted recovery')
for name in ('describe_before', 'describe_after'):
    value = read('gcp/recovery/' + name + '.stdout')
    need(value['name'] == closed['target']['instance'] and value['status'] == 'TERMINATED' and
         value['lastStartTimestamp'] == closed['generation'] and value['labels']['project'] == 'e-hgp', 'exact stopped target')
need(all(c['exit_code'] == 0 for c in closed['commands']), 'recovery command failure')
print(json.dumps(dict(status='passed', files=len(manifest), source_files=len(source_observed), local=summaries,
                     ctests=14, gpu_benchmark_executed=False, targeted_shutdown_certified=True,
                     contract_qualified=False), sort_keys=True))
