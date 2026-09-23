#!/usr/bin/env python3
"""Run the pinned audit-only batch probe and compare discrete outputs to baseline."""
import argparse
import hashlib
import json
import os
import subprocess
import time
from pathlib import Path

EXPECTED_MANIFEST_SHA = '6e64125abddbfcd589ba61cf747e493e455adec4beeaea81c2739ce98fe16095'


def sha(blob):
    return hashlib.sha256(blob).hexdigest()


def baseline_path(base, density):
    if density == 'full':
        return base / 'full/SUMMARY.json'
    return base / density / 'full/SUMMARY.json'


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('density', choices=('quarter', 'half', 'full'))
    ap.add_argument('--binary', type=Path, required=True)
    ap.add_argument('--inputs', type=Path, required=True)
    ap.add_argument('--baseline-root', type=Path, required=True)
    ap.add_argument('--out', type=Path, required=True)
    args = ap.parse_args()
    if sha((args.inputs / 'MANIFEST.json').read_bytes()) != EXPECTED_MANIFEST_SHA:
        raise RuntimeError('source input manifest hash mismatch')
    manifest = json.loads((args.inputs / 'MANIFEST.json').read_text())
    meta = manifest['datasets'][args.density]['full']
    input_path = args.inputs / meta['points_file']
    id_path = args.inputs / meta['raw_return_ids_file']
    if sha(input_path.read_bytes()) != meta['points_sha256'] or \
       sha(id_path.read_bytes()) != meta['raw_return_ids_sha256']:
        raise RuntimeError('input bytes differ from manifest')
    out = args.out
    if out.exists():
        raise RuntimeError(f'refusing existing run: {out}')
    trace = out / 'trace'
    trace.mkdir(parents=True)
    env = os.environ.copy()
    env['MHGP9_AUDIT_TRACE_DIR'] = str(trace)
    env['MHGP9_AUDIT_RAW_IDS'] = str(id_path)
    cmd = [str(args.binary), str(input_path), '5', '8', '--s=8', '--static=8',
           '--grid=1mm', '--lever=q34_batch_filter=1', '--lever=q34_gpu_filter=0']
    files_before = {'input': sha(input_path.read_bytes()), 'ids': sha(id_path.read_bytes()),
                    'binary': sha(args.binary.read_bytes())}
    started = time.monotonic()
    with (out / 'stdout.json').open('wb') as stdout, (out / 'stderr.txt').open('wb') as stderr:
        result = subprocess.run(cmd, env=env, stdout=stdout, stderr=stderr,
                                check=False, timeout=1800)
    elapsed = time.monotonic() - started
    files_after = {'input': sha(input_path.read_bytes()), 'ids': sha(id_path.read_bytes()),
                   'binary': sha(args.binary.read_bytes())}
    if result.returncode != 0 or files_after != files_before:
        raise RuntimeError(f'probe returned {result.returncode}, changed files: '
                           f'{files_after != files_before}, stderr: '
                           f'{(out / "stderr.txt").read_text()[:1000]}')
    probe = json.loads((out / 'stdout.json').read_text())
    if probe['status'] != 'complete_relative':
        raise RuntimeError(f'probe status {probe["status"]}')
    baseline = json.loads((baseline_path(args.baseline_root, args.density).parent / 'batch.stdout').read_text())
    keys = ('input', 'options', 'generator', 'ledger', 'catalogue', 'orders',
            'tower_digest', 'tower_work', 'status', 'reason')
    for key in keys:
        if probe[key] != baseline[key]:
            raise RuntimeError(f'baseline discrete field differs: {key}')
    summary = {'schema': 'mhgp9_lazy_prefix_run_v1', 'source_commit':
               'c265a5dae4dd92059fc78acc0a1d7f52de9c1435',
               'density': args.density, 'k': 5, 'command': cmd,
               'returncode': result.returncode, 'elapsed_shadow_s': elapsed,
               'file_sha256': files_before, 'stdout_sha256': sha((out / 'stdout.json').read_bytes()),
               'stderr_sha256': sha((out / 'stderr.txt').read_bytes()),
               'trace_part_sha256': {p.name: sha(p.read_bytes()) for p in sorted(trace.glob('part_*.bin'))},
               'ledger': {k: probe['ledger'][k] for k in
                          ('dead_core_loads', 'dead_core_form_sites', 'core_sites',
                           'core_closed_edges', 'dead_core_uniform_tests')},
               'tower_digest': probe['tower_digest'],
               'baseline_discrete_equal': True}
    (out / 'RUN.json').write_text(json.dumps(summary, indent=2, sort_keys=True) + '\n')
    print(json.dumps({'density': args.density, 'k': 5,
                      'loads': summary['ledger']['dead_core_loads'],
                      'forms': summary['ledger']['core_sites'],
                      'digest': summary['tower_digest'],
                      'baseline_equal': summary['baseline_discrete_equal']}))


if __name__ == '__main__':
    main()
