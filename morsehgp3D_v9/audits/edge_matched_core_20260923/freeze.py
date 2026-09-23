#!/usr/bin/env python3
"""Keep small, path-independent receipt summaries; leave bulk traces in /tmp."""
import argparse
import hashlib
import json
from pathlib import Path

HERE = Path(__file__).resolve().parent
DENSITIES = ('quarter', 'half', 'full')
CASES = ('full', 'quarter_x_neg_y_neg', 'quarter_x_neg_y_nonneg',
         'quarter_x_nonneg_y_neg', 'quarter_x_nonneg_y_nonneg')


def sha(data):
    return hashlib.sha256(data).hexdigest()


def write_new(path, value):
    if path.exists():
        raise ValueError(f'refusing existing receipt: {path}')
    path.write_text(json.dumps(value, indent=2, sort_keys=True) + '\n')


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--before-root', type=Path, required=True)
    ap.add_argument('--after-root', type=Path, required=True)
    ap.add_argument('--out', type=Path, default=HERE)
    args = ap.parse_args()
    args.out.mkdir(parents=True, exist_ok=True)
    before_patch = json.loads(args.before_root.joinpath('PATCH_MANIFEST.json').read_text())
    after_patch = json.loads(args.after_root.joinpath('PATCH_MANIFEST.json').read_text())
    if before_patch['source_commit'] != after_patch['source_commit'] or \
       before_patch['source_sha256'] != after_patch['source_sha256'] or \
       before_patch['trace_header_sha256'] != sha(HERE.joinpath('edge_trace_audit.hpp').read_bytes()) or \
       after_patch['trace_header_sha256'] != sha(HERE.joinpath('edge_trace_after_audit.hpp').read_bytes()):
        raise ValueError('source/trace provenance differs')
    receipt = {'schema': 'mhgp9_edge_matched_core_receipt_v1',
               'source_commit': before_patch['source_commit'],
               'source_file_sha256': before_patch['source_sha256'],
               'source_archive_sha256': before_patch['git_archive_sha256'],
               'before_trace': {'injected_sha256': before_patch['injected_sha256'],
                                'header_sha256': before_patch['trace_header_sha256'],
                                'patch_metadata_sha256': sha(json.dumps({k: v for k, v in before_patch.items()
                                                                           if k != 'source_dir'},
                                                                          sort_keys=True, separators=(',', ':')).encode()),
                                'binary_sha256': sha(args.before_root.joinpath('build/mhgp9_tower_probe').read_bytes())},
               'after_trace': {'injected_sha256': after_patch['injected_sha256'],
                               'header_sha256': after_patch['trace_header_sha256'],
                               'patch_metadata_sha256': sha(json.dumps({k: v for k, v in after_patch.items()
                                                                          if k != 'source_dir'},
                                                                         sort_keys=True, separators=(',', ':')).encode()),
                               'binary_sha256': sha(args.after_root.joinpath('build/mhgp9_tower_probe').read_bytes())},
               'cases': [], 'post_full': {}}
    for density in DENSITIES:
        run_root = args.before_root if density == 'full' else args.before_root / density
        decomp = json.loads(args.before_root.joinpath(f'DECOMPOSITION_{density.upper()}.json').read_text())
        if decomp['density'] != density or not all(decomp['checks'].values()):
            raise ValueError(f'{density}: decomposition not verified')
        write_new(args.out / f'DECOMPOSITION_{density.upper()}.json', decomp)
        for case in CASES:
            folder = run_root / case
            summary = json.loads(folder.joinpath('SUMMARY.json').read_text())
            if summary['case'] != case or summary.get('density', 'full') != density or \
               summary['trace']['duplicate_records'] or \
               summary['batch']['binary_sha256'] != receipt['before_trace']['binary_sha256']:
                raise ValueError(f'{density}/{case}: invalid case summary')
            engine = json.loads(folder.joinpath('engine.stdout').read_text())
            batch = json.loads(folder.joinpath('batch.stdout').read_text())
            if engine['status'] != batch['status'] or engine['status'] != 'complete_relative' or \
               engine['catalogue'] != batch['catalogue'] or engine['orders'] != batch['orders'] or \
               engine['tower_digest'] != batch['tower_digest']:
                raise ValueError(f'{density}/{case}: engine/batch outputs differ')
            if sha(folder.joinpath('engine.stdout').read_bytes()) != summary['engine']['stdout_sha256'] or \
               sha(folder.joinpath('batch.stdout').read_bytes()) != summary['batch']['stdout_sha256']:
                raise ValueError(f'{density}/{case}: stdout SHA mismatch')
            receipt['cases'].append({
                'density': density, 'sector': case, 'sites': summary['input_sites'],
                'input_sha256': summary['input_sha256'], 'raw_ids_sha256': summary['raw_ids_sha256'],
                'input_fnv': summary['input_fnv'], 'catalogue_balls': batch['catalogue']['balls'],
                'tower_digest': batch['tower_digest'], 'core_loads': summary['trace']['records'],
                'core_forms': summary['trace']['sum_core_sites'],
                'engine_chain_cpu_s': engine['chain_cpu_s'],
                'batch_chain_cpu_s': batch['chain_cpu_s'],
                'engine_chain_wall_ms': engine['times_ms']['chain_total'],
                'batch_chain_wall_ms': batch['times_ms']['chain_total'],
                'engine_stdout_sha256': summary['engine']['stdout_sha256'],
                'batch_stdout_sha256': summary['batch']['stdout_sha256'],
                'trace_parts': summary['trace']['parts']})
    after = json.loads(args.after_root.joinpath('POST_CORE_FULL.json').read_text())
    if not all(after['checks'].values()) or after['after_binary_sha256'] != receipt['after_trace']['binary_sha256']:
        raise ValueError('post-core diagnostic not verified')
    write_new(args.out / 'POST_CORE_FULL.json', after)
    after_case = json.loads(args.after_root.joinpath('full/SUMMARY.json').read_text())
    receipt['post_full'] = {'input_sha256': after_case['input_sha256'],
                            'raw_ids_sha256': after_case['raw_ids_sha256'],
                            'core_loads': after_case['trace']['records'],
                            'core_forms': after_case['trace']['sum_core_sites'],
                            'core_closed_edges': after_case['trace']['core_closed_edges'],
                            'engine_stdout_sha256': after_case['engine']['stdout_sha256'],
                            'batch_stdout_sha256': after_case['batch']['stdout_sha256'],
                            'trace_parts': after_case['trace']['parts']}
    write_new(args.out / 'RESULTS.json', receipt)
    print(json.dumps({'cases': len(receipt['cases']),
                      'source_commit': receipt['source_commit'],
                      'full_cross_forms': decomp['categories']['cross']['full_forms']}, sort_keys=True))


if __name__ == '__main__':
    main()
