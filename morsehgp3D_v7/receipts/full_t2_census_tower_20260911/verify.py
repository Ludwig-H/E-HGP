#!/usr/bin/env python3
"""Read-only verification of the bounded census/FULL qualification receipt."""
import hashlib
import json
from pathlib import Path
import sys


def need(condition, reason):
    if not condition:
        raise ValueError(reason)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def verify(root):
    manifest = json.loads((root / 'manifest.json').read_text())
    need(manifest['schema'] == 'mhgp7-bounded-t2-census-full-v1', 'schema')
    need(manifest['public_status'] == 'not_claimed' and manifest['GCP_used'] is False,
         'scope')
    for name, expected in manifest['files'].items():
        path = root / name
        need(not Path(name).is_absolute() and '..' not in Path(name).parts and
             path.is_file() and not path.is_symlink(), 'file_path:' + name)
        need(sha(path) == expected, 'file_hash:' + name)
    need(manifest['source_commit'] == 'c03f6be8488453486b112811071827a96303ec86', 'source_commit')
    totals = {}
    for mode in ('o2', 'san'):
        capture = root / 'runs' / mode
        before = json.loads((capture / 'sources.before.json').read_text())
        need(before == json.loads((capture / 'sources.after.json').read_text()), 'sources_stable:' + mode)
        for name, expected in before.items():
            need(sha(root / 'source' / name) == expected, 'frozen_source:' + name)
        summary = json.loads((capture / 'summary.json').read_text())
        need(summary['status'] == 'passed' and summary['stable_sources'] is True, 'summary:' + mode)
        expected_names = ['compile', 'historical', 'line12', 'shell14', 'spatial12', 'rejects',
                          'mutant-assignment', 'mutant-open', 'mutant-adjacency', 'mutant-census', 'invalid-args']
        need([r['name'] for r in summary['commands']] == expected_names, 'command_inventory')
        for row in summary['commands']:
            name = row['name']
            recorded = json.loads((capture / (name + '.result.json')).read_text())
            intent = json.loads((capture / (name + '.intent.json')).read_text())
            expected = 1 if name.startswith('mutant-') else (2 if name == 'invalid-args' else 0)
            need(row == recorded and row['exit'] == row['expected_exit'] == intent['expected_exit'] == expected,
                 'command_exit:' + name)
            need(row['causal_diagnostic_matched'] is True, 'causal:' + name)
            if intent['causal_diagnostic']:
                need(intent['causal_diagnostic'] in (capture / (name + '.stderr')).read_text(), 'diagnostic:' + name)
        compiler = json.loads((capture / 'compile.intent.json').read_text())['command']
        for flag in ('-std=c++20', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-MMD'):
            need(flag in compiler, 'compiler_flag:' + flag)
        need(('-fsanitize=address,undefined' in compiler) == (mode == 'san'), 'sanitizer_compiler')
        if mode == 'san':
            environment = json.loads((capture / 'shell14.intent.json').read_text())['sanitizer_environment']
            need(environment['ASAN_OPTIONS'] == 'detect_leaks=1:halt_on_error=1', 'leak_detection')
        historical = json.loads((capture / 'historical.stdout').read_text())
        need(historical == {'status': 'passed', 'oracle_mebs': 1022, 'oracle_components': 14724}, 'historical')
        need(json.loads((capture / 'rejects.stdout').read_text()) == {'status': 'passed', 'rejections': 9}, 'rejections')
        values = []
        for fixture in ('line12', 'shell14', 'spatial12'):
            result = json.loads((capture / (fixture + '.stdout')).read_text())
            need(result['status'] == 'passed' and result['scope'] == 'bounded_real_census_FULL_K1_K10', 'fixture_scope')
            need(result['clouds'] == 2 and result['orders'] == 180 and result['census_runs'] == 6 and
                 result['physical_tower_pairs'] == 16, 'fixture_towers')
            need(result['high_order_facets'] > 0 and result['high_order_verticals'] > 0, 'K9_K10_floor')
            if fixture == 'shell14':
                need(result['shell12_rows'] == 6, 'shell12_floor')
            if fixture == 'spatial12':
                need(result['q3_rows'] > 0 and result['q4_rows'] > 0, 'spatial_support_floor')
            values.append(result)
        totals[mode] = values
    need(totals['o2'] == totals['san'], 'O2_SAN_same_results')
    failed = root / 'history' / 'o2_r1'
    need(json.loads((failed / 'line12.result.json').read_text())['exit'] == 1 and
         'T2.census.exact_geometry_population' in (failed / 'line12.stderr').read_text(), 'failed_first_comparator_preserved')
    return {'status': 'passed', 'scope': manifest['schema'], 'files': len(manifest['files']),
            'towers_per_build': 54, 'orders_per_build': 540, 'GCP_used': False,
            'public_status': 'not_claimed'}


if __name__ == '__main__':
    try:
        if len(sys.argv) > 2:
            raise ValueError('usage: verify.py [receipt_directory]')
        receipt = Path(sys.argv[1]).resolve() if len(sys.argv) == 2 else Path(__file__).resolve().parent
        print(json.dumps(verify(receipt), sort_keys=True))
    except (ValueError, KeyError, OSError, TypeError) as error:
        print('FAIL ' + str(error), file=sys.stderr)
        sys.exit(1)
