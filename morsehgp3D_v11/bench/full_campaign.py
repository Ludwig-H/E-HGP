#!/usr/bin/env python3
"""Whole-cloud exact FULL campaign; every invocation follows successful native qualification."""
import argparse
from pathlib import Path
import subprocess
import sys
import time

import catalogue_profiles as profiles
import full_semantic as semantic

base, need = profiles.base, semantic.need
SCHEMA = 'ehgp.v11.full_campaign.v2'
TIMEOUT = 60
BUDGET = 8 * 1024**3
WORK = {'cells', 'replayed_cells', 'plateaus', 'traces', 'unions', 'continuations', 'ancestor_hops',
        'descent_steps', 'vertical_descents', 'vertical_checks', 'part_meb_presentations',
        'trace_meb_presentations', 'census_point_tests'}


def unsigned(event, keys):
    need(all(type(event[key]) is int and 0 <= event[key] < 2**64 for key in keys), 'unsigned event values')


def optimization(value):
    need(type(value) is int and 0 <= value <= 3, 'optimization mode outside 0..3')
    return value


def collect(row, case, output, bits):
    """A successful process still needs a complete stream and a structurally valid FULL artifact."""
    started = time.monotonic()
    try:
        need(not row['stderr'], 'successful native process wrote stderr')
        events = row['events']
        need([e['phase'] for e in events] == ['cloud', 'domain', 'full', 'exit'], 'complete FULL phase stream')
        cloud, domain, full, end = events
        need(full['status'] == end['status'] == 'ok' and full['reason'] == end['reason'] == 'none', 'native verdict')
        unsigned(cloud, ('read_ns', 'cloud_ns', 'cloud_peak_bytes', 'sites', 'points'))
        unsigned(domain, ('index_ns', 'domain_ns', 'catalogue_balls', 'pool_ns', 'sort_ns', 'count_ns', 'fill_ns'))
        unsigned(full, ('coord_bits', 'kmax', 'workers', 'optimizations', 'wall_ns', 'index_ns', 'domain_ns', 'forest_ns',
                        'peak_reserved_bytes', 'reserved_after_bytes'))
        need(cloud['sites'] == cloud['points'] == case['count'], 'whole input cardinality')
        need(full['coord_bits'] == bits and full['kmax'] == row['kmax'] and full['workers'] == row['workers'] and
             full['optimizations'] == optimization(row['optimizations']),
             'requested native parameters')
        need(domain['index_ns'] == full['index_ns'] and domain['domain_ns'] == full['domain_ns'], 'duplicate durations')
        need(sum(domain[key] for key in ('sort_ns', 'count_ns', 'fill_ns')) <= domain['domain_ns'], 'domain stage walls')
        need(sum(full[key] for key in ('index_ns', 'domain_ns', 'forest_ns')) <= full['wall_ns'], 'FULL stage walls')
        cpu = full['cpu_seconds']
        need(type(cpu) in (int, float) and 0 <= cpu < float('inf'), 'finite CPU duration')
        need(cloud['cloud_peak_bytes'] <= BUDGET, 'input reservations')
        need(0 <= full['reserved_after_bytes'] <= full['peak_reserved_bytes'] <= BUDGET, 'native reservations')
        need(domain['catalogue_balls'] > 0 and len(full['orders']) == row['kmax'], 'nonempty whole tower')
        value = semantic.inspect(output, bits, row['kmax'], case['count'])
        for k, (order, decoded) in enumerate(zip(full['orders'], value['orders']), 1):
            unsigned(order, ('k', 'births', 'nodes', 'edges', 'verticals', 'node_capacity', 'edge_capacity'))
            need(order['k'] == decoded['order'] == k and all(order[key] == decoded[key] for key in
                 ('births', 'nodes', 'edges', 'verticals')), 'native/dump order counts')
            need(order['node_capacity'] == 2 * order['births'] - 1 and
                 order['edge_capacity'] == 2 * order['births'] - 2 and
                 order['nodes'] <= order['node_capacity'] and order['edges'] <= order['edge_capacity'], 'capacities')
            need(set(order['work']) == WORK, 'FULL work fields')
            unsigned(order['work'], WORK)
        row.update(status='ok', semantic=value, full_ms=full['wall_ns'] / 1e6,
                   whole_peak_reserved_bytes=max(cloud['cloud_peak_bytes'], full['peak_reserved_bytes']),
                   cloud_ms=cloud['cloud_ns'] / 1e6, read_ms=cloud['read_ns'] / 1e6, pool_ms=domain['pool_ns'] / 1e6,
                   stage_ms={key[:-3]: full[key] / 1e6 for key in ('index_ns', 'domain_ns', 'forest_ns')},
                   full_within_200ms=full['wall_ns'] <= 200_000_000,
                   cloud_pool_full_ms=(cloud['cloud_ns'] + domain['pool_ns'] + full['wall_ns']) / 1e6)
    except (OSError, ValueError, KeyError, TypeError, OverflowError) as error:
        base.attempt_error(row, 'FULL_semantic', error, 'invalid_output')
    row['semantic_wall_seconds'] = time.monotonic() - started


