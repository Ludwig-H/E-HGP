#!/usr/bin/env python3
"""Freeze the audit adapter and published e3af11a7 sources, build and judge."""
from __future__ import annotations
import argparse
from datetime import datetime, timezone
import hashlib
import json
import os
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


def build(name, sanitize):
    require(name.isidentifier(), 'invalid build name')
    receipt, archive = BASE / (name+'_BUILD.json'), BASE / (name+'_sources.zip')
    snapshot, output = BASE/'.snapshot'/name, BASE/'.build'/name
    require(not any(p.exists() for p in (receipt, archive, snapshot, output)), 'refuse overwrite')
    paths = subprocess.check_output(['git', 'ls-tree', '-r', '--name-only', COMMIT,
                                     'morsehgp3D_v8/src'], cwd=ROOT, text=True).splitlines()
    paths = [p for p in paths if p.endswith(('.hpp', '.cpp'))]
    paths += ['morsehgp3D_v8/bench/front_fixtures.hpp']
    payloads = {p: subprocess.check_output(['git', 'show', COMMIT+':'+p], cwd=ROOT) for p in paths}
    own = [BASE/p for p in ('node_pool.hpp', 'bridge.hpp', 'support_io.hpp', 'probe.cpp', 'gate.cpp', 'build.py')]
    own += [BASE.parent/'q2_order_lidar_20260914/lidar_order_probe.cpp']
    pins = {str(p.relative_to(ROOT)): sha(p) for p in own}
    payloads.update({str(p.relative_to(ROOT)): p.read_bytes() for p in own})
    snapshot.mkdir(parents=True)
    output.mkdir(parents=True)
    compiler = 'clang++' if sanitize else 'g++'
    record = dict(schema='mhgp8_audit_pool_bridge_build_v1', status='building', source_commit=COMMIT,
                  audit_adapter=True, public_status='not_claimed', gcp_used=False,
                  sanitizer=sanitize, started_utc=datetime.now(timezone.utc).isoformat(),
                  compiler=subprocess.check_output([compiler, '--version'], text=True),
                  platform=platform.platform(), worktree=subprocess.check_output(
                      ['git', 'status', '--short'], cwd=ROOT, text=True), audit_inputs=pins,
                  source_sha256={p: hashlib.sha256(data).hexdigest() for p, data in payloads.items()}, steps=[])

    def save():
        receipt.write_text(json.dumps(record, indent=2, sort_keys=True, allow_nan=False)+'\n')

    def run(command, expected=0, env=None):
        step = dict(command=command, expected_returncode=expected)
        record['steps'].append(step)
        save()
        r = subprocess.run(command, cwd=ROOT, capture_output=True, text=True, timeout=240, env=env)
        step.update(returncode=r.returncode, stdout=r.stdout, stderr=r.stderr)
        save()
        require(r.returncode == expected, 'build or gate failed: '+r.stderr[-3000:])

    save()
    try:
        with zipfile.ZipFile(archive, 'x', compression=zipfile.ZIP_DEFLATED) as z:
            for p, data in payloads.items():
                dest = snapshot/p
                dest.parent.mkdir(parents=True, exist_ok=True)
                dest.write_bytes(data)
                z.writestr(p, data)
        record.update(archive=str(archive.relative_to(ROOT)), archive_sha256=sha(archive))
        own_snapshot = snapshot/BASE.relative_to(ROOT)
        common = [compiler, '-std=c++20', '-Wall', '-Wextra', '-Wpedantic', '-Werror',
                  '-I', str(snapshot/'morsehgp3D_v8/src'), '-I', str(snapshot/'morsehgp3D_v8/bench')]
        common += ['-O1', '-g', '-fno-omit-frame-pointer', '-fsanitize=address,undefined'] if sanitize else ['-O2', '-DNDEBUG']
        # q2_census.cpp is included unmodified by bridge.hpp, exactly once.
        cpp = [str(snapshot/p) for p in paths if p.endswith('.cpp') and not p.endswith('/q2_census.cpp')]
        gate, probe = output/'gate', output/'probe'
        run(common+[str(own_snapshot/'gate.cpp')]+cpp+['-o', str(gate)])
        environment = dict(os.environ)
        if sanitize:
            environment.update(ASAN_OPTIONS='detect_leaks=1:halt_on_error=1', UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1')
        record['gate_environment'] = {k: environment[k] for k in ('ASAN_OPTIONS', 'UBSAN_OPTIONS') if k in environment}
        run([str(gate)], env=environment)
        record['gate_sha256'] = sha(gate)
        if not sanitize:
            run(common+[str(own_snapshot/'probe.cpp')]+cpp+['-o', str(probe)])
            run([str(probe)], expected=1)
            record.update(binary=str(probe.relative_to(ROOT)), binary_sha256=sha(probe))
        require(all(sha(snapshot/p) == h for p,h in record['source_sha256'].items()), 'snapshot changed')
        require(all(sha(ROOT/p) == h for p,h in pins.items()), 'audit source changed')
        record['status'] = 'passed'
    except BaseException as error:
        record.update(status='failed', error_type=type(error).__name__, error=str(error))
        raise
    finally:
        record['finished_utc'] = datetime.now(timezone.utc).isoformat()
        save()
    print(json.dumps(dict(status=record['status'], receipt=str(receipt))))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--name', default='r1')
    parser.add_argument('--sanitize', action='store_true')
    args = parser.parse_args()
    build(args.name, args.sanitize)
