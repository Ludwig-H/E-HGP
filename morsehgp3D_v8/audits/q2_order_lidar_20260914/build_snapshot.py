#!/usr/bin/env python3
"""Build an immutable published product plus the explicit LiDAR audit adapter."""
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
COMMIT = 'e3af11a7b2ecba4a71929c7112610ff2618f813e'


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def require(ok, message):
    if not ok:
        raise RuntimeError(message)


def build(name):
    require(name.isidentifier(), 'invalid capture name')
    receipt, archive = BASE / (name + '_BUILD.json'), BASE / (name + '_sources.zip')
    snapshot, output = BASE / '.snapshot' / name, BASE / '.build' / name
    require(not any(p.exists() for p in (receipt, archive, snapshot, output)), 'refuse overwrite')
    product = subprocess.check_output(['git', 'ls-tree', '-r', '--name-only', COMMIT,
                                       'morsehgp3D_v8/src'], cwd=ROOT, text=True).splitlines()
    product = [p for p in product if p.endswith(('.hpp', '.cpp'))]
    product.append('morsehgp3D_v8/tests/q2_witness_order_gate.cpp')
    payloads = {p: subprocess.check_output(['git', 'show', COMMIT + ':' + p], cwd=ROOT) for p in product}
    audit = [BASE / p for p in ('build_snapshot.py', 'lidar_order_probe.cpp', 'measure.py')]
    audit += [BASE.parent / 'q2_front_20260914/measure.py',
              BASE.parent / 'q2_front_20260914/lidar_q2_probe.cpp',
              BASE.parent / 'q2_sibling_20260914/measure.py']
    audit_pins = {str(p.relative_to(ROOT)): sha(p) for p in audit}
    payloads.update({str(p.relative_to(ROOT)): p.read_bytes() for p in audit})
    snapshot.mkdir(parents=True)
    output.mkdir(parents=True)
    record = dict(schema='mhgp8_q2_order_lidar_build_v1', status='building', source_commit=COMMIT,
                  started_utc=datetime.now(timezone.utc).isoformat(), platform=platform.platform(),
                  public_status='not_claimed', compiler=subprocess.check_output(['g++', '--version'], text=True),
                  worktree=subprocess.check_output(['git', 'status', '--short'], cwd=ROOT, text=True),
                  audit_inputs=audit_pins, source_sha256={p: hashlib.sha256(v).hexdigest()
                                                       for p, v in payloads.items()}, steps=[])

    def save():
        receipt.write_text(json.dumps(record, indent=2, sort_keys=True) + '\n')

    def run(command, expected=0):
        step = dict(command=command, expected_returncode=expected)
        record['steps'].append(step)
        save()
        completed = subprocess.run(command, cwd=ROOT, capture_output=True, text=True, timeout=180)
        step.update(returncode=completed.returncode, stdout=completed.stdout, stderr=completed.stderr)
        save()
        require(completed.returncode == expected, 'build or gate failed')

    save()
    try:
        with zipfile.ZipFile(archive, 'x', compression=zipfile.ZIP_DEFLATED) as z:
            for path, data in payloads.items():
                target = snapshot / path
                target.parent.mkdir(parents=True, exist_ok=True)
                target.write_bytes(data)
                z.writestr(path, data)
        record.update(archive=str(archive.relative_to(ROOT)), archive_sha256=sha(archive))
        save()
        common = ['g++', '-std=c++20', '-O2', '-DNDEBUG', '-Wall', '-Wextra', '-Wpedantic', '-Werror',
                  '-I', str(snapshot / 'morsehgp3D_v8/src')]
        cpp = [str(snapshot / p) for p in product if '/src/' in p and p.endswith('.cpp')]
        gate = output / 'q2_witness_order_gate'
        run(common + [str(snapshot / 'morsehgp3D_v8/tests/q2_witness_order_gate.cpp')] + cpp + ['-o', str(gate)])
        run([str(gate), '--selftest'])
        record['gate_sha256'] = sha(gate)
        binary = output / 'lidar_order_probe'
        run(common + [str(snapshot / (BASE / 'lidar_order_probe.cpp').relative_to(ROOT))] + cpp + ['-o', str(binary)])
        run([str(binary)], expected=1)
        record.update(binary=str(binary.relative_to(ROOT)), binary_sha256=sha(binary))
        require(all(sha(snapshot / p) == h for p, h in record['source_sha256'].items()), 'changed snapshot')
        require(all(sha(ROOT / p) == h for p, h in audit_pins.items()), 'changed audit input')
        record['status'] = 'passed'
    except BaseException as error:
        record.update(status='failed', error=type(error).__name__ + ': ' + str(error))
        raise
    finally:
        record['finished_utc'] = datetime.now(timezone.utc).isoformat()
        save()
    print(json.dumps(dict(status=record['status'], receipt=str(receipt))))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--name', default='r1')
    build(parser.parse_args().name)