def measure(exe, case, request, args, checkpoint):
    bits, kmax, workers, repetition = (request[key] for key in ('coord_bits', 'kmax', 'workers', 'repetition'))
    mode = optimization(request['optimizations'])
    need(mode == optimization(args.optimizations), 'request optimization differs from campaign')
    output, argv = profiles.invocation(exe, case, bits, kmax, args, workers, repetition, mode)
    row = dict(request, argv=argv, timeout_seconds=TIMEOUT, whole_input=True, count=case['count'],
               exit_code=None, stdout='', stderr='', events=[], errors=[], status='exited')
    started = time.monotonic()
    try:
        result = subprocess.run(argv, stdin=subprocess.DEVNULL, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                                timeout=TIMEOUT, check=False)
        row.update(exit_code=result.returncode, stdout=result.stdout.decode('utf-8', 'backslashreplace'),
                   stderr=result.stderr.decode('utf-8', 'backslashreplace'),
                   status='exited' if result.returncode == 0 else 'refused' if result.returncode == 2 else 'failed')
    except subprocess.TimeoutExpired as error:
        row.update(stdout=(error.stdout or b'').decode('utf-8', 'backslashreplace'),
                   stderr=(error.stderr or b'').decode('utf-8', 'backslashreplace'), status='timeout')
        base.attempt_error(row, 'process', error, 'timeout')
    except OSError as error:
        base.attempt_error(row, 'launch', error, 'launch_error')
    row['process_wall_seconds'] = time.monotonic() - started
    for line in row['stdout'].splitlines():
        if line.strip():
            try:
                row['events'].append(base.event_json(line))
            except (ValueError, TypeError) as error:
                base.attempt_error(row, 'events', error, 'invalid_output')
    if row['status'] == 'exited':
        row['status'] = 'pending_semantic'
    checkpoint(row)
    if row['status'] == 'pending_semantic':
        row['status'] = 'exited'
        collect(row, case, output, bits)
    try:
        output.unlink(missing_ok=True)
    except OSError as error:
        base.attempt_error(row, 'cleanup', error, 'artifact_error')
    return row


def schedule(optimizations=0):
    optimization(optimizations)
    lidar = sorted(name for name in profiles.COUNTS if name.startswith('lidar'))
    result = []
    for kmax, repetitions in ((5, (0, 1, 2)), (10, (0,))):
        for repetition in repetitions:
            for name in lidar:
                for bits in (21, 24):
                    result.append(dict(case=name, coord_bits=bits, kmax=kmax, workers=48, repetition=repetition,
                                       optimizations=optimizations))
    return result


def identity(row):
    return tuple(row[key] for key in ('case', 'coord_bits', 'kmax', 'workers', 'repetition', 'optimizations'))


def comparisons(rows, requested):
    result = []
    for name, kmax, mode in sorted({(r['case'], r['kmax'], optimization(r['optimizations'])) for r in requested}):
        expected = [r for r in requested if (r['case'], r['kmax'], r['optimizations']) == (name, kmax, mode)]
        found = [r for r in rows if (r['case'], r['kmax'], r['optimizations']) == (name, kmax, mode) and r['status'] == 'ok']
        semantic_equal = len({r['semantic']['sha256'] for r in found}) <= 1
        raw_equal = all(len({r['semantic']['raw_sha256'] for r in found if r['coord_bits'] == bits}) <= 1
                        for bits in (21, 24))
        work_equal = len({tuple(tuple(sorted(o['work'].items())) for o in r['events'][2]['orders']) for r in found}) <= 1
        equal = semantic_equal and raw_equal and work_equal
        result.append(dict(case=name, kmax=kmax, optimizations=mode, requested=len(expected), successful=[identity(r) for r in found],
                           semantic_equal=semantic_equal, same_profile_bytes_equal=raw_equal, work_equal=work_equal,
                           status='different' if not equal else 'equal' if len(found) == len(expected) else 'incomplete'))
    return result


