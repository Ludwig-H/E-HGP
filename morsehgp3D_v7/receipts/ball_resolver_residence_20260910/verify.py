#!/usr/bin/env python3
"""Read-only portable proof reader; effective with python -O; never runs tools."""
import hashlib
import json
from pathlib import Path
import sys

BASE = Path(__file__).resolve().parent

def need(condition, reason):
    if not condition:
        raise RuntimeError(reason)

def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def within(path):
    result = path.resolve()
    need(result.is_relative_to(BASE) and result.is_file(), 'missing/outside proof path: ' + str(path))
    return result

def read(path):
    return json.loads(within(path).read_text())

def main():
    manifest = read(BASE / 'MANIFEST.json')
    expected_files = set(manifest['files']) | {'MANIFEST.json'}
    actual_files = {str(path.relative_to(BASE)) for path in BASE.rglob('*') if path.is_file() and '__pycache__' not in path.parts}
    need(actual_files == expected_files, 'manifest file set differs')
    for name, digest in manifest['files'].items():
        path = within(BASE / name)
        need(sha(path) == digest, 'manifest SHA differs: ' + name)
        need(not path.read_bytes().startswith(b'\x7fELF'), 'ELF forbidden')
    exceptions = read(BASE / 'DECLARED_EXCEPTIONS.json')
    exception_map = {(row['receipt'], row['command']): row for row in exceptions}
    used_exceptions = set()
    command_count = 0
    for receipt_path in sorted(BASE.rglob('receipt.json')):
        receipt = read(receipt_path)
        relative = str(receipt_path.relative_to(BASE))
        family = BASE / relative.split('/')[0]
        source_root = family
        for name in ['source', 'candidate']:
            if (receipt_path.parent / name).is_dir():
                source_root = receipt_path.parent / name
        for name, digest in receipt.get('sources_before', {}).items():
            need(sha(within(source_root / name)) == digest, 'source SHA differs: ' + relative + '/' + name)
        if 'sources_after' in receipt:
            need(receipt['sources_after'] == receipt['sources_before'], 'source drift in receipt: ' + relative)
        elif receipt.get('sources_before'):
            need(any(row['receipt'] == relative for row in exceptions), 'unclosed source capture: ' + relative)
        if receipt.get('sources_after'):
            need(receipt.get('antidrift') is True, 'antidrift flag absent: ' + relative)
        for number, command in enumerate(receipt['commands']):
            name = command.get('name', 'compile' if number == 0 else 'selftest')
            for stream in ['stdout', 'stderr']:
                path = within(receipt_path.parent / (name + '.' + stream))
                need(sha(path) == command[stream + '_sha256'], 'command stream SHA differs: ' + relative)
            if command['returncode'] != command['expected']:
                key = (relative, name)
                need(key in exception_map, 'undeclared failed command: ' + str(key))
                declaration = exception_map[key]
                need(command['returncode'] == declaration['actual'] and command['expected'] == declaration['expected'],
                     'declared exception code differs')
                need(declaration['stderr_contains'] in (receipt_path.parent / (name + '.stderr')).read_text(),
                     'declared exception stderr differs')
                used_exceptions.add(key)
            command_count += 1
    need(used_exceptions == set(exception_map), 'declared exceptions not exercised')
    combined = BASE / 'combined'
    for gate in ['full_ball_tower_gate', 'full_ball_work_gate', 'full_coverage_certificate_gate']:
        a = within(combined / 'run_o2' / (gate + '_selftest.stdout')).read_bytes()
        b = within(combined / 'run_san' / (gate + '_selftest.stdout')).read_bytes()
        need(a == b, 'combined O2/SAN mismatch: ' + gate)
    for gate in ['facet_resolver_cache_gate']:
        a = within(combined / 'run_cache_repaired_o2' / (gate + '_selftest.stdout')).read_bytes()
        b = within(combined / 'run_cache_repaired_san' / (gate + '_selftest.stdout')).read_bytes()
        need(a == b, 'repaired O2/SAN mismatch')
    summary = read(BASE / 'summary.json')
    for name, digest in summary['headers'].items():
        need(sha(within(combined / 'combined_repaired' / name)) == digest, 'active snapshot SHA mismatch')
    tower = read(combined / 'run_o2/full_ball_tower_gate_selftest.stdout')
    need(tower['clouds'] == 28 and tower['orders'] == 112 and tower['vertical_checks'] == 45948 and tower['growth_snapshots'] == 8,
         'bounded tower scope differs')
    cache = read(combined / 'run_cache_repaired_o2/facet_resolver_cache_gate_selftest.stdout')
    need(cache['clouds'] == 28 and cache['cache_hits'] == 556 and cache['seeds'] == 352 and cache['released_slots'] == 2752,
         'cache work nonvacuity differs')
    paired = read(combined / 'run_n1000/results.json')
    a, b = paired
    for field in ['input_digest', 'payload_digest', 'nodes', 'parent_refs', 'contributions', 'vertical_refs']:
        need(a[field] == b[field], 'paired output differs: ' + field)
    need(b['resolver_meb_calls'] < a['resolver_meb_calls'] and b['resolver_supports'] < a['resolver_supports'],
         'physical resolver saving vacuous')
    need(b['cache_released_slots'] == b['cache_slots'] and b['cache_seed_stores'] > 0, 'cache not seeded/released')
    need(b['output_arenas_with_vertical_capacity_bytes'] < a['output_arenas_with_vertical_capacity_bytes'], 'output arena saving vacuous')
    initial = read(BASE / 'comparison/run_final_diagnostics/results.json')
    for n in [400, 1000]:
        rows = [row for row in initial if row['n'] == n]
        need(len({row['payload_digest'] for row in rows}) == 1 and len({row['input_digest'] for row in rows}) == 1,
             'initial paired digest differs')
        branches = {kind: next(row for row in rows if row['variant'] == kind) for kind in ['baseline', 'plain', 'seed']}
        need(branches['seed']['resolver_meb_calls'] < branches['plain']['resolver_meb_calls'] < branches['baseline']['resolver_meb_calls'],
             'cache/seed attribution vacuous')
    need(next(row for row in initial if row['n'] == 1000)['payload_digest'] == b['payload_digest'], 'initial/final payload differs')
    for name in summary['mutants']:
        mutant = read(combined / ('run_mutant_' + name) / 'receipt.json')
        need(mutant['commands'][-1]['returncode'] == 1, 'causal mutant survived: ' + name)
    print(json.dumps({'status': 'passed', 'files': len(manifest['files']), 'commands': command_count,
        'declared_historical_exceptions': len(used_exceptions), 'combined_gates': 4, 'causal_mutants': 6,
        'paired_payload': b['payload_digest'], 'authority': 'bounded_relative_not_50k_or_GPU', 'gcp_used': False}, sort_keys=True))

if __name__ == '__main__':
    try:
        main()
    except (RuntimeError, OSError, KeyError, ValueError) as error:
        print('FAIL:', error, file=sys.stderr)
        raise SystemExit(1)
