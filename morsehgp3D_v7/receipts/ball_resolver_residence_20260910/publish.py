#!/usr/bin/env python3
"""Publish only text/source proof artifacts; never binaries or active code."""
import hashlib
import json
from pathlib import Path
import shutil

ROOT = Path('/workspaces/E-HGP')
BASE = ROOT / 'build/v7_combined_resolver_20260910'
OLD = ROOT / 'build/v7_ball_resolver_opt_20260910_r2'
OUT = ROOT / 'morsehgp3D_v7/receipts/ball_resolver_residence_20260910'
SUFFIXES = {'.hpp', '.h', '.cpp', '.cu', '.cuh', '.py', '.json', '.md', '.patch', '.d', '.stdout', '.stderr', '.txt', '.sha256'}

def copy(source, dest):
    if source.is_dir():
        for path in sorted(source.rglob('*')):
            if path.is_file() and path.suffix in SUFFIXES:
                copy(path, dest / path.relative_to(source))
    elif source.suffix in SUFFIXES:
        if source.read_bytes().startswith(b'\x7fELF'):
            raise RuntimeError('ELF artifact refused')
        dest.parent.mkdir(parents=True, exist_ok=True)
        if dest.exists():
            if dest.read_bytes() != source.read_bytes():
                raise RuntimeError('published artifact already differs: ' + str(dest))
        else:
            shutil.copy2(source, dest)

combined = ['baseline', 'combined', 'combined_repaired', 'run_o2', 'run_san', 'run_probe_build',
    'run_n1000', 'run_cache_repaired_o2', 'run_cache_repaired_san', 'record.py',
    'record_cache_repaired.py', 'freeze.py', 'instrument_probe.py', 'prepare_repaired.py',
    'baseline_original_pins.json', 'combined_original_pins.json', 'mutant.py']
combined += ['run_mutant_' + name for name in ['identity', 'normalization', 'seed', 'release', 'growth_grouped', 'inert_grouped']]
for name in combined:
    copy(BASE / name, OUT / 'combined' / name)
comparison = ['baseline_final', 'cache_final', 'README.md', 'summary.json', 'final_gate_pins.json',
    'full_ball_tower.patch', 'full_ball_tower_probe.patch', 'record_final.py', 'record_quiet.py',
    'cache_mutants.py', 'summarize.py']
comparison += ['run_final_' + name for name in ['baseline_o2', 'plain_o2', 'seed_o2', 'plain_san', 'seed_san', 'diagnostics']]
comparison += ['run_quiet_diagnostics']
comparison += ['run_final_mutant_' + name for name in ['identity', 'normalization', 'order']]
for name in comparison:
    copy(OLD / name, OUT / 'comparison' / name)
exceptions = [
    {'receipt': 'combined/run_san/receipt.json', 'command': 'facet_resolver_cache_gate_selftest',
     'expected': 0, 'actual': 1, 'stderr_contains': 'alloc-dealloc-mismatch',
     'reason': 'Historical new test allocator omitted nothrow overloads; repaired test is independently green.'},
    {'receipt': 'comparison/run_final_mutant_order/receipt.json', 'command': 'selftest',
     'expected': 1, 'actual': 0, 'stderr_contains': '',
     'reason': 'Negative exploration: complete sorted padded keys already encode cardinality; reset is not independently necessary.'},
]
(OUT / 'DECLARED_EXCEPTIONS.json').write_text(json.dumps(exceptions, indent=2) + '\n')
copy(Path(__file__), OUT / 'publish.py')
summary = {'schema': 'full-resolver-residence-qualified-relative-v1', 'public_status': 'not_claimed',
    'gcp_used': False, 'memory_qualification': '../full_tower_residence_20260910/README.md',
    'headers': {name: hashlib.sha256((ROOT / 'morsehgp3D_v7' / name).read_bytes()).hexdigest()
        for name in ['src/forest/full_ball_tower.hpp', 'src/forest/full_coverage_certificate.hpp', 'tests/facet_resolver_cache_gate.cpp']},
    'gates': {'tower': 'combined/run_o2/full_ball_tower_gate_selftest.stdout',
              'work': 'combined/run_o2/full_ball_work_gate_selftest.stdout',
              'structure': 'combined/run_o2/full_coverage_certificate_gate_selftest.stdout',
              'cache': 'combined/run_cache_repaired_o2/facet_resolver_cache_gate_selftest.stdout'},
    'paired_results': json.loads((BASE / 'run_n1000/results.json').read_text()),
    'mutants': ['identity', 'normalization', 'seed', 'release', 'growth_grouped', 'inert_grouped'],
    'timing_status': 'shared_local_diagnostic_not_contract', 'ELF_included': False}
(OUT / 'summary.json').write_text(json.dumps(summary, indent=2) + '\n')
print('published text/source proofs to', OUT)
