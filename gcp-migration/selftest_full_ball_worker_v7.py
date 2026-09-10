#!/usr/bin/env python3
"""Pure format/plan/import-pin gates: never run NVCC, a worker or GCP."""
from __future__ import annotations

import copy
import importlib.util
import json
from pathlib import Path
import tempfile


def module(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    result = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(result)
    return result


HERE = Path(__file__).resolve().parent
worker = module('new_ball_worker', HERE / 'full_ball_worker_v7.py')
support = module('old_pinned_worker', HERE / 'full_probe_worker_v7.py')
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
        raise ValueError('mutant survived: ' + reason)


def row(n, kmax, s, kind):
    values = dict(schema=worker.SCHEMA, status='completed_relative', public_status='not_claimed',
                  contract_qualified=False, backend=worker.BACKENDS[kind], n=n, s=s,
                  kmax=min(n, kmax), threads=48, seed=3, coord=65536, orders=min(n, kmax),
                  input_digest='a' * 64, payload_digest='b' * 64)
    values.update({key: 0 for key in worker.COUNTERS})
    values.update(raw=200000, unique=150000, balls=120000, nodes=n * 2, contributions=n * 2)
    values.update({key: 0.1 for key in worker.TIMINGS})
    values['total_s'] = 0.8
    return values


def main():
    need(worker.sha(HERE / 'full_probe_worker_v7.py') == worker.SUPPORT_SHA256, 'support source pin')
    accepted, rejected_wires, paired = 0, 0, 0
    for n, kmax, s in ((8, 10, 8), (50000, 10, 8), (50000, 5, 8), (50000, 10, 10), (50000, 5, 12)):
        summaries = {}
        for kind in ('cpu', 'gpu'):
            original = row(n, kmax, s, kind)
            summary = worker.probe_summary(json.dumps(original), 0, n, kmax, s, kind, support.strict_json)
            need(summary['reported_complete'], 'synthetic complete tower')
            accepted += 1
            summaries[kind] = summary
            def refused(raw, code=0):
                return not worker.probe_summary(raw, code, n, kmax, s, kind, support.strict_json)['reported_complete']
            raw = json.dumps(original)
            invalid = ['', raw + '\n' + raw, raw.replace('{', '{"n":0,', 1)]
            invalid += [raw.replace('{', '{"nested":{"same":0,"same":1},', 1)]
            invalid += [raw.replace('{', '{"nonfinite":' + value + ',', 1)
                        for value in ('NaN', 'Infinity', '-Infinity', '1e999')]
            for field, value in (('backend', 'stub-hote'), ('backend', worker.BACKENDS['gpu' if kind == 'cpu' else 'cpu']),
                                 ('schema', 'old'), ('status', 'partial'), ('contract_qualified', True),
                                 ('n', n + 1), ('orders', min(n, kmax) - 1), ('kmax', True), ('threads', 1),
                                 ('s', s + 1), ('raw', -1), ('unique', 300000), ('balls', 0),
                                 ('nodes', n - 1), ('parent_refs', True), ('total_s', 0.1), ('census_s', -0.1),
                                 ('input_digest', 'a' * 63), ('payload_digest', 'G' * 64)):
                changed = copy.deepcopy(original)
                changed[field] = value
                invalid.append(json.dumps(changed))
            for field in ('payload_digest', 'vertical_refs', 'tower_s', 'status'):
                changed = copy.deepcopy(original)
                del changed[field]
                invalid.append(json.dumps(changed))
            for bad in invalid:
                need(refused(bad), 'malformed/partial/false identity rejected')
                rejected_wires += 1
            need(refused(raw, 2) and refused(raw, -9), 'nonzero process cannot promote')
        need(worker.pair_summary(**summaries)['matched'], 'matched CPU/CUDA counters and digest')
        paired += 1
        for field, value in (('payload_digest', 'c' * 64), ('input_digest', 'c' * 64), ('raw', 123),
                             ('representatives', 9), ('same_radius_steps', 8), ('vertical_refs', 17)):
            bad = copy.deepcopy(summaries)
            bad['gpu']['signature'][field] = value
            need(worker.pair_summary(**bad)['status'] == 'diverged', 'counter/digest mismatch')
        need(worker.pair_summary(summaries['cpu'], dict(reported_complete=False))['status'] == 'incomplete',
             'incomplete pair cannot match')
    gate = ('backend=CUDA device=NVIDIA RTX PRO 6000 Blackwell Server Edition sm=12.0\n'
            'census_route_gate=passed backend=CUDA checks=2000 compared=200 extra_shells=3 '
            'q2=100 q3=70 q4=30 rejections=17 failures=0\n')
    need(worker.device_gate_summary(gate, 0)['status'] == 'passed', 'CUDA gate receipt')
    for bad in (gate.replace('CUDA', 'stub-hote'), gate.replace('sm=12.0', 'sm=8.6'),
                gate.replace('failures=0', 'failures=1'), gate.replace('rejections=17', 'rejections=0'),
                gate.replace('compared=200', 'compared=0'), gate.replace('q4=30', 'q4=0'),
                gate.replace('extra_shells=3', 'extra_shells=0'), gate + 'extra\n', gate.splitlines()[0]):
        reject(lambda bad=bad: worker.device_gate_summary(bad, 0), 'CUDA gate mutant')
    reject(lambda: worker.device_gate_summary(gate, 2), 'nonzero CUDA gate')
    plans = worker.compile_commands('/usr/bin/g++', '/usr/local/cuda/bin/nvcc', Path('/source'), Path('/output'))
    need(set(plans) == {'cpu', 'gate', 'gpu'}, 'three isolated binaries')
    for kind, argv in plans.items():
        need('-O3' in argv and '-std=c++20' in argv and '-MMD' in argv and '-MF' in argv, 'strict build/dependency capture')
        if kind == 'cpu':
            need(all(flag in argv for flag in ('-Wall', '-Wextra', '-Wpedantic', '-Werror', '-pthread')), 'strict CPU flags')
        else:
            need('-arch=sm_120' in argv and '-fmad=false' in argv and '--expt-relaxed-constexpr' in argv and
                 '-Xcompiler=-Wall,-Wextra,-Wpedantic,-Werror,-pthread' in argv, 'strict CUDA flags')
            need('-DMHGP7_FULL_BALL_CUDA=1' in argv if kind == 'gpu' else worker.DEVICE_GATE in argv[-3],
                 'isolated CUDA mode/gate source')
    for s in (10, 12):
        need(worker.additional_s_plan(1000, 100, s)['status'] == 'planned' and
             worker.additional_s_plan(300, 100, s)['status'] == 'not_attempted', 'observed-cost launch planning')
    for remaining, observed, s in ((float('nan'), 2, 10), (100, -1, 10), (True, 1, 10), (100, 1, 8)):
        reject(lambda: worker.additional_s_plan(remaining, observed, s), 'invalid planning values')
    with tempfile.TemporaryDirectory(prefix='mhgp7-worker-pin-') as directory:
        root = Path(directory).resolve()
        manifest = {}
        for name in (worker.SUPPORT, worker.LEGACY_PROBE, worker.PROBE, worker.DEVICE_GATE):
            path = root / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes((HERE / 'full_probe_worker_v7.py').read_bytes() if name == worker.SUPPORT else b'fixture\n')
            manifest[name] = worker.sha(path)
        need(worker.source_map(root, manifest) == manifest, 'pinned source scope accepted')
        loaded = worker.load_support(root, manifest)
        need(loaded.Worker.__module__ == 'mhgp7_reviewed_full_session_support', 'exact-path pinned import')
        need(not list(root.rglob('__pycache__')), 'immutable support import creates no bytecode')
        for name in ('morsehgp3D_v7/tests/unrelated.cpp', '../escape', '/outside'):
            reject(lambda name=name: worker.source_map(root, dict(manifest, **{name: 'a' * 64})), 'scope/traversal')
        altered = dict(manifest)
        altered[worker.SUPPORT] = 'a' * 64
        reject(lambda: worker.load_support(root, altered), 'unreviewed support import')
        (root / worker.SUPPORT).write_text('raise RuntimeError("must never execute")\n')
        reject(lambda: worker.load_support(root, manifest), 'changed executable support rejected before import')
        reject(lambda: worker.source_map(root, manifest), 'changed source rejected')
    print(json.dumps(dict(status='passed', checks=checks, pure_rejections=rejects,
                          accepted_complete_summaries=accepted, malformed_wires_rejected=rejected_wires,
                          matched_pairs=paired, subprocess_invoked=False, CUDA_executed=False, GCP_used=False), sort_keys=True))


if __name__ == '__main__':
    main()
