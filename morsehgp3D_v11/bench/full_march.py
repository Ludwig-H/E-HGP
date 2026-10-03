#!/usr/bin/env python3
"""Same-source CPU FULL baseline versus x86-64-v3, paired and interleaved; no IPO or algorithm change."""
import argparse
from pathlib import Path
import sys
import time

import full_campaign as full
import march_variants as variants

need, base, profiles = full.need, full.base, full.profiles
SCHEMA = 'ehgp.v11.full_march_campaign.v1'


def identity(row):
    return (row['build_variant'],)+full.identity(row)


def schedule(repetitions=2, mode=4095):
    need(type(repetitions) is int and 2 <= repetitions <= 4, 'two to four paired repetitions required')
    full.optimization(mode)
    pairs = [(name, bits) for name in sorted(profiles.COUNTS) if name.startswith('lidar') for bits in (21, 24)]
    rows = []
    for repetition in range(repetitions):
        for ordinal, (name, bits) in enumerate(pairs):
            order = ('baseline', 'v3') if (repetition+ordinal) % 2 == 0 else ('v3', 'baseline')
            for variant in order:
                rows.append(dict(build_variant=variant, case=name, coord_bits=bits, kmax=5, workers=48,
                                 repetition=repetition, optimizations=mode))
    return rows


def signature(row):
    domain, event = row['events'][1:3]
    return (tuple(sorted(domain['catalogue_work'].items())),
            tuple((tuple(sorted(o['work'].items())), tuple(o['parallel'][k] for k in sorted(full.parallel.COUNTS)),
                   tuple(o['vertical_parallel'][k] for k in sorted(full.vertical.COUNTS))) for o in event['orders']))


def comparisons(rows, requested):
    expected_ids = [identity(r) for r in requested]
    ids = [identity(r) for r in rows]
    need(len(set(ids)) == len(ids) and len(set(expected_ids)) == len(expected_ids) and
         set(ids) <= set(expected_ids), 'march attempt inventory')
    results = []
    for case in sorted({r['case'] for r in requested}):
        expected = [r for r in requested if r['case'] == case]
        found = [r for r in rows if r['case'] == case and r['status'] == 'ok']
        semantic_equal = len({r['semantic']['sha256'] for r in found}) <= 1
        raw_equal = all(len({r['semantic']['raw_sha256'] for r in found if r['coord_bits'] == bits}) <= 1
                        for bits in (21, 24))
        work_equal = len({signature(r) for r in found}) <= 1
        equal = semantic_equal and raw_equal and work_equal
        results.append(dict(case=case, requested=len(expected), successful=[identity(r) for r in found],
                            semantic_equal=semantic_equal, same_profile_bytes_equal=raw_equal, work_equal=work_equal,
                            status='different' if not equal else 'equal' if len(found) == len(expected) else 'incomplete'))
    return results


def validate_attempt(row, record, requested):
    need(identity(row) == identity(requested) and row['argv'][0] == record['path'] and
         row['coord_bits'] == record['coord_bits'] and row['build_variant'] == record['build_variant'],
         'attempt build identity')
    # The collector has already validated current events even on a cached semantic result.
    row['build_configuration'] = record['configuration']
    row['executable_sha256'] = record['sha256']
    row['semantic_cache_scope'] = record['build_variant']


def run(args):
    need(type(args.budget_seconds) is int and 120 <= args.budget_seconds <= 700, 'march campaign budget')
    started = time.monotonic()
    selected_leaf = full.leaf_size(args, 5)
    requested = schedule(args.repetitions, args.optimizations)
    args.out.mkdir(parents=True, exist_ok=True)
    args.work.mkdir(parents=True, exist_ok=False)
    builds, mutant_proof = variants.checked_builds(args)
    manifest, manifest_hash = profiles.inputs(args.data)
    cases = {c['name']: c for c in manifest['cases']}
    caches = {name: full.reuse.SummaryCache() for name in ('baseline', 'v3')}
    for name in caches:
        (args.work/name).mkdir()
    report = dict(schema=SCHEMA, complete=False, conforming=False, requested=requested,
                  builds=list(builds.values()), qualification_sha256=base.digest(args.qualification),
                  mutant_inheritance=mutant_proof, manifest=manifest, manifest_sha256=manifest_hash,
                  repetitions=args.repetitions, optimizations=args.optimizations, leaf_size=selected_leaf,
                  budget_seconds=args.budget_seconds, timeout_seconds=full.TIMEOUT, work_schema=full.WORK_SCHEMA,
                  semantic_reuse_enabled=True, semantic_cache_scopes=['baseline', 'v3'],
                  semantic_reuse_scope='Separate variant caches; full rehash before each reuse, current events always checked',
                  scope='CPU FULL K1..5, same source/options/input, compiler target only; no IPO or GPU',
                  timing_scope='FULL wall contains index/domain/forest; Cloud/Pool/IO/decoding reported separately',
                  omission_policy='Budget only; a failed attempt never suppresses another variant',
                  runs=[], launch_intents=[], not_run=[], comparisons=[])
    path = args.out/'full_march.json'

    def save():
        report['comparisons'] = comparisons(report['runs'], requested)
        base.save(path, report)

    save()
    for request in requested:
        if time.monotonic()-started+full.TIMEOUT+20 >= args.budget_seconds:
            report['not_run'].append(dict(request, reason='campaign_budget_before_launch'))
            save()
            continue
        variant = request['build_variant']
        record = builds[variant, request['coord_bits']]
        call_args = argparse.Namespace(**vars(args))
        call_args.work = args.work/variant
        intent = full.launch_intent(Path(record['path']), cases[request['case']], request['coord_bits'], 5,
                                   call_args, 48, request['repetition'], full.TIMEOUT, args.optimizations)
        intent.update(build_variant=variant, build_configuration=record['configuration'], executable_sha256=record['sha256'])
        report['launch_intents'].append(intent)
        save()
        ordinal = len(report['runs'])

        def checkpoint(row):
            need(len(report['runs']) == ordinal, 'single march checkpoint')
            validate_attempt(row, record, request)
            report['runs'].append(row)
            save()

        row = full.measure(Path(record['path']), cases[request['case']], request, call_args, checkpoint,
                           semantic_cache=caches[variant])
        need(len(report['runs']) == ordinal+1, 'march checkpoint missing')
        validate_attempt(row, record, request)
        report['runs'][ordinal] = row
        save()
        print('%s %s B%d r%d %s' % (request['case'], variant, request['coord_bits'], request['repetition'], row['status']),
              flush=True)
    report['complete'] = True
    report['conforming'] = not report['not_run'] and len(report['runs']) == len(requested) and all(
        r['status'] == 'ok' for r in report['runs']) and all(c['status'] == 'equal' for c in report['comparisons'])
    report['campaign_wall_seconds'] = time.monotonic()-started
    save()
    return 0 if report['conforming'] else 1


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('builds', 'data', 'out', 'work', 'qualification'):
        parser.add_argument('--'+name, type=Path, required=True)
    parser.add_argument('--budget-seconds', type=int, default=680)
    parser.add_argument('--repetitions', type=int, choices=range(2, 5), default=2)
    parser.add_argument('--optimizations', type=int, default=4095)
    parser.add_argument('--leaf-size', type=int, choices=range(8, 257), default=16)
    args = parser.parse_args()
    try:
        return run(args)
    except (OSError, ValueError, KeyError, TypeError) as error:
        print('REFUS '+str(error), file=sys.stderr)
        return 2


if __name__ == '__main__':
    raise SystemExit(main())