def run(args):
    mode = optimization(args.optimizations)
    args.out.mkdir(parents=True, exist_ok=True)
    args.work.mkdir(parents=True, exist_ok=False)
    started = time.monotonic()
    builds = profiles.checked_builds(args, executable='mhgp11_full_bench')
    supplement = profiles.checked_supplement(args.supplement)
    manifest, manifest_hash = profiles.inputs(args.data)
    cases = {row['name']: row for row in manifest['cases']}
    requested = schedule(mode)
    report = dict(schema=SCHEMA, complete=False, conforming=False, manifest=manifest, manifest_sha256=manifest_hash,
                  qualification_sha256=base.digest(args.qualification), supplement_sha256=supplement,
                  builds=list(builds.values()), requested=requested, requested_runs=len(requested),
                  timeout_seconds=TIMEOUT, budget_seconds=args.budget_seconds, leaf_size=16, max_leaf=256,
                  optimizations=mode,
                  scope='CPU FULL K1..K exact merge forests and closed verticals; unit weights; whole nonground frames',
                  timing_scope='FULL wall: index + catalogue/lookup + forests/verticals; Cloud/Pool/IO separate',
                  excluded='ground segmentation; preparation of staged integer inputs; point projection; GPU',
                  memory_scope='native Buffer reservations including Cloud; not RSS nor Python decoder',
                  precision='same 1mm integer input in u21/u24; wider arithmetic, not finer input precision',
                  semantic_scope='structure and exact rational identity; geometry independently gated on small fixtures',
                  repetitions='three fresh processes per LiDAR K5/profile; K10 one process',
                  runs=[], launch_intents=[], not_run=[], comparisons=[], full_schedule_completed=False)
    path = args.out / 'full.json'

    def save():
        report['comparisons'] = comparisons(report['runs'], requested)
        base.save(path, report)

    save()
    failed_baselines = set()
    for request in requested:
        key = request['case'], request['coord_bits']
        reason = ('same_profile_K5_first_attempt_failed' if key in failed_baselines else
                  'campaign_budget_before_launch' if time.monotonic() - started + TIMEOUT + 20 >= args.budget_seconds else None)
        if reason:
            report['not_run'].append(dict(request, reason=reason)); save(); continue
        ordinal = len(report['runs'])
        report['launch_intents'].append(profiles.launch_intent(Path(builds[request['coord_bits']]['path']),
            cases[request['case']], request['coord_bits'], request['kmax'], args, request['workers'],
            request['repetition'], TIMEOUT, mode))
        save()

        def checkpoint(row):
            need(len(report['runs']) == ordinal and identity(row) == identity(request), 'attempt checkpoint')
            report['runs'].append(row); save()

        row = measure(Path(builds[request['coord_bits']]['path']), cases[request['case']], request, args, checkpoint)
        need(len(report['runs']) == ordinal + 1, 'missing checkpoint')
        report['runs'][ordinal] = row
        if row['status'] != 'ok' and request['kmax'] == 5 and request['repetition'] == 0:
            failed_baselines.add(key)
        save()
        print('%s B%d K%d W%d r%d o%d %s' % (*identity(request), row['status']), flush=True)
    report['complete'] = True
    report['full_schedule_completed'] = not report['not_run'] and len(report['runs']) == len(requested)
    report['conforming'] = report['full_schedule_completed'] and all(r['status'] == 'ok' for r in report['runs']) and all(
        r['status'] == 'equal' for r in report['comparisons'])
    report['campaign_wall_seconds'] = time.monotonic() - started
    save()
    return 0 if report['conforming'] else 1


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for option in ('builds', 'data', 'out', 'work', 'qualification', 'supplement'):
        parser.add_argument('--' + option, type=Path, required=True)
    parser.add_argument('--budget-seconds', type=int, default=900)
    parser.add_argument('--optimizations', type=int, choices=range(4), default=0)
    args = parser.parse_args()
    if not 90 <= args.budget_seconds <= 1800:
        parser.error('budget outside 90..1800 seconds')
    try:
        return run(args)
    except (OSError, ValueError, KeyError, TypeError, StopIteration) as error:
        print('full_campaign_refused: ' + type(error).__name__, flush=True)
        return 2


if __name__ == '__main__':
    sys.exit(main())
