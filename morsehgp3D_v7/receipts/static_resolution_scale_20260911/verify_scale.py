#!/usr/bin/env python3
"""Read closed local static scale probes and the frozen binary/source evidence."""
from __future__ import annotations

import hashlib
import json
from pathlib import Path
import re
import sys

HERE = Path(__file__).resolve().parent
BINARY_SHA = 'f71f31190f5d50ac70f8332f969c6baa50549536bd08836702e6ba01ae7fcdce'

def need(ok: bool, reason: str) -> None:
    if not ok:
        raise ValueError(reason)

def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()

def main() -> None:
    need(len(sys.argv) >= 2 and all(arg in ('8000', '16000', '32000') for arg in sys.argv[1:]), 'sizes')
    need(len(set(sys.argv[1:])) == len(sys.argv[1:]), 'unique_sizes')
    frozen = {p.relative_to(HERE / 'source').as_posix(): sha(p)
              for p in sorted((HERE / 'source').rglob('*')) if p.is_file()}
    results = []
    for size in sorted(map(int, sys.argv[1:])):
        directory = HERE / f'n{size}_s8_static4'
        receipt = json.loads((directory / 'receipt.json').read_text())
        need(receipt['status'] == 'completed' and receipt['sources_and_binary_stable'] and
             receipt['exit_code'] == 0 and receipt['shared_host'] and not receipt['GCP_used'], 'receipt_status')
        before = json.loads((directory / 'sources_before.json').read_text())
        after = json.loads((directory / 'sources_after.json').read_text())
        need(before == after == frozen, 'sources_frozen')
        for when in ('before', 'after'):
            need((directory / ('binary_' + when + '.sha256')).read_text().strip() == BINARY_SHA, 'binary_identity')
        command = json.loads((directory / 'command.json').read_text())
        need(command['exit_code'] == 0 and command['ended_ns'] >= command['started_ns'], 'closed_command')
        need(command['argv'][-5:] == [f'--n={size}', '--s=8', '--kmax=10', '--threads=1', '--static-threads=4'], 'command_configuration')
        for stream in ('stdout', 'stderr'):
            need(sha(directory / stream) == command[stream + '_sha256'], 'raw_stream_hash')
        metadata = json.loads((directory / 'reference.json').read_text())
        need(sha(directory / 'reference.stdout') == metadata['sha256'], 'reference_output_pin')
        actual = json.loads((directory / 'stdout').read_text())
        reference = json.loads((directory / 'reference.stdout').read_text())
        comparison = json.loads((directory / 'comparison.json').read_text())
        need(actual['status'] == 'completed_relative' and actual['orders'] == 10 and
             actual['contract_qualified'] is False and actual['public_status'] == 'not_claimed', 'bounded_authority')
        excluded = {'anchor_hits', 'intruder_queries', 'same_radius_steps', 'validation_meb_calls',
                    'resolver_meb_calls', 'resolver_supports_tested'}
        fields = sorted(k for k in reference if not (k.endswith('_s') or k.startswith('resolver_cache_') or k in excluded))
        need(fields == comparison['fields_compared'] and len(fields) >= 20, 'comparison_field_domain')
        need(all(k in actual and actual[k] == reference[k] for k in fields), 'independent_rechecked_comparison')
        need(comparison['all_nonwork_fields_matched'] and not comparison['timing_speedup_claim'] and
             not comparison['contract_qualified'], 'comparison_authority')
        rows = actual['static_orders']
        need([r['K'] for r in rows] == list(range(1,11)) and
             all(0 <= r['seeded_unique'] <= r['unique'] <= r['requests'] for r in rows), 'static_count_shape')
        requests = sum(r['requests'] for r in rows)
        unique = sum(r['unique'] for r in rows)
        seeds = sum(r['seeded_unique'] for r in rows)
        need(requests > unique > seeds > 0, 'static_count_nonvacuity')
        need(actual['resolver_meb_calls'] == actual['anchor_hits'] + actual['intruder_queries'] and
             actual['resolver_meb_calls'] > 0, 'actual_MEB_accounting')
        stderr = (directory / 'stderr').read_text()
        rss = re.search(r'Maximum resident set size \(kbytes\): (\d+)', stderr)
        need(rss is not None, 'peak_RSS_measured')
        need('stage_complete=full_ball_tower' in stderr and 'stage_complete=payload_digest' in stderr,
             'full_tower_and_digest_completed')
        results.append(dict(n=size, total_s=actual['total_s'], tower_s=actual['tower_s'], nodes=actual['nodes'],
            mebs=actual['resolver_meb_calls'], supports=actual['resolver_supports_tested'], requests=requests,
            unique=unique, seeds=seeds, static_capacity=actual['static_sampled_retained_capacity_peak_bytes'],
            peak_RSS_KiB=int(rss.group(1)), payload_digest=actual['payload_digest'], fields_compared=len(fields)))
    ratios = []
    for first, second in zip(results, results[1:]):
        ratios.append(dict(n0=first['n'], n1=second['n'], ratios={name: second[name]/first[name]
            for name in ('nodes', 'mebs', 'supports', 'requests', 'unique', 'seeds', 'static_capacity', 'peak_RSS_KiB')}))
    print(json.dumps(dict(status='verified_closed_local_static_scale', results=results, doubling_physical_ratios=ratios,
        shared_host=True, timing_speedup_claim=False, public_status='not_claimed', GCP_used=False)))

if __name__ == '__main__':
    main()
