#!/usr/bin/env python3
"""Capture and build the provisional q2 integration inside the audit only."""
from __future__ import annotations

import argparse
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import platform
import subprocess
import zipfile

BASE = Path(__file__).resolve().parent
ROOT = BASE.parents[2]


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def run(command):
    result = subprocess.run(command, capture_output=True, text=True)
    return dict(command=command, returncode=result.returncode,
                stdout=result.stdout, stderr=result.stderr)


def write(path, value):
    path.write_text(json.dumps(value, indent=2, sort_keys=True, allow_nan=False) + '\n')


def build(name):
    require(name.isidentifier(), 'name must be a plain identifier')
    receipt = BASE / (name + '_BUILD.json')
    snapshot = BASE / '.snapshot' / name
    output = BASE / '.build' / name
    archive = BASE / (name + '_sources.zip')
    require(not any(p.exists() for p in (receipt, snapshot, output, archive)),
            'refuse to replace a capture')
    earlier = json.loads((BASE.parent / 'lidar08_20260914/BUILD.json').read_text())
    names = [p for p in earlier['source_sha256'] if '/src/' in p]
    names += ['morsehgp3D_v8/src/pipeline/wspd_q2_census.hpp',
              'morsehgp3D_v8/tests/wspd_q2_census_gate.cpp',
              str((BASE / 'lidar_q2_probe.cpp').relative_to(ROOT))]
    payloads = {p: (ROOT / p).read_bytes() for p in sorted(names)}
    require(all((ROOT / p).read_bytes() == data for p, data in payloads.items()),
            'source changed during capture')
    snapshot.mkdir(parents=True)
    output.mkdir(parents=True)
    with zipfile.ZipFile(archive, 'x', compression=zipfile.ZIP_DEFLATED) as zipped:
        for path, data in payloads.items():
            destination = snapshot / path
            destination.parent.mkdir(parents=True, exist_ok=True)
            destination.write_bytes(data)
            info = zipfile.ZipInfo(path, date_time=(2026, 9, 14, 0, 0, 0))
            info.compress_type = zipfile.ZIP_DEFLATED
            info.external_attr = 0o644 << 16
            zipped.writestr(info, data)
    value = dict(schema='mhgp8_q2_lidar_snapshot_v1', status='building',
                 public_status='not_claimed', scope='provisional_q2_front_census_only',
                 started_utc=datetime.now(timezone.utc).isoformat(),
                 base_head=run(['git', 'rev-parse', 'HEAD']),
                 worktree=run(['git', 'status', '--short']),
                 platform=platform.platform(), compiler=run(['g++', '--version']),
                 builder_sha256=sha(Path(__file__)), archive_sha256=sha(archive),
                 source_sha256={p: hashlib.sha256(data).hexdigest() for p, data in payloads.items()},
                 runs=[])
    write(receipt, value)
    common = ['g++', '-std=c++20', '-O2', '-DNDEBUG', '-Wall', '-Wextra',
              '-Wpedantic', '-Werror', '-I', str(snapshot / 'morsehgp3D_v8/src')]
    sources = [str(snapshot / p) for p in names if '/src/' in p and p.endswith('.cpp')]
    targets = [('wspd_q2_census_gate', 'morsehgp3D_v8/tests/wspd_q2_census_gate.cpp'),
               ('lidar_q2_probe', str((BASE / 'lidar_q2_probe.cpp').relative_to(ROOT)))]
    try:
        for target, source in targets:
            binary = output / target
            record = dict(target=target, compile=run(common + [str(snapshot / source)] + sources + ['-o', str(binary)]))
            value['runs'].append(record)
            write(receipt, value)
            require(record['compile']['returncode'] == 0, 'compilation failed: ' + target)
            record['binary_sha256'] = sha(binary)
            if target.endswith('_gate'):
                record['execution'] = run([str(binary), '--selftest'])
                write(receipt, value)
                require(record['execution']['returncode'] == 0, 'geometry gate failed')
            print(json.dumps({'target': target, 'status': 'passed'}), flush=True)
        require(all(sha(snapshot / p) == expected for p, expected in value['source_sha256'].items()),
                'snapshot changed during build')
        value['status'] = 'passed'
    except BaseException as error:
        value['status'] = 'failed'
        value['error'] = type(error).__name__ + ': ' + str(error)
        raise
    finally:
        value['live_sources_changed_since_snapshot'] = [
            p for p, expected in value['source_sha256'].items() if sha(ROOT / p) != expected]
        value['finished_utc'] = datetime.now(timezone.utc).isoformat()
        write(receipt, value)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--name', required=True)
    build(parser.parse_args().name)
