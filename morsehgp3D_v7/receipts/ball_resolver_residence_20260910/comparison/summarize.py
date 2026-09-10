#!/usr/bin/env python3
"""Close private qualification proofs; keep raw failed mutant evidence intact."""
import difflib
import hashlib
import json
from pathlib import Path

BASE = Path('/workspaces/E-HGP/build/v7_ball_resolver_opt_20260910_r2')

def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

summary = {'schema': 'resolver-cache-private-summary-v1', 'gcp_used': False, 'gates': {}, 'mutants': {}}
geometry = None
for mode in ['baseline_o2', 'plain_o2', 'seed_o2', 'plain_san', 'seed_san']:
    run = BASE / ('run_final_' + mode)
    receipt = json.loads((run / 'receipt.json').read_text())
    if receipt.get('antidrift') is not True:
        raise RuntimeError(mode + ': no antidrift')
    gate = json.loads((run / 'full_ball_tower_gate_selftest.stdout').read_text())
    obj = {key: value for key, value in gate.items() if key not in ['anchor_hits', 'intruder_queries', 'same_radius_steps']}
    if geometry is None:
        geometry = obj
    if obj != geometry:
        raise RuntimeError(mode + ': bounded oracle output differs')
    summary['gates'][mode] = {'receipt_sha256': sha(run / 'receipt.json'), 'oracle': gate,
        'work_stdout_sha256': sha(run / 'full_ball_work_gate_selftest.stdout')}
for kind in ['plain', 'seed']:
    for gate in ['full_ball_tower_gate', 'full_ball_work_gate', 'resolver_cache_gate']:
        a = BASE / ('run_final_' + kind + '_o2') / (gate + '_selftest.stdout')
        b = BASE / ('run_final_' + kind + '_san') / (gate + '_selftest.stdout')
        if a.read_bytes() != b.read_bytes():
            raise RuntimeError(kind + ': O2/SAN stdout differs: ' + gate)
for mode in ['identity', 'normalization', 'order']:
    run = BASE / ('run_final_mutant_' + mode)
    receipt = json.loads((run / 'receipt.json').read_text())
    after = {str(p.relative_to(run / 'candidate')): sha(p)
        for p in sorted((run / 'candidate').rglob('*')) if p.is_file() and p.suffix in ['.cpp', '.hpp', '.cu', '.cuh']}
    if after != receipt['sources_before']:
        raise RuntimeError(mode + ': mutant source drift')
    summary['mutants'][mode] = {'receipt_sha256': sha(run / 'receipt.json'), 'antidrift_rechecked': True,
        'returncode': receipt['commands'][-1]['returncode'],
        'stderr': (run / 'selftest.stderr').read_text()}
summary['mutants']['order']['interpretation'] = (
    'Survives: full sorted zero-padded distinct nonnegative keys already encode K>=2. '
    'Per-order reset bounds residence and namespace, not an independently necessary semantic condition for this exact layout.')
for name in ['src/forest/full_ball_tower.hpp', 'bench/full_ball_tower_probe.cpp']:
    before = BASE / 'baseline_final' / name
    after = BASE / 'cache_final' / name
    diff = ''.join(difflib.unified_diff(before.read_text().splitlines(keepends=True), after.read_text().splitlines(keepends=True),
        fromfile='a/morsehgp3D_v7/' + name, tofile='b/morsehgp3D_v7/' + name))
    patch = BASE / (Path(name).stem + '.patch')
    patch.write_text(diff)
    summary[name] = {'baseline_sha256': sha(before), 'candidate_sha256': sha(after), 'patch_sha256': sha(patch)}
diagnostics = BASE / 'run_final_diagnostics'
if (diagnostics / 'summary.json').is_file():
    receipt = json.loads((diagnostics / 'receipt.json').read_text())
    if receipt.get('antidrift') is not True:
        raise RuntimeError('diagnostics source drift')
    summary['diagnostics'] = json.loads((diagnostics / 'summary.json').read_text())
    summary['diagnostics_receipt_sha256'] = sha(diagnostics / 'receipt.json')
    rows = json.loads((diagnostics / 'results.json').read_text())
    for n in [400, 1000]:
        branches = {kind: [row for row in rows if row['n'] == n and row['variant'] == kind][0]
                    for kind in ['baseline', 'plain', 'seed']}
        a, b, c = (branches[kind] for kind in ['baseline', 'plain', 'seed'])
        if not (c['resolver_meb_calls'] < b['resolver_meb_calls'] < a['resolver_meb_calls'] and
                c['resolver_supports'] < b['resolver_supports'] < a['resolver_supports']):
            raise RuntimeError('physical MEB/support saving must be nonvacuous for both mechanisms')
        for row in rows:
            if row['n'] != n or row['variant'] == 'baseline':
                continue
            if row['cache_queries'] != row['cache_hits'] + row['anchor_hits']:
                raise RuntimeError('one exact disposition per lookup')
            if row['resolver_meb_calls'] != row['anchor_hits'] + row['intruder_queries']:
                raise RuntimeError('paid MEB count is not initial misses plus actual descents')
            if row['cache_stores'] != row['anchor_hits'] + row['cache_seed_stores']:
                raise RuntimeError('physical store count differs')
            if row['cache_reset_slots'] != row['cache_slots'] * (row['kmax'] - 1):
                raise RuntimeError('reset physical count differs')
            if row['cache_bytes'] != row['cache_slots'] * 48:
                raise RuntimeError('entry memory count differs')
            if not 16 * n <= row['cache_slots'] < 32 * n:
                raise RuntimeError('linear capacity interval differs')
            if row['variant'] == 'seed' and not row['cache_seed_stores'] > 0:
                raise RuntimeError('seed path vacuous')
            if row['variant'] == 'plain' and row['cache_seed_stores'] != 0:
                raise RuntimeError('plain arm used seeds')
    summary['physical_counter_relations'] = 'passed: nonvacuous saving, exact calls/dispositions/stores/resets/bytes'
quiet = BASE / 'run_quiet_diagnostics'
if (quiet / 'summary.json').is_file():
    receipt = json.loads((quiet / 'receipt.json').read_text())
    if receipt.get('antidrift') is not True:
        raise RuntimeError('supplemental triplet source drift')
    reference = json.loads((diagnostics / 'results.json').read_text())
    for row in json.loads((quiet / 'results.json').read_text()):
        other = next(r for r in reference if r['n'] == row['n'] and r['variant'] == row['variant'])
        for field in ['input_digest', 'payload_digest', 'resolver_meb_calls', 'resolver_supports', 'resolver_power_tests']:
            if row[field] != other[field]:
                raise RuntimeError('supplemental triplet object/work mismatch')
    summary['supplemental_triplet'] = json.loads((quiet / 'summary.json').read_text())
    summary['supplemental_triplet_receipt_sha256'] = sha(quiet / 'receipt.json')
    summary['supplemental_triplet_isolation'] = 'not_claimed: no compiler before; one compiler at final context snapshot'
(BASE / 'summary.json').write_text(json.dumps(summary, indent=2) + '\n')
print(json.dumps(summary.get('diagnostics', summary['gates']), indent=2))
