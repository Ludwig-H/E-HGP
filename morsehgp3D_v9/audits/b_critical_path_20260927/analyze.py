#!/usr/bin/env python3
"""Rejudge the closed G4 capture, then audit timing scope; no cloud calls."""
import hashlib
import importlib.util
import json
from pathlib import Path
import sys

sys.dont_write_bytecode = True
HERE = Path(__file__).resolve().parent
V9 = HERE.parents[1]
ROOT = V9.parent
PINS = {
    'src/chain/tower_chain.cpp': '7d1eeac6d08ebd1f92142dfae28447824c570f39ed4d18817f736da4117e5297',
    'src/tower/forest/full_ball_tower.hpp': '124d3e9b52b1e6155e5a2ffd2a32cda7e5b403dda21155da2949b9ca8c90c0d0',
    'src/gen/pipeline/wspd_q34.cpp': 'a1273c12eb14154a54a7b2733548d46f1f0c947c8d936ba995daa9a00d30f623',
    'audits/b_gpu_next_20260927/readback.py': 'ba536b5a902055eb843877ef157119f9eefa34842594ee6a444baa71f4680d17',
}


def need(value, reason):
    if not value:
        raise ValueError(reason)


def analyze(snapshot):
    for path, pin in PINS.items():
        need(hashlib.sha256((V9 / path).read_bytes()).hexdigest() == pin,
             'reviewed source changed: ' + path)
    path = V9 / 'audits/b_gpu_next_20260927/readback.py'
    spec = importlib.util.spec_from_file_location('critical_path_g4_authority', path)
    reader = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(reader)
    receipt = V9 / 'receipts/g4_core_warm_20260927'
    authority = reader.build(receipt, snapshot)
    manifest = json.loads((receipt / 'source_manifest.json').read_text())
    for path, pin in PINS.items():
        if path.startswith('src/'):
            need(manifest.get('morsehgp3D_v9/' + path) == pin,
                 'G4 and reviewed source differ: ' + path)
    rows = []
    # ON repetitions; first pass only has the detailed phase ledger.
    for index in (0, 3):
        value = json.loads((receipt / 'vm' / f'probe_{index}.stdout').read_text())
        times, q34, tower = value['times_ms'], value['q34_batch'], value['tower_phases_ms']
        levers = value['options']['levers']
        need(levers['q2_during_device'] and levers['q2_early_census'] and
             levers['q34_dead_core'] and levers['tower_overlap_static'] and
             levers['tower_pipelined_tail'], 'timing scope depends on these levers')
        stages = {key: times[key] for key in ('prepare', 'gen_index', 'q34', 'q2_wait',
                  'q2_census_wait', 'merge', 'tower_index', 'census', 'tower')}
        unassigned = times['chain_total'] - sum(stages.values())
        need(unassigned >= -0.01 and 0 <= q34['filter_ms'] <= times['q34'] and
             0 <= tower['encode'] <= times['tower'], 'phase nesting')
        rows.append(dict(index=index, first_chain_ms=times['chain_total'],
            disjoint_outer_stages_ms=stages, unassigned_outer_ms=unassigned,
            overlapped_q2_ms=times['q2'], overlapped_early_census_ms=times['q2_census'],
            s2_filter_ms=q34['filter_ms'], s2_filter_kernel_ms=q34['filter_kernel_ms'],
            q34_outside_filter_ms=times['q34'] - q34['filter_ms'],
            tower_encode_window_ms=tower['encode'],
            sum_encode_by_k_ms=sum(tower['encode_by_k']),
            tower_outside_encode_ms=times['tower'] - tower['encode'],
            S=value['q34_batch']['survivors'], P=value['generator']['q34_expanded_pairs'],
            catalogue_balls=value['catalogue']['balls'],
            nodes=sum(order['nodes'] for order in value['orders'])))
    return dict(status='passed', scope='historical_first_pass_timing_scope_not_new_benchmark',
                GCP_calls=False, source_commit=authority['source_commit'],
                reviewed_source_pins=PINS, rows=rows)


if __name__ == '__main__':
    need(len(sys.argv) == 2, 'usage: analyze.py <closed-capture-snapshot.tar.gz>')
    print(json.dumps(analyze(Path(sys.argv[1])), indent=2, sort_keys=True))
