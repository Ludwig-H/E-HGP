#!/usr/bin/env python3
"""Pure worker boundary tests; no CUDA, subprocess, cloud or VM mutation."""
from __future__ import annotations

import copy
import importlib.util
import json
from pathlib import Path
import tempfile


HERE = Path(__file__).resolve().parent
spec = importlib.util.spec_from_file_location('anchor_meb_worker', HERE / 'anchor_meb_worker_v7.py')
worker = importlib.util.module_from_spec(spec)
spec.loader.exec_module(worker)
checks = 0
rejects = 0


def need(ok, reason):
    global checks
    checks += 1
    if not ok:
        raise ValueError(reason)


def reject(call, reason):
    global rejects
    try:
        call()
    except (ValueError, OSError):
        rejects += 1
    else:
        raise ValueError('boundary case survived: ' + reason)


def main():
    need(worker.sha(HERE / 'full_probe_worker_v7.py') == worker.SUPPORT_SHA256, 'reviewed support pin')
    need(worker.sha(HERE.parent / worker.STRICT_HOST) == worker.STRICT_HOST_SHA256, 'strict adapter pin')
    original = dict(status='passed', backend='CUDA', sm='12.0', scope=worker.SCOPE,
                    device_executed=True, gcp_used=False, device_name='synthetic SM120 for parser only',
                    checks=16592, compared=605, q1=82, q2=393, q3=110, q4=20,
                    extra_shells=197, rejections=44, failures=0, causal_flags=4,
                    abi_version=1, request_bytes=72, selection_bytes=112, positions=350,
                    resident_h2d_bytes=8400, batch_h2d_bytes=111320, batch_d2h_bytes=67760,
                    batch_launches=1, host_validation_powers=1997, host_materializations=605,
                    reported_selection_powers=9697)
    raw = json.dumps(original)
    need(worker.device_gate_summary(raw + '\n', 0) == original, 'complete synthetic device wire')
    for code in (1, 2, -9, False, None):
        reject(lambda code=code: worker.device_gate_summary(raw, code), 'nonzero or invalid exit code')
    bad_wires = ['', raw + '\n' + raw, '[]', 'null', raw.replace('{', '{"checks":0,', 1)]
    bad_wires += [raw.replace('{', '{"nested":{"a":0,"a":1},', 1)]
    bad_wires += [raw.replace('{', '{"not_finite":' + value + ',', 1)
                  for value in ('NaN', 'Infinity', '-Infinity', '1e999')]
    for field, value in (('status', 'partial'), ('backend', 'stub-hote'), ('sm', '8.6'),
                         ('scope', 'full_tower'), ('device_executed', False), ('device_executed', 1),
                         ('gcp_used', True), ('device_name', ''), ('device_name', 'x' * 513),
                         ('compared', 0), ('compared', True), ('checks', 1), ('q1', 0), ('q2', -1),
                         ('q3', 111), ('q4', 0), ('extra_shells', 0), ('rejections', 0), ('failures', 1),
                         ('causal_flags', 0), ('abi_version', 2), ('request_bytes', 71),
                         ('selection_bytes', 111), ('positions', 0), ('resident_h2d_bytes', 1),
                         ('batch_h2d_bytes', 1), ('batch_d2h_bytes', 1), ('batch_launches', 2),
                         ('host_validation_powers', 0), ('host_materializations', 604),
                         ('reported_selection_powers', False)):
        changed = copy.deepcopy(original)
        changed[field] = value
        bad_wires.append(json.dumps(changed))
    for field in original:
        changed = copy.deepcopy(original)
        del changed[field]
        bad_wires.append(json.dumps(changed))
    for bad in bad_wires:
        reject(lambda bad=bad: worker.device_gate_summary(bad, 0), 'malformed/partial/false device wire')

    argv = worker.compile_command('/cuda/nvcc', Path('/source'), Path('/output'), Path('/tooling'))
    for flag in ('-O3', '-DNDEBUG', '-std=c++20', '-arch=sm_120', '-fmad=false', '--expt-relaxed-constexpr',
                 '-Xcompiler=-Wall,-Wextra,-Wpedantic,-Werror,-pthread', '-MMD', '-MF'):
        need(flag in argv, 'strict device build flags')
    need(argv[argv.index('-ccbin') + 1] == '/output/nvcc_strict_host.py', 'reviewed host adapter')
    need(argv[-1] == '/tooling/anchor_meb_gate' and argv[-3] == '/source/' + worker.DEVICE_GATE,
         'real gate TU and executable outside capture')
    need(not any('FULL_BALL_CUDA' in value for value in argv), 'not the census/FULL probe')
    key_argv = worker.compile_command('/cuda/nvcc', Path('/source'), Path('/output'), Path('/tooling'), 'key_gate')
    need(key_argv[-1] == '/tooling/ball_key_gate' and key_argv[-3] == '/source/' + worker.KEY_GATE,
         'optional arithmetic gate source and binary')
    reject(lambda: worker.compile_command('nvcc', Path('/s'), Path('/o'), Path('/t'), 'other'), 'unknown gate')
    arithmetic = dict(status='passed', backend='CUDA', device_executed=True, public_status='not_claimed',
                      vectors_sha256='a' * 64, cases=13500, checked_words=324000, raw_cases=4000,
                      gcd_cases=2000, division_cases=4000, support_cases=3500, rejected_cases=20,
                      input_bytes=112, output_bytes=96, alignment=8, device_arch=1200)
    need(worker.key_gate_summary(json.dumps(arithmetic), 0, 'a' * 64) == arithmetic, 'synthetic arithmetic wire')
    for key, value in (('backend', 'host_stub'), ('device_executed', False), ('device_arch', 0),
                       ('cases', 13499), ('checked_words', 323999), ('rejected_cases', 0),
                       ('gcd_cases', True), ('public_status', 'exact'), ('vectors_sha256', 'b' * 64),
                       ('alignment', 16), ('input_bytes', 111), ('output_bytes', 95)):
        changed = dict(arithmetic)
        changed[key] = value
        reject(lambda changed=changed: worker.key_gate_summary(json.dumps(changed), 0, 'a' * 64),
               'arithmetic authority/ABI/corpus boundary')
    reject(lambda: worker.key_gate_summary(json.dumps(arithmetic), 1, 'a' * 64), 'failed arithmetic process')

    with tempfile.TemporaryDirectory(prefix='mhgp7-meb-worker-pin-') as directory:
        root = Path(directory).resolve()
        manifest = {}
        for name in (worker.SUPPORT, worker.LEGACY_PROBE, worker.DEVICE_GATE, worker.DEVICE_FIXTURE, worker.STRICT_HOST):
            path = root / name
            path.parent.mkdir(parents=True, exist_ok=True)
            original_path = (HERE / 'full_probe_worker_v7.py' if name == worker.SUPPORT else
                             HERE.parent / worker.STRICT_HOST if name == worker.STRICT_HOST else None)
            path.write_bytes(original_path.read_bytes() if original_path else b'fixture only\n')
            manifest[name] = worker.sha(path)
        need(worker.source_map(root, manifest) == manifest, 'snapshot accepted')
        reject(lambda: worker.source_map(root, dict(manifest, **{worker.KEY_GATE: 'a' * 64})),
               'partial optional arithmetic gate rejected')
        loaded = worker.load_support(root, manifest)
        need(loaded.Worker.__module__ == 'mhgp7_reviewed_full_session_support', 'exact pinned import')
        need(not list(root.rglob('__pycache__')), 'support import does not write snapshot')
        for name in (worker.SUPPORT, worker.LEGACY_PROBE, worker.DEVICE_GATE, worker.DEVICE_FIXTURE, worker.STRICT_HOST):
            changed = dict(manifest)
            del changed[name]
            reject(lambda changed=changed: worker.source_map(root, changed), 'mandatory source absent')
        for name in ('morsehgp3D_v7/tests/other.cpp', '../escape', '/outside',
                     'morsehgp3D_v7/src/../other.hpp', 'morsehgp3D_v7//src/other.hpp'):
            reject(lambda name=name: worker.source_map(root, dict(manifest, **{name: 'a' * 64})), 'scope/traversal')
        adapter = root / worker.STRICT_HOST
        adapter.write_bytes(adapter.read_bytes() + b'\n# altered\n')
        changed = dict(manifest)
        changed[worker.STRICT_HOST] = worker.sha(adapter)
        reject(lambda: worker.source_map(root, changed), 'self-consistent unreviewed adapter')
        adapter.write_bytes((HERE.parent / worker.STRICT_HOST).read_bytes())
        support = root / worker.SUPPORT
        support.write_text('raise RuntimeError("must never import")\n')
        changed = dict(manifest)
        changed[worker.SUPPORT] = worker.sha(support)
        reject(lambda: worker.load_support(root, changed), 'unreviewed support import')
        reject(lambda: worker.source_map(root, changed), 'unreviewed support map')
        reject(lambda: worker.source_map(root, manifest), 'source changed')
    print(json.dumps(dict(status='passed', checks=checks, rejections=rejects,
                          malformed_wires=len(bad_wires), subprocess_invoked=False,
                          device_executed=False, GCP_used=False), sort_keys=True))


if __name__ == '__main__':
    main()
