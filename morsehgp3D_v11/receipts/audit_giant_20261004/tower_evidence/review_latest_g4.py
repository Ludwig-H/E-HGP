#!/usr/bin/env python3
"""Read the published closed worker archive; no raw KITTI, native, fit or cloud."""
import argparse
from collections import Counter
import hashlib
import io
import json
from pathlib import PurePosixPath
import subprocess
import tarfile

parser = argparse.ArgumentParser()
parser.add_argument('--repo', required=True)
args = parser.parse_args()
pin = '0af635a71'
prefix = 'morsehgp3D_v11/receipts/developpement_20261004/bouts_g4/'
checks = 0


def need(condition, message):
    global checks
    checks += 1
    if not condition:
        raise ValueError(message)


def blob(path):
    return subprocess.check_output(['git', '-C', args.repo, 'show', pin + ':' + path])


receipt = json.loads(blob(prefix + 'sessions/claudebouts1/receipt.json'))
archive = blob(prefix + 'sessions/claudebouts1/results.tar.gz')
need(hashlib.sha256(archive).hexdigest() == receipt['results_sha256'], 'closed archive hash')
need(receipt['commit'] == receipt['worker_source'].removeprefix('commit:') and
     receipt['commit'].startswith('f1a53fe1c'), 'worker source/commit')
need(receipt['status'] == 'completed' and receipt['worker_exit_code'] == 0 and
     receipt['evidence_grade'] == 'pushed_commit', 'source and completed worker')
need(receipt['targeted_shutdown_certified'] is True and receipt['stop_exit_code'] == 0 and
     receipt['observed_after']['status'] == 'TERMINATED', 'certified target stop')
need(receipt['generation'] == receipt['closing_generation'] == receipt['observed_after']['lastStartTimestamp'],
     'same target generation')
need(receipt['results_verified'] is True and receipt['remote_summary']['failed_commands'] == [],
     'retrieval and commands completed')

bouts = json.loads(blob(prefix + 'bouts_lot1.json'))['bouts']
inputs = {r['name']: r for r in bouts}
need(len(inputs) == len(bouts) == 360, 'unique requested bouts')
with tarfile.open(fileobj=io.BytesIO(archive), mode='r:gz') as tar:
    files = {}
    for member in tar.getmembers():
        p = PurePosixPath(member.name)
        need(not p.is_absolute() and '..' not in p.parts and (member.isdir() or member.isfile()), 'safe receipt members')
        if member.isfile():
            name = member.name.removeprefix('./')
            need(name not in files, 'no duplicate receipt payload')
            files[name] = tar.extractfile(member).read()
    manifest = files['results/MANIFEST.sha256'].decode()
    expected = {}
    for line in manifest.splitlines():
        digest, name = line.split('  ', 1)
        name = 'results/' + name.removeprefix('./')
        need(name not in expected, 'unique hash manifest rows')
        expected[name] = digest
    need(set(expected) == set(files) - {'results/MANIFEST.sha256'}, 'exhaustive result manifest')
    for name, digest in expected.items():
        need(hashlib.sha256(files[name]).hexdigest() == digest, 'result payload hash')
    gate = json.loads(files['results/cmd/001_gate/files/gate.json'])
    need(gate['verdict'] == 'conforme' and gate['disagreements'] == [], 'bounded exact gate')
    need(len(gate['fixtures']) == 12 and all(f['ok'] is True for f in gate['fixtures']), 'all fixtures passed')
    need(len(gate['mutants']) == 4 and all(f['killed'] is True for f in gate['mutants'].values()), 'four mutants killed')
    results = [json.loads(data) for name, data in files.items()
               if name.startswith('results/cmd/002_bouts/files/lidar/') and name.endswith('.json')]

got = {r['name']: r for r in results}
need(len(got) == len(results) == len(inputs) and set(got) == set(inputs), 'exhaustive unique measured cases')
counts = {}
for name, r in got.items():
    need(r['status'] == 'ok' and set(r['orders']) == {'2', '3', '5', '10'}, 'case and four orders')
    need(r['export']['coord_bits'] == 21 and r['export']['kmax'] == 10 and r['export']['reason'] == 'none',
         'actual export scope')
    kind = inputs[name]['kind']
    c = counts.setdefault(kind, dict(bouts=0, hdbscan_echoue=0, gagnes=0, gagnes_tous_ordres=0, perdus=0))
    c['bouts'] += 1
    fails = {k: min(r['orders'][k]['hdbscan']['best']) <= 0.5 for k in r['orders']}
    hgp_ok = {k: min(r['orders'][k]['margin_r']['best']) > 0.5 for k in r['orders']}
    wins = [k for k in r['orders'] if fails[k] and hgp_ok[k]]
    c['hdbscan_echoue'] += any(fails.values())
    c['gagnes'] += bool(wins)
    c['gagnes_tous_ordres'] += bool(wins) and all(fails.values())
    c['perdus'] += any(not hgp_ok[k] and not fails[k] for k in r['orders'])
published = json.loads(blob('Zoltan/demos/bouts_hgp/evalues.json'))['comptes']
need(counts == published, 'recomputed published counts')
out = dict(commit=pin, worker_source=receipt['worker_source'], checks=checks, verdict='conforme',
           results_sha256=receipt['results_sha256'], results_manifest_sha256=hashlib.sha256(manifest.encode()).hexdigest(),
           payloads_verified=len(expected), cases=len(results), sites_min=min(r['sites'] for r in results),
           sites_max=max(r['sites'] for r in results), object_observations=sum(r['objects'] for r in results),
           sequence_counts=dict(sorted(Counter(r['name'][1:3] for r in results).items())),
           counts=counts, gate={k: gate[k] for k in ('clouds', 'comparisons', 'radius_sites')},
           targeted_shutdown_certified=True,
           scope='best blocks of annotation-selected crops, C++ FULL export + Python points; not flat selection or whole-frame timing')
print(json.dumps(out, indent=2, sort_keys=True))
